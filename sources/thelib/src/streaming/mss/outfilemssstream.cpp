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


#ifdef HAS_PROTOCOL_MSS
#include "streaming/mss/outfilemssstream.h"
#include "streaming/mss/mssmanifest.h"
#include "streaming/mss/mssfragment.h"
#include "streaming/mss/msspublishingpoint.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "protocols/http/httpmssliveingest.h"
#include "protocols/http/basehttpprotocolhandler.h"

namespace MicrosoftSmoothStream {

	SessionInfo::SessionInfo()
	: fragmentSequence(1), 
		pManifest(NULL), 
        isFirstRun(true), 
		videoChunkCount(0),	
        audioChunkCount(0), 
		totalAudioDuration(0), 
        totalVideoDuration(0),
		startAudioTimeTicks(0),
		startVideoTimeTicks(0),
		trackIdCounter(0), 
        manifestInitialized(false), 
		pPublishingPoint(NULL) {

	}

	void SessionInfo::Reset() {
		ClearQueue(retainedAudioFragments);
		ClearQueue(retainedVideoFragments);
		ClearQueue(staleAudioFragments);
		ClearQueue(staleVideoFragments);
		audioLookAheadQue.clear();
		videoLookAheadQue.clear();
		fragmentSequence = 1;
		videoChunkCount = 0;
		audioChunkCount = 0;
		totalAudioDuration = 0;
		totalVideoDuration = 0;
		startAudioTimeTicks = 0;
		startVideoTimeTicks = 0;
        trackIdCounter = 0;
	}

	void SessionInfo::ClearQueue(queue<RetainedFragmentInfo>& refQueue) {
		queue<RetainedFragmentInfo> empty;
		std::swap(refQueue, empty);
	}

    map<string, SessionInfo> OutFileMSSStream::_sessionInfo;
    string OutFileMSSStream::_prevGroupName;
    string OutFileMSSStream::_prevLocalStreamName;
    map<string, MSSManifest*> OutFileMSSStream::_prevManifest;
    map<string, MSSPublishingPoint*> OutFileMSSStream::_prevPublishingPoint;
    map<string, GroupState> OutFileMSSStream::_groupStates;

    OutFileMSSStream::OutFileMSSStream(BaseProtocol *pProtocol, string name,
            Variant &settings)
    : BaseOutFileStream(pProtocol, ST_OUT_FILE_MSS, name),
    //audio
    _audioRate(-1),
    //video
    _videoRate(-1),
    _settings(settings),
    _pGenericProcessDataSetup(NULL),
	_pApplication(NULL),
    _chunkOnIDR(false),
    _pVideoFragment(NULL),
    _pAudioFragment(NULL),
    _chunkLength(0),
    _isLastFragment(false),
    _overwriteDestination(false),
    _isRolling(false),
	_isLive(false),
    _playlistLength(0),
    _videoChunked(false),
	_addStreamFlag(0),
	_removeStreamFlag(0){
		_pApplication = pProtocol->GetApplication();
    }

    OutFileMSSStream* OutFileMSSStream::GetInstance(BaseClientApplication *pApplication,
            string name, Variant &settings) {
        //1. Create a dummy protocol
        PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

        //2. create the parameters
        Variant parameters;
        parameters["customParameters"]["mssConfig"] = settings;

        //3. Initialize the protocol
        if (!pDummyProtocol->Initialize(parameters)) {
            FATAL("Unable to initialize passthrough protocol");
            pDummyProtocol->EnqueueForDelete();
            return NULL;
        }

        //4. Set the application
        pDummyProtocol->SetApplication(pApplication);

        //5. Create the MSS stream
        OutFileMSSStream *pOutMSS = new OutFileMSSStream(pDummyProtocol, name, settings);
        if (!pOutMSS->SetStreamsManager(pApplication->GetStreamsManager())) {
            FATAL("Unable to set the streams manager");
            delete pOutMSS;
            pOutMSS = NULL;
            return NULL;
        }
        pDummyProtocol->SetDummyStream(pOutMSS);

        //6. Done
        return pOutMSS;
    }

    OutFileMSSStream::~OutFileMSSStream() {
        // Set the last fragment flag
        _isLastFragment = true;

		// Close the fragment
		CloseFragment(false);
		CloseFragment(true);

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

        if (_ismType == "isml" && !_sessionInfo.empty()) {
			if (NULL != _sessionInfo[_sessionKey].pPublishingPoint &&
                _sessionInfo[_sessionKey].pPublishingPoint->GetReferenceCount() > 1) {
				_sessionInfo[_sessionKey].pPublishingPoint->DecrementReferenceCount();
				_sessionInfo[_sessionKey].pPublishingPoint = NULL;
                _sessionInfo.erase(_sessionKey);
			}
			else if (NULL != _sessionInfo[_sessionKey].pPublishingPoint &&
				_sessionInfo[_sessionKey].pPublishingPoint->GetReferenceCount() == 1) {
				delete _sessionInfo[_sessionKey].pPublishingPoint;
				_sessionInfo[_sessionKey].pPublishingPoint = NULL;
                _prevGroupName = "";
                _prevPublishingPoint.erase(_groupName);
                _groupStates.erase(_groupName);
                _sessionInfo.erase(_sessionKey);
            }
        }
	}

    void OutFileMSSStream::GetStats(Variant &info, uint32_t namespaceId) {
        BaseOutStream::GetStats(info, namespaceId);
        info["mssSettings"] = _settings;
    }

    bool OutFileMSSStream::IsCompatibleWithType(uint64_t type) {
        return TAG_KIND_OF(type, ST_IN_NET_RTMP)
                || TAG_KIND_OF(type, ST_IN_NET_RTP)
                || TAG_KIND_OF(type, ST_IN_NET_TS)
                || TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
                || TAG_KIND_OF(type, ST_IN_FILE)
                || TAG_KIND_OF(type, ST_IN_NET_EXT);
    }

    bool OutFileMSSStream::SignalPlay(double &dts, double &length) {
        return true;
    }

    bool OutFileMSSStream::SignalPause() {
        return true;
    }

    bool OutFileMSSStream::SignalResume() {
        return true;
    }

    bool OutFileMSSStream::SignalSeek(double &dts) {
        return true;
    }

    bool OutFileMSSStream::SignalStop() {
        return true;
    }

    void OutFileMSSStream::SignalAttachedToInStream() {
    }

    void OutFileMSSStream::SignalDetachedFromInStream() {
        _pProtocol->EnqueueForDelete();
    }

    void OutFileMSSStream::SignalStreamCompleted() {
        _pProtocol->EnqueueForDelete();
    }

    void OutFileMSSStream::SignalAudioStreamCapabilitiesChanged(
            StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
            AudioCodecInfo *pNew) {
        GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
        if (_pAudioFragment != NULL)
            _pAudioFragment->ForceClose();
    }

    void OutFileMSSStream::SignalVideoStreamCapabilitiesChanged(
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

    bool OutFileMSSStream::FeedData(uint8_t *pData, uint32_t dataLength,
            uint32_t processedLength, uint32_t totalLength,
            double pts, double dts, bool isAudio) {
        uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
        uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
        packetsCount++;
        bytesCount += dataLength;
        return GenericProcessData(pData, dataLength, processedLength, totalLength,
                pts, dts, isAudio);
    }

    void OutFileMSSStream::InitializeManifest() {
		if (_ismType == "ismc") { //client manifest
			if (NULL != _sessionInfo[_sessionKey].pManifest) {
				delete _sessionInfo[_sessionKey].pManifest;
			}
			_sessionInfo[_sessionKey].pManifest = new MSSManifest(_isLive);
			_sessionInfo[_sessionKey].pManifest->IncrementReferenceCount();
			_prevManifest[_groupName] = _sessionInfo[_sessionKey].pManifest;
		} else if (_ismType == "isml") {//server manifest a.k.a. live ingest
			if (NULL != _sessionInfo[_sessionKey].pPublishingPoint) {
				delete _sessionInfo[_sessionKey].pPublishingPoint;
			}
			_sessionInfo[_sessionKey].pPublishingPoint = new MSSPublishingPoint();
			_sessionInfo[_sessionKey].pPublishingPoint->IncrementReferenceCount();
			_prevPublishingPoint[_groupName] = _sessionInfo[_sessionKey].pPublishingPoint;
			_sessionInfo[_sessionKey].manifestInitialized = true;
		}
        _prevGroupName = _groupName;
        _prevLocalStreamName = _localStreamName;
        _sessionInfo[_sessionKey].Reset();
        _sessionInfo[_sessionKey].isFirstRun = false;
    }

    bool OutFileMSSStream::FinishInitialization(
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

        //FINEST("\n%s", STR(_settings.ToString()));
        _chunkOnIDR = (bool) _settings["chunkOnIDR"];
        _chunkLength = ((double) _settings["chunkLength"]) * MSS_TIMESCALE;

        // Initialize the rest
        _overwriteDestination = (bool) _settings["overwriteDestination"];
        _isRolling = (bool) (_settings["playlistType"] == "rolling");
		_isLive = (bool)_settings["isLive"];
        _playlistLength = (uint32_t) _settings["playlistLength"];
        _staleRetentionCount = (uint32_t) _settings["staleRetentionCount"];
        _localStreamName = (string) _settings["localStreamName"];
        _groupName = (string) _settings["groupName"];
		// denotes the type of manifest to create
		_ismType = (string) _settings.GetValue("ismType", false);
        _sessionKey = _groupName + _localStreamName;
        // custom settings
        _settings["lookahead"] = MSS_LOOKAHEAD;
		bool isAutoMSS = (bool)_settings["isAutoMSS"];
		if (!isAutoMSS) {
			if (_prevGroupName != _groupName) {
				InitializeManifest();
			} else {
				uint16_t referenceCount = (_ismType == "ismc") ? _prevManifest[_groupName]->GetReferenceCount()
					: _prevPublishingPoint[_groupName]->GetReferenceCount();
				if (referenceCount == 1 && !(_sessionInfo[_sessionKey].isFirstRun)
					&& ((bool)_settings["cleanupDestination"])) {
					InitializeManifest();
				} else {
					if (_ismType == "ismc") {
						_sessionInfo[_sessionKey].pManifest = _prevManifest[_groupName];
						if (_prevLocalStreamName != _localStreamName) {
							_prevManifest[_groupName]->IncrementReferenceCount();
						}
					} else if (_ismType == "isml") {
						_sessionInfo[_sessionKey].pPublishingPoint = _prevPublishingPoint[_groupName];
						_sessionInfo[_sessionKey].manifestInitialized = false;
						if (_prevLocalStreamName != _localStreamName) {
							_prevPublishingPoint[_groupName]->IncrementReferenceCount();
						}
					}
					_sessionInfo[_sessionKey].isFirstRun = false;				
				}
			}
		} else {
			InitializeManifest();
		}

		uint32_t bandwidth = (uint32_t)_settings["bandwidth"];
		if (bandwidth == 0)
			bandwidth = (uint32_t)(_pGenericProcessDataSetup->_pStreamCapabilities->GetTransferRate());
		uint32_t audiorateFolder, videorateFolder;
		audiorateFolder = videorateFolder = bandwidth;
		uint32_t audioBitrate = (uint32_t)_settings["audioBitrate"];
		uint32_t videoBitrate = (uint32_t)_settings["videoBitrate"];
		if (audioBitrate > 0)
			audiorateFolder = audioBitrate;
		if (videoBitrate > 0)
			videorateFolder = videoBitrate;

		string videoFolder, audioFolder;

		if (!isAutoMSS && _settings["localStreamNames"].MapSize() > 1 && _ismType == "ismc") {
			string groupTargetFolder = (string)_settings["groupTargetFolder"];
			if (groupTargetFolder[groupTargetFolder.size() - 1] != PATH_SEPARATOR) {
				groupTargetFolder += PATH_SEPARATOR;
				_settings["groupTargetFolder"] = groupTargetFolder;
			}

			if (!createFolder(groupTargetFolder, true)) {
				FATAL("Unable to create target folder %s", STR(groupTargetFolder));
				return false;
			}

			videoFolder = format("%s%"PRIu32"%c%s%c", STR(groupTargetFolder), videorateFolder, PATH_SEPARATOR, MSS_VIDEO_FOLDER_NAME, PATH_SEPARATOR);
			audioFolder = format("%s%"PRIu32"%c%s%c", STR(groupTargetFolder), audiorateFolder, PATH_SEPARATOR, MSS_AUDIO_FOLDER_NAME, PATH_SEPARATOR);
		} else {
			// Create the correct path for the manifest file and fragments
			_targetFolder = (string)_settings["targetFolder"];
			if (_targetFolder[_targetFolder.size() - 1] != PATH_SEPARATOR) {
				_targetFolder += PATH_SEPARATOR;
				_settings["targetFolder"] = _targetFolder;
			}

			if (!createFolder(_targetFolder, true)) {
				FATAL("Unable to create target folder %s", STR(_targetFolder));
				return false;
			}
			videoFolder = format("%s%s%c%"PRIu32"%c", STR(_targetFolder), MSS_VIDEO_FOLDER_NAME, PATH_SEPARATOR, videorateFolder, PATH_SEPARATOR);
			audioFolder = format("%s%s%c%"PRIu32"%c", STR(_targetFolder), MSS_AUDIO_FOLDER_NAME, PATH_SEPARATOR, audiorateFolder, PATH_SEPARATOR);
		}

		createFolder(videoFolder, true);
		createFolder(audioFolder, true);

		_settings["videoFolder"] = videoFolder;
		_settings["audioFolder"] = audioFolder;
		_sessionInfo[_sessionKey].videoFolder = videoFolder;
		_sessionInfo[_sessionKey].audioFolder = audioFolder;

        if (_ismType == "ismc") {
			if (!_sessionInfo[_sessionKey].pManifest->Init(_settings, _pGenericProcessDataSetup->_pStreamCapabilities)) {
				FATAL("Unable to initialize Manifest Writer");
				return false;
			}
            _sessionInfo[_sessionKey].pManifest->CreateDefault();
            GetEventLogger()->LogMSSPlaylistUpdated(_sessionInfo[_sessionKey].pManifest->GetManifestPath());
        } else if (_sessionInfo[_sessionKey].manifestInitialized && _ismType == "isml") {
            FOR_MAP(_settings["streamNames"], string, Variant, i) {
                string currentStreamName = (string)MAP_VAL(i);
		        map<uint32_t, BaseStream*> streams = _pApplication->GetStreamsManager()->FindByTypeByName(ST_IN_NET, currentStreamName, true, false);

                if (streams.empty()) {
                    FATAL("Stream `%s` does not exist", STR(currentStreamName));
                    return false;
                }
            }

            // Parse publishingPoint parameter and build a parameter out of it
             Variant parameters;
            parameters["customParameters"] = _settings;
            string publishingPoint = _settings["publishingPoint"];
            size_t startDelimiter = publishingPoint.find_first_of('/') + 1;
            size_t lastDelimiter = publishingPoint.find_last_of(':');
            string liveIngestHost = publishingPoint.substr(startDelimiter + 1, lastDelimiter - (startDelimiter + 1));
            string temp =  publishingPoint.substr(lastDelimiter + 1, publishingPoint.length() - (lastDelimiter + 1));
            uint16_t liveIngestPort = (uint16_t)atoi(temp.substr(0, temp.find_first_of('/')).c_str());
            startDelimiter = temp.find_first_of('/');
            temp = temp.substr(startDelimiter, temp.length() - startDelimiter);
            //Generate a timestamp to be appended in Events id when ingestMode = loop
            string ingestMode = _settings["ingestMode"];
            if (ingestMode == "loop" && temp.find("Events") != string::npos) {
                time_t now = getutctime();
                struct tm* currTime = NULL;
                char availabilityStartTime[30];
                currTime = localtime(&now);
                strftime (availabilityStartTime, sizeof(availabilityStartTime), "%Y-%m-%d-%H:%M:%S", currTime);
                string localTimestamp = availabilityStartTime;
                //Append the timestamp to the Events id
                string::iterator replaceBegin = std::find(temp.begin(), temp.end(), '(');
                string::iterator replaceEnd = std::find(replaceBegin, temp.end(), ')');
                string eventId;
                string::iterator replaceLooper = replaceBegin;
                while (replaceLooper++ != replaceEnd - 1) {
                    eventId += *replaceLooper;
                }
                eventId += localTimestamp;
                temp.replace(++replaceBegin, replaceEnd, eventId);
            }
            parameters["host"] = liveIngestHost;
            parameters["port"] = liveIngestPort;
            parameters["httpHost"] = format("%s:%d", STR(liveIngestHost), liveIngestPort);
            parameters["publishingPoint"] = temp;

			// save publishing point
			if (!_sessionInfo[_sessionKey].pPublishingPoint->Init(_settings, _pApplication)) {
				FATAL("Unable to initialize Publishing Point");
				return false;
			}
			_sessionInfo[_sessionKey].pPublishingPoint->CreateDefault();

	        // Initiate the HTTP connection to Smooth Streaming server (in case of live ingest)
            BaseHTTPAppProtocolHandler* pProtocolHandler = NULL;
            if (NULL != _pApplication) {
                string scheme = "msshttp";
                pProtocolHandler = (BaseHTTPAppProtocolHandler*)_pApplication->GetProtocolHandler(scheme);
            }
            if (pProtocolHandler == NULL) {
                FATAL("Unable to find protocol handler for scheme msshttp in application %s", STR(GetName()));
                return false;
            }
            if (!pProtocolHandler->SendPOSTRequest(CONF_PROTOCOL_OUTBOUND_HTTP_MSS_INGEST, parameters)) {
                FATAL("Failed to connect to Smooth Streaming server");
                return false;
            }      
        }

        return true;
    }

    bool OutFileMSSStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
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
        double duration = _pVideoFragment->Duration();

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
                        if (!NewFragment(pts, dts, false, false)) {
                            FATAL("Unable to open new fragment");
                            return false;
                        }
						_videoChunked = true;
                    }
                } else {
                    //11. We don't need to chunk on IDR, so we chunk anyway because
                    //the duration condition is met
                    if (!NewFragment(pts, dts, false, false)) {
                        FATAL("Unable to open new fragment");
                        return false;
                    }
					_videoChunked = true;
                }
            }
        }

        //12. Push the data into the fragment
        if (_pVideoFragment->PushVideoData(buffer, pts, dts, isKeyFrame)) {
            _lastVideoTS = dts;
            return true;
        } else {
            GetEventLogger()->LogMSSChunkError("Could not write video sample to " + _pVideoFragment->GetPath());
            return false;
        }
    }

    bool OutFileMSSStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
        // Store the last audio TS
        //_lastAudioTS = dts;

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
        double duration = _pAudioFragment->Duration();

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
			//   chunk the audio if it has equal or more duration with the video
			if (duration >= _chunkLength) {
                //9. Check and see if we need to chunk on IDR
                if (_chunkOnIDR) {
                    //10. Only open a new track if we don't have a video track
					if (_pGenericProcessDataSetup->_hasVideo) {
                        if (_videoChunked) {
                            if (!NewFragment(pts, dts, false, true)) {
                                FATAL("Unable to open new fragment");
                                return false;
                            }
						    _videoChunked = false;
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
                        if (_videoChunked) {
						    if (!NewFragment(pts, dts, false, true)) {
							    FATAL("Unable to open new fragment");
							    return false;
						    }
						    _videoChunked = false;
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
            GetEventLogger()->LogMSSChunkError("Could not write audio sample to " + _pAudioFragment->GetPath());
            return false;
        }
    }

    bool OutFileMSSStream::IsCodecSupported(uint64_t codec) {
        return (codec == CODEC_VIDEO_H264)
                || (codec == CODEC_AUDIO_AAC)
                ;
    }

    bool OutFileMSSStream::OpenFragment(double pts, double dts, double additionalFramesPercent, bool isAudio) {
        //1. Check and see if we have a fragment already opened
        MSSFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        if ( pFragment != NULL) {
            FATAL("Fragment already opened");
            GetEventLogger()->LogMSSChunkError("Opening a fragment but is already open.");
            return false;
        }

		string fragmentFullPath;
		string targetPath = isAudio ? _sessionInfo[_sessionKey].audioFolder : _sessionInfo[_sessionKey].videoFolder;
		//2. Compute the fragment path
		if (isAudio) {
			if (_sessionInfo[_sessionKey].startAudioTimeTicks == 0)
				_sessionInfo[_sessionKey].startAudioTimeTicks = (uint64_t)getutctime() * MSS_TIMESCALE;
		} else {
			if (_sessionInfo[_sessionKey].startVideoTimeTicks == 0)
				_sessionInfo[_sessionKey].startVideoTimeTicks = (uint64_t)getutctime() * MSS_TIMESCALE;
		}

		fragmentFullPath = format("%s%"PRIu64".fmp4", STR(targetPath),
			isAudio ? _sessionInfo[_sessionKey].startAudioTimeTicks : _sessionInfo[_sessionKey].startVideoTimeTicks);

        // Test if we can overwrite the fragment
		if (fileExists(fragmentFullPath)) {
            if (_overwriteDestination) {
                if (!deleteFile(fragmentFullPath)) {
                    GetEventLogger()->LogMSSChunkError("Could not delete " + fragmentFullPath);
                    return false;
                }
            } else {
                string error = format("Fragment file %s already present and overwrite is not permitted!", STR(fragmentFullPath));
                FATAL("%s", STR(error));
                GetEventLogger()->LogMSSChunkError(error);
                return false;
            }
        }

		//DEBUG("Creating fragment: %s", STR(fragmentFullPath));
		//3. Create the fragment
		pFragment = new MSSFragment(fragmentFullPath,
			_sessionInfo[_sessionKey].fragmentSequence++,
			1.0 * _chunkLength / MSS_TIMESCALE,
			isAudio ? _audioRate : _videoRate,
			additionalFramesPercent,
			pts,
			dts,
			isAudio,
            _groupStates[_groupName]);

		if (isAudio) {
			_pAudioFragment = pFragment;
		} else {
			_pVideoFragment = pFragment;
		}

		//4. Do initialization
        if (_ismType == "isml") {
		    //set the Xml String in the MSS Fragment
            if (NULL != _sessionInfo[_sessionKey].pPublishingPoint)
		        pFragment->SetXmlString(_sessionInfo[_sessionKey].pPublishingPoint->GetXmlString()); 

            // Determine appropriate trak Id based on order of stream input
            Variant listofStreams = _settings["streamNames"];
            uint16_t streamIndex = 0;
            for (; streamIndex < listofStreams.MapSize(); ++streamIndex) {
                if (listofStreams[streamIndex] == _localStreamName)
                    break;
            }
            uint16_t trakIdNormalizer = 0;
            if (_pGenericProcessDataSetup->_hasAudio)
                ++trakIdNormalizer;
            if (_pGenericProcessDataSetup->_hasVideo)
                ++trakIdNormalizer;
            trakIdNormalizer *= streamIndex;

            if (_sessionInfo[_sessionKey].trackIDs[isAudio] == 0)
                _sessionInfo[_sessionKey].trackIDs[isAudio] = ++_sessionInfo[_sessionKey].trackIdCounter + trakIdNormalizer;
        } 

        if (!pFragment->Initialize(_pGenericProcessDataSetup, _pApplication, GetInStream(), _ismType, 
            _settings["streamNames"], _sessionInfo[_sessionKey].trackIDs[isAudio])) {
			string error = "Unable to initialize " + fragmentFullPath;
			FATAL("%s", STR(error));
			GetEventLogger()->LogMSSChunkError(error);
			return false;
		}

		pFragment->SetLastTS(isAudio ? _lastAudioTS : _lastVideoTS, isAudio);

		_chunkInfo.Reset();
		_chunkInfo["file"] = fragmentFullPath;
		_chunkInfo["startTime"] = Variant::Now();
		GetEventLogger()->LogMSSChunkCreated(_chunkInfo);

        return true;
    }

    bool OutFileMSSStream::CloseFragment(bool isAudio, double pts) {

        //1. Check and see if no fragment is currently opened
        MSSFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        if (pFragment == NULL) {
            FATAL("Fragment not opened");
            GetEventLogger()->LogMSSChunkError("Closing a fragment that was not opened.");
            return false;
        }

		uint64_t currentDuration = pFragment->GetTotalDuration(isAudio);
		uint64_t& currentUTCStamp = isAudio ? _sessionInfo[_sessionKey].startAudioTimeTicks : _sessionInfo[_sessionKey].startVideoTimeTicks;
		uint64_t adjustedDuration = currentDuration;

		//Set the tfxd atom
		if (_ismType == "ismc") {
			// synchronize video and audio duration to avoid player freeze-up
			if (isAudio && _pGenericProcessDataSetup->_hasVideo) {
				if (_sessionInfo[_sessionKey].videoLookAheadQue.size() > 0) {
					uint64_t fragment_duration = (_sessionInfo[_sessionKey].videoLookAheadQue.end() - 1)->duration;
					if (fragment_duration > currentDuration)
						adjustedDuration = fragment_duration;
				}
			} else if (!isAudio && _pGenericProcessDataSetup->_hasAudio) {
				if (NULL != _pAudioFragment
					&& _pAudioFragment->GetTotalDuration(true) > currentDuration)
					adjustedDuration = _pAudioFragment->GetTotalDuration(true);
			}

			pFragment->SetTfxdAbsoluteTimestamp(currentUTCStamp);
			pFragment->SetTfxdFragmentDuration(adjustedDuration);
			pFragment->Finalize();

			if (_sessionInfo[_sessionKey].pManifest == NULL) {
				FATAL("Manifest not opened");
				GetEventLogger()->LogMSSChunkError("Manifest not opened, closing fragment.");

				_chunkInfo.Reset();
				_chunkInfo["file"] = pFragment->GetPath();
				_chunkInfo["startTime"] = pFragment->GetStartTime();
				_chunkInfo["stopTime"] = Variant::Now();
				_chunkInfo["frameCount"] = pFragment->GetFrameCount();
				GetEventLogger()->LogMSSChunkClosed(_chunkInfo);

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
		}

        // update lookahead que
	    vector<LookAhead> &lookAheadQue = isAudio ? _sessionInfo[_sessionKey].audioLookAheadQue : _sessionInfo[_sessionKey].videoLookAheadQue;
		LookAhead newLookAhead = { pFragment->GetPath(), 0, 0, 0, adjustedDuration, currentUTCStamp };
		ADD_VECTOR_END(lookAheadQue, newLookAhead);

		if (_ismType == "ismc") {
			(lookAheadQue.end() - 1)->tfxdOffset = pFragment->GetTfxdOffset();
			(lookAheadQue.end() - 1)->freeOffset = pFragment->GetFreeOffset();
		}

        // update the manifest only when the que is full (enough fragments for lookahead)
        if (lookAheadQue.size() > MSS_LOOKAHEAD) {
			vector<LookAhead>::iterator iteratorFirstLookAhead = lookAheadQue.begin();
			if (_ismType == "ismc") {
				IOBuffer atomtfrf_buffer;
				// create the tfrf headers if this is the first entry
				uint8_t atom_tfrf[] = {
					0x00, 0x00, 0x00, 0x3D, // atom size
					0x75, 0x75, 0x69, 0x64, // atom tag = uuid
					0xD4, 0x80, 0x7E, 0xF2, // user type (next 16 bytes)
					0xCA, 0x39, 0x46, 0x95,
					0x8E, 0x54, 0x26, 0xCB,
					0x9E, 0x46, 0xA7, 0x9F,
					0x01, 0x00, 0x00, 0x00, // 1 byte version, 3 bytes flags
					0x02,					// fragment count
				};
				atomtfrf_buffer.IgnoreAll();
				atomtfrf_buffer.ReadFromBuffer(atom_tfrf, sizeof(atom_tfrf));

				vector<LookAhead>::iterator iteratorLookAhead;
				for (iteratorLookAhead = iteratorFirstLookAhead + 1; iteratorLookAhead != lookAheadQue.end(); ++iteratorLookAhead) {
					atomtfrf_buffer.ReadFromRepeat(0, 8);
					EHTONLLP(GETIBPOINTER(atomtfrf_buffer) + GETAVAILABLEBYTESCOUNT(atomtfrf_buffer) - 8, iteratorLookAhead->manifestStamp);

					atomtfrf_buffer.ReadFromRepeat(0, 8);
					EHTONLLP(GETIBPOINTER(atomtfrf_buffer) + GETAVAILABLEBYTESCOUNT(atomtfrf_buffer) - 8, iteratorLookAhead->duration);
				}

				//add the tfrf atom
				File fileFragment;
				if (!fileFragment.Initialize(iteratorFirstLookAhead->fileName, FILE_OPEN_MODE_WRITE)) {
					FATAL("Unable to open fragment %s", STR(iteratorFirstLookAhead->fileName));
					return false;
				}
				fileFragment.SeekTo(iteratorFirstLookAhead->tfxdOffset - GETAVAILABLEBYTESCOUNT(atomtfrf_buffer));
				fileFragment.WriteBuffer(GETIBPOINTER(atomtfrf_buffer), GETAVAILABLEBYTESCOUNT(atomtfrf_buffer));

				fileFragment.SeekTo(iteratorFirstLookAhead->freeOffset);
				uint32_t atomFreeSize = 0;
				fileFragment.ReadUI32(&atomFreeSize);
				atomFreeSize -= GETAVAILABLEBYTESCOUNT(atomtfrf_buffer);
				fileFragment.SeekTo(iteratorFirstLookAhead->freeOffset);
				fileFragment.WriteUI32(atomFreeSize & 0xffffffff);

				fileFragment.Flush();
				fileFragment.Close();

				uint32_t& chunkCount = isAudio ? ++_sessionInfo[_sessionKey].audioChunkCount : ++_sessionInfo[_sessionKey].videoChunkCount;

				if (chunkCount > _sessionInfo[_sessionKey].pManifest->GetChunkCount(isAudio)) { //make sure only one entry is added in the manifest for every ABR entry
					XmlElements* pStreamIndexEntry = new XmlElements("c", "");
					pStreamIndexEntry->AddAttribute("d", iteratorFirstLookAhead->duration);
					pStreamIndexEntry->AddAttribute("t", iteratorFirstLookAhead->manifestStamp);
					if (isAudio) {
						_sessionInfo[_sessionKey].pManifest->SetChunkCount(isAudio, _sessionInfo[_sessionKey].audioChunkCount);
						if (!_isLive) {
							_sessionInfo[_sessionKey].totalAudioDuration += iteratorFirstLookAhead->duration;
							if (_sessionInfo[_sessionKey].totalAudioDuration > _sessionInfo[_sessionKey].totalVideoDuration) {
								_sessionInfo[_sessionKey].pManifest->SetDuration(_sessionInfo[_sessionKey].totalAudioDuration);
							}
						}
						pStreamIndexEntry->AddAttribute("n", _sessionInfo[_sessionKey].audioChunkCount);
						if (_sessionInfo[_sessionKey].pManifest->AddStreamIndexEntry(pStreamIndexEntry, 2)) {
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
							delete pStreamIndexEntry;
							pStreamIndexEntry = NULL;
						}
					} else {
						_sessionInfo[_sessionKey].pManifest->SetChunkCount(isAudio, _sessionInfo[_sessionKey].videoChunkCount);
						if (!_isLive) {
							_sessionInfo[_sessionKey].totalVideoDuration += iteratorFirstLookAhead->duration;
							if (_sessionInfo[_sessionKey].totalVideoDuration > _sessionInfo[_sessionKey].totalAudioDuration) {
								_sessionInfo[_sessionKey].pManifest->SetDuration(_sessionInfo[_sessionKey].totalVideoDuration);
							}
						}
						pStreamIndexEntry->AddAttribute("n", _sessionInfo[_sessionKey].videoChunkCount);
						if (_sessionInfo[_sessionKey].pManifest->AddStreamIndexEntry(pStreamIndexEntry, 1)) {
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
							delete pStreamIndexEntry;
							pStreamIndexEntry = NULL;
						}
					}
				}
			}

			// deque oldest entry since its lookahead table has been filled
			lookAheadQue.erase(iteratorFirstLookAhead);
        }

		// the following codes are for server live ingest
        if (_ismType == "isml") {
			uint64_t absoluteTimestamp = 0;

			// compute the timestamp
			if (lookAheadQue.size() >= 2) {
				lookAheadQue[lookAheadQue.size() - 1].timeStamp = lookAheadQue[lookAheadQue.size() - 2].timeStamp
					+ lookAheadQue[lookAheadQue.size() - 2].duration;
				absoluteTimestamp = lookAheadQue[lookAheadQue.size() - 1].timeStamp;
			}

			pFragment->SetTfxdAbsoluteTimestamp(absoluteTimestamp * MSSLI_ADJUST_TIMESCALE);
			pFragment->SetTfxdFragmentDuration(adjustedDuration * MSSLI_ADJUST_TIMESCALE);

		    if (_isLastFragment) {  // if it's the last fragment, Add an empty Mfra box. See page 58 of MS-SSTR specification
                if (_groupStates[_groupName].trakCounts > 1) {
                    --_groupStates[_groupName].trakCounts;
                } else { 
			        pFragment->AddMfraBox();
                }
		    }
            pFragment->Finalize();
    
            //Ingest the fragments to Smooth Streaming server using http post
            //=============================================================>
            HTTPMssLiveIngest* pInstance = HTTPMssLiveIngest::GetInstance();
            if (NULL == pInstance) {
                FATAL("NULL instance of HTTP ingestor");
                _prevGroupName = "";
                GetProtocol()->EnqueueForDelete();
                EnqueueForDelete();
                return false;
            }
            File file;
            if (!file.Initialize(pFragment->GetPath(), FILE_OPEN_MODE_READ)) {
                FATAL("Unable to initialize fragment file for read!");
                return false;
            }
            Variant chunkSizes = pFragment->GetLiveIngestMessageSizes();
            IOBuffer postMessage;
            if (chunkSizes.HasKey("firstFragment")) {         
                //POST ftyp header
                postMessage.IgnoreAll();
                postMessage.ReadFromFs(file, (uint32_t)chunkSizes["ftyp"]);
                if (!pInstance->SendFragments(GETIBPOINTER(postMessage), (uint32_t)GETAVAILABLEBYTESCOUNT(postMessage))) {
                    FATAL("Unable to POST ftyp header");
                    return false;
                }
                //seek to isml(1st uuid) offset
                if (!file.SeekTo((uint64_t)chunkSizes["ftyp"])) {
				    FATAL("Unable to seek into file");
				    return false;
			    }
                postMessage.IgnoreAll();
                postMessage.ReadFromFs(file, (uint32_t)chunkSizes["ismlmoov"]);
                //POST isml + moov header
                if (!pInstance->SendFragments(GETIBPOINTER(postMessage), (uint32_t)GETAVAILABLEBYTESCOUNT(postMessage))) {
                    FATAL("Unable to POST isml + moov header");
                    return false;
                }
                //seek to moof offset
                if (!file.SeekTo((uint64_t)chunkSizes["ftyp"] + (uint64_t)chunkSizes["ismlmoov"])) {
				    FATAL("Unable to seek into file");
				    return false;
			    }
            }
            
            //POST moof + mdat header
            postMessage.IgnoreAll();
            postMessage.ReadFromFs(file, (uint32_t)chunkSizes["moofmdat"]);
            if (!pInstance->SendFragments(GETIBPOINTER(postMessage), (uint32_t)GETAVAILABLEBYTESCOUNT(postMessage))) {
                FATAL("Unable to POST moof + mdat header");
                return false;
            }

            if (chunkSizes.HasKey("mfra")) {
                //seek to mfra offset
                if (!file.SeekTo((uint64_t)chunkSizes["moofmdat"])) {
				    FATAL("Unable to seek into file");
				    return false;
			    }
                //POST mfra header
                postMessage.IgnoreAll();
                postMessage.ReadFromFs(file, (uint32_t)chunkSizes["mfra"]);
                if (!pInstance->SendFragments(GETIBPOINTER(postMessage), (uint32_t)GETAVAILABLEBYTESCOUNT(postMessage))) {
                    FATAL("Unable to POST mfra header");
                    return false;
                }
                pInstance->SignalEndOfPOST(); // this is the last fragment, signal the http connection to complete
                _groupStates.erase(_groupName); // erase the group state as this is the end of ingest.
            }
            //Ingest to Smooth Streaming server complete
            //<=========================================
        } //<== server live ingest

        if (_isRolling) {
            RetainedFragmentInfo fragmentInfo;
            fragmentInfo.fragmentFullPath = pFragment->GetPath();
			fragmentInfo.manifestEntry = format("%"PRIu64, currentUTCStamp);
			fragmentInfo.fragmentDuration = adjustedDuration;
			size_t queueSize, staleQueueSize;

            if (isAudio) {
                _sessionInfo[_sessionKey].retainedAudioFragments.push(fragmentInfo);
                _sessionInfo[_sessionKey].staleAudioFragments.push(fragmentInfo);
				queueSize = _sessionInfo[_sessionKey].retainedAudioFragments.size();
				staleQueueSize = _sessionInfo[_sessionKey].staleAudioFragments.size();
            } else {
                _sessionInfo[_sessionKey].retainedVideoFragments.push(fragmentInfo);
                _sessionInfo[_sessionKey].staleVideoFragments.push(fragmentInfo);
				queueSize = _sessionInfo[_sessionKey].retainedVideoFragments.size();
				staleQueueSize = _sessionInfo[_sessionKey].staleVideoFragments.size();
            }

            while (queueSize > _playlistLength + MSS_LOOKAHEAD) {
				string manifestStreamIndexTs;		
				if (isAudio) {
					if (_ismType == "ismc") {
						manifestStreamIndexTs = _sessionInfo[_sessionKey].retainedAudioFragments.front().manifestEntry;
						_sessionInfo[_sessionKey].pManifest->RemoveStreamIndexEntry(manifestStreamIndexTs, 2);
						//subtract the duration of the removed item to the total duration
						if (!_isLive)
							_sessionInfo[_sessionKey].totalAudioDuration -= _sessionInfo[_sessionKey].retainedAudioFragments.front().fragmentDuration;
					}
					_sessionInfo[_sessionKey].retainedAudioFragments.pop();
					queueSize = _sessionInfo[_sessionKey].retainedAudioFragments.size();
				} else {
					if (_ismType == "ismc") {
						manifestStreamIndexTs = _sessionInfo[_sessionKey].retainedVideoFragments.front().manifestEntry;
						_sessionInfo[_sessionKey].pManifest->RemoveStreamIndexEntry(manifestStreamIndexTs, 1);
						//subtract the duration of the removed item to the total duration
						if (!_isLive)
							_sessionInfo[_sessionKey].totalVideoDuration -= _sessionInfo[_sessionKey].retainedVideoFragments.front().fragmentDuration;
					}
					_sessionInfo[_sessionKey].retainedVideoFragments.pop();
					queueSize = _sessionInfo[_sessionKey].retainedVideoFragments.size();
				}

				if (!_isLive && _ismType == "ismc") {
					uint64_t retainedDuration = _sessionInfo[_sessionKey].totalAudioDuration > _sessionInfo[_sessionKey].totalVideoDuration
						? _sessionInfo[_sessionKey].totalAudioDuration : _sessionInfo[_sessionKey].totalVideoDuration;
					//update the duration
					_sessionInfo[_sessionKey].pManifest->SetDuration(retainedDuration);
				}
            }

			// update the manifest file to disk
			if (_ismType == "ismc") {
				if (isAudio) {
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

            while (staleQueueSize > _playlistLength + MSS_LOOKAHEAD + _staleRetentionCount + 1) {
				string fragmentPath;
				if (isAudio) {
					fragmentPath = _sessionInfo[_sessionKey].staleAudioFragments.front().fragmentFullPath;
					_sessionInfo[_sessionKey].staleAudioFragments.pop();
					staleQueueSize = _sessionInfo[_sessionKey].staleAudioFragments.size();
				} else {
					fragmentPath = _sessionInfo[_sessionKey].staleVideoFragments.front().fragmentFullPath;
					_sessionInfo[_sessionKey].staleVideoFragments.pop();
					staleQueueSize = _sessionInfo[_sessionKey].staleVideoFragments.size();
				}
                // delete the fragment from disk
				if (!deleteFile(fragmentPath)) {
					WARN("Unable to delete file %s", STR(fragmentPath));
                }
            }
        }

		currentUTCStamp += adjustedDuration;

        _chunkInfo.Reset();
		_chunkInfo["file"] = pFragment->GetPath();
		_chunkInfo["startTime"] = pFragment->GetStartTime();
		_chunkInfo["stopTime"] = Variant::Now();
		_chunkInfo["frameCount"] = pFragment->GetFrameCount();
		GetEventLogger()->LogMSSChunkClosed(_chunkInfo);

        // Delete the fragment
        if (isAudio) {
            delete _pAudioFragment;
            _pAudioFragment = NULL;
        } else {
            delete _pVideoFragment;
            _pVideoFragment = NULL;
        }

        //3. Done
        return true;
    }

    bool OutFileMSSStream::NewFragment(double pts, double dts, bool forced, bool isAudio) {
        MSSFragment *pFragment = isAudio ? _pAudioFragment : _pVideoFragment;
        //1. Check and see if we have a fragment opened
        if (pFragment == NULL) {
            FATAL("Fragment not opened");
            GetEventLogger()->LogMSSChunkError("Current fragment is not open.");
            return false;
        }

        //2. Get the string representation in case we need to report an error
        //string temp = (string) * pFragment;

        //3. Get the current additionalFramesPercent from the current frame
        //we will use this as abase for the new value used in the new fragment
        //we are going to open

		double additionalFramesPercent = pFragment->AdditionalFramesPercent();

		//4. If this current fragment was closed by force (no more frames slots)
		//than increase the additionalFramesPercent by 0.5%
		if (forced) {
			//WARN("additionalFramesPercent bumped up: %.2f%% -> %.2f%%",
			//		additionalFramesPercent * 100.0,
			//		(additionalFramesPercent + 0.005)*100.0);
			additionalFramesPercent += 0.005;
		}
		// close the fragment only if we are creating a client fragments
		//5. Close the fragment
		if (!CloseFragment(isAudio, pts)) {
			string error = "Unable to close fragment";
			FATAL("%s", STR(error));
			GetEventLogger()->LogMSSChunkError(error);
			return false;
		}

        // Boundary check
        if ((_sessionInfo[_sessionKey].fragmentSequence % 65536) == 0) {
            _sessionInfo[_sessionKey].fragmentSequence = 1;
        }

        //7. Open a new one
        return OpenFragment(pts, dts, additionalFramesPercent, isAudio);
    }

    bool OutFileMSSStream::InsertSequenceHeaders(double pts, double dts) {
        if (_pGenericProcessDataSetup->_hasVideo) {
            VideoCodecInfoH264 *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
            IOBuffer &temp = pInfo->GetRTMPRepresentation();
            if (!_pVideoFragment->PushVideoData(temp, pts, dts, true)) {
                FATAL("Unable to write Video sequence headers!");
                return false;
            }
        }

        if (_pGenericProcessDataSetup->_hasAudio) {
            AudioCodecInfoAAC *pInfo = _pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
            IOBuffer &temp = pInfo->GetRTMPRepresentation();
            if (!_pAudioFragment->PushAudioData(temp, pts, dts)) {
                FATAL("Unable to write Audio sequence headers!");
                return false;
            }
        }

        return true;
    }
}

#endif	/* HAS_PROTOCOL_MSS */
