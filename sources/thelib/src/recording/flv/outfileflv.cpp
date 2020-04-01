/**
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
**/



#include "application/baseclientapplication.h"
#include "streaming/nalutypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/streamstypes.h"
#include "protocols/baseprotocol.h"
#include "recording/flv/outfileflv.h"
#include "streaming/baseinstream.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/amf0serializer.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"

#define FLV_TAG_AUDIO		0x08
#define FLV_TAG_VIDEO		0x09
#define FLV_TAG_METADATA	0x12

OutFileFLV::OutFileFLV(BaseProtocol *pProtocol, string name, Variant &settings)
: BaseOutRecording(pProtocol, ST_OUT_FILE_RTMP_FLV, name, settings) {
	_pFile = NULL;
	_prevTagSize = 0;
	_chunkLength = 0;
	_waitForIDR = false;
	_lastVideoDts = -1;
	_lastAudioDts = -1;
	_chunkCount = 0;
	_startTime = "";
	_frameCount = 0;
}

OutFileFLV* OutFileFLV::GetInstance(
		BaseClientApplication *pApplication, string name, Variant &settings) {

	//1. Create a dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//2. create the parameters
	Variant parameters;
	parameters["customParameters"]["recordConfig"] = settings;

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(parameters)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//4. Set the application
	pDummyProtocol->SetApplication(pApplication);

	//5. Create the FLV file
	OutFileFLV *pOutFileFLV = new OutFileFLV(pDummyProtocol, name, settings);
	if (!pOutFileFLV->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutFileFLV;
		pOutFileFLV = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutFileFLV);
	pOutFileFLV->SetApplication(pApplication);

	//6. Done
	return pOutFileFLV;
}

OutFileFLV::~OutFileFLV() {

	if (_pApp != NULL) {
		_pApp->GetRecordSession()[GetName()].ResetFlag(RecordingSession::FLAG_FLV);
	}

	if (_pFile != NULL) {
		UpdateDuration();
		delete _pFile;
		_pFile = NULL;
	}
}

void OutFileFLV::SetApplication(BaseClientApplication *pApplication) {
	_pApp = pApplication;
}

BaseClientApplication *OutFileFLV::GetApplication() {
	return _pApp;
}

bool OutFileFLV::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	// include user defined variables (starting with "_") into the event
	if (_settings.MapSize() > 0) {
		FOR_MAP(_settings, string, Variant, i) {
			if (MAP_KEY(i).substr(0, 1) == "_") {
				_chunkInfo[MAP_KEY(i)] = MAP_VAL(i);
			}
		}
	}

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_SIZE, //naluMarkerType,
			false, //insertPDNALU,
			true, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
	pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
	pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = true;

	//misc setup
	pGenericProcessDataSetup->_timeBase = 0;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;
	_waitForIDR = (bool) _settings["waitForIDR"];
	uint32_t length = (uint32_t) _settings["chunkLength"];
//	if ((length > 0) && (length < 15)) {
//		length = 15;
//	}
	_chunkLength = double (length) * 1000.0; //convert seconds to milliseconds
	//FINEST("---- waitForIDR=%d, chunkLength=%d", (uint8_t) _waitForIDR, (uint32_t) _chunkLength);

	if (!InitializeFLVFile(pGenericProcessDataSetup)) {
		string error = "Unable to initialize FLV file";
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		if (_pFile != NULL) {
			delete _pFile;
			_pFile = NULL;
		}
		return false;
	}

	return true;
}

bool OutFileFLV::PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) {
	if (_pFile == NULL) {
		FATAL("FLV File not opened for writing");
		return false;
	}

	if (_lastVideoDts < 0)
		_lastVideoDts = dts;
	dts -= _lastVideoDts;

	uint32_t bufferLength = GETAVAILABLEBYTESCOUNT(buffer);
	EHTONLP(_tagHeader, bufferLength);
	_tagHeader[0] = FLV_TAG_VIDEO;
	EHTONAP(_tagHeader + 4, (uint32_t) dts);
	if (!_pFile->WriteBuffer(_tagHeader, 11)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteBuffer(GETIBPOINTER(buffer), bufferLength)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteUI32(bufferLength + 11, true)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	_frameCount++;

	if ((_chunkLength > 0) && //enabled
			(dts > 0) && //valid
			(dts > _chunkLength) &&
			((!_waitForIDR) || (_waitForIDR && isKeyFrame))) {
		SplitFile();
	}

	return true;
}

bool OutFileFLV::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	if (_pFile == NULL) {
		FATAL("FLV File not opened for writing");
		return false;
	}

	if (_lastAudioDts < 0)
		_lastAudioDts = dts;
	dts -= _lastAudioDts;

	uint32_t bufferLength = GETAVAILABLEBYTESCOUNT(buffer);
	EHTONLP(_tagHeader, bufferLength);
	_tagHeader[0] = FLV_TAG_AUDIO;
	EHTONAP(_tagHeader + 4, (uint32_t) dts);
	if (!_pFile->WriteBuffer(_tagHeader, 11)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteBuffer(GETIBPOINTER(buffer), bufferLength)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteUI32(bufferLength + 11, true)) {
		FATAL("Unable to write FLV content");
		return false;
	}

	if ((_chunkLength > 0) && //enabled
			(dts > 0) && //valid
			(dts > _chunkLength)) {
		SplitFile();
	}

	return true;
}

bool OutFileFLV::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264)
			|| (codec == CODEC_AUDIO_AAC)
			;
}

bool OutFileFLV::WriteFLVHeader(bool hasAudio, bool hasVideo) {
	if (_pFile != NULL) {
		delete _pFile;
		_pFile = NULL;
	}

	uint64_t preset = (uint64_t)_settings["preset"];
	bool dateFolderStructure = ((bool)_settings["dateFolderStructure"]) || (preset == 1);

	_pFile = new File();
	string filePath = (string) _settings["computedPathToFile"];
	if (_chunkLength > 0) {
		if (dateFolderStructure) {
			string recordFolder = filePath;
			size_t slashPos = filePath.length();
			int count = 0;
			do {
				slashPos = filePath.find_last_of(PATH_SEPARATOR, slashPos - 1);
			} while ((++count < 3) && (slashPos != string::npos));
			if (slashPos != string::npos) {
				recordFolder = filePath.substr(0, slashPos);
			}
			// Append latest localstreamname and date to truncated file path (original pathToFile)
			time_t now = time(NULL);
			Timestamp tsNow = *localtime(&now);
			char datePart[9];
			char timePart[7];
			strftime(datePart, 9, "%Y%m%d", &tsNow);
			strftime(timePart, 7, "%H%M%S", &tsNow);
			recordFolder += format("%c%s%c%s%c", PATH_SEPARATOR, STR(GetName()), PATH_SEPARATOR,
				datePart, PATH_SEPARATOR);
			if (!fileExists(recordFolder)) {
				if (!createFolder(recordFolder, true)) {
					FATAL("Unable to create folder %s", STR(recordFolder));
					return false;
				}
			}
			// Append filename in streamname-date-time format
			filePath = recordFolder + format("%s-%s-%s.mp4", STR(GetName()), datePart, timePart);
		} else {
			// To be consistent with mp4
			string count = format("_part%04"PRIu32, _chunkCount);
			replace(filePath, ".flv", count);
			filePath += ".flv";

			_chunkCount++;
		}
	}

	if (!_pFile->Initialize(filePath, FILE_OPEN_MODE_TRUNCATE)) {
		string error = "Unable to open file " + filePath;
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	_chunkInfo["file"] = filePath;
	_chunkInfo["startTime"] = Variant::Now();
	GetEventLogger()->LogRecordChunkCreated(_chunkInfo);

	uint8_t _flvHeader[] = {'F', 'L', 'V', 1, 0, 0, 0, 0, 9, 0, 0, 0, 0};
	if (hasAudio)
		_flvHeader[4] |= 0x04;
	if (hasVideo)
		_flvHeader[4] |= 0x01;
	if (!_pFile->WriteBuffer(_flvHeader, sizeof (_flvHeader))) {
		string error = "Unable to write flv header";
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}
	return true;
}

bool OutFileFLV::InitializeFLVFile(GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!WriteFLVHeader(pGenericProcessDataSetup->_hasAudio,
			pGenericProcessDataSetup->_hasVideo)) {
		return false;
	}

	if (!WriteMetaData(pGenericProcessDataSetup)) {
		FATAL("Unable to write FLV metadata");
		return false;
	}

	if (!WriteCodecSetupBytes(pGenericProcessDataSetup)) {
		FATAL("Unable to write FLV codec setup bytes");
		return false;
	}

	return true;
}

bool OutFileFLV::WriteFLVMetaData() {
	_buff.IgnoreAll();
	_buff.ReadFromRepeat(0, 11);
	_metadata.IsArray(true);
	_metadata["creationdate"] = Variant::Now();
	_metadata["duration"] = (double) 0;

	// Instantiate the AMF serializer and a buffer that will contain the data
	AMF0Serializer amf;

	// Serialize it
	string temp = "onMetaData";
	amf.WriteShortString(_buff, temp, true);
	amf.Write(_buff, _metadata);

	// Get total length of metadata
	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(_buff) - 11;
	EHTONLP(GETIBPOINTER(_buff), dataLength);
	GETIBPOINTER(_buff)[0] = FLV_TAG_METADATA;

	_buff.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buff) + GETAVAILABLEBYTESCOUNT(_buff) - 4, dataLength + 11);

	// Write metadata to file
	return _pFile->WriteBuffer(GETIBPOINTER(_buff), GETAVAILABLEBYTESCOUNT(_buff));
}

bool OutFileFLV::WriteMetaData(GenericProcessDataSetup *pGenericProcessDataSetup) {

	pGenericProcessDataSetup->_pStreamCapabilities->GetRTMPMetadata(_metadata);

	return WriteFLVMetaData();
}

bool OutFileFLV::WriteFLVCodecAudio(AudioCodecInfoAAC *pInfoAudio) {
	if (pInfoAudio == NULL)
		return false;

	IOBuffer &temp = pInfoAudio->GetRTMPRepresentation();
	uint32_t packetLength = GETAVAILABLEBYTESCOUNT(temp);
	memset(_tagHeader, 0, sizeof (_tagHeader));
	EHTONLP(_tagHeader, packetLength);

	_tagHeader[0] = FLV_TAG_AUDIO;

	if (!_pFile->WriteBuffer(_tagHeader, 11)) {
		FATAL("Unable to write FLV content");
		return false;
	}

	if (!_pFile->WriteBuffer(GETIBPOINTER(temp), packetLength)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteUI32(packetLength + 11, true)) {
		FATAL("Unable to write FLV content");
		return false;
	}

	return true;
}

bool OutFileFLV::WriteFLVCodecVideo(VideoCodecInfoH264 *pInfoVideo) {
	if (pInfoVideo == NULL)
		return false;

	IOBuffer &temp = pInfoVideo->GetRTMPRepresentation();
	uint32_t packetLength = GETAVAILABLEBYTESCOUNT(temp);
	memset(_tagHeader, 0, sizeof (_tagHeader));
	EHTONLP(_tagHeader, packetLength);
	_tagHeader[0] = FLV_TAG_VIDEO;
	if (!_pFile->WriteBuffer(_tagHeader, 11)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteBuffer(GETIBPOINTER(temp), packetLength)) {
		FATAL("Unable to write FLV content");
		return false;
	}
	if (!_pFile->WriteUI32(packetLength + 11, true)) {
		FATAL("Unable to write FLV content");
		return false;
	}

	return true;
}

bool OutFileFLV::WriteCodecSetupBytes(GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (_pFile == NULL) {
		FATAL("FLV File not opened for writing");
		return false;
	}

	AudioCodecInfoAAC *pInfoAudio = NULL;
	if (pGenericProcessDataSetup->_hasAudio &&
			(pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC)) {
		pInfoAudio = pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
		if (!WriteFLVCodecAudio(pInfoAudio))
			return false;
	}

	VideoCodecInfoH264 *pInfoVideo = NULL;
	if (pGenericProcessDataSetup->_hasVideo &&
			(pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)) {
		pInfoVideo = pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
		if (!WriteFLVCodecVideo(pInfoVideo))
			return false;
	}

	return true;
}

void OutFileFLV::UpdateDuration() {
	if (!_pFile->Flush())
		return;
	string path = _pFile->GetPath();
	_pFile->Close();

	_chunkInfo["file"] = path;
	_chunkInfo["startTime"] = _startTime;
	_chunkInfo["stopTime"] = Variant::Now();
	_chunkInfo["frameCount"] = _frameCount;
	GetEventLogger()->LogRecordChunkClosed(_chunkInfo);

	if (!_pFile->Initialize(path, FILE_OPEN_MODE_WRITE))
		return;

	if (!_pFile->SeekEnd())
		return;

	if (!_pFile->SeekBehind(4))
		return;

	uint32_t lastTagSize = 0;
	if (!_pFile->ReadUI32(&lastTagSize, true))
		return;

	if (!_pFile->SeekEnd())
		return;

	if (!_pFile->SeekBehind(4 + lastTagSize - 4))
		return;

	uint32_t ts = 0;
	if (!_pFile->ReadSUI32(&ts))
		return;

	if (!_pFile->SeekTo(0))
		return;

	if (!_pFile->SeekAhead(13))
		return;

	uint32_t metadataSize = 0;
	if (!_pFile->ReadUI32(&metadataSize, true))
		return;

	metadataSize &= 0x00ffffff;
	if (!_pFile->SeekAhead(0x19))
		return;

	uint32_t fileSize = (uint32_t) _pFile->Size();

	for (uint32_t i = 0; i < metadataSize && i < fileSize; i++) {
		uint64_t mask = 0;
		if (!_pFile->PeekUI64(&mask, true))
			return;
		if (mask == 0x6475726174696F6ELL) {
			_pFile->SeekAhead(9);
			double tempDouble = (double) ts / 1000.0;
			AMF0Serializer serializer;
			IOBuffer tempBuffer;
			serializer.WriteDouble(tempBuffer, tempDouble, false);
			if (!_pFile->WriteBuffer(GETIBPOINTER(tempBuffer), GETAVAILABLEBYTESCOUNT(tempBuffer)))
				return;
			return;
		} else {
			if (!_pFile->SeekAhead(1))
				return;
		}
	}
}

bool OutFileFLV::SplitFile() {

	_lastVideoDts = -1;
	_lastAudioDts = -1;

	UpdateDuration();

	StreamCapabilities * pStreamCapabilities = BaseOutStream::GetCapabilities();
	if (pStreamCapabilities == NULL) {
		return false;
	}

	bool hasAudioAAC = (CODEC_AUDIO_AAC == pStreamCapabilities->GetAudioCodecType());
	AudioCodecInfoAAC *pInfoAudio = NULL;
	if (hasAudioAAC) {
		pInfoAudio = pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
	}

	bool hasVideoH264 = (CODEC_VIDEO_H264 == pStreamCapabilities->GetVideoCodecType());
	VideoCodecInfoH264 *pInfoVideo = NULL;
	if (hasVideoH264) {
		pInfoVideo = pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
	}

	if (!WriteFLVHeader(hasAudioAAC, hasVideoH264) ||
			!WriteFLVMetaData() ||
			(hasAudioAAC && !WriteFLVCodecAudio(pInfoAudio)) ||
			(hasVideoH264 && !WriteFLVCodecVideo(pInfoVideo))) {
		return false;
	}

	return true;
}
