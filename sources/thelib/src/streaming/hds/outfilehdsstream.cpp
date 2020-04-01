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


#ifdef HAS_PROTOCOL_HDS
#include "streaming/hds/outfilehdsstream.h"
#include "streaming/hds/hdsmanifest.h"
#include "streaming/hds/hdsfragment.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "mediaformats/writers/fmp4/atom_abst.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"

OutFileHDSStream::OutFileHDSStream(BaseProtocol *pProtocol, string name,
		Variant &settings)
: BaseOutFileStream(pProtocol, ST_OUT_FILE_HDS, name) {
	//audio
	_audioRate = -1;

	//video
	_videoRate = -1;

	_settings = settings;
	_pGenericProcessDataSetup = NULL;
	_fragmentStartupTime = -1;
	_chunkOnIDR = false;
	_pFragment = NULL;
	_pManifest = NULL;
	_fragmentSequence = 1;
	_chunkLength = 0;
	_targetFolder = "";

	_pAbstBox = NULL;
	_bootstrapFilename = "bootstrap";
	_bootstrapFilePath = "";
	_isLastFragment = false;
	_isFirstFragment = true;
	_overwriteDestination = false;

	_isRolling = false;
	_playlistLength = 0;
	//_staleRetentionCount = 0;

	//_lastAudioTS = -1;
	_audioCodecsSent = false;
	_videoCodecsSent = false;
}

OutFileHDSStream* OutFileHDSStream::GetInstance(BaseClientApplication *pApplication,
		string name, Variant &settings) {
	//1. Create a dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//2. create the parameters
	Variant parameters;
	parameters["customParameters"]["hdsConfig"] = settings;

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(parameters)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//4. Set the application
	pDummyProtocol->SetApplication(pApplication);

	//5. Create the HDS stream
	OutFileHDSStream *pOutHDS = new OutFileHDSStream(pDummyProtocol, name, settings);
	if (!pOutHDS->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutHDS;
		pOutHDS = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutHDS);

	//6. Done
	return pOutHDS;
}

OutFileHDSStream::~OutFileHDSStream() {
	// Set the last fragment flag
	_isLastFragment = true;

	// Close the fragment
	CloseFragment(-1);

	// Update the bootstrap
	_pAbstBox->WriteBootStrap(_bootstrapFilePath);

	bool deleteFiles =
			(_pProtocol != NULL)
			&&(_pProtocol->GetCustomParameters().HasKeyChain(V_BOOL, true, 1, "IsTaggedForDeletion"))
			&&((bool)_pProtocol->GetCustomParameters()["IsTaggedForDeletion"]);

	// Delete the instance of Manifest writer
	if (NULL != _pManifest) {
		if (deleteFiles) {
			//_pManifest->RemoveMediaAndSave();
			GetEventLogger()->LogHDSMasterPlaylistUpdated(_pManifest->GetMasterManifestPath());
		}

		delete _pManifest;
		_pManifest = NULL;
	}

	// Delete the fragment instance
	if (NULL != _pFragment) {
		delete _pFragment;
		_pFragment = NULL;
	}

	// Delete instance of ABST atom
	if (NULL != _pAbstBox) {
		delete _pAbstBox;
		_pAbstBox = NULL;
	}

	if (deleteFiles) {
		// Delete the files
		deleteFolder(_targetFolder, true);
	}

	GetEventLogger()->LogStreamClosed(this);
}

void OutFileHDSStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseOutStream::GetStats(info, namespaceId);
	info["hdsSettings"] = _settings;
}

bool OutFileHDSStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_FILE)
            || TAG_KIND_OF(type, ST_IN_NET_EXT);
}

bool OutFileHDSStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutFileHDSStream::SignalPause() {
	return true;
}

bool OutFileHDSStream::SignalResume() {
	return true;
}

bool OutFileHDSStream::SignalSeek(double &dts) {
	return true;
}

bool OutFileHDSStream::SignalStop() {
	return true;
}

void OutFileHDSStream::SignalAttachedToInStream() {
}

void OutFileHDSStream::SignalDetachedFromInStream() {
	_pProtocol->EnqueueForDelete();
}

void OutFileHDSStream::SignalStreamCompleted() {
	_pProtocol->EnqueueForDelete();
}

void OutFileHDSStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
	if (_pFragment != NULL)
		_pFragment->ForceClose();
}

void OutFileHDSStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);

	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}

	if (_pFragment != NULL)
		_pFragment->ForceClose();
}

bool OutFileHDSStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
	uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
	packetsCount++;
	bytesCount += dataLength;
	return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
}

bool OutFileHDSStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	_pGenericProcessDataSetup = pGenericProcessDataSetup;

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_SIZE, //naluMarkerType,
			false, //insertPDNALU,
			true, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
	_pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
	_pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = true;

	//misc setup
	_pGenericProcessDataSetup->_timeBase = 0;
	_pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

	//Get the audio/video rates which will be later used to
	//open fragments
	if (_pGenericProcessDataSetup->_hasAudio) {
		_audioRate = _pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec()->_samplingRate;
	}
	if (_pGenericProcessDataSetup->_hasVideo) {
		_videoRate = 30;
	}

	//FINEST("\n%s", STR(_settings.ToString()));
	_chunkOnIDR = (bool) _settings["chunkOnIDR"];
	_chunkLength = ((double) _settings["chunkLength"])*1000.0;
	_chunkBaseName = (string) _settings["chunkBaseName"];

	// Create the correct path for the manifest file and fragments
	_targetFolder = (string) _settings["targetFolder"];
	if (_targetFolder[_targetFolder.size() - 1] != PATH_SEPARATOR) {
		_targetFolder += PATH_SEPARATOR;
		_settings["targetFolder"] = _targetFolder;
	}

	if (!createFolder(_targetFolder, true)) {
		FATAL("Unable to create target folder %s", STR(_targetFolder));
		return false;
	}

	// Initialize the rest
	_overwriteDestination = (bool) _settings["overwriteDestination"];
	_isRolling = (bool) (_settings["playlistType"] == "rolling");
	_playlistLength = (uint32_t) _settings["playlistLength"];
	//DEBUG("_playlistLength %"PRIu32, _playlistLength);
	_staleRetentionCount = (uint32_t) _settings["staleRetentionCount"];
	//DEBUG("_staleRetentionCount %"PRIu32, _staleRetentionCount);

	// Initialize the manifest file
	if (NULL == _pManifest) {
		_pManifest = new HDSManifest();
		
		// Set the bootstrap name here before the actual initialization
		_pManifest->SetBootStrapName(_bootstrapFilename);

		if (!_pManifest->Init(_settings, _pGenericProcessDataSetup->_pStreamCapabilities)) {
			FATAL("Unable to initialize Manifest Writer");

			return false;
		}

		_pManifest->CreateDefault();

		GetEventLogger()->LogHDSMasterPlaylistUpdated(_pManifest->GetMasterManifestPath());
		GetEventLogger()->LogHDSChildPlaylistUpdated(_pManifest->GetManifestPath());
	}

	// Initialize the instance of ABST atom/Bootstrap
	if (NULL == _pAbstBox) {
		_pAbstBox = new Atom_abst;
		// Default initialization
		_pAbstBox->Initialize();

		// Indicate if this is a rolling type along with the rolling limit
		_pAbstBox->SetRolling(_isRolling, _playlistLength);

		// Set the bootstrap filepath
		_bootstrapFilePath = _targetFolder + _bootstrapFilename;

		// Set the fragment path
		string path = _targetFolder + _chunkBaseName + "Seg1";
		_pAbstBox->SetPath(path);
	}

	return true;
}

bool OutFileHDSStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	//1. no fragment opened
	if (_pFragment == NULL) {
		//2. skip frames until the first key frame if necessary
		if ((_chunkOnIDR) && (!isKeyFrame))
			return true;
		//3. Open the fragment
		if (!OpenFragment(pts, dts, 1.04)) {
			FATAL("Unable to open fragment");
			return false;
		}
	}

	//4. Get the duration
	double duration = _pFragment->Duration();

	//5. Check and see if we must close the fragment because no more frames
	//can be appended to it
	if (_pFragment->MustClose()) {
		//6. compute the force flag. One of the following conditions should be true
		// - _chunkOnIDR is true and the current frame is not an IDR
		// - accumulated duration is smaller than _chunkLength
		//forced flag will be used to automatically adjust the additionalFramesPercent
		//when true, additionalFramesPercent will be increased by 1 percent from
		//the previous value
		bool forced = false;
		forced |= ((_chunkOnIDR) && (!isKeyFrame));
		forced |= (duration <= _chunkLength);

		//7. create the new fragment
		if (!NewFragment(pts, dts, forced)) {
			FATAL("Unable to open new fragment");
			return false;
		}
	} else {
		//8. inspect the duration and decide if we can TRY to open a new chunk
		if (duration >= _chunkLength) {
			//9. Check and see if we need to chunk on IDR
			if (_chunkOnIDR) {
				//10. Check and see if the current frame is an IDR. If is not,
				//than we continue on this chunk
				if (isKeyFrame) {
					if (!NewFragment(pts, dts, false)) {
						FATAL("Unable to open new fragment");
						return false;
					}
				}
			} else {
				//11. We don't need to chunk on IDR, so we chunk anyway because
				//the duration condition is met
				if (!NewFragment(pts, dts, false)) {
					FATAL("Unable to open new fragment");
					return false;
				}
			}
		}
	}

	// Check if we have the video codecs/sequence headers not sent yet
	if (!_videoCodecsSent) {
		VideoCodecInfoH264 *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
		IOBuffer &temp = pInfo->GetRTMPRepresentation();
		if (!_pFragment->PushVideoData(temp, pts, dts, true)) {
			FATAL("Unable to write Video sequence headers!");
			return false;
		}

		_videoCodecsSent = true;
	}

	//12. Push the data into the fragment
	if (_pFragment->PushVideoData(buffer, pts, dts, isKeyFrame)) {
		return true;
	} else {
		GetEventLogger()->LogHDSChunkError("Could not write video sample to " + _pFragment->GetPath());
		return false;
	}
}

bool OutFileHDSStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	// Store the last audio TS
	//_lastAudioTS = dts;

	//1. no fragment opened
	if (_pFragment == NULL) {
		//2. skip all the frames if we need to _chunkOnIDR but ONLY when
		//video track is present. Basically, when video is present AND _chunkOnIDR,
		//is true, all audio frames will be dropped until the fragment is opened
		//by the PushVideoData logic
		if ((_chunkOnIDR) && (_pGenericProcessDataSetup->_hasVideo))
			return true;
		//3. Open the fragment
		if (!OpenFragment(pts, dts, 1.04)) {
			FATAL("Unable to open fragment");
			return false;
		}
	}

	//4. Get the duration
	double duration = _pFragment->Duration();

	//5. Check and see if we must close the fragment because no more frames
	//can be appended to it
	if (_pFragment->MustClose()) {
		//6. compute the force flag. One of the following conditions should be true
		// - _chunkOnIDR is true and we have a video track
		// - accumulated duration is smaller than _chunkLength
		//forced flag will be used to automatically adjust the additionalFramesPercent
		//when true, additionalFramesPercent will be increased by 1 percent from
		//the previous value
		bool forced = false;
		forced |= ((_chunkOnIDR) && (_pGenericProcessDataSetup->_hasVideo));
		forced |= (duration <= _chunkLength);

		//7. create the new fragment
		if (!NewFragment(pts, dts, forced)) {
			FATAL("Unable to open new fragment");
			return false;
		}
	} else {
		//8. inspect the duration and decide if we can TRY to open a new chunk
		if (duration >= _chunkLength) {
			//9. Check and see if we need to chunk on IDR
			if (_chunkOnIDR) {
				//10. Only open a new track if we don't have a video track
				if (!_pGenericProcessDataSetup->_hasVideo) {
					if (!NewFragment(pts, dts, false)) {
						FATAL("Unable to open new fragment");
						return false;
					}
				}
			} else {
				//11. We don't need to chunk on IDR, so we chunk anyway because
				//the duration condition is met
				if (!NewFragment(pts, dts, false)) {
					FATAL("Unable to open new fragment");
					return false;
				}
			}
		}
	}

	// Check if we have the video codecs/sequence headers not sent yet
	if (!_audioCodecsSent) {
		AudioCodecInfoAAC *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
		IOBuffer &temp = pInfo->GetRTMPRepresentation();
		if (!_pFragment->PushAudioData(temp, pts, dts)) {
			FATAL("Unable to write Audio sequence headers!");
			return false;
		}

		_audioCodecsSent = true;
	}

	//12. Push the data into the fragment
	if (_pFragment->PushAudioData(buffer, pts, dts)) {
		return true;
	} else {
		GetEventLogger()->LogHDSChunkError("Could not write audio sample to " + _pFragment->GetPath());
		return false;
	}
}

bool OutFileHDSStream::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264)
			|| (codec == CODEC_AUDIO_AAC)
			;
}

bool OutFileHDSStream::OpenFragment(double pts, double dts, double additionalFramesPercent) {
	//1. Check and see if we have a fragment already opened
	if (_pFragment != NULL) {
		FATAL("Fragment already opened");
		GetEventLogger()->LogHDSChunkError("Opening a fragment but is already open.");
		return false;
	}

	//2. Compute the fragment path
	string fragmentFullPath = format("%s%sSeg1-Frag%"PRIu32,
			STR(_targetFolder),
			STR(_chunkBaseName),
			_fragmentSequence);

	// Test if we can overwrite the fragment
	if (fileExists(fragmentFullPath)) {
		if (_overwriteDestination) {
			if (!deleteFile(fragmentFullPath)) {
				GetEventLogger()->LogHDSChunkError("Could not delete " + fragmentFullPath);
				return false;
			}
		} else {
			string error = format("Fragment file %s already present and overwrite is not permitted!", STR(fragmentFullPath));
			FATAL("%s", STR(error));
			GetEventLogger()->LogHDSChunkError(error);
			return false;
		}
	}

	//DEBUG("Creating fragment: %s", STR(fragmentFullPath));
	//3. Create the fragment
	_pFragment = new HDSFragment(fragmentFullPath,
			_fragmentSequence++,
			_pAbstBox,
			_chunkLength / 1000.0,
			_audioRate,
			_videoRate,
			additionalFramesPercent,
			pts,
			dts);

	//4. Do initialization
	if (!_pFragment->Initialize()) {
		string error = "Unable to initialize " + fragmentFullPath;
		FATAL("%s", STR(error));
		GetEventLogger()->LogHDSChunkError(error);
		return false;
	}

	// Insert the sequence headers/codec setup
	//	if (!InsertSequenceHeaders(dts, _lastAudioTS)) {
	//		FATAL("No sequence headers written!");
	//		return false;
	//	}
	// Transferred to individual PushAudio/Video Data methods

	// Reset these flags for each new fragment
	_audioCodecsSent = false;
	_videoCodecsSent = false;

	//5. Done
	//FINEST(" Open %s", STR(*_pFragment));

	// Record the first timestamp of this fragment
	_fragmentStartupTime = dts;

	if (_isRolling) {
		_retainedFragments.push(fragmentFullPath);
		//DEBUG("Size: %"PRIz"u", _retainedFragments.size());
		while (_retainedFragments.size() > _playlistLength + _staleRetentionCount + 1) {
			string temp = _retainedFragments.front();
			//DEBUG("temp: %s", STR(temp));
			if (!deleteFile(temp)) {
				WARN("Unable to delete file %s", STR(temp));
			}

			_retainedFragments.pop();
		}
	}

	_chunkInfo["file"] = fragmentFullPath;
	_chunkInfo["startTime"] = _pFragment->GetStartTime();
	GetEventLogger()->LogHDSChunkCreated(_chunkInfo);

	return true;
}

bool OutFileHDSStream::CloseFragment(double lastTS) {

	//1. Check and see if no fragment is currently opened
	if (_pFragment == NULL) {
		FATAL("Fragment not opened");
		GetEventLogger()->LogHDSChunkError("Closing a fragment that was not opened.");
		return false;
	}

	// Update the bitrate if necessary
	// Don't update if this is the last fragment, since at this point, we
	// might not have a valid stream capabilities
	//TODO: need to check exactly why this is the case
	if (!_isLastFragment && _pManifest->AdjustBitRate()) {
		GetEventLogger()->LogHDSMasterPlaylistUpdated(_pManifest->GetMasterManifestPath());
		GetEventLogger()->LogHDSChildPlaylistUpdated(_pManifest->GetManifestPath());
	}

	// Update the ABST
	if (_isFirstFragment) {
		_pFragment->UpdateAbst(true, lastTS);
		_isFirstFragment = false;
	} else {
		_pFragment->UpdateAbst(false, lastTS);
	}

	// Check if this is the last fragment to be written
	if (_isLastFragment) {
		_pFragment->FinalizeAbst();
	}

	/*
	 * Complete the fragment, this might be the one causing the issue that 
	 * is seeing. My hunch would be that the fragments are not properly finalized
	 * and closed on the destructor.
	 * So force it by calling a method.
	 */
	_pFragment->Finalize();

	_chunkInfo["file"] = _pFragment->GetPath();
	_chunkInfo["startTime"] = _pFragment->GetStartTime();
	_chunkInfo["stopTime"] = Variant::Now();
	_chunkInfo["frameCount"] = _pFragment->GetFrameCount();
	GetEventLogger()->LogHDSChunkClosed(_chunkInfo);

	//2. Delete the fragment
	//FINEST("Close %s", STR(*_pFragment));
	delete _pFragment;
	_pFragment = NULL;

	//3. Done
	return true;
}

bool OutFileHDSStream::NewFragment(double pts, double dts, bool forced) {
	//1. Check and see if we have a fragment opened
	if (_pFragment == NULL) {
		FATAL("Fragment not opened");
		GetEventLogger()->LogHDSChunkError("Current fragment is not open.");
		return false;
	}

	//2. Get the string representation in case we need to report an error
	string temp = (string) * _pFragment;

	//3. Get the current additionalFramesPercent from the current frame
	//we will use this as abase for the new value used in the new fragment
	//we are going to open
	double additionalFramesPercent = _pFragment->AdditionalFramesPercent();

	//4. If this current fragment was closed by force (no more frames slots)
	//than increase the additionalFramesPercent by 0.5%
	if (forced) {
		//WARN("additionalFramesPercent bumped up: %.2f%% -> %.2f%%",
		//		additionalFramesPercent * 100.0,
		//		(additionalFramesPercent + 0.005)*100.0);
		additionalFramesPercent += 0.005;
	}

	//5. Close the fragment
	if (!CloseFragment(dts)) {
		string error = "Unable to close " + _pFragment->GetPath();
		FATAL("%s", STR(error));
		GetEventLogger()->LogHDSChunkError(error);
		return false;
	}

	// Update the bootstrap file
	_pAbstBox->WriteBootStrap(_bootstrapFilePath);

	//DEBUG("Closing fragment: %"PRIu32, _fragmentSequence);

	// Boundary check
	if ((_fragmentSequence % 65536) == 0) {
		_fragmentSequence = 1;
	}

	// Boundary check
	if ((_fragmentSequence % 65536) == 0) {
		_fragmentSequence = 1;
	}

	//7. Open a new one
	return OpenFragment(pts, dts, additionalFramesPercent);
}

bool OutFileHDSStream::InsertSequenceHeaders(double pts, double dts) {
	if (_pGenericProcessDataSetup->_hasVideo) {
		VideoCodecInfoH264 *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
		IOBuffer &temp = pInfo->GetRTMPRepresentation();
		if (!_pFragment->PushVideoData(temp, pts, dts, true)) {
			FATAL("Unable to write Video sequence headers!");
			return false;
		}
	}

	if (_pGenericProcessDataSetup->_hasAudio) {
		AudioCodecInfoAAC *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
		IOBuffer &temp = pInfo->GetRTMPRepresentation();
		if (!_pFragment->PushAudioData(temp, pts, dts)) {
			FATAL("Unable to write Audio sequence headers!");
			return false;
		}
	}

	return true;
}

#endif	/* HAS_PROTOCOL_HDS */
