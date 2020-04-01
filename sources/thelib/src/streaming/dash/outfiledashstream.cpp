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


#ifdef HAS_PROTOCOL_DASH
#include "streaming/dash/outfiledashstream.h"
#include "streaming/dash/dashmanifest.h"
#include "streaming/dash/dashfragment.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "mediaformats/writers/mp4initsegment/outfilemp4initsegment.h"

namespace MpegDash {

     SessionInfo::SessionInfo()
     : fragmentSequence(1), 
	   pManifest(NULL), 
	   isFirstRun(true), 
	   totalAudioDuration(0), 
	   totalVideoDuration(0),
	   startAudioTimeTicks(0),
	   startVideoTimeTicks(0) {
     }

    void SessionInfo::Reset() {
		ClearQueue(retainedAudioFragments);
		ClearQueue(retainedVideoFragments);
		ClearQueue(staleAudioFragments);
		ClearQueue(staleVideoFragments);
        fragmentSequence = 1;
		totalAudioDuration = 0;
		totalVideoDuration = 0;
		startAudioTimeTicks = 0;
		startVideoTimeTicks = 0;
    }

	void SessionInfo::ClearQueue(queue<RetainedFragmentInfo>& refQueue) {
		queue<RetainedFragmentInfo> empty;
		std::swap(refQueue, empty);
	}

    map<string, SessionInfo> OutFileDASHStream::_sessionInfo;
    string OutFileDASHStream::_prevGroupName;
    string OutFileDASHStream::_prevLocalStreamName;
    map<string, DASHManifest*> OutFileDASHStream::_prevManifest;

    OutFileDASHStream::OutFileDASHStream(BaseProtocol *pProtocol, string name,
            Variant &settings)
    : BaseOutFileStream(pProtocol, ST_OUT_FILE_DASH, name),
    _audioRate(-1),
    _videoRate(-1),
    _settings(settings),
    _pGenericProcessDataSetup(NULL),
    _chunkOnIDR(false),
    _pVideoFragment(NULL),
    _pAudioFragment(NULL),
    _chunkLength(0),
    _isLastFragment(false),
    _overwriteDestination(false),
    _isRolling(false),
    _isLive(true),
    _playlistLength(0),
	_lastAudioTS(0),
	_lastVideoTS(0),
	_lastVideoDuration(0),
    _addStreamFlag(0),
    _removeStreamFlag(0),
	_avDurationDelta(0) {
        _repAudioId.clear();
        _repAudioId.str(std::string());
        _repVideoId.clear();
        _repVideoId.str(std::string());
    }

    OutFileDASHStream* OutFileDASHStream::GetInstance(BaseClientApplication *pApplication,
            string name, Variant &settings) {
        //1. Create a dummy protocol
        PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

        //2. create the parameters
        Variant parameters;
        parameters["customParameters"]["dashConfig"] = settings;

        //3. Initialize the protocol
        if (!pDummyProtocol->Initialize(parameters)) {
            FATAL("Unable to initialize passthrough protocol");
            pDummyProtocol->EnqueueForDelete();
            return NULL;
        }

        //4. Set the application
        pDummyProtocol->SetApplication(pApplication);

        //5. Create the DASH stream
        OutFileDASHStream *pOutDASH = new OutFileDASHStream(pDummyProtocol, name, settings);
        if (!pOutDASH->SetStreamsManager(pApplication->GetStreamsManager())) {
            FATAL("Unable to set the streams manager");
            delete pOutDASH;
            pOutDASH = NULL;
            return NULL;
        }
        pDummyProtocol->SetDummyStream(pOutDASH);

        //6. Done
        return pOutDASH;
    }

    OutFileDASHStream::~OutFileDASHStream() {
        // Set the last fragment flag
        _isLastFragment = true;

        // Close the fragment
		CloseFragment(true);
		CloseFragment(false);

        bool deleteFiles =
                (_pProtocol != NULL)
                &&(_pProtocol->GetCustomParameters().HasKeyChain(V_BOOL, true, 1, "IsTaggedForDeletion"))
                &&((bool)_pProtocol->GetCustomParameters()["IsTaggedForDeletion"]);

        // Delete the fragment instance
        if (NULL != _pVideoFragment) {
            delete _pVideoFragment;
            _pVideoFragment = NULL;
        }

        if (NULL != _pAudioFragment) {
            delete _pAudioFragment;
            _pAudioFragment = NULL;
        }

        if (deleteFiles) {
            // Delete the files
            deleteFolder(_targetFolder, true);
        }

        GetEventLogger()->LogStreamClosed(this);
    }

    void OutFileDASHStream::GetStats(Variant &info, uint32_t namespaceId) {
        BaseOutStream::GetStats(info, namespaceId);
        info["dashSettings"] = _settings;
    }

    bool OutFileDASHStream::IsCompatibleWithType(uint64_t type) {
        return TAG_KIND_OF(type, ST_IN_NET_RTMP)
                || TAG_KIND_OF(type, ST_IN_NET_RTP)
                || TAG_KIND_OF(type, ST_IN_NET_TS)
                || TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
                || TAG_KIND_OF(type, ST_IN_FILE)
                || TAG_KIND_OF(type, ST_IN_NET_EXT);
    }

    void OutFileDASHStream::SignalDetachedFromInStream() {
        _pProtocol->EnqueueForDelete();
    }

    void OutFileDASHStream::SignalStreamCompleted() {
        _pProtocol->EnqueueForDelete();
    }

    void OutFileDASHStream::SignalAudioStreamCapabilitiesChanged(
            StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
            AudioCodecInfo *pNew) {
        GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
        if (_pAudioFragment != NULL)
            _pAudioFragment->ForceClose();
	}

	void OutFileDASHStream::SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew) {
		GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);

		// If this is SPS/PPS update, continue
		if (IsVariableSpsPps(pOld, pNew)) {
			return;
		}

		if (_pVideoFragment != NULL)
			_pVideoFragment->ForceClose();
	}

    bool OutFileDASHStream::FeedData(uint8_t *pData, uint32_t dataLength,
            uint32_t processedLength, uint32_t totalLength,
            double pts, double dts, bool isAudio) {
        uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
        uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
        packetsCount++;
        bytesCount += dataLength;
        return GenericProcessData(pData, dataLength, processedLength, totalLength,
                pts, dts, isAudio);
    }

    void OutFileDASHStream::InitializeManifest() {
        if (NULL != _sessionInfo[_sessionKey].pManifest) {
            delete _sessionInfo[_sessionKey].pManifest;
        }
        _sessionInfo[_sessionKey].pManifest = new DASHManifest(_isLive);
        _prevGroupName = _groupName;
        _prevLocalStreamName = _localStreamName;
        _sessionInfo[_sessionKey].pManifest->UpdateReferenceCount();
        _prevManifest[_groupName] = _sessionInfo[_sessionKey].pManifest;
        _sessionInfo[_sessionKey].Reset();
        _sessionInfo[_sessionKey].isFirstRun = false;
    }

    bool OutFileDASHStream::FinishInitialization(
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
                false, //insertRTMPPayloadHeader,
                true, //insertSPSPPSBeforeIDR,
                true //aggregateNALU
                );

        //audio setup
        _pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
        _pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;

        //misc setup
        _pGenericProcessDataSetup->_timeBase = 0;
        _pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

        //Get the audio/video rates which will be later used to
        //open fragments
        if (_pGenericProcessDataSetup->_hasAudio) {
            _audioRate = _pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec()->_samplingRate;
        }
        if (_pGenericProcessDataSetup->_hasVideo) {
            _videoRate = (uint64_t) ((_pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec()->GetFPS() + 1) * 1.5);
        }

        _chunkOnIDR = (bool) _settings["chunkOnIDR"];
        _chunkLength = ((double) _settings["chunkLength"]) * DASH_TIMESCALE;

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
        _isLive = (bool) _settings["dynamicProfile"];
        _playlistLength = (uint32_t) _settings["playlistLength"];
        _staleRetentionCount = (uint32_t) _settings["staleRetentionCount"];
        uint32_t bandwidth = (uint32_t) _settings["bandwidth"];
        _localStreamName = (string) _settings["localStreamName"];
        _groupName = (string) _settings["groupName"];
        _sessionKey = _groupName + _localStreamName;
		_isAutoDash = (bool)_settings["isAutoDASH"];
        // Update the bandwidth
        if (bandwidth == 0) {
            bandwidth = (uint32_t) (_pGenericProcessDataSetup->_pStreamCapabilities->GetTransferRate());
        }
        _bandwidth = format("%"PRIu32, bandwidth);

        // Allocate the manifest file
		if (!_isAutoDash) {
			if (!_isLive) {
				_videoRelativeFolder = format("%s/%s/%s", STR(_localStreamName), DASH_VIDEO_FOLDER_NAME, STR(_bandwidth));
				_audioRelativeFolder = format("%s/%s/%s", STR(_localStreamName), DASH_AUDIO_FOLDER_NAME, STR(_bandwidth));
			}

			if (_prevGroupName != _groupName) {
				InitializeManifest();
			} else {
				if (_prevManifest[_groupName]->GetReferenceCount() == 1 && !(_sessionInfo[_sessionKey].isFirstRun)
					&& ((bool)_settings["cleanupDestination"])) {
					InitializeManifest();
				} else {
					_sessionInfo[_sessionKey].pManifest = _prevManifest[_groupName];
					_sessionInfo[_sessionKey].isFirstRun = false;
					if (_prevLocalStreamName != _localStreamName) {
						_prevManifest[_groupName]->UpdateReferenceCount();
					}
				}
			}
		} else {
			if (!_isLive) {
				_videoRelativeFolder = format("%s/%s", DASH_VIDEO_FOLDER_NAME, STR(_bandwidth));
				_audioRelativeFolder = format("%s/%s", DASH_AUDIO_FOLDER_NAME, STR(_bandwidth));
			}
			InitializeManifest();
		}

        // Initialize manifest to stream specifics
        if (!_sessionInfo[_sessionKey].pManifest->Init(_settings, _pGenericProcessDataSetup->_pStreamCapabilities)) {
            FATAL("Unable to initialize Manifest Writer");
            return false;
        }
        _sessionInfo[_sessionKey].pManifest->CreateDefault();
        GetEventLogger()->LogDASHPlaylistUpdated(_sessionInfo[_sessionKey].pManifest->GetManifestPath());

        _sessionInfo[_sessionKey].videoFolder = (string) _settings["videoFolder"];
        _sessionInfo[_sessionKey].audioFolder = (string) _settings["audioFolder"];

        Variant settings;
        string name = "segInitwriter";
		string videoPath = format("%sseg_init.mp4", STR(_sessionInfo[_sessionKey].videoFolder));
		string audioPath = format("%sseg_init.mp4", STR(_sessionInfo[_sessionKey].audioFolder));
        settings["chunkLength"] = _settings["chunkLength"];
        OutFileMP4InitSegment* pMP4InitSegment;
        XmlElements* pSegmentListEntry;

        if (_pGenericProcessDataSetup->_hasAudio) {
            settings["computedPathToFile"] = audioPath;
            pMP4InitSegment = OutFileMP4InitSegment::GetInstance(_pApplication, name, settings);
            pMP4InitSegment->CreateInitSegment(GetInStream(), true);

            if (pMP4InitSegment) {
                delete pMP4InitSegment;
                pMP4InitSegment = NULL;
            }
            // Adds the initialization segment to the manifest list
            _repAudioId << _localStreamName << "Audio" << _bandwidth;
            if (!_isLive) {
                pSegmentListEntry = new XmlElements("Initialization", "");
				pSegmentListEntry->AddAttribute("sourceURL", format("%s/seg_init.mp4", STR(_audioRelativeFolder)));
                if (!_sessionInfo[_sessionKey].pManifest->AddSegmentListEntry(pSegmentListEntry, 2, _repAudioId.str())) {
                    delete pSegmentListEntry;
                }
            }
        }

        if (_pGenericProcessDataSetup->_hasVideo) {
            settings["computedPathToFile"] = videoPath;
            pMP4InitSegment = OutFileMP4InitSegment::GetInstance(_pApplication, name, settings);
            pMP4InitSegment->CreateInitSegment(GetInStream(), false);

            if (pMP4InitSegment) {
                delete pMP4InitSegment;
                pMP4InitSegment = NULL;
            }
            // Adds the initialization segment to the manifest list
            _repVideoId << _localStreamName << "Video" << _bandwidth;
            if (!_isLive) {
                pSegmentListEntry = new XmlElements("Initialization", "");
				pSegmentListEntry->AddAttribute("sourceURL", format("%s/seg_init.mp4", STR(_videoRelativeFolder)));
                if (!_sessionInfo[_sessionKey].pManifest->AddSegmentListEntry(pSegmentListEntry, 1, _repVideoId.str())) {
                    delete pSegmentListEntry;
                }
            }
        }

        return true;
    }

    bool OutFileDASHStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
            bool isKeyFrame) {
        //1. no fragment opened
        if (_pVideoFragment == NULL) {
            //2. skip frames until the first key frame if necessary
            if ((_chunkOnIDR) && (!isKeyFrame)) {
                _lastVideoTS = dts;
                return true;
            }
            //3. Open the fragment
            if (!OpenFragment(pts, dts, 1.04, false)) {
                FATAL("Unable to open fragment");
                return false;
            }
        }

        //4. Get the duration
		double duration = _pVideoFragment->GetTotalDuration(false);

        //5. Check and see if we must close the fragment because no more frames
        //can be appended to it
        if (_pVideoFragment->MustClose()) {
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
            if (!NewFragment(pts, dts, forced, false)) {
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
						if (_lastVideoDuration == 0) {
							if (!NewFragment(pts, dts, false, false)) {
								FATAL("Unable to open new fragment");
								return false;
							}
							_lastVideoDuration = duration;
						}
                    }
                } else {
                    //11. We don't need to chunk on IDR, so we chunk anyway because
                    //the duration condition is met
					if (_lastVideoDuration == 0) {
						if (!NewFragment(pts, dts, false, false)) {
							FATAL("Unable to open new fragment");
							return false;
						}
						_lastVideoDuration = duration;
					}
                }
            }
        }

        //12. Push the data into the fragment
        if (_pVideoFragment->PushVideoData(buffer, pts, dts, isKeyFrame)) {
            _lastVideoTS = dts;
            return true;
        } else {
            GetEventLogger()->LogDASHChunkError("Could not write video sample to " + _pVideoFragment->GetPath());
            return false;
        }
    }

    bool OutFileDASHStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
        //1. no fragment opened
        if (_pAudioFragment == NULL) {
            //2. skip all the frames if we need to _chunkOnIDR but ONLY when
            //video track is present. Basically, when video is present AND _chunkOnIDR,
            //is true, all audio frames will be dropped until the fragment is opened
            //by the PushVideoData logic
            if ((_chunkOnIDR) &&
                    (_pGenericProcessDataSetup->_hasVideo) &&
                    (_pVideoFragment == NULL)) {
                _lastAudioTS = dts;
                return true;
            }
            //3. Open the fragment
            if (!OpenFragment(pts, dts, 1.04, true)) {
                FATAL("Unable to open fragment");
                return false;
            }
        }

        //4. Get the duration
		double duration = _pAudioFragment->GetTotalDuration(true);

        //5. Check and see if we must close the fragment because no more frames
        //can be appended to it
        if (_pAudioFragment->MustClose()) {
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
            if (!NewFragment(pts, dts, forced, true)) {
                FATAL("Unable to open new fragment");
                return false;
            }
        } else {
            //8. inspect the duration and decide if we can TRY to open a new chunk
			if (duration >= _chunkLength) {
                //9. Check and see if we need to chunk on IDR
                if (_chunkOnIDR) {
                    //10. Only open a new track if we don't have a video track
                    if (_pGenericProcessDataSetup->_hasVideo) {
						if (_lastVideoDuration > 0 && duration >= _lastVideoDuration) {
                            if (!NewFragment(pts, dts, false, true)) {
                                FATAL("Unable to open new fragment");
                                return false;
                            }
							_avDurationDelta = duration - _lastVideoDuration;
							_lastVideoDuration = 0;
                        }
                    } else {
                        if (!NewFragment(pts, dts, false, true)) {
                            FATAL("Unable to open new fragment");
                            return false;
                        }
                    }
                } else {
                    //11. We don't need to chunk on IDR, so we chunk anyway because
                    //the duration condition is met
                    if (_pGenericProcessDataSetup->_hasVideo) {
						if (_lastVideoDuration > 0 && duration >= _lastVideoDuration) {
                            if (!NewFragment(pts, dts, false, true)) {
                                FATAL("Unable to open new fragment");
                                return false;
                            }
							_avDurationDelta = (duration - _lastVideoDuration);
							_lastVideoDuration = 0;
                        }
                    } else {
                        if (!NewFragment(pts, dts, false, true)) {
                            FATAL("Unable to open new fragment");
                            return false;
                        }
                    }
                }
            }
        }

        //12. Push the data into the fragment
        if (_pAudioFragment->PushAudioData(buffer, pts, dts)) {
            _lastAudioTS = dts;
            return true;
        } else {
            GetEventLogger()->LogDASHChunkError("Could not write audio sample to " + _pAudioFragment->GetPath());
            return false;
        }
    }

    bool OutFileDASHStream::IsCodecSupported(uint64_t codec) {
        return (codec == CODEC_VIDEO_H264)
                || (codec == CODEC_AUDIO_AAC);
    }

    bool OutFileDASHStream::OpenFragment(double pts, double dts, double additionalFramesPercent, bool isAudio) {
        //1. Check and see if we have a fragment already opened
        DASHFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        if (pFragment != NULL) {
            FATAL("Fragment already opened");
            GetEventLogger()->LogDASHChunkError("Opening a fragment but is already open.");
            return false;
        }

        //2. Compute the fragment path
        string targetPath = isAudio ? _sessionInfo[_sessionKey].audioFolder : _sessionInfo[_sessionKey].videoFolder;
        string fragmentFullPath = format("%ssegment_%"PRIu64".m4s", STR(targetPath),
			isAudio ? _sessionInfo[_sessionKey].startAudioTimeTicks : _sessionInfo[_sessionKey].startVideoTimeTicks);
        
        // Test if we can overwrite the fragment
        if (fileExists(fragmentFullPath)) {
            if (_overwriteDestination) {
                if (!deleteFile(fragmentFullPath)) {
                    GetEventLogger()->LogDASHChunkError("Could not delete " + fragmentFullPath);
                    return false;
                }
            } else {
                string error = format("Fragment file %s already present and overwrite is not permitted!", STR(fragmentFullPath));
                FATAL("%s", STR(error));
                GetEventLogger()->LogDASHChunkError(error);
                return false;
            }
        }

        //3. Create the fragment
        pFragment = new DASHFragment(fragmentFullPath,
                _sessionInfo[_sessionKey].fragmentSequence++,
                1.0 * _chunkLength / DASH_TIMESCALE,
                isAudio ? _audioRate : _videoRate,
                additionalFramesPercent,
                pts,
                dts,
                isAudio);

        if (isAudio) {
            _pAudioFragment = pFragment;
        } else {
            _pVideoFragment = pFragment;
        }

        //4. Do initialization
        if (!pFragment->Initialize()) {
            string error = "Unable to initialize " + fragmentFullPath;
            FATAL("%s", STR(error));
            GetEventLogger()->LogDASHChunkError(error);
            return false;
        }
        pFragment->SetLastTS(isAudio ? _lastAudioTS : _lastVideoTS, isAudio);

        _chunkInfo["file"] = fragmentFullPath;
        _chunkInfo["startTime"] = Variant::Now();
        GetEventLogger()->LogDASHChunkCreated(_chunkInfo);

        return true;
    }

    bool OutFileDASHStream::CloseFragment(bool isAudio, double pts) {

        //1. Check and see if no fragment is currently opened
        DASHFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        if (pFragment == NULL) {
            FATAL("Fragment not opened");
            GetEventLogger()->LogDASHChunkError("Closing a fragment that was not opened.");
            return false;
        }

		uint32_t currentDuration = pFragment->GetTotalDuration(isAudio);
		uint64_t& currentUTCStamp = isAudio ? _sessionInfo[_sessionKey].startAudioTimeTicks : _sessionInfo[_sessionKey].startVideoTimeTicks;
		uint32_t adjustedDuration = currentDuration;

		// add up the audio video duration delta to avoid freeze up
		// when refreshing the player
		if (_isLive) {
			if (!isAudio && _pGenericProcessDataSetup->_hasAudio) {
				if (NULL != _pAudioFragment
					&& _avDurationDelta > 0) {
					adjustedDuration = currentDuration + (uint32_t)_avDurationDelta;
					_avDurationDelta = 0;
				}
			}
		}

		pFragment->SetFragmentDecodeTime(currentUTCStamp); //for tfdt atom
		//for sidx atom
		pFragment->SetTimeScale(DASH_TIMESCALE);
		pFragment->SetEarliestPresentationTime(currentUTCStamp);
		pFragment->SetSubSegmentDuration(currentDuration);

        pFragment->Finalize();

		if (_sessionInfo[_sessionKey].pManifest == NULL) {
			FATAL("Manifest not opened");
			GetEventLogger()->LogDASHChunkError("Manifest not opened, closing fragment.");

			// Delete the fragment on error
			if (isAudio) {
				delete _pAudioFragment;
				_pAudioFragment = NULL;
			} else {
				delete _pVideoFragment;
				_pVideoFragment = NULL;
			}
			return false;
		}

        if (_isLive) {
			// Update the manifest SegmentTimeline entry
			XmlElements* pSegmentTimelineEntry = new XmlElements("S", "");
			pSegmentTimelineEntry->AddAttribute("t", currentUTCStamp);
			pSegmentTimelineEntry->AddAttribute("d", currentDuration);
			if (isAudio) {
				if (_sessionInfo[_sessionKey].pManifest->AddSegmentTimelineEntry(pSegmentTimelineEntry, 2, _repAudioId.str())) {
					// update the manifest file to disk
					if (_pGenericProcessDataSetup->_hasVideo) {
						_addStreamFlag |= 1;
						if (_addStreamFlag == 3) {
							_sessionInfo[_sessionKey].pManifest->Create();
							_addStreamFlag = 0;
						}
					} else {
						_sessionInfo[_sessionKey].pManifest->Create();
					}
				} else {
					delete pSegmentTimelineEntry;
					pSegmentTimelineEntry = NULL;
				}
			} else {
				if (_sessionInfo[_sessionKey].pManifest->AddSegmentTimelineEntry(pSegmentTimelineEntry, 1, _repVideoId.str())) {
					// update the manifest file to disk
					if (_pGenericProcessDataSetup->_hasAudio) {
						_addStreamFlag |= 2;
						if (_addStreamFlag == 3) {
							_sessionInfo[_sessionKey].pManifest->Create();
							_addStreamFlag = 0;
						}
					} else {
						_sessionInfo[_sessionKey].pManifest->Create();
					}
				} else {
					delete pSegmentTimelineEntry;
					pSegmentTimelineEntry = NULL;
				}
			}
        } else {
			string targetRelativePath = isAudio ? _audioRelativeFolder : _videoRelativeFolder;
			string fragmentRelativePath = format("%s/segment_%"PRIu64".m4s", STR(targetRelativePath), currentUTCStamp);
			pFragment->SetRelativePath(fragmentRelativePath);

            // Update the manifest SegmentList entry
            XmlElements* pSegmentListEntry = new XmlElements("SegmentURL", "");
            pSegmentListEntry->AddAttribute("media", fragmentRelativePath);
            if (isAudio) {
				_sessionInfo[_sessionKey].totalAudioDuration += currentDuration;
				if (_sessionInfo[_sessionKey].totalAudioDuration > _sessionInfo[_sessionKey].totalVideoDuration) {
					_sessionInfo[_sessionKey].pManifest->SetDuration(_sessionInfo[_sessionKey].totalAudioDuration);
				}
                if (_sessionInfo[_sessionKey].pManifest->AddSegmentListEntry(pSegmentListEntry, 2, _repAudioId.str())) {
					// update the manifest file to disk
					if (_pGenericProcessDataSetup->_hasVideo) {
						_addStreamFlag |= 1;
						if (_addStreamFlag == 3) {
							_sessionInfo[_sessionKey].pManifest->Create();
							_addStreamFlag = 0;
						}
					} else {
						_sessionInfo[_sessionKey].pManifest->Create();
					}
                } else {
					delete pSegmentListEntry;
					pSegmentListEntry = NULL;
                }
            } else {
				_sessionInfo[_sessionKey].totalVideoDuration += currentDuration;
				if (_sessionInfo[_sessionKey].totalVideoDuration > _sessionInfo[_sessionKey].totalAudioDuration) {
					_sessionInfo[_sessionKey].pManifest->SetDuration(_sessionInfo[_sessionKey].totalVideoDuration);
				}
                if (_sessionInfo[_sessionKey].pManifest->AddSegmentListEntry(pSegmentListEntry, 1, _repVideoId.str())) {
					// update the manifest file to disk
					if (_pGenericProcessDataSetup->_hasAudio) {
						_addStreamFlag |= 2;
						if (_addStreamFlag == 3) {
							_sessionInfo[_sessionKey].pManifest->Create();
							_addStreamFlag = 0;
						}
					} else {
						_sessionInfo[_sessionKey].pManifest->Create();
					}
                } else {
					delete pSegmentListEntry;
					pSegmentListEntry = NULL;
                }
            }
        }	
		
		currentUTCStamp += adjustedDuration;

        //2. If rolling, perform manifest adjustment and fragment cleanup
        if (_isRolling) {
            RetainedFragmentInfo retainedFragmentInfo;
            retainedFragmentInfo.absoluteFragmentPath = pFragment->GetPath();

			if (_isLive) {
				retainedFragmentInfo.fragmentDecodeTime = pFragment->GetFragmentDecodeTime();
			} else {
				retainedFragmentInfo.relativeFragmentPath = pFragment->GetRelativePath();
				retainedFragmentInfo.fragmentDuration = currentDuration;
			}

			size_t queueeSize, staleQueueSize;
            if (isAudio) {
                _sessionInfo[_sessionKey].retainedAudioFragments.push(retainedFragmentInfo);
                _sessionInfo[_sessionKey].staleAudioFragments.push(retainedFragmentInfo);
				queueeSize = _sessionInfo[_sessionKey].retainedAudioFragments.size();
				staleQueueSize = _sessionInfo[_sessionKey].staleAudioFragments.size();
            } else {
                _sessionInfo[_sessionKey].retainedVideoFragments.push(retainedFragmentInfo);
                _sessionInfo[_sessionKey].staleVideoFragments.push(retainedFragmentInfo);
				queueeSize = _sessionInfo[_sessionKey].retainedVideoFragments.size();
				staleQueueSize = _sessionInfo[_sessionKey].staleVideoFragments.size();
            }

            // remove manifest entry
            while (queueeSize > _playlistLength) {
				string manifestEntry;
                if (_isLive) {
					if (isAudio) {
						manifestEntry = format("%"PRIu64, _sessionInfo[_sessionKey].retainedAudioFragments.front().fragmentDecodeTime);
						_sessionInfo[_sessionKey].pManifest->RemoveSegmentTimelineEntry(manifestEntry, 2, _repAudioId.str());
					} else {
						manifestEntry = format("%"PRIu64, _sessionInfo[_sessionKey].retainedVideoFragments.front().fragmentDecodeTime);
						_sessionInfo[_sessionKey].pManifest->RemoveSegmentTimelineEntry(manifestEntry, 1, _repVideoId.str());
					}
				}
				else {
					if (isAudio) {
						manifestEntry = _sessionInfo[_sessionKey].retainedAudioFragments.front().relativeFragmentPath;
						_sessionInfo[_sessionKey].pManifest->RemoveSegmentListURL(manifestEntry, 2, _repAudioId.str());
						//subtract the duration of the removed item to the total duration
						_sessionInfo[_sessionKey].totalAudioDuration -= _sessionInfo[_sessionKey].retainedAudioFragments.front().fragmentDuration;
					} else {
						manifestEntry = _sessionInfo[_sessionKey].retainedVideoFragments.front().relativeFragmentPath;
						_sessionInfo[_sessionKey].pManifest->RemoveSegmentListURL(manifestEntry, 1, _repVideoId.str());
						//subtract the duration of the removed item to the total duration
						_sessionInfo[_sessionKey].totalVideoDuration -= _sessionInfo[_sessionKey].retainedVideoFragments.front().fragmentDuration;
					}

					double retainedDuration = _sessionInfo[_sessionKey].totalAudioDuration > _sessionInfo[_sessionKey].totalVideoDuration 
						? _sessionInfo[_sessionKey].totalAudioDuration : _sessionInfo[_sessionKey].totalVideoDuration;
					//update the duration
					_sessionInfo[_sessionKey].pManifest->SetDuration(retainedDuration);
				}

                // update the manifest file to disk
                if (isAudio) {
					_sessionInfo[_sessionKey].retainedAudioFragments.pop();
					queueeSize = _sessionInfo[_sessionKey].retainedAudioFragments.size();
                    if (_pGenericProcessDataSetup->_hasVideo) {
                        _removeStreamFlag |= 1;
                        if (_removeStreamFlag == 3) {
                            _sessionInfo[_sessionKey].pManifest->Create();
                            _removeStreamFlag = 0;
                        }
                    } else {
                        _sessionInfo[_sessionKey].pManifest->Create();
                    }
                } else {
					_sessionInfo[_sessionKey].retainedVideoFragments.pop();
					queueeSize = _sessionInfo[_sessionKey].retainedVideoFragments.size();
                    if (_pGenericProcessDataSetup->_hasAudio) {
                        _removeStreamFlag |= 2;
                        if (_removeStreamFlag == 3) {
                            _sessionInfo[_sessionKey].pManifest->Create();
                            _removeStreamFlag = 0;
                        }
                    } else {
                        _sessionInfo[_sessionKey].pManifest->Create();
                    }
                }
            }

            // remove fragment from disk
			while (staleQueueSize > _playlistLength + _staleRetentionCount) {
				string fragmentPath;
				if (isAudio) {
					fragmentPath = _sessionInfo[_sessionKey].staleAudioFragments.front().absoluteFragmentPath;
					_sessionInfo[_sessionKey].staleAudioFragments.pop();
					staleQueueSize = _sessionInfo[_sessionKey].staleAudioFragments.size();
				} else {
					fragmentPath = _sessionInfo[_sessionKey].staleVideoFragments.front().absoluteFragmentPath;
					_sessionInfo[_sessionKey].staleVideoFragments.pop();
					staleQueueSize = _sessionInfo[_sessionKey].staleVideoFragments.size();
				}

				if (!deleteFile(fragmentPath)) {
					WARN("Unable to delete file %s", STR(fragmentPath));
				}
			}
        }

        _chunkInfo["file"] = pFragment->GetPath();
        _chunkInfo["startTime"] = pFragment->GetStartTime();
        _chunkInfo["stopTime"] = Variant::Now();
        _chunkInfo["frameCount"] = pFragment->GetFrameCount();
        GetEventLogger()->LogDASHChunkClosed(_chunkInfo);

        //4. Delete the fragment
        if (isAudio) {
            delete _pAudioFragment;
            _pAudioFragment = NULL;
        } else {
            delete _pVideoFragment;
            _pVideoFragment = NULL;
        }

        //4. Done
        return true;
    }

    bool OutFileDASHStream::NewFragment(double pts, double dts, bool forced, bool isAudio) {
        DASHFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        //1. Check and see if we have a fragment opened
        if (pFragment == NULL) {
            FATAL("Fragment not opened");
            GetEventLogger()->LogDASHChunkError("Current fragment is not open.");
            return false;
        }

        //2. Get the string representation in case we need to report an error
        string temp = (string) * pFragment;

        //3. Get the current additionalFramesPercent from the current frame
        //we will use this as abase for the new value used in the new fragment
        //we are going to open
        double additionalFramesPercent = pFragment->AdditionalFramesPercent();

        //4. If this current fragment was closed by force (no more frames slots)
        //than increase the additionalFramesPercent by 0.5%
        if (forced) {
            additionalFramesPercent += 0.005;
        }

        //5. Close the fragment
        if (!CloseFragment(isAudio, pts)) {
            string error = "Unable to close fragment";
            FATAL("%s", STR(error));
            GetEventLogger()->LogDASHChunkError(error);
            return false;
        }

        // Boundary check
        if ((_sessionInfo[_sessionKey].fragmentSequence % 65536) == 0) {
            _sessionInfo[_sessionKey].fragmentSequence = 1;
        }

        //7. Open a new one
        return OpenFragment(pts, dts, additionalFramesPercent, isAudio);
    }
}
#endif	/* HAS_PROTOCOL_DASH */
