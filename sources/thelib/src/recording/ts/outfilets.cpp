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



#include "streaming/streamstypes.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "recording/ts/outfilets.h"
#include "streaming/hls/filetssegment.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"

OutFileTS::OutFileTS(BaseProtocol *pProtocol, string name, Variant &settings)
: BaseOutRecording(pProtocol, ST_OUT_FILE_TS, name, settings) {
	_pCurrentSegment = NULL;
	_lastPatPmtSpsPpsTimestamp = -1;
	_chunkLength = 0;
	_waitForIDR = false;
	_chunkCount = 0;
	_chunkDts = -1;
}

OutFileTS* OutFileTS::GetInstance(
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

	//5. Create the TS stream
	OutFileTS *pOutFileTS = new OutFileTS(pDummyProtocol, name, settings);
	if (!pOutFileTS->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutFileTS;
		pOutFileTS = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutFileTS);

	string filePath = (string) settings["computedPathToFile"];
	pOutFileTS->_chunkInfo["file"] = filePath;
	pOutFileTS->_chunkInfo["startTime"] = Variant::Now();
	pOutFileTS->GetEventLogger()->LogRecordChunkCreated(pOutFileTS->_chunkInfo);

	pOutFileTS->SetApplication(pApplication);

	//6. Done
	return pOutFileTS;
}

OutFileTS::~OutFileTS() {

	if (_pApp != NULL) {
		_pApp->GetRecordSession()[GetName()].ResetFlag(RecordingSession::FLAG_TS);
	}

	if (_pCurrentSegment != NULL) {
		delete _pCurrentSegment;
	}
}

void OutFileTS::SetApplication(BaseClientApplication *pApplication) {
	_pApp = pApplication;
}

BaseClientApplication *OutFileTS::GetApplication() {
	return _pApp;
}

bool OutFileTS::FinishInitialization(
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
			NALU_MARKER_TYPE_0001, //naluMarkerType,
			true, //insertPDNALU,
			false, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
	pGenericProcessDataSetup->audio.aac._insertADTSHeader = true;
	pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;

	//misc setup
	pGenericProcessDataSetup->_timeBase = -1;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;
	_waitForIDR = (bool) _settings["waitForIDR"];
	uint32_t length = (uint32_t) _settings["chunkLength"];
	_chunkLength = double (length) * 1000.0; //convert seconds to milliseconds

	if (_pCurrentSegment != NULL) {
		FATAL("Segment already opened");
		return false;
	}
	_pCurrentSegment = new FileTSSegment(_segmentVideoBuffer,
			_segmentAudioBuffer, _segmentPatPmtAndCountersBuffer);

	if (!FormatFileName())
		return false;

	if (!_pCurrentSegment->Init(_newSettings, pGenericProcessDataSetup->_pStreamCapabilities)) {
		string error = "Unable to initialize TS file " + _pCurrentSegment->GetPath();
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	return true;
}

bool OutFileTS::PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) {
	if (!WritePatPmt(dts)) {
		FATAL("Unable to write PAT/PMT/SPS/PPS");
		return false;
	}

        Variant dummy;
	if (!_pCurrentSegment->PushVideoData(buffer, (int64_t) (pts * 90.0), (int64_t) (dts * 90.0), dummy)) {
		string error = "Unable to write TS video to " + _pCurrentSegment->GetPath();
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	if (_chunkDts < 0) {
		_chunkDts = dts;
	}
	if ((_chunkLength > 0) && //enabled
			(dts > 0) && //valid
			(_chunkDts > 0) && //valid
			(dts - _chunkDts > _chunkLength) &&
			((!_waitForIDR) || (_waitForIDR && isKeyFrame))) {
		//INFO("dts=%3.1f, _chunkDts=%3.1f, _chunkLength=%3.1f, _waitForIDR=%d, isKeyFrame=%d",
		//		dts, _chunkDts, _chunkLength, _waitForIDR, isKeyFrame);
		_chunkDts = dts;
		SplitFile();
	}

	return true;
}

bool OutFileTS::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	if (!WritePatPmt(dts)) {
		FATAL("Unable to write PAT/PMT/SPS/PPS");
		return false;
	}

	if (!_pCurrentSegment->PushAudioData(buffer, (int64_t) (pts * 90.0), (int64_t) (dts * 90.0))) {
		string error = "Unable to write TS audio to " + _pCurrentSegment->GetPath();
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	if (_chunkDts < 0) {
		_chunkDts = dts;
	}
	if ((_chunkLength > 0) && //enabled
			(dts > 0) && //valid
			(_chunkDts > 0) && //valid
			(dts - _chunkDts > _chunkLength)) {
		//INFO("dts=%3.1f, _chunkDts=%3.1f, _chunkLength=%3.1f", dts, _chunkDts, _chunkLength);
		_chunkDts = dts;
		SplitFile();
	}

	return true;
}

bool OutFileTS::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264)
			|| (codec == CODEC_AUDIO_AAC)
			;
}

bool OutFileTS::WritePatPmt(double ts) {
	if ((_lastPatPmtSpsPpsTimestamp >= 0)
			&& ((ts - _lastPatPmtSpsPpsTimestamp) < PAT_PMT_INTERVAL)) {
		return true;
	}
	if (!_pCurrentSegment->WritePATPMT()) {
		FATAL("Unable to write PAT/PMT");
		return false;
	}

	_lastPatPmtSpsPpsTimestamp = ts;

	return true;
}

bool OutFileTS::SplitFile() {

	string path = _pCurrentSegment->GetPath();
	_chunkInfo["file"] = path;
	_chunkInfo["startTime"] = _pCurrentSegment->GetStartTime();
	_chunkInfo["stopTime"] = Variant::Now();
	_chunkInfo["frameCount"] = _pCurrentSegment->GetFrameCount();
	GetEventLogger()->LogRecordChunkClosed(_chunkInfo);
	if (_pCurrentSegment != NULL) {
		delete _pCurrentSegment;
		_pCurrentSegment = NULL;
	}

	StreamCapabilities * pStreamCapabilities = BaseOutStream::GetCapabilities();
	if (pStreamCapabilities == NULL) {
		return false;
	}

	_pCurrentSegment = new FileTSSegment(_segmentVideoBuffer,
		_segmentAudioBuffer, _segmentPatPmtAndCountersBuffer);

	if (!FormatFileName())
		return false;

	if (!_pCurrentSegment->Init(_newSettings, pStreamCapabilities)) {
		string error = "Unable to initialize TS file " + (string)_newSettings["computedPathToFile"];
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	_chunkInfo["file"] = _newSettings["computedPathToFile"];
	_chunkInfo["startTime"] = Variant::Now();
	GetEventLogger()->LogRecordChunkCreated(_chunkInfo);

	return true;
}
bool OutFileTS::FormatFileName() {
	_newSettings.Reset();
	_newSettings = _settings;
	string filePath = (string)_newSettings["computedPathToFile"];
	uint64_t preset = (uint64_t)_settings["preset"];
	bool dateFolderStructure = ((bool)_settings["dateFolderStructure"]) || (preset == 1);

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
			string count = format("_part%04"PRIu32, _chunkCount);
			replace(filePath, ".ts", count);
			filePath += ".ts";
			_chunkCount++;
		}
		_newSettings["computedPathToFile"] = filePath;
	}
	return true;
}
