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


#ifdef HAS_PROTOCOL_HLS
#include "streaming/hls/hlsplaylist.h"
#include "streaming/hls/filetssegment.h"
#include "streaming/hls/fileaacsegment.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "streaming/streamdebug.h"
#include "protocols/drm/drmdefines.h"

#define HLS_HAS_VIDEO (_pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)
#ifdef HAS_G711
#define HLS_HAS_AUDIO ((_pCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC) || ((_pCapabilities->GetAudioCodecType() & CODEC_AUDIO_G711) == CODEC_AUDIO_G711))
#else
#define HLS_HAS_AUDIO (_pCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC)
#endif	/* HAS_G711 */
#define SEGMENT_SEQUENCE(s) ((((s)%65536)==0)?1:(((s)%65536)))
#define HLS_VERSION(x) format("#EXT-X-VERSION:%"PRIu32"\r\n",x)

HLSPlaylist::HLSPlaylist() {
	_audioOnly = false;
	_cleanupOnClose = false;
	_createMasterPlaylist = false;
	_staleRetentionCount = 0;
	_overwriteDestination = false;
	_instreamDetached = false;
	_playlistLength = 0;
	_chunkLength = 0;
    _targetDuration = 0;
	_appending = false;
	_pCurrentSegment = NULL;
	_segmentStartTs = -1;
    _firstSegmentStartTs = 0;
    _fileStartTs = 0;
	_pCapabilities = NULL;
	_segmentSequence = 1;
	_audioCount = 0;
	_audioStart = -1;
	_lastAudioComputedTs = 0;
	GETCLOCKS(_playlistCreationTime, double);
	_playlistCreationTime = _playlistCreationTime / (double) CLOCKS_PER_SECOND * 1000.0;
	_groupTimestamp = _playlistCreationTime;
	_masterPlaylistInitialized = false;
	_waitForKeyFrame = true;
	_segmentLastTs = 0;
	_currentKeyCount = 0;
	_drmType = "";
#ifdef HAS_PROTOCOL_DRM
	_drmKey = "";
	_drmId = 0;
	_pDRMQueue = NULL;
	_keyURL = "";
#endif /* HAS_PROTOCOL_DRM */
	memset(_currentKey, 0, sizeof (_currentKey));
	memset(_currentIV, 0, sizeof (_currentIV));
	_pEventLogger = NULL;
	_discontinuity = false;
	_hasMilliseconds = false;
	_maxChunkLength = 0;
	_useSystemTime = false;
	_offsetTime = 0;
	_version = 2;
	_playlistTypeCursor = 0;
	_hasMilliseconds = false;
	_minimumChunkSize = 1000;
    _targetDurationCursor = 0;
    _useByteRange = false;
    _fileLength = 0;
    _isLastFragment = false;
    _playlistSequence = 1;
	_startOffset = 0;
	_maxChunkSize = 0;
#ifdef HAS_STREAM_DEBUG
	_pStreamDebugFile = NULL;
#endif /* HAS_STREAM_DEBUG */
}

HLSPlaylist::~HLSPlaylist() {
    _isLastFragment = true;
	CloseSegment(true);
	ClosePlaylist();
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL) {
		delete _pStreamDebugFile;
		_pStreamDebugFile = NULL;
	}
#endif /* HAS_STREAM_DEBUG */
	//playlist files deleted if corresponding stream is removed (cleanupOnClose must be set to true)
	if (_cleanupOnClose && _instreamDetached) {
		if (fileExists(_targetFolder)) {
			deleteFolder(_targetFolder, true);
			deleteFile(_targetFolder);
		}
		//remove also from Master Playlist
		// Parse the playlist
		map<string, string> playlist = ParsePlaylist(_masterPlaylistPath);
		if (playlist.size() != 0) {
			// Compute the child playlist path
			string uri = format("%s/%s",
					STR(_localStreamName),
					STR(_playlistName));

			// Remove it from the playlist
			playlist.erase(uri);
		}

		if (playlist.size() == 0) {
			//if the previously deleted child playlist is the last one, delete the master playlist.
			deleteFile(_masterPlaylistPath);
		} else {
			// Save back the playlist
			if (!SaveMasterPlaylist(playlist, _masterPlaylistPath)) {
				WARN("Unable to save master playlist %s", STR(_masterPlaylistPath));
			}
		}
	}
}

string HLSPlaylist::GetSystemTime() {
	time_t now = time(NULL);
	struct tm tim = *(localtime(&now));
	char s[30];
	strftime(s, 30, "%Y-%m-%dT%H-%M-%S", &tim);
	return (string) s;
}
void HLSPlaylist::SetVersion(uint8_t version) {
	_version = version;
	//EXT-X-VERSION: see https://developer.apple.com/library/ios/technotes/tn2288/_index.html
	_hasMilliseconds = (version >= 3);
	_hasPlaylistType = (version >= 3);
}
bool HLSPlaylist::Init(Variant &settings, StreamCapabilities *pCapabilities,
		string streamInfo, uint32_t streamId) { 
	_groupTimestamp = (double) settings["timestamp"];
#ifdef HAS_STREAM_DEBUG
	_pStreamDebugFile = new StreamDebugFile(format("./hls_%"PRIu32".bin", streamId),
			sizeof (StreamDebugHLS));
#endif /* HAS_STREAM_DEBUG */
	_streamInfo = streamInfo;
	_pCapabilities = pCapabilities;
	if ((_pCapabilities->GetAudioCodecType() != CODEC_AUDIO_AAC)
			&& (_pCapabilities->GetVideoCodecType() != CODEC_VIDEO_H264)) {
		FATAL("In stream is not transporting H.264/AAC content");
		return false;
	}

	_targetFolder = (string) settings["targetFolder"];
	if (_targetFolder[_targetFolder.size() - 1] != PATH_SEPARATOR)
		_targetFolder += PATH_SEPARATOR;
	if (!createFolder(_targetFolder, true)) {
		FATAL("Unable to create target folder %s", STR(_targetFolder));
		return false;
	}
	//FINEST("_targetFolder: %s", STR(_targetFolder));

	_overwriteDestination = (bool) settings["overwriteDestination"];
	//FINEST("_overwriteDestination: %d", _overwriteDestination);

	_playlistPath = _targetFolder + (string) settings["playlistName"];
	//FINEST("_playlistPath: %s", STR(_playlistPath));
    
	_hlsResume = (bool)settings["hlsResume"];

	if (fileExists(_playlistPath)) {
		if (_overwriteDestination) {
			if (!_hlsResume) {
				//if hls resume enabled, don't delete yet
				if (!deleteFile(_playlistPath)) {
					return false;
				}
			}
		} else {
			FATAL("playlist file %s already present and overwrite is not permitted", STR(_playlistPath));
			return false;
		}
	}

	_chunkBaseName = (string) settings["chunkBaseName"];
	_origChunkBaseName = _chunkBaseName;
	_chunkBaseName += format("_%"PRIu64, (uint64_t) _playlistCreationTime);
	//FINEST("_chunkBaseName: %s", STR(_chunkBaseName));

	_playlistLength = (uint32_t) settings["playlistLength"];
	//FINEST("_playlistLength: %"PRIu32, _playlistLength);

	_staleRetentionCount = (uint32_t) settings["staleRetentionCount"];
	//FINEST("_staleRetentionCount: %"PRIu32, _staleRetentionCount);

	_chunkLength = (uint32_t) settings["chunkLength"];
	//FINEST("_chunkLength: %"PRIu32, _chunkLength);

	_appending = (string) settings["playlistType"] == "appending";
	//FINEST("_appending: %d", _appending);

	_chunkOnIDR = (bool) settings["chunkOnIDR"];
    _waitForKeyFrame = _chunkOnIDR;
	//FINEST("_chunkOnIDR: %d", _chunkOnIDR);

	_masterPlaylistPath = (string) settings["masterPlaylistPath"];
	//FINEST("_masterPlaylistPath: %s", STR(_masterPlaylistPath));

	_localStreamName = (string) settings["localStreamName"];
	//FINEST("_localStreamName: %s", STR(_localStreamName));

	_playlistName = (string) settings["playlistName"];
	//FINEST("_playlistName: %s", STR(_playlistName));

	_bandwidth = (uint32_t) settings["bandwidth"];
	//FINEST("_bandwidth: %"PRIu32, _bandwidth);

	_createMasterPlaylist = (bool) settings["createMasterPlaylist"];
	//FINEST("_createMasterPlaylist: %d", _createMasterPlaylist);

	_cleanupDestination = (bool) settings["cleanupDestination"];
	//FINEST("_cleanupDestination: %d", _cleanupDestination);

	_maxChunkLength = (uint32_t) settings["maxChunkLength"];
	_maxChunkSize = _maxChunkLength * PAYLOAD_LENGTH_FACTOR;

	_useSystemTime = (bool) settings["useSystemTime"];
	//_useSystemTime = (bool) true;

	_offsetTime = (uint32_t) settings["offsetTime"];

	_cleanupOnClose = (bool) settings["cleanupOnClose"];
    _useByteRange = (bool) settings["useByteRange"];
    _fileLength = (uint32_t) settings["fileLength"];
	_startOffset = (uint32_t) settings["startOffset"];

#ifdef HAS_PROTOCOL_DRM
	_drmType = (string) settings["drmType"];
	//FINEST("DRMType=%s", _drmType.c_str());
	if (_drmType == DRM_TYPE_NONE) {
		_drmType = "";
	}
	if ((_drmType == DRM_TYPE_VERIMATRIX) &&
			(_pDRMQueue == NULL)) {
		FATAL("Encryption with Verimatrix DRM was requested but not configured. Using RMS DRM.");
		_drmType = DRM_TYPE_RMS;
	}
#else /* HAS_PROTOCOL_DRM */
	_drmType = (bool) settings["encryptStream"] ? DRM_TYPE_RMS : DRM_TYPE_NONE;
#endif /* HAS_PROTOCOL_DRM */
//	if ((_drmType == DRM_TYPE_NONE) || (_drmType == "")) {
//		_version = 2;
//	}

	_AESKeyCount = (uint32_t) settings["AESKeyCount"];
	if (_AESKeyCount < 1) {
		_AESKeyCount = 1;
	}
	//FINEST("_AESKeyCount: %"PRIu32, _AESKeyCount);
	
	SetVersion((uint8_t) settings["hlsVersion"]);

	if (_cleanupDestination) {
		// delete the files one at a time (wildcard doesn't work)
		vector<string> files;
		if (listFolder(_targetFolder, files)) {

			FOR_VECTOR_ITERATOR(string, files, i) {
				string &file = VECTOR_VAL(i);
				//FINEST("Delete file %s",STR(file));
				string name, extension;
				splitFileName(file, name, extension);
				extension = lowerCase(extension);
				if ((extension == "ts") || (extension == "m3u8") 
					|| (extension == "key") || (extension == "aac")) {
					deleteFile(file);
				}
			}
		}
	}

	_audioOnly = (bool)settings["audioOnly"];

	if (fileExists(_playlistPath) && _hlsResume) {
		if (!AppendToChildPlaylist(_playlistFile)) {
			FATAL("Unable to open appending playlist");
			return false;
		}
	} 

	if (!InitMasterPlaylist()) {
		FATAL("Unable to init master playlist");
		return false;
	}

	return true;
}

bool HLSPlaylist::PushVideoData(IOBuffer &buffer, double pts, double dts, Variant& videoParamsSizes) {
	if (_audioOnly) {
		_waitForKeyFrame = false;
		return true;
	}
	if (_waitForKeyFrame) {
		if (!(bool)videoParamsSizes["isKeyFrame"])
			return true;
		_waitForKeyFrame = false;
	}
	if (_pCurrentSegment == NULL) {
		if (!OpenSegment(pts, dts)) {
			FATAL("Unable to open segment");
			return false;
		}
	}
	double currentChunkLength = pts - _segmentStartTs;

	if (_chunkOnIDR && ForceNewChunk(pts)) {
		if (!CreateNewSegment(pts, dts)) {
			FATAL("Unable to create new segment");
			return false;
		}
	} else if ((currentChunkLength) >= (_chunkLength * 1000.0)) {
		if (_chunkOnIDR) {
			if ((bool)videoParamsSizes["isKeyFrame"]) {
				if (!CreateNewSegment(pts, dts)) {
					FATAL("Unable to create new segment");
					return false;
				}
			}
		} else {
			if (!CreateNewSegment(pts, dts)) {
				FATAL("Unable to create new segment");
				return false;
			}
		}
	} 
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL)
		_pStreamDebugFile->PushHLS(false, pts, dts, 0);
#endif /* HAS_STREAM_DEBUG */
	if (_pCurrentSegment->PushVideoData(buffer, (int64_t)(pts * 90.0), (int64_t)(dts * 90.0), videoParamsSizes)) {
		_segmentLastTs = _segmentLastTs < pts ? pts : _segmentLastTs;
		return true;
	} else {
		_pEventLogger->LogHLSChunkError("Could not write video sample to " + _pCurrentSegment->GetPath());
		return false;
	}
}

bool HLSPlaylist::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	double currentChunkLength = pts - _segmentStartTs;
	if ((!_chunkOnIDR) || (!HLS_HAS_VIDEO) || (_audioOnly)) {
		if (_chunkOnIDR && ForceNewChunk(pts)) {
			if (!CreateNewSegment(pts, dts)) {
				FATAL("Unable to create new segment");
				return false;
			}
		} else if ((currentChunkLength) >= (_chunkLength * 1000.0)) {
			if (!CreateNewSegment(pts, dts)) {
				FATAL("Unable to create new segment");
				return false;
			}
		} 
	} else {
		if (_waitForKeyFrame)
			return true;
	}
	if (_audioStart < 0)
		_audioStart = pts;
	double computedTs = _audioStart + ((((double) _audioCount * 1024.0) /
			(double) _pCapabilities->GetAudioCodec()->_samplingRate)) * 1000.0;
	if (_lastAudioComputedTs > computedTs) {
		WARN("Back timestamp");
		computedTs = _lastAudioComputedTs;
	}
	if (_pCurrentSegment == NULL) {
		if (!OpenSegment(computedTs, dts)) {
			FATAL("Unable to open segment");
			return false;
		}
	}
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL)
		_pStreamDebugFile->PushHLS(true, computedTs, computedTs, 0);
#endif /* HAS_STREAM_DEBUG */
	if (!_pCurrentSegment->PushAudioData(buffer, (int64_t) (computedTs * 90.0), (int64_t) (computedTs * 90.0))) {
		FATAL("Unable to feed audio data");
		_pEventLogger->LogHLSChunkError("Could not write audio sample to " + _pCurrentSegment->GetPath());
		return false;
	}
	_segmentLastTs = _segmentLastTs < computedTs ? computedTs : _segmentLastTs;

	_lastAudioComputedTs = computedTs;
	_audioCount++;
	if (computedTs - _audioStart >= 10000) {
		//FINEST("Sync audio");
		_audioCount = 0;
		_audioStart = -1;
	}

	return true;
}

bool HLSPlaylist::PushMetaData(string const& vmfMetadata, int64_t pts) {
	if (_pCurrentSegment == NULL) {
		FATAL("An existing segment is needed before a metadata can be written");
		return false;
	}

	if (!_pCurrentSegment->PushMetaData(vmfMetadata, pts * 90)) {
		FATAL("Unable to feed metadata");
		return false;
	}
	return true;
}

void HLSPlaylist::RemoveFromMasterPlaylist(Variant &streamConfig) {
	if (!(bool)streamConfig["createMasterPlaylist"]) {
		return;
	}

	//1. Get the master playlist path
	string masterPlaylistPath = (string) streamConfig["masterPlaylistPath"];

	//2. Parse the playlist
	map<string, string> playlist = ParsePlaylist(masterPlaylistPath);
	if (playlist.size() <= 1)
		return;

	//3. Compute the child playlist path
	string uri = format("%s/%s",
			STR(streamConfig["localStreamName"]),
			STR(streamConfig["playlistName"]));

	//4. Remove it from the playlist
	playlist.erase(uri);

	//5. Save back the playlist
	if (!SaveMasterPlaylist(playlist, masterPlaylistPath)) {
		WARN("Unable to save master playlist %s", STR(masterPlaylistPath));
	}
}

void HLSPlaylist::ForceCut() {
	if (_pCurrentSegment == NULL)
		return;
	double pts = _segmentLastTs;
	if (pts < 0)
		pts = _lastAudioComputedTs;
	if (pts < 0)
		pts = 0;
	_segmentPatPmtAndCountersBuffer.IgnoreAll();
	CreateNewSegment(pts, pts);
}

void HLSPlaylist::SetEventLogger(EventLogger *eventLogger) {
	_pEventLogger = eventLogger;
}

#ifdef HAS_PROTOCOL_DRM

bool HLSPlaylist::SetDRMQueue(DRMKeys *pDRMQueue) {
	if ((_pDRMQueue != NULL) ||
			(pDRMQueue == NULL)) {
		return false;
	}
	_pDRMQueue = pDRMQueue;
	_pDRMQueue->CreateDRMEntry(); //create for next stream
	uint32_t temp = _pDRMQueue->GetUnusedId();
	if (temp == 0) {
		return false;
	}
	_drmId = temp;
	return true;
}

#endif /* HAS_PROTOCOL_DRM */

bool HLSPlaylist::PrepareNewKey() {
	if (_drmType == "") {
		FATAL("Invalid DRM type");
		return false;
	}
#ifdef HAS_PROTOCOL_DRM
	_drmKey = "";
	_keyURL = "";
	if (_drmType == DRM_TYPE_VERIMATRIX) {
		//Create new key for next segment
		if (_drmId == 0) {
			FATAL("Invalid DRM ID");
			return false;
		}
		DRMKeys::_DRMEntry entry = _pDRMQueue->GetKey(_drmId);
		_drmKey = entry._key;
		if (_drmKey.size() != 16) {
			FATAL("Invalid DRM key: %s (id=%d, length=%d, #keys=%d, #entries=%d)", _drmKey.c_str(),
					_drmId, (uint32_t) _drmKey.size(), _pDRMQueue->GetKeyCount(_drmId), _pDRMQueue->GetDRMEntryCount());
			return false;
		}
		//FINEST("id=%d, key=%s, loc=%s, pos=%d", _drmId, STR(entry._key), STR(entry._location), entry._position);

		memcpy(_currentKey, _drmKey.c_str(), 16); //use key received from Verimatrix

		//Prepare URL for key
		//example: https://74.62.179.10/CAB/keyfile?r=22&t=VOD&p=37
		_keyURL = format("%s?r=%"PRIu32"&t=VOD&p=%"PRIu32"", STR(entry._location), _drmId, entry._position);
	} else {
#else /* HAS_PROTOCOL_DRM */
	{
#endif /* HAS_PROTOCOL_DRM */
		//Create new key for next segment
		if (!_pCurrentSegment->CreateKey(_currentKey, _currentIV)) {
			FATAL("Unable to create key");
			_pEventLogger->LogHLSChunkError("Unable to create key for " + _pCurrentSegment->GetPath());
			return false;
		}

		//Set filename for key file
		_keyFilename = GetSystemTime() + ".key";
		string pathKey = _targetFolder + _keyFilename;

		//Save key to file for next segment
		File file;
		if (!file.Initialize(pathKey, FILE_OPEN_MODE_TRUNCATE)) {
			FATAL("Unable to open file %s", STR(pathKey));
			return false;
		}
		if (!file.WriteBuffer(_currentKey, sizeof (_currentKey))) {
			string error = format("Unable to save key to file %s", STR(pathKey));
			FATAL("%s", STR(error));
			_pEventLogger->LogHLSChunkError(error);
			return false;
		}
	}
	return true;
}

bool HLSPlaylist::OpenSegment(double pts, double dts) {
	if (!_useByteRange && _pCurrentSegment != NULL) {
		FATAL("Segment already opened");
		_pEventLogger->LogHLSChunkError(_pCurrentSegment->GetPath() + " is already opened.");
		return false;
	}

	string segmentPath = _targetFolder + format(_audioOnly ? "%s_%"PRIu32".aac" : "%s_%"PRIu32".ts",
			STR(_chunkBaseName),
			_appending ? ((uint32_t) _segmentSequence) : SEGMENT_SEQUENCE(_segmentSequence));
	if (!_useByteRange && fileExists(segmentPath)) {
		if (_overwriteDestination) {
			if (!deleteFile(segmentPath)) {
				_pEventLogger->LogHLSChunkError("Cannot delete " + segmentPath);
				return false;
			}
		} else {
			string error = format("Segment file %s already present and overwrite is not permitted", STR(segmentPath));
			FATAL("%s", STR(error));
			_pEventLogger->LogHLSChunkError(error);
			return false;
		}
	}
    bool createFragment = false;
    if (_useByteRange) {
        if (NULL == _pCurrentSegment)
            createFragment = true;
    } else {
        createFragment = true;
    }

    if (createFragment) {
        if (_audioOnly)
            _pCurrentSegment = new FileAACSegment(_segmentAudioBuffer, _drmType);
        else
            _pCurrentSegment = new FileTSSegment(_segmentVideoBuffer, _segmentAudioBuffer,
                _segmentPatPmtAndCountersBuffer, _drmType);
    }

	if (_drmType != "") {
		_currentKeyCount = (_currentKeyCount + 1) % _AESKeyCount;
		if (_currentKeyCount == 1) {
			if (!PrepareNewKey()) {
				string error = format("No valid key for segment file %s", STR(segmentPath));
				FATAL("%s", STR(error));
				_pEventLogger->LogHLSChunkError(error);
				return false;
			}
		}
	}

	_settings["computedPathToFile"] = segmentPath;
    _settings["useByteRange"] = _useByteRange;

	if (!_pCurrentSegment->Init(_settings, _pCapabilities, _currentKey, _currentIV)) {
		FATAL("Unable to open segment");
		_pEventLogger->LogHLSChunkError("Unable to open " + segmentPath);
		return false;
	}
	_segmentStartTs = pts;
    if (_firstSegmentStartTs == 0)
        _firstSegmentStartTs = _segmentStartTs;

    if (_fileStartTs == 0)
        _fileStartTs = _segmentStartTs;

	_pEventLogger->LogHLSChunkCreated(segmentPath, _playlistCreationTime,
			_segmentStartTs);
	return true;
}

bool HLSPlaylist::CloseSegment(bool endOfPlaylist) {
	if (_pCurrentSegment == NULL) {
		WARN("Segment is not opened");
		return true;
	}

	GetBandwidth();
    _pCurrentSegment->FlushPacket();
    _settings["segmentSize"] = _pCurrentSegment->getCurrentFileSize() - (uint64_t) _settings["segmentStartOffset"];

	if (_appending) {
		if (!CloseSegmentAppending(endOfPlaylist)) {
			FATAL("Unable to close appending segment");
			_pEventLogger->LogHLSChunkError("Unable to close appending segment " + _pCurrentSegment->GetPath());
			return false;
		}
	} else {
		if (!CloseSegmentRolling(endOfPlaylist)) {
			FATAL("Unable to close rolling segment");
			_pEventLogger->LogHLSChunkError("Unable to close rolling segment " + _pCurrentSegment->GetPath());
			return false;
		}
	}

	_pEventLogger->LogHLSChunkClosed(_pCurrentSegment->GetPath(), _playlistCreationTime,
			_segmentStartTs, _segmentLastTs - _segmentStartTs);

    bool closeSegment = false;
    if (_useByteRange) {
       if (((_segmentLastTs - _fileStartTs) >= (_fileLength * 1000.0)) || _isLastFragment) {
           _fileStartTs = 0;
           closeSegment = true;
       }
    } else {
        closeSegment = true;
    }

	if (closeSegment && NULL != _pCurrentSegment) {
        delete _pCurrentSegment;
        _pCurrentSegment = NULL;
    }

    ++_playlistSequence;

    if (closeSegment) {
        _segmentSequence++;

        if ((_segmentSequence % 65536) == 0) {
            GETCLOCKS(_playlistCreationTime, double);
            _playlistCreationTime = _playlistCreationTime / (double) CLOCKS_PER_SECOND * 1000.0;
        }
    }
	return true;
}

void HLSPlaylist::movePlaylist(string const& src, string const& dest) {
	File srcFile;
	srcFile.Initialize(src, FILE_OPEN_MODE_READ);
	string tmpSrcContent;
	srcFile.ReadAll(tmpSrcContent);
	srcFile.Close();
	deleteFile(src);

	File destFile;
	destFile.Initialize(dest, FILE_OPEN_MODE_TRUNCATE);
	destFile.WriteString(tmpSrcContent);
	destFile.Flush();
	destFile.Close();
}

bool HLSPlaylist::CloseSegmentAppending(bool endOfPlaylist) {
	if (_segmentLastTs - _segmentStartTs < _minimumChunkSize) {
		WARN("Item number %"PRIu64" on stream %s skipped. Duration less than %.3fs: %.3fs",
				_segmentSequence,
				STR(_streamInfo),
				_minimumChunkSize / 1000.0,
				(_segmentLastTs - _segmentStartTs) / 1000.0);
		_segmentSequence--;
		_discontinuity = true;
		return true;
	}
	string segmentPath = format(_audioOnly ? "%s_%"PRIu32".aac" : "%s_%"PRIu32".ts",
			STR(_chunkBaseName),
			(uint32_t) _segmentSequence);
	if (!fileExists(_targetFolder + segmentPath)) {
		WARN("Segment was manually deleted from disk: %s", STR(_targetFolder + segmentPath));
		return true;
	}
	
	string tmpPlaylistPath = _playlistPath + ".tmp";
    double duration = _segmentLastTs - _segmentStartTs;
    if (_targetDuration < _chunkLength)
        _targetDuration = _chunkLength;
    if ((duration / 1000.0) > _targetDuration)
        _targetDuration = ((uint32_t)(duration / 1000.0)) + 1;

	if (!fileExists(_playlistPath)) {
		if (!_playlistFile.Initialize(tmpPlaylistPath, FILE_OPEN_MODE_TRUNCATE)) {
			FATAL("Unable to open appending playlist");
			return false;
		}

		string header = "";
		header += "#EXTM3U\r\n";
		header += HLS_VERSION(_version);
		if (_hasPlaylistType) {
			header += "#EXT-X-PLAYLIST-TYPE:EVENT\r\n";
			_playlistTypeCursor = _playlistFile.Cursor() + header.length() - 7;
		}
        _targetDurationCursor = _playlistFile.Cursor() + header.length() + 22;
		header += format("#EXT-X-TARGETDURATION:%"PRIu32"\r\n", _targetDuration);
		header += "#EXT-X-MEDIA-SEQUENCE:1\r\n";
		if (_startOffset != 0) {
			header += format("#EXT-X-START:TIME-OFFSET=%"PRIu32"\r\n", _startOffset);
		}
        header += GetProgramDateTime(_firstSegmentStartTs) + "\r\n";

		if (!_playlistFile.WriteString(header)) {
			FATAL("Unable to write appending file header");
			_playlistFile.Close();
			deleteFile(tmpPlaylistPath);
			return false;
		}

		if (!_masterPlaylistInitialized) {
			if (!InitMasterPlaylist()) {
				FATAL("Unable to init master playlist");
				_playlistFile.Close();
				deleteFile(tmpPlaylistPath);
				return false;
			}
		}
		_playlistFile.Close();
	} else {
		movePlaylist(_playlistPath, tmpPlaylistPath);
	}

	string item;
	if (_discontinuity) {
		item += "#EXT-X-DISCONTINUITY\r\n";
		//		item += format("#EXT-X-MEDIA-SEQUENCE:%"PRIu32"\r\n",
		//				(uint32_t) (SEGMENT_SEQUENCE(_segmentSequence)));
		_discontinuity = false;
		WARN("Discontinuity encountered on stream %s for chunk number %"PRIu32,
				STR(_streamInfo), (uint32_t) (SEGMENT_SEQUENCE(_segmentSequence)));
	}

	//Add "#EXT-X-KEY" tag in playlist
	if (_drmType != "") {
		if (_currentKeyCount == 1) { //new key has been generated
#ifdef HAS_PROTOCOL_DRM
			if (_drmType == DRM_TYPE_VERIMATRIX) {
				item += format("#EXT-X-KEY:METHOD=AES-128,URI=\"%s\",IV=0x%s\r\n",
						STR(_keyURL),
						STR(hex(_currentIV, 16)));
			} else if (_drmType == DRM_TYPE_RMS) {
				item += format("#EXT-X-KEY:METHOD=AES-128,URI=\"%s\",IV=0x%s\r\n",
						STR(_keyFilename),
						STR(hex(_currentIV, 16)));
			} else if (_drmType == DRM_TYPE_SAMPLE_AES) {
				item += format("#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"%s\",IV=0x%s\r\n",
						STR(_keyFilename),
						STR(hex(_currentIV, 16)));
			}
#else /* HAS_PROTOCOL_DRM */
			if (_drmType == DRM_TYPE_RMS) {
				item += format("#EXT-X-KEY:METHOD=AES-128,URI=\"%s\",IV=0x%s\r\n",
						STR(_keyFilename),
						STR(hex(_currentIV, 16)));
			} else if (_drmType == DRM_TYPE_SAMPLE_AES) {
				item += format("#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"%s\",IV=0x%s\r\n",
						STR(_keyFilename),
						STR(hex(_currentIV, 16)));
			}
#endif /* HAS_PROTOCOL_DRM */
		}
	}
	//End add "#EXT-X-KEY" tag

	if (_version >= 3)
		item += format("#EXTINF:%1.3f,\r\n", duration / 1000.0);
	else
		item += format("#EXTINF:%1.0f,\r\n", duration / 1000.0);

	if (_useByteRange) {
		uint64_t dataBytes = (uint64_t)_settings["segmentSize"];
		uint64_t offsetBytes = (uint64_t)_settings["segmentStartOffset"];
		item += format("#EXT-X-BYTERANGE:%"PRIu64"@%"PRIu64"\r\n", dataBytes, offsetBytes);
	}

	item += segmentPath + "\r\n";

	if (!_playlistFile.Initialize(tmpPlaylistPath, FILE_OPEN_MODE_APPEND)) {
		FATAL("Unable to open appending playlist");
		return false;
	}

	if (!_playlistFile.WriteString(item)) {
		FATAL("Unable to append item to appending playlist");
		return false;
	}

	_playlistFile.Close();
    //updates the EXT-X-TARGETDURATION
	if (!_playlistFile.Initialize(tmpPlaylistPath, FILE_OPEN_MODE_READ)) {
		FATAL("Unable to open appending playlist");
		return false;
	}
	string buf;
	if (!_playlistFile.ReadAll(buf)) {
		FATAL("Playlist filesize too large");
		_playlistFile.Close();
		return false;
	}

	string prevDuration;
	size_t newPos = buf.find("#EXT-X-MEDIA-SEQUENCE", (size_t)_targetDurationCursor);
	size_t len = newPos - (size_t)_targetDurationCursor;
	prevDuration = buf.substr((size_t)_targetDurationCursor, len);
    string newDuration = format("%"PRIu64"\r\n", _targetDuration);
	buf.replace((size_t)_targetDurationCursor, prevDuration.length(), newDuration);

	_playlistFile.Close();		
	if (!_playlistFile.Initialize(tmpPlaylistPath, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to read appending playlist");
		return false;
	}

	if (!_playlistFile.WriteString(buf)) {
		FATAL("Unable to write appending file header");
		_playlistFile.Close();
		return false;
	}
	_playlistFile.Close();

	//Reads the content of the temporary playlist then overwrite it to
	//the playlist
	movePlaylist(tmpPlaylistPath, _playlistPath);

	_pEventLogger->LogHLSChildPlaylistUpdated(_playlistPath);

	return true;
}

bool HLSPlaylist::CloseSegmentRolling(bool endOfPlaylist) {
	Variant item;
	double duration = _segmentLastTs - _segmentStartTs;
	item["duration"] = (double) (duration / 1000.0);
	if (_useByteRange) {
		item["dataBytes"] = _settings["segmentSize"];
		item["offsetBytes"] = _settings["segmentStartOffset"];
	}
	item["segmentStartTs"] = _segmentStartTs;
	item["realSequence"] = _segmentSequence;
	item["sequence"] = (uint32_t) (SEGMENT_SEQUENCE(_segmentSequence));
	item["name"] = format(_audioOnly ? "%s_%"PRIu32".aac": "%s_%"PRIu32".ts",
			STR(_chunkBaseName),
			(uint32_t) item["sequence"]);
	item["path"] = _targetFolder + (string) item["name"];

	if (duration < _minimumChunkSize) {
		WARN("Item number %"PRIu64" on stream %s skipped. Duration less than %.3fs: %.3fs",
			_segmentSequence,
			STR(_streamInfo),
			_minimumChunkSize / 1000.0,
			duration / 1000.0);
		_segmentSequence--;
		return true;
	}

	item["discontinuity"] = _discontinuity;
	_discontinuity = false;

	if (duration > 0) {
		if (_drmType != "") {
#ifdef HAS_PROTOCOL_DRM
			if (_drmType == DRM_TYPE_VERIMATRIX) {
				item["keyFileName"] = _keyURL;
			} else {
				item["keyFileName"] = _keyFilename;
			}
#else /* HAS_PROTOCOL_DRM */
			item["keyFileName"] = _keyFilename;
#endif /* HAS_PROTOCOL_DRM */
			item["isNewKey"] = (_currentKeyCount == 1) ? true : false;
			item["IV"] = hex(_currentIV, 16);
		}

		_rollingItems[_playlistSequence] = item;
		while (_rollingItems.size() > _playlistLength) {
			_staleItems[MAP_KEY(_rollingItems.begin())] = MAP_VAL(_rollingItems.begin());
			_rollingItems.erase(_rollingItems.begin());
		}
	} else {
		if (!deleteFile((string) item["path"])) {
			WARN("Unable to delete file %s", STR(item["path"]));
		}
	}
	string tmpPlaylistPath = _playlistPath + ".tmp";
	if (!_playlistFile.Initialize(tmpPlaylistPath, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to open rolling playlist");
		return false;
	}

    if (_targetDuration < _chunkLength)
        _targetDuration = _chunkLength;
    if ((duration / 1000.0) > _targetDuration)
        _targetDuration = ((uint32_t)(duration / 1000.0)) + 1;

    bool deleteFragment = false;

	if (_rollingItems.size() > 0) {
		string header = "";
		header += "#EXTM3U\r\n";
		header += HLS_VERSION(_version);
		header += "#EXT-X-ALLOW-CACHE:NO\r\n";
		header += format("#EXT-X-TARGETDURATION:%"PRIu32"\r\n", _targetDuration);
		header += format("#EXT-X-MEDIA-SEQUENCE:%"PRIu32"\r\n",
				(uint32_t) MAP_VAL(_rollingItems.begin())["sequence"]);
		double segmentStartTs = _hlsResume ? _segmentStartTs :
								(double) MAP_VAL(_rollingItems.begin())["segmentStartTs"];
		if (_startOffset != 0) {
			header += format("#EXT-X-START:TIME-OFFSET=%"PRIu32"\r\n", _startOffset);
		}
		header += GetProgramDateTime(segmentStartTs) + "\r\n";
	
		if (!_playlistFile.WriteString(header)) {
			FATAL("Unable to write rolling file header");
			_playlistFile.Close();
			return false;
		}	

		bool firstChunk = true;
		FOR_MAP(_rollingItems, uint64_t, Variant, i) {
			string item = "";
			if (!_staleItems.empty()) {
				uint64_t staleSequence = (uint64_t) MAP_VAL(_staleItems.begin())["realSequence"];
				uint64_t rollingSequence = (uint64_t) MAP_VAL(_rollingItems.begin())["realSequence"];
				if (staleSequence != rollingSequence)
					deleteFragment = true;
			}

			if (!firstChunk && (bool) MAP_VAL(i)["discontinuity"]) {
				item += "#EXT-X-DISCONTINUITY\r\n";
				WARN("Discontinuity encountered on stream %s for chunk number %"PRIu32,
					STR(_streamInfo), (uint32_t) (SEGMENT_SEQUENCE(_segmentSequence)));
			}

			//Add "#EXT-X-KEY" tag in playlist
			if (_drmType != "") {
				if ((bool)MAP_VAL(i)["isNewKey"] || i == _rollingItems.begin()) {
					if (_drmType == DRM_TYPE_RMS) {
						item += format("#EXT-X-KEY:METHOD=AES-128,URI=\"%s\",IV=0x%s\r\n",
								STR(MAP_VAL(i)["keyFileName"]),
								STR(MAP_VAL(i)["IV"]));
					} else if (_drmType == DRM_TYPE_SAMPLE_AES) {
						item += format("#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"%s\",IV=0x%s\r\n",
								STR(MAP_VAL(i)["keyFileName"]),
								STR(MAP_VAL(i)["IV"]));
					}
				}
			}
			//End add "#EXT-X-KEY" tag

			if (_hasMilliseconds)
				item += format("#EXTINF:%1.3f,\r\n",
					(double) MAP_VAL(i)["duration"]);
			else
				item += format("#EXTINF:%1.0f,\r\n",
					(double) MAP_VAL(i)["duration"]);

			if (_useByteRange) {
				uint64_t dataBytes = (uint64_t) MAP_VAL(i)["dataBytes"];
				uint64_t offsetBytes = (uint64_t) MAP_VAL(i)["offsetBytes"];
				item += format("#EXT-X-BYTERANGE:%"PRIu64"@%"PRIu64"\r\n", dataBytes, offsetBytes);
			}

			item += format("%s\r\n", STR(MAP_VAL(i)["name"]));

			if (firstChunk) {
				firstChunk = false;
			}

			if (!_playlistFile.WriteString(item)) {
				FATAL("Unable to append item to appending playlist");
				_playlistFile.Close();
				return false;
			}
		}

		_playlistFile.Close();

		//Reads the content of the temporary playlist then overwrite it to
		//the playlist
		movePlaylist(tmpPlaylistPath, _playlistPath);

		_pEventLogger->LogHLSChildPlaylistUpdated(_playlistPath);
	}

	while (_staleItems.size() > _staleRetentionCount) {
		string segmentPath = MAP_VAL(_staleItems.begin())["path"];
		if (deleteFragment && fileExists(segmentPath)) {
			if (!deleteFile(segmentPath)) {
				WARN("Unable to delete file %s", STR(segmentPath));
			}
		}
		_staleItems.erase(_staleItems.begin());
	}

	if (!_masterPlaylistInitialized) {
		if (!InitMasterPlaylist()) {
			FATAL("Unable to initialize master playlist");
			return false;
		}
	}

	return true;
}

bool HLSPlaylist::CreateNewSegment(double pts, double dts) {
	_segmentLastTs = pts;
	if (!CloseSegment(false)) {
		FATAL("Unable to close segment");
		return false;
	}
	if (!OpenSegment(pts, dts)) {
		FATAL("Unable to open segment");
		return false;
	}
	return true;
}

bool HLSPlaylist::ClosePlaylist() {
	if (!_playlistFile.Initialize(_playlistPath, FILE_OPEN_MODE_APPEND)) {
		FATAL("Unable to open playlist");
		return false;
	}

//	string item = "EXT-X-PLAYLIST-TYPE:VOD\r\n#EXT-X-ENDLIST\r\n";
	string item = "#EXT-X-ENDLIST\r\n";
	if (!_playlistFile.WriteString(item)) {
		FATAL("Unable to append item to appending playlist");
		_playlistFile.Close();
		return false;
	}

	// Update EXT-X-PLAYLIST-TYPE from EVENT to VOD, stating that the playlist will not change, unless overwritten
	_playlistFile.Close();
	if (_hasPlaylistType && _appending) {
		if (!_playlistFile.Initialize(_playlistPath, FILE_OPEN_MODE_READ)) {
			FATAL("Unable to open appending playlist");
			return false;
		}
		string buf;
		if (!_playlistFile.ReadAll(buf)) {
			FATAL("Playlist filesize too large");
			_playlistFile.Close();
			return false;
		}

		string playlistMode = "EVENT";
		_playlistFile.Close();
		buf.replace((size_t)_playlistTypeCursor, playlistMode.length(), "VOD");
		
		if (!_playlistFile.Initialize(_playlistPath, FILE_OPEN_MODE_TRUNCATE)) {
			FATAL("Unable to read appending playlist");
			return false;
		}

		if (!_playlistFile.WriteString(buf)) {
			FATAL("Unable to write appending file header");
			_playlistFile.Close();
			return false;
		}
	}
	_pEventLogger->LogHLSChildPlaylistUpdated(_playlistPath);

#ifdef HAS_PROTOCOL_DRM
	if ((_pDRMQueue != NULL) &&
			(_drmId != 0)) {
		_pDRMQueue->DeleteDRMEntry(_drmId);
		_drmId = 0;
	}
#endif /* HAS_PROTOCOL_DRM */

	return true;
}

bool HLSPlaylist::InitMasterPlaylist() {
	if (!_createMasterPlaylist) {
		_masterPlaylistInitialized = true;
		return true;
	}

	map<string, string> playlist = ParsePlaylist(_masterPlaylistPath);

	//3. Compute the entries inside the playlist
	string uri = format("%s/%s", STR(_localStreamName), STR(_playlistName));

	string content = "";
	File file;
	if (fileExists(_masterPlaylistPath)) {
		if (!file.Initialize(_masterPlaylistPath, FILE_OPEN_MODE_READ)) {
			FATAL("Unable to open master playlist in InitPlaylist");
			file.Close();
		} else {
			file.ReadAll(content);
			file.Close();
		}
	}

	string comment = "";
	if (content.find("#EXT-X-VERSION:") == std::string::npos) {
		comment += HLS_VERSION(_version);
	}

	if (_audioOnly) {
		if (_version >= 6) {
			comment += format("#EXT-X-STREAM-INF:BANDWIDTH=%"PRIu32",CODECS=\"mp4a.40.2\"", GetBandwidth());
		} else {
			comment += format("#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=%"PRIu32",CODECS=\"mp4a.40.2\"", GetBandwidth());
		}
	} else if (_version >= 4 && _version <= 5) {
		//CODECS is level 3 Constrained Baseline Profile and AAC audio
		comment += format("#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=%"PRIu32",CODECS=\"avc1.42c01e,mp4a.40.2\"", GetBandwidth());
	} else if (_version >= 6) {
		comment += format("#EXT-X-STREAM-INF:BANDWIDTH=%"PRIu32",CODECS=\"avc1.42c01e,mp4a.40.2\"", GetBandwidth());
	} else {
		comment += format("#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=%"PRIu32, GetBandwidth());
	}

	if (!_audioOnly) {
		uint32_t width = GetWidth();
		uint32_t height = GetHeight();
		if (width != 0 && height != 0)
			comment += format(",RESOLUTION=%"PRIu32"x%"PRIu32, width, height);
	}

	playlist[uri] = comment;

	if (!SaveMasterPlaylist(playlist, _masterPlaylistPath)) {
		FATAL("Unable to save master playlist");
		return false;
	}

	_masterPlaylistInitialized = true;

	_pEventLogger->LogHLSMasterPlaylistUpdated(_masterPlaylistPath);

	return true;
}

map<string, string> HLSPlaylist::ParsePlaylist(string path) {
	map<string, string> playlist;
	string content = "";
	if (!fileExists(path))
		return playlist;

	File file;
	if (!file.Initialize(path, FILE_OPEN_MODE_READ)) {
		FATAL("Unable to open master playlist");
		file.Close();
		return playlist;
	}
	file.ReadAll(content);
	file.Close();

	//2. Split the content into components
	vector<string> components;
	split(content, "\r\n", components);
	if (components.size() == 0) {
		WARN("Invalid master playlist.");
		return playlist;
	}
	if (components.size() < 1) {
		WARN("Invalid master playlist.");
		return playlist;
	}

	uint32_t i = 0;
	while (i < components.size()) {
		if (components[i].find("#EXT-X-STREAM-INF:") != std::string::npos) {
			break;
		}
		i++;
	}

	while (i != 0 && i < components.size()) {
		if (components[i].find("#EXT-X-STREAM-INF:") != std::string::npos) {
			if (components[i-1].find("#EXT-X-VERSION:") != std::string::npos) {
				playlist[components[i + 1]] = components[i-1] + "\r\n" + components[i];
			} else {
				playlist[components[i + 1]] = components[i];
			}
		}
		i++;
	}

	return playlist;
}

bool HLSPlaylist::SaveMasterPlaylist(map<string, string> &playlist, string path) {
	//4. create the content
	string content = "#EXTM3U\r\n";
	uint32_t headerLen = (uint32_t) content.length();
	FOR_MAP(playlist, string, string, i) {
		if (MAP_VAL(i).find("#EXT-X-VERSION:") != std::string::npos) {
			string versionStr = MAP_VAL(i) + "\r\n" + MAP_KEY(i) + "\r\n";
			content.insert(headerLen, versionStr);
		} else {
			content += MAP_VAL(i) + "\r\n";
			content += MAP_KEY(i) + "\r\n";
		}
	}

	//5. Save the content
	File file;
	if (!file.Initialize(path, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to open master playlist");
		return false;
	}

	if (!file.WriteString(content)) {
		FATAL("Unable to serialize master playlist");
		return false;
	}

	if (!file.Flush()) {
		FATAL("Unable to serialize master playlist");
		return false;
	}

	file.Close();

	return true;
}

uint32_t HLSPlaylist::GetBandwidth() {
	if (_bandwidth != 0) {
		return _bandwidth;
	}
	_bandwidth = (uint32_t) _pCapabilities->GetTransferRate();
	if (_bandwidth != 0) {
		INFO("Bandwidth detected on stream %s: %"PRIu32"kb/s. Invalidating the master playlist",
				STR(_localStreamName),
				_bandwidth / 1024);
		_masterPlaylistInitialized = false;
		return _bandwidth;
	} else {
		//FINEST("Bandwidth not detected. Returning dummy value");
		return 1000;
	}
}

string HLSPlaylist::GetProgramDateTime(double segmentStartTs) {
	Timestamp tm;
	memset(&tm, 0, sizeof (tm));

	if (_useSystemTime) {
		time_t now = time(NULL);
		tm = *(localtime(&now));
	} else {
		time_t timestamp = (time_t) ((_groupTimestamp + segmentStartTs) / 1000.0);
		tm = *gmtime(&timestamp);
	}
	if (_offsetTime != 0) {
		tm.tm_sec += _offsetTime;
		mktime(&tm);
	}
	char tempBuff[128] = {0};
	string result = "#EXT-X-PROGRAM-DATE-TIME:";
	if (_hasMilliseconds) {
		strftime(tempBuff, 127, "%Y-%m-%dT%H:%M:%S", &tm);
		result += tempBuff;
		if (_useSystemTime) {
			result += ".000+00:00";
		} else {
			result += format(".%03"PRIu64"+00:00", (uint64_t) (_groupTimestamp + segmentStartTs) % 1000ULL);
		}
	} else {
		strftime(tempBuff, 127, "%Y-%m-%dT%H:%M:%S+00:00", &tm);
		result += tempBuff;
	}
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL)
		_pStreamDebugFile->PushHLS(false, segmentStartTs, segmentStartTs, (uint32_t) timestamp);
#endif /* HAS_STREAM_DEBUG */
	return result;
}

uint32_t HLSPlaylist::GetWidth() {
	VideoCodecInfo *vCodec = _pCapabilities->GetVideoCodec();
	return vCodec != NULL ? vCodec->_width : 0;
}

uint32_t HLSPlaylist::GetHeight() {
	VideoCodecInfo *vCodec = _pCapabilities->GetVideoCodec();
	return vCodec != NULL ? vCodec->_height : 0;
}

//this will parse the whole child playlist file in cases when hls resume capability is enabled to restore _rollingItems
bool HLSPlaylist::RefreshRollingList(string const &buf) {
	size_t substrPos = 0;
	size_t len = 0;
	size_t newPos = 0;
	string searchStr;
	string name;
	string keyFileName;
	string IV;
	uint64_t segmentSequence = 0;
	_playlistSequence = 1;
	bool isNewKey = false;
	_segmentSequence = segmentSequence;
	bool discontinuity = false;

	double duration = 0;
	uint64_t dataBytes = 0;
	uint64_t offsetBytes = 0;

	while (substrPos < buf.length()) {
		Variant item;

		//check for discontinuity
		searchStr = "#EXT-X-DISCONTINUITY";
		newPos = buf.find(searchStr, substrPos);
		if (_playlistSequence != 1 && newPos != std::string::npos && (name == "" || newPos < substrPos + searchStr.length())) {
			substrPos = newPos;
			len = searchStr.length();
			substrPos += len;
			discontinuity = true;
		}

		//parse for AES encryption
		searchStr = "#EXT-X-KEY:METHOD=AES-128,URI=\"";
		if (_drmType == DRM_TYPE_SAMPLE_AES) {
			searchStr = "#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"";
		}
		newPos = buf.find(searchStr, substrPos);
		if (newPos != std::string::npos && (keyFileName == "" || newPos < substrPos + searchStr.length())) {
			substrPos = newPos;
			len = searchStr.length();
			substrPos += len;
			searchStr = "\",IV=0x";
			newPos = buf.find(searchStr, substrPos);
			if (newPos != std::string::npos) {
				len = newPos - substrPos;
				keyFileName = buf.substr(substrPos, len);
				substrPos = newPos;
				len = searchStr.length();
				substrPos += len;
			}
			searchStr = "#EXTINF:";
			newPos = buf.find(searchStr, substrPos);
			if (newPos != std::string::npos) {
				len = newPos - substrPos;
				IV = buf.substr(substrPos, len);
			}
			trim(IV);
		}

		//search for duration
		searchStr = "#EXTINF:";
		substrPos = buf.find(searchStr, substrPos);
		if (substrPos == std::string::npos) {
			break;
		}
		len = searchStr.length();
		substrPos += len;
		newPos = buf.find(",", substrPos);
		len = newPos - substrPos;
		searchStr = buf.substr(substrPos, len);
		duration = (double) atof(searchStr.c_str());
		substrPos = newPos + 1;

		//search for byte range data
		if (_useByteRange) {
			searchStr = "#EXT-X-BYTERANGE:";
			substrPos = buf.find(searchStr, substrPos);
			len = searchStr.length();
			substrPos += len;
			newPos = buf.find("@", substrPos);
			searchStr = buf.substr(substrPos, len);
			dataBytes = (uint64_t) atoi(searchStr.c_str());
			substrPos = newPos + 1;
			newPos = buf.find("\n", substrPos);
			len = newPos - substrPos;
			searchStr = buf.substr(substrPos, len);
			trim(searchStr);
			offsetBytes = (uint64_t) atoi(searchStr.c_str());
			substrPos = newPos + 1;
		}

		//get substring to store name
		name = "";
		searchStr = _audioOnly ? ".aac" : ".ts";
		newPos = buf.find(searchStr, substrPos);
		len = newPos - substrPos;
		name = buf.substr(substrPos, len);

		//remove whitespaces
		trim(name);

		//get segment sequence
		uint64_t multiplier = 1;
		for (size_t i = name.length() - 1; name[i] != '_'; i--) {
			segmentSequence += (name[i] - '0') * multiplier;
			multiplier *= 10;
		}
		name += searchStr;
		substrPos = newPos;
		substrPos += searchStr.length();
		isNewKey = ((segmentSequence % _AESKeyCount) == 1);

		item["name"] = name;
		item["sequence"] = (uint32_t) SEGMENT_SEQUENCE(segmentSequence);
		item["realSequence"] = segmentSequence;
		item["path"] = _targetFolder + (string) item["name"];
		item["duration"] = duration;
		item["discontinuity"] = discontinuity;
		if (_drmType != "") {
			item["keyFileName"] = keyFileName;
			item["IV"] = IV;
			item["isNewKey"] = isNewKey;
		}
		if (_useByteRange) {
			item["dataBytes"] = dataBytes;
			item["offsetBytes"] = offsetBytes;
		}

		_rollingItems[_playlistSequence] = item;
		_segmentSequence = segmentSequence;
		segmentSequence = 0;
		_playlistSequence++;
		discontinuity = false;
		isNewKey = false;
		//keyFileName and IV won't be reinitialized because it is carried over until replaced.
	}
	//update the discontinuity to true
    _discontinuity = true;
	//update to next segment sequence minimum value should be 1
	++_segmentSequence;

	return true;
}

//this method updates the segment sequence
void HLSPlaylist::UpdateSegmentSequence(string const &buf) {
	size_t substrPos;
	string temp = buf;

	substrPos = temp.find(_origChunkBaseName);
	if (substrPos == std::string::npos) {
		INFO("No segment has been created yet");
		_segmentSequence = 1;
		return;
	}

	uint64_t segmentSequence = 0;
	string substr = _audioOnly ? ".aac" : ".ts";

	substrPos = temp.rfind(substr);
	if (substrPos != std::string::npos) {
		//if the substring is found, delete the whole line until the end
		temp.erase(substrPos, std::string::npos);
	}

	size_t len = (size_t) temp.length();
	uint64_t multiplier = 1;
	for (size_t i = len - 1; temp[i] != '_'; i--) {
		if (isdigit(temp[i])) {
			segmentSequence += (temp[i] - '0') * multiplier;
			multiplier *= 10;
		}
	}
	_segmentSequence = ++segmentSequence;
}

bool HLSPlaylist::AppendToChildPlaylist(File &playlistFile) {
	if (!playlistFile.Initialize(_playlistPath, FILE_OPEN_MODE_READ)) {
		FATAL("Unable to open appending playlist");
		return false;
	}

	string buf;
	if (!playlistFile.ReadAll(buf)) {
		FATAL("Playlist filesize too large");
		playlistFile.Close();
		return false;
	}

	playlistFile.Close();

	if (!_appending) {
		vector<string> filesList;
		listFolder(_targetFolder, filesList, true, false, false); 
		RefreshRollingList(buf);
		RefreshStaleList(filesList);
	}
	
	size_t substrPos = buf.find("#EXT-X-ENDLIST");
	if (substrPos != std::string::npos) {
		//if the substring is found, delete the whole line until the end
		buf.erase(substrPos, std::string::npos);
	}

	if (_appending) {
		//update of segment sequence is already done in RefreshRollingList
		UpdateSegmentSequence(buf);
	}
	_discontinuity = true;

	if (!playlistFile.Initialize(_playlistPath, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to read appending playlist");
		return false;
	}

	if (_appending) {
		string playlistString = "#EXT-X-PLAYLIST-TYPE:";
		_playlistTypeCursor = (uint64_t) buf.find(playlistString) + playlistString.length();

		string playlistMode = "VOD";
		if (buf.find("#EXT-X-PLAYLIST-TYPE:VOD") != string::npos) {
			buf.replace((size_t)_playlistTypeCursor, playlistMode.length(), "EVENT");
		}
	}
	string durationString = "#EXT-X-TARGETDURATION:";
	_targetDurationCursor = (uint64_t)buf.find(durationString) + durationString.length();

	if (!playlistFile.WriteString(buf)) {
		FATAL("Unable to write appending file header");
		playlistFile.Close();
		return false;
	}

	playlistFile.Close();
	return true;

}

//method used to identify if instream associated with playlist is removed
void HLSPlaylist::SetStreamRemoved(bool instreamDetached) {
	_instreamDetached = instreamDetached;
}

void HLSPlaylist::RefreshStaleList(vector<string> &filesList) {
	//done in a separate loop so that find method will only be used a fewer times
	FOR_VECTOR_ITERATOR(string, filesList, i) {
		if (VECTOR_VAL(i).find(".m3u8") != string::npos) {
			filesList.erase(i);
			break;
		}
	}

	//remove the files which are already part of the _rollingItems map
	uint64_t maxSequence = 0;
	for (vector<string>::iterator it = filesList.begin(); it != filesList.end();) {
		bool erased = false;
		FOR_MAP(_rollingItems, uint64_t, Variant, j) {
			string name = MAP_VAL(j)["name"];
			if (it->find(name) != string::npos) {
				it = filesList.erase(it);
				erased = true;
				break;
			}
			if ((uint32_t) MAP_VAL(j)["sequence"] > maxSequence)
				maxSequence = (uint64_t) MAP_VAL(j)["sequence"];
		}
		if (!erased) {
			++it;
		}
	}

	//fill in _staleItems map
	FOR_VECTOR(filesList,i) {
		Variant item;
		string name = filesList[i];
		uint32_t segmentSequence = 0;
		//keys used for encryption must not be deleted
		if (filesList[i].find(".key") == string::npos) {
			size_t pos = name.find_last_of("_");
			size_t nextpos = name.find(".", pos);
			string stringNumber = (pos != string::npos && nextpos != string::npos) ? name.substr(pos + 1, nextpos - pos - 1) : "";
			segmentSequence = (uint32_t) atoi(stringNumber.c_str());
			if (segmentSequence <= maxSequence) {
				item["path"] = (string) name;
				item["realSequence"] = (uint32_t) segmentSequence;
				_staleItems[(uint64_t)(i + 1)] = item;
			} else {
				//delete files first which were not put in the playlist due to immediate shut down
				if (fileExists(name)) {
					deleteFile(name);
				}
			}
		}
	}

	uint64_t staleCount = (uint64_t) _staleItems.size();
	//reassign keys for map _rollingItems
	map<uint64_t, Variant> tempRollingItems(_rollingItems);
	_rollingItems.clear();
	FOR_MAP(tempRollingItems, uint64_t, Variant, i) {
		_rollingItems[(uint64_t) (MAP_KEY(i) + staleCount)] = MAP_VAL(i);
	}
	_playlistSequence += staleCount;
}

bool HLSPlaylist::ForceNewChunk(double pts) {
	double currentChunkLength = pts - _segmentStartTs;
	uint64_t currentFileSize = _pCurrentSegment->getCurrentFileSize();
	bool maxChunkLengthReached = ((currentChunkLength >= _maxChunkLength * 1000.0) && (_maxChunkLength != 0));
	bool maxChunkSizeReached = ((_maxChunkSize > 0) && (currentFileSize > _maxChunkSize));
	if (maxChunkLengthReached || maxChunkSizeReached) {
		DEBUG("Forcing new chunk. maxChunkLength:%.3fms currentChunkLength:%.3fms, maxChunkSize:%"PRIu64" bytes currentChunkSize:%"PRIu64 "bytes",
				_maxChunkLength * 1000.0, currentChunkLength, _maxChunkSize, currentFileSize);
	}
	return (maxChunkLengthReached || maxChunkSizeReached);
}


#endif /* HAS_PROTOCOL_HLS */
