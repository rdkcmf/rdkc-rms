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
/**
* wrtcconnection.cpp - create and funnel data through a WebRTC Connection
*
**/

#ifdef HAS_PROTOCOL_WEBRTC
#include "protocols/wrtc/wrtcsdp.h"
#include "protocols/rtp/sdp.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/iceudpprotocol.h"
#include "protocols/wrtc/icetcpprotocol.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "protocols/rtp/streaming/outnetrtpudph264stream.h"
#include "protocols/rtp/connectivity/inboundconnectivity.h"
#include "protocols/ssl/inbounddtlsprotocol.h"
#include "protocols/ssl/outbounddtlsprotocol.h"
#include "application/baseclientapplication.h"
#include "streaming/baseinstream.h"
#include "streaming/streamstypes.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "application/clientapplicationmanager.h"
#include "utils/misc/timeprobemanager.h"

#define PENDING_STREAM_REGISTERED	0x01
#define PENDING_HAS_VCODEC			0x02
#define PENDING_HAS_ACODEC			0x04
#define PENDING_COMPLETE			0x07

#define ENABLE_ICE_UDP
#define ENABLE_ICE_TCP

#define MAX_ZERO_SEND_THRESHOLD 5 // kludge for the sctp->sendData returning ZERO, and CHROME version >= 62
#define STUN_REFRESH_RATE_SEC 15 // refresh rate in seconds before new set of bind requests are sent out
#define WRTC_CAPAB_MAX_HEARTBEAT 10 // maximum value of heartbeats with no responses before considered stale

#define TOGGLE_COMMAND  "TogglingAudio"

uint32_t WrtcConnection::_sessionCounter = 0;

WrtcConnection::WrtcConnection()
	: BaseRTSPProtocol(PT_WRTC_CNX) {
	_sessionCounter++;
	_pFastTimer = _pSlowTimer = NULL;
	_bestIce = NULL;
	_started = false;
	_stopping = false;
	_gotSDP = false;
	_sentSDP = false;
	_createdIces = false;
	_pSDP = NULL;
	_pSDPInfo = NULL;
	_pDtls = NULL;
	_pSctp = NULL;
	_pOutNetMP4Stream = NULL;
	_dataChannelEstablished = false;
	_fullMp4 = false;
	_clientId = "";
	_rmsClientId = "";
	_cmdReceived = "Unknown";
	_stunDomain = "";
	_turnDomain = "";
	
	_isStreamAttached = false;
	_pCertificate = NULL;
	_pOutboundConnectivity = NULL;
	_pOutStream = NULL;
	_commandChannelId = 0;

	_pInboundConnectivity = NULL;
	_pTalkDownStream = NULL;
	_is2wayAudioSupported = false;

	_mediaContext.hasAudio = false;
	_mediaContext.hasVideo = false;

	_isPlaylist = false;
	_useSrtp = false;
	_retryConnectionProcess = false;
	_slowTimerCounter = 0;

	_waiting = false;
	_pendingFlag = 0;
	_pPendingInStream = NULL;
	SetControlled();	// default as controlled (let browser be pushy)
	SetRmsCapabilities(); // set rms enabled capabilities
	
	_ticks = 0;
	_capabPeer.hasHeartbeat = false;
	_capabRms.hasHeartbeat = false;
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
	_hbNoAckCounter = 0;
	_capabRms.hasHeartbeat = true;
#endif // WRTC_CAPAB_HAS_HEARTBEAT

	_checkHSState = false;

	RTCPReceiver::ResetNackStats();

}

//TODO: This will be removed later on
WrtcConnection::WrtcConnection(WrtcSigProtocol * pSig, Variant & settings)
	: BaseRTSPProtocol(PT_WRTC_CNX) {
	_sessionCounter++;
	_pSig = pSig;
	_pFastTimer = _pSlowTimer = NULL;
	_bestIce = NULL;
	_started = false;
	_stopping = false;
	_gotSDP = false;
	_sentSDP = false;
	_createdIces = false;
	_pSDP = NULL;
	_pSDPInfo = NULL;
	_pDtls = NULL;
	_pSctp = NULL;
	_pOutNetMP4Stream = NULL;
	_dataChannelEstablished = false;
	_fullMp4 = false;
	_clientId = "";
	_rmsClientId = "";
	_cmdReceived = "Unknown";
	_stunDomain = "";
	_turnDomain = "";
	
	_isStreamAttached = false;
	_pCertificate = NULL;

	_customParameters = settings;
	_streamChannelId = 0xFFFFFFFF;
	
	_pOutboundConnectivity = NULL;
	_pOutStream = NULL;
	_commandChannelId = 0;

	_pInboundConnectivity = NULL;
	_pTalkDownStream = NULL;
	_is2wayAudioSupported = false;

	_isPlaylist = false;
	_useSrtp = false;
	_retryConnectionProcess = false;
	_slowTimerCounter = 0;
	
	_waiting = false;
	_pendingFlag = 0;
	_pPendingInStream = NULL;

	SetControlled();	// default as controlled (let browser be pushy)
	SetRmsCapabilities(); // set rms enabled capabilities
	
	_ticks = 0;
	_capabPeer.hasHeartbeat = false;
	_capabRms.hasHeartbeat = false;
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
	_hbNoAckCounter = 0;
	_capabRms.hasHeartbeat = true;
	_sctpAlive = false;
#endif // WRTC_CAPAB_HAS_HEARTBEAT

	 _checkHSState = false;

	 RTCPReceiver::ResetNackStats();
}

WrtcConnection::~WrtcConnection() {
	_sessionCounter--;
	
	//Output the time we had streaming going to this client
	std::stringstream sstrm;
	sstrm << GetId();
	string probSess("wrtcplayback");
	probSess += sstrm.str();
	uint64_t ts = PROBE_TRACK_TIME_MS(STR(probSess) );

	//Retrieve the time to establish the wrtc connection and delete it after retrieval
	probSess += "wrtcConn";
	uint64_t wconTs = PROBE_GET_STORED_TIME(STR(probSess),true);

	//Retrieve the time taken to send the first frame and delete it after retrieval
	probSess += "firstFrame";
	uint64_t wconFFTs = PROBE_GET_STORED_TIME(STR(probSess),true);

	// Only print this if we have a session to report
	if( ts > 0 ) {
		INFO("RMS stopped streaming to client");
		INFO("RMS stopped streaming event:%"PRId64",%"PRId32"", ts, GetId() );

		// Log Peer/Ice states at the end of session
		if (_pSig != NULL) {
			INFO("The peer state is %s", STR(_pSig->GetPeerState()));
			INFO("Peer State Transition: %s-closed", STR(_pSig->GetPeerState()));
			_pSig->SetPeerState(WRTC_PEER_CLOSED);

			INFO("The ice state is %s", STR(_pSig->GetIceState()));
			INFO("Ice State Transition: %s-closed", STR(_pSig->GetIceState()));
			_pSig->SetIceState(WRTC_ICE_CLOSED);
		}

		if(HasAudio()) {
			INFO("RMS stopped live audio streaming to client");
		}

		// connection failure logging
		if(!wconTs) {

			if(ts < 2000) {
                                INFO("RMS unable to peer in less than 2 sec");
                        }
                        else if(ts < 2500) {
                                INFO("RMS unable to peer in less than 2.5 sec");
                        }
			else if(ts < 3000) {
				INFO("RMS unable to peer in less than 3 sec");
			}
			else if(ts < 6000) {
				INFO("RMS unable to peer in less than 6 sec");
			}
			else if(ts < 10000) {
				INFO("RMS unable to peer in less than 10 sec");
			}
			else {
				INFO("RMS unable to peer in more than 10 sec");
			}

		}

		// first frame failure logging
		if(wconTs && !wconFFTs) {

			if(ts < 2000) {
				INFO("RMS unable to send frame in less than 2 sec");
			}
			else if(ts < 2500) {
				INFO("RMS unable to send frame in less than 2.5 sec");
			}
			else if(ts < 3000) {
				INFO("RMS unable to send frame in less than 3 sec");
			}
			else if(ts < 6000) {
				INFO("RMS unable to send frame in less than 6 sec");
			}
			else if(ts < 10000) {
				INFO("RMS unable to send frame in less than 10 sec");
			}
			else if(ts < 20000) {
				INFO("RMS unable to send frame in less than 20 sec");
			}
			else {
				INFO("RMS unable to send frame in more than 20 sec");
			}

		}

		PROBE_CLEAR_TIME(STR(probSess));

		/* 	Session statistics Info : { RMS Session Statistics:  <RMS Client ID>, <RMS IPVersion>, <Player Client ID>, <Player IPVersion>, <Session Total duration>, <Time taken to send First Frame>, 
							<Time taken to connect Peer>, <Relay/Reflex>, <Received Command from Player> }

			RMS Session Candidate IPversion Type: { <RMS  IPVersion> ,<Player IPVersion> }
		*/
		if(_bestIce) {
			Candidate * pCan = _bestIce->GetBestCandidate();
			if(pCan) {
				INFO("RMS Session Statistics:%s,%s,%"PRIu64",%"PRIu64",%"PRIu64",%s,%s",
					STR(_clientId), (pCan->IsIpv6() ? STR("IPV6") : STR("IPV4")), ts, wconFFTs, wconTs,
					(pCan->IsRelay() ? STR("relay") : (pCan->IsReflex() ? STR("reflex") : STR("unknown"))),
					STR(_cmdReceived));
				/* INFO("RMS Session Statistics: %s, %s, %s, %s, %"PRIu64", %"PRIu64", %"PRIu64", %s, %s",
					STR(_rmsClientId), (_bestIce && _bestIce->IsIpv6() ? STR("IPV6") : STR("IPV4")),
					STR(_clientId), (pCan->IsIpv6() ? STR("IPV6") : STR("IPV4")), ts, wconFFTs, wconTs,
					(pCan->IsRelay() ? STR("relay") : (pCan->IsReflex() ? STR("reflex") : STR("unknown"))),
					STR(_cmdReceived)); */

				INFO("RMS Session Candidate IPversion Type:%s,%s", (_bestIce && _bestIce->IsIpv6() ? STR("IPV6") : STR("IPV4")),
					(pCan->IsIpv6() ? STR("IPV6") : STR("IPV4")));
			}
			_cmdReceived = "Unknown";
		}

		// Stream info of the session
		BaseOutStream * outStream = GetOutboundStream();
		if (NULL != outStream) {
			// get out stream stats
			Variant stats;
			outStream->GetStats(stats);

			// video info of the session
			uint64_t bytesCount = stats["video"]["bytesCount"];
			uint64_t droppedBytesCount = stats["video"]["droppedBytesCount"];
			uint64_t packetsCount = stats["video"]["packetsCount"];
			uint64_t droppedPacketsCount = stats["video"]["droppedPacketsCount"];
			uint8_t videoProfile = stats["video"]["profile"];

			NackStats nackStats;
			nackStats = RTCPReceiver::GetNackStats();

			/* Video Session: { <Player Client ID>, <Profile>, <Bytes Sent>, <Bytes Dropped>, <Packets Sent>,
						<Packets Dropped> } */
			INFO("Video Session: %s, 0x%"
					PRIx8", %"PRId64", %"PRId64", %"PRId64", %"PRId64", %"PRIu32", %"PRIu32"", STR(_clientId),
						videoProfile, bytesCount, droppedBytesCount, packetsCount, droppedPacketsCount, nackStats.pRequested, nackStats.pTransmitted);

			// Audio info of the session
                        bytesCount = stats["audio"]["bytesCount"];
                        droppedBytesCount = stats["audio"]["droppedBytesCount"];
                        packetsCount = stats["audio"]["packetsCount"];
                        droppedPacketsCount = stats["audio"]["droppedPacketsCount"];

			/* Audio Session: { <Player Client ID>, <Bytes Sent>, <Bytes Dropped>, <Packets Sent>, <Packets Dropped> } */
			INFO("Audio Session:%s,%"
					PRId64",%"PRId64",%"PRId64",%"PRId64"", STR(_clientId), bytesCount, droppedBytesCount, packetsCount, droppedPacketsCount);
		}

		// Use the session counter to report the instances
		if (_sessionCounter) {
			INFO("Number of active client connections: %"PRIu32, (_sessionCounter - 1));
		} else {
			INFO("No active webrtc connections.");
		}
	}

//#ifdef WEBRTC_DEBUG
		FINE("WrtcConnection deleted.");
//#endif
	_stopping = true;

	if(IsSctpAlive()) {
		if (_pSctp != NULL ) {
			delete _pSctp;
			_pSctp = NULL;
		}
		_sctpAlive = false;
	}

	if (_pFastTimer != NULL) {
		delete _pFastTimer;
		_pFastTimer = NULL;
	}

	if (_pSlowTimer != NULL) {
		delete _pSlowTimer;
		_pSlowTimer = NULL;
	}

	// Connection was established, streaming has started, and everyone was happy
	// We no longer wait for the connection to get stale or terminated by the
	// other end, but rather close it to help prevent the "mem increase" scenario
	// UPDATE: since we already have a mechanism to remove the ice protocols
	// we can safely terminate the remaining ones on the list without having to
	// check if the connection did properly establish or not
	FOR_VECTOR(_ices, i) {
		_ices[i]->Terminate();
	}
	_ices.clear();
	
	_bestIce = NULL;

	if (_pSig != NULL) {
		INFO("Closing Player Client ID:%s", STR(_clientId));
		_pSig->UnlinkConnection(this);
	}

	_rtcpReceiver.SetOutNetRTPStream(NULL);
	RTCPReceiver::ResetNackStats();

	if (_pOutStream != NULL){
		delete _pOutStream;
		_pOutStream = NULL;
	}

	if (_pOutboundConnectivity != NULL) {
		_pOutboundConnectivity->SetOutStream(NULL);
		delete _pOutboundConnectivity;
		_pOutboundConnectivity = NULL;
	}

	if (_pInboundConnectivity != NULL) {
		delete _pInboundConnectivity;
		_pInboundConnectivity = NULL;
	}
	if(_pTalkDownStream != NULL) {
		delete _pTalkDownStream;
		_pTalkDownStream = NULL;
	}
	if (_pSDP != NULL) {
		delete _pSDP;
		_pSDP = NULL;
	}
	
	if (_pSDPInfo != NULL) {
		delete _pSDPInfo;
		_pSDPInfo = NULL;
	}

	if (_pCertificate != NULL) {
		delete _pCertificate;
		_pCertificate = NULL;
	}

	if (_pOutNetMP4Stream != NULL && _isStreamAttached) {
		// Unregister from this stream
		if (_fullMp4) {
			((OutNetMP4Stream *)_pOutNetMP4Stream)->UnRegisterOutputProtocol(GetId());
		} else {
			((OutNetFMP4Stream *)_pOutNetMP4Stream)->UnRegisterOutputProtocol(GetId());
		}

		_pOutNetMP4Stream = NULL;
	}

	RemovePlaylist();

}

void WrtcConnection::RemovePlaylist() {
	if (_isPlaylist && !_streamName.empty()) {
		BaseClientApplication *pApp = GetLastKnownApplication();
		if (pApp) {
			Variant message;
			message["header"] = "removeConfig";
			message["payload"]["command"] = message["header"];
			message["payload"]["localStreamName"] = _streamName;
			pApp->SignalApplicationMessage(message);
		}
	}
}

bool WrtcConnection::Initialize(Variant &parameters) {
	_customParameters = parameters;

	return true;
}


bool WrtcConnection::IsVod(string streamName) {
	if (streamName.find_last_of(".") == string::npos) return false;
	string extension = streamName.substr(streamName.find_last_of("."));
	return extension == ".vod" || extension == ".lst";
}

bool WrtcConnection::IsLazyPull(string streamName) {
	if (streamName.find_last_of(".") == string::npos) return false;
	string extension = streamName.substr(streamName.find_last_of("."));
	return extension == ".vod";
}

bool WrtcConnection::IsWaiting() {
	return _waiting;
}

bool WrtcConnection::SetStreamName(string streamName, bool useSrtp) {
	// This is a bit weird to have here, but since this object gets instantiated way before 
	// actual client join, this serves as a good proxy for the start of the peering connection
	std::stringstream sstrm;
	sstrm << GetId();
	string probSess("wrtcplayback");
	probSess += sstrm.str();
	uint64_t ts = PROBE_TRACK_INIT_MS(STR(probSess) );
	INFO("Starting to peer:%"PRId64"", ts);

	_streamName = streamName;
	_useSrtp = useSrtp;

	if (!_useSrtp) {
		// This is an fmp4
		return SendStreamAsMp4(streamName);
	}

	BaseClientApplication *pApp = GetApplication();
	if (pApp == NULL) {
		FATAL("Unable to get application");
		return false;
	}

	StreamsManager *pSM = pApp->GetStreamsManager();
	if (pSM == NULL) {
		FATAL("Unable to get Streams Manager");
		return false;
	}

	if (IsVod(streamName)) {
		if (IsLazyPull(streamName)) {
			//check first if there is an existing lazypull stream
			BaseInStream *pInStream = RetrieveInStream(streamName, pSM);
			if (pInStream != NULL) {
				return CreateOutStream(streamName, pInStream, pSM);
			}
		}
		StreamMetadataResolver *pSMR = pApp->GetStreamMetadataResolver();
		if (pSMR == NULL) {
			FATAL("No stream metadata resolver found");
			return false;
		}

		Metadata metadata;
		metadata["callback"]["protocolID"] = GetId();
		metadata["callback"]["streamName"] = streamName;

		if (!pSMR->ResolveMetadata(streamName, metadata, true)) { // lazy pull should be executing here. 
			FATAL("Unable to obtain metadata for stream name %s", STR(streamName));
			return false;
		}

		if (metadata.type() == MEDIA_TYPE_VOD) {
//			_pendingInputStreamName = metadata.completeFileName();
			_waiting = true;
			return true;
		}
		else if (metadata.type() == MEDIA_TYPE_LST) {
			metadata["_isVod"] = "1";
			if (SetupPlaylist(streamName, metadata, true)) {
				_waiting = true;
				return true;
			}
		}

		return false;
	} else {
		BaseInStream *pInStream = RetrieveInStream(streamName, pSM);
		if (pInStream != NULL) {
			return CreateOutStream(streamName, pInStream, pSM);
		}
		
		return false;
	}
	return true;
}

/** @description: Set the mobile client id
 *  @param[in] :  string - client id
 *  @return: void
 */
void WrtcConnection::SetClientId( string clientId ) {
	_clientId = clientId;
}

/** @description: Set the RMS client id
 *  @param[in] :  string - rms client id
 *  @return: void
 */
void WrtcConnection::SetRMSClientId( string rmsClientId ) {
	_rmsClientId = rmsClientId;
}

BaseInStream *WrtcConnection::RetrieveInStream(string streamName, StreamsManager *pSM) {
	map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(ST_IN_NET,
		streamName, true, false);

	if (streams.size() == 0) {
		FATAL("Unable to find stream %s", STR(streamName));
		return NULL;
	}

	BaseInStream *pInStream = (BaseInStream *)MAP_VAL(streams.begin());

	if (!pInStream->IsCompatibleWithType(ST_OUT_NET_RTP)) {
		FATAL("Incompatible stream for SRTP");
		return NULL;
	}

	return pInStream;
}

#ifdef PLAYBACK_SUPPORT
bool WrtcConnection::CreateInStream(string streamName, StreamsManager *pSM) {

	if (!_is2wayAudioSupported) {
		INFO("No talk down request from the client");
		return false;
	}

	uint32_t bandwidthHint = 0;
	uint8_t rtcpDetectionInterval = 15;
	int16_t a = -1;
	int16_t b = -1;
	string talkdown = streamName;
	talkdown.append("talkdown");
	INFO("The talkdown stream name is %s", STR(talkdown));

	if(NULL == _pInboundConnectivity) {
		_pInboundConnectivity = new InboundConnectivity((BaseRTSPProtocol *)this, talkdown, bandwidthHint, rtcpDetectionInterval, a, b);
	}

	Variant audioTrack;
	audioTrack["isTcp"] = true;
	audioTrack["portsOrChannels"]["data"] = (uint16_t) 0;
	audioTrack["portsOrChannels"]["rtcp"] = (uint16_t) 1;

	if (!_pInboundConnectivity->AddTrack( audioTrack, true)) {
		FATAL("Unable to add audio track");
		return false;
	}

	Variant inboundParams;
	inboundParams["hasAudio"] = true;
	inboundParams["hasVideo"] = false;

	if (!_pInboundConnectivity->Initialize(inboundParams)) {
                FATAL("Unable to initialize inbound connectivity");
                return false;
        }

	BaseInStream *pInStream =(BaseInStream *) _pInboundConnectivity->GetInStream();
	if(pInStream) {
		_pTalkDownStream = new OutDeviceTalkDownStream(this, streamName);
		if (!_pTalkDownStream->SetStreamsManager(pSM)) {
			FATAL("Unable to set the streams manager for talkdown");
			delete _pTalkDownStream;
			_pTalkDownStream = NULL;
			return false;
		}
		if (!(pInStream)->Link(_pTalkDownStream)) {
			FATAL("Unable to link streams for talkdown");
			delete _pTalkDownStream;
			_pTalkDownStream = NULL;
			return false;
		}
	}
	else {
		FATAL("Unable to retrieve streams");
		return false;
	}
	//INFO("Created talkdown stream for audio out 0x%x", _pTalkDownStream);

	return true;
}
#else
bool WrtcConnection::CreateInStream(string streamName, StreamsManager *pSM) {
	INFO("No Playback support available on the device");
	return false;
}
#endif

bool WrtcConnection::CreateOutStream(string streamName, BaseInStream *pInStream, StreamsManager *pSM) {
	PROBE_POINT("webrtc", "sess1", "create_out_stream", true);
	_pOutStream = new OutNetRTPUDPH264Stream(this, streamName, false, true, true);
	if (!_pOutStream->SetStreamsManager(pSM)) {
		FATAL("Unable to set the streams manager");
		delete _pOutStream;
		_pOutStream = NULL;
		return false;
	}

	_pOutStream->SetPayloadType(107, false);
	_pOutStream->SetPayloadType(106, true);

	_pOutboundConnectivity = new OutboundConnectivity(false, this);
	if (!_pOutboundConnectivity->Initialize()) {
		FATAL("Unable to initialize outbound connectivity");
		return false;
	}

	_pOutStream->SetConnectivity(_pOutboundConnectivity);
	_pOutboundConnectivity->SetOutStream(_pOutStream);

	// TODO: Check SDP for video and audio
	StreamCapabilities *pCaps = pInStream->GetCapabilities();
#ifdef HAS_G711
	bool hasAudio = pCaps->GetAudioCodec() != NULL && (pCaps->GetAudioCodecType() == CODEC_AUDIO_AAC || (pCaps->GetAudioCodecType() & CODEC_AUDIO_G711) == CODEC_AUDIO_G711);
#else
	bool hasAudio = pCaps->GetAudioCodec() != NULL && pCaps->GetAudioCodecType() == CODEC_AUDIO_AAC;
#endif /* HAS_G711 */
	bool hasVideo = pCaps->GetVideoCodec() != NULL && pCaps->GetVideoCodecType() == CODEC_VIDEO_H264;
	_pOutboundConnectivity->HasAudio(hasAudio);
	_pOutboundConnectivity->HasVideo(hasVideo);

	// update the media context
	HasAudio(hasAudio);
	HasVideo(hasVideo);

	if (!pInStream->Link(_pOutStream)) {
		FATAL("Unable to link streams");
		delete _pOutStream;
		_pOutStream = NULL;
		return false;
	}

	// attach outstream to rtcpreceiver
	_rtcpReceiver.SetOutNetRTPStream(_pOutStream);
	return true;
}

void WrtcConnection::Start(bool hasNewCandidate) {
	if( !_createdIces ) {
		// We are starting, so we should have stun/turn set
		// Create the ice objects, enough so that each stun and turn IP is used on each interface
		// Filter for ipv6 vs ipv4
		_createdIces = SpawnIceProtocols();
	}

	if (_started || _stopping || (hasNewCandidate && _started)) {
//#ifdef WEBRTC_DEBUG
		DEBUG("Start() called AGAIN - ignoring...!");
//#endif
		// Check, if this is a new candidate AND we already started
		if (hasNewCandidate && _started) {
			FINE("New candidate received...");
			// New candidate received, and the process has already started, make
			// the necessary adjustments.
			// Only do this if we have not successfully established
			// a connection yet. Unless we want to try if this candidate
			// is more reliable/faster.
			if (_bestIce && _bestIce->GetBestCandidate()) {
				// At this point, we already have a connection
				// TODO: do we retry still?
			} else {
				// No connection established yet, retry the candidates
				StartFastTimer(); // restart the fast timer to try the new candidate
				_retryConnectionProcess = true; // indicate that we want to restart
			}
		}
		
		return;
	}
	
	// Sanity check...
	if (!InitializeCertificate()) {
		WARN("Failed to initialize certificates!");
		return;
	}
			
	// Issue an SDP if we haven't received one yet
	if (!_gotSDP) {
		if (_sentSDP) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Would have sent SDP TWICE!!");
//#endif
		} else {
			_sentSDP = true;	// b2: track this, sending twice is bad
			// create a Controlling SDP
			_isControlled = false;
			_pSDP = new WrtcSDP(SDP_TYPE_CONTROLLING, _pCertificate);
			_pSDP->SetSourceStream(_pOutStream);
			string sdpStr = _pSDP->GenerateSDP(NULL, _useSrtp, _useSrtp, true);
			INFO("SDP offer is %s", STR(sdpStr));
			_pSig->SendSDP(sdpStr, true);	// send offer

			// manually setting hasAudio and hasVideo for now. should be dependent on sdp
			GetCustomParameters()["hasAudio"] = false;
			GetCustomParameters()["hasVideo"] = false;
		}
	} else if (_pSDP && _pSDPInfo) {
		if (_sentSDP) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Would have sent SDP TWICE!!");
//#endif
		} else {
			_sentSDP = true;	// b2: track this, sending twice is bad

			string sdpStr = _pSDP->GenerateSDP(_pSDPInfo, _useSrtp, _useSrtp, true);
			INFO("SDP answer is %s", STR(sdpStr));
			_pSig->SendSDP(sdpStr, false);	// send answer
		}
	}

	if (!_pSDP) {
		WARN("WebRTC didn't generate an SDP!");
		return;
	}

	if (!_pSDPInfo) {
		INFO("WebRTC didn't receive an SDP");
		return;
	}

	// complete and start each STUN agent (BaseIceProtocol)
	FOR_VECTOR(_ices, i) {
		// fill in the SDP exchanged User Info
		_ices[i]->SetIceUser(_pSDP->GetICEUsername(), _pSDP->GetICEPassword(),
				_pSDPInfo->iceUsername, _pSDPInfo->icePassword);
		_ices[i]->SetIceFingerprints(_pSDP->GetFingerprint(), _pSDPInfo->fingerprint);
		_ices[i]->SetIceControlling(!_isControlled);
		// now start the stun agent
		_ices[i]->Start(0);
		_started = true;
	}

	FINE("WebRTC connection started...");
	StartFastTimer();
}

bool WrtcConnection::SetSDP(string sdp, bool offer) {
	_gotSDP = true;
	FINE("Received SDP from the player is %s", STR(sdp));

	string raw = sdp;
	SDP peerSdp;

	//3. Parse the SDP
	if (!SDP::ParseSDP(peerSdp, raw, true)) {
		FATAL("Unable to parse the peer SDP, so by default 2 way audio support is off");
		//return false;
	}
	else {
		Variant audioTrack = peerSdp.GetTrackLine(0, "audio");
		_is2wayAudioSupported = (bool)audioTrack[SDP_A]["sendrecv"];
		INFO("2 way audio support from the peer is %d", _is2wayAudioSupported);

		if((bool)audioTrack[SDP_A]["sendrecv"]) {

			// Get the application
			BaseClientApplication *pApp = NULL;
			if ((pApp = GetApplication()) == NULL) {
				FATAL("Unable to get the application");
				return false;
			}

			StreamsManager *pSM = pApp->GetStreamsManager();
			if (pSM == NULL) {
				FATAL("Unable to get Streams Manager");
				return false;
			}

			bool ret = CreateInStream(_streamName, pSM);
			if(ret) {
				INFO("Successfully created the inbound stream for audio out");
			}
			else {
				FATAL("Error while creating the inbound stream for audio out");
			}
		}
		else {
			INFO("No talk down request from the client");
		}
	}

	_pSDPInfo = WrtcSDP::ParseSdpStr2(sdp);
	
	if (_pSDPInfo == NULL) {
		FATAL("Generated SDP info is null!");
		return false;
	}

	INFO("Client Session Info: %s.", STR(_pSDPInfo->sessionInfo));

//#ifdef WEBRTC_DEBUG
	DEBUG("Got SDP: %s", offer ? (char *)"offer" : (char *)"answer");
	DEBUG("ParseSdpStr2 returned %s", _pSDPInfo ? "ok" : "NULL");
//#endif

	if (offer) { // then peer expects an answer
		// Sanity check...
		if (!InitializeCertificate()) {
			return false;
		}

		_pSDP = new WrtcSDP(SDP_TYPE_CONTROLLED, _pCertificate);
	}

	// Set the correct mid value regardless of offer or answer
	_pSig->SetSdpMid(_pSDPInfo->mid);

	return true;
}

bool WrtcConnection::Tick() {
	if (_pFastTimer) {
		// we are running the fast timer
		if (!FastTick()) {
//#ifdef WEBRTC_DEBUG
			DEBUG("$b2$ Switching to slow timer");
//#endif
			StartSlowTimer();
		}
	}
	else {
		SlowTick();
	}
	return true;
}

bool WrtcConnection::FastTick() {
	bool reset = false;
	if (_retryConnectionProcess) {
		// Check if there is a need to restart the connection process
		reset = true;
		_retryConnectionProcess = false;
	}

	bool more = false;
	bool alreadyConnected = false;
	FOR_VECTOR(_ices, i) {
		// Tick returns false if done (true if it wants more ticks)
		more |= _ices[i]->FastTick(reset, alreadyConnected);
		if (alreadyConnected) {
			// It has connected already, bail out!
			more = false;
			break;
		}
	}
	// if done (!more), then find the best result
	// and change to slow ticks
	//
	if (!more) {
		FOR_VECTOR(_ices, i) {
			BaseIceProtocol * pStun = _ices[i];	// do the index once
			if (!pStun->IsDead()) {
				pStun->Select(); // have the stun check himself
				if (!_bestIce || (pStun->IsBetterThan(_bestIce))) {
					_bestIce = pStun;
				}
			}
		}
		// panic stuff if we didn't find a connection!
		if (!_bestIce) {
			FATAL("No valid STUN interface found!");

			// Shutdown signalling protocol, as well as this session if no interface found
			// Doesn't happen today, but should be a good future proofing
			_pSig->Shutdown(false, true);
		} else {
			Candidate * pCan = _bestIce->GetBestCandidate();
			if (!pCan) {

				if (_pSig != NULL) {
					INFO("Ice State Transition: %s-failed", STR(_pSig->GetIceState()));
					_pSig->SetIceState(WRTC_ICE_FAILED);
				}

				FATAL("STUN has no valid candidate! (%s [%s])", STR(_bestIce->GetHostIpStr()), (_bestIce->IsTcpType() ? STR("TCP") : STR("UDP")));

				// The crash is probably caused by this, there is no peering
				// connection but has still proceeded with the slow tick
				// We should be scheduled for deletion
				EnqueueForDelete();
			} else {

				if (_pSig != NULL) {
					INFO("Ice State Transition: %s-connected", STR(_pSig->GetIceState()));
					_pSig->SetIceState(WRTC_ICE_CONNECTED);
				}

				INFO("WebRTC connection established: STUN (%s [%s]), Candidate (%s).", 
						STR(_bestIce->GetHostIpStr()), (_bestIce->IsTcpType() ? STR("TCP") : STR("UDP")), STR(pCan->GetShortString()));
				PROBE_POINT("webrtc", "sess1", "candidate_selected", false);
				//Output the time it took to get to this point
				std::stringstream sstrm;
				sstrm << GetId();
				string probSess("wrtcplayback");
				probSess += sstrm.str();
				uint64_t ts = PROBE_TRACK_TIME_MS(STR(probSess) );
				INFO("WebRTC connection event:%"PRId64"", ts);
				//we need to store this because comcast really wants to see this number again in a log later...
				//adding token to identify this specific val
				probSess += "wrtcConn";
				PROBE_STORE_TIME(probSess,ts);

				if (_sessionCounter) {
					INFO("Number of active client connections: %"PRIu32, (_sessionCounter - 1));
				} else {
					INFO("No active webrtc connections.");
				}

				// Report STUN message to peer
				// $ToDo$ should I only do this if I'm (!_isControlled)??
				_bestIce->ReportBestCandidate();

				// Now that we have a best candidate, setup the srtp or data channel
				if (!InitializeDataChannel()) {
					FATAL("Unable to start data channel!");
					// We should be scheduled for deletion
					EnqueueForDelete();
				}
				/*
				if (_useSrtp) {
					if (!InitializeSrtp()) {
						FATAL("Unable to start SRTP!");
					}
				}
				*/

// Commenting the below block to clean up the unused stuns as we think this would fix RDKC-4665(as per tiage/RM, RDKC-4665 occurs due to the fix provided for RDKC-4066)
#if 0
				// At this point we have a connection path and are setting up protocol stacks. we should now 
				// cleanup the stuns that are not going to be used so that we dont have a bunch of extra processing 
				// and messages being sent.
				INFO("Deleting unused stuns: %d", _ices.size());
				// Dont delete the TCP stun as it still has some asynchronous stuff going on.  There is only one
				// and it will cleanup separately.
				BaseIceProtocol * tcpStun( NULL );
				FOR_VECTOR(_ices, i) {
					if ( _bestIce != _ices[i] && !_ices[i]->IsTcpType() )
						_ices[i]->Terminate();
					if ( _ices[i]->IsTcpType() ) {
						tcpStun = _ices[i];
					}
				}
				// Now that we've spun the unused to terminate/delete, clear the list and reinsert just the chosen one
				_ices.clear();
				_ices.push_back( _bestIce );
				_ices.push_back( tcpStun );
				INFO("Stuns remaning after the cull: %d", _ices.size());
#endif
			}
		}
		
		return false;	// cause switch to slow tick
	}
	
	return true;	// continue the ticking!!
}

bool WrtcConnection::SlowTick() {
	// Session might already have been scheduled for deletion
	if (IsEnqueueForDelete()) {
		return false;
	}

	if(_pDtls && _checkHSState) {
		int8_t dtlsState = _pDtls->GetHandshakeState();
		INFO("The current state of DTLS handshake is %d", dtlsState);

		// No need to check the state once handshake has been successfully completed
		if(dtlsState == 1) {
			_checkHSState = false;
		}
	}

	// Increment the tick counter here so that we can use it for the channel
	// validation as well
	_ticks++;

	// Check the validity of the channel first
	if (IsSctpAlive()) {
		// Channel is Invalid
		if (!_pSctp->IsDataChannelValid(_commandChannelId)) {

			// It would seem that when using a relay, there is a chance that the datachannel
			// is not even ready yet. This issue got worse since we changed the slowtick
			// from 15 seconds to 1 second
			// We now give some allowance similar to the STUN refresh
			if (_ticks < STUN_REFRESH_RATE_SEC) {
				DEBUG("Datachannel not ready yet.");
				return true;
			}

			// tear down the connection only if channel is not been connected at least once
			if (_pSctp->IsDataChannelConnectedOnce(_commandChannelId)) {
				FATAL("Datachannel no longer valid.");
				EnqueueForDelete();
				return false;
			}
		}
		// Data channel is valid
		else {
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
			// Send the heartbeat message
			if (!SendHeartbeat()) {
				if (_pSig != NULL) {
					INFO("Ice State Transition: %s-disconnected", STR(_pSig->GetIceState()));
					_pSig->SetIceState(WRTC_ICE_DISCONNECTED);
				}
				FATAL("Could not send heartbeat!");
				EnqueueForDelete();
				return false;
			}
			// Now check if we have response from client
			if (_capabPeer.hasHeartbeat && (_hbNoAckCounter++ > WRTC_CAPAB_MAX_HEARTBEAT)) {
				if (_pSig != NULL) {
					INFO("Ice State Transition: %s-disconnected", STR(_pSig->GetIceState()));
					_pSig->SetIceState(WRTC_ICE_DISCONNECTED);
				}
				FATAL("No heartbeat response from player!");
				EnqueueForDelete();
				return false;
			}
#endif // WRTC_CAPAB_HAS_HEARTBEAT
		}
	}


	// Only proceed below if it is time to send the bind request
	// Which is every STUN_REFRESH_RATE_SEC (15s)
	if ((_ticks % STUN_REFRESH_RATE_SEC)) {
		return true;
	}

	if (_bestIce) {
		_slowTimerCounter++;
		
		if (!(_bestIce->SlowTick(false))) {
			// There is no valid candidate (failure of connection establishment)
			// there should be a number of retry on the slow timer and should not wait
			// for RRS to disconnect the connection
			
			if (_slowTimerCounter > 2 && _pSig != NULL ) {
				// Delete this connection after 30s
				_pSig->EnqueueForDelete();
			}
		}
	}

	return true;
}


void WrtcConnection::OverrideIceMaxRetries(int newMaxRetries) {
	FOR_VECTOR(_ices, i) {
		_ices[i]->SetMaxRetries (newMaxRetries);
	}
}

//
// setters called from WrtcSigProtocol
// note: ipPort is a URL string: "x.y.z.z:pppp"
//
bool WrtcConnection::SetStunServer(string ipPortStr) {
	// saving for logging purposes
	_stunDomain = ipPortStr;
	//UPDATE: We should resolve any domains at this point to IP address
	// Moved here so that we resolve only once
	if (!ResolveDomainName(ipPortStr, _stunIps)) {
		FATAL("Error setting STUN server!");
		return false;
	}
	
	return true;
}

bool WrtcConnection::SetTurnServer(string username, string credential, string ipPort) {
	// saving for logging purposes
	_turnDomain = ipPort;
	//UPDATE: We should resolve any domains at this point to IP address
	// Moved here so that we resolve only once
	if (!ResolveDomainName(ipPort, _turnIps)) {
		FATAL("Error setting STUN server!");
		return false;
	}
	_turnUser = username;
	_turnCreds = credential;

	//we typically get turn server configs second, and so if the stun servers are blank then we should use the turn server for stun
	// if we get a valid stun server later this collection will be overridden, so there is no risk in doing this
	if( _stunIps.size() <= 0 ) { 
		_stunDomain = ipPort;
		_stunIps = _turnIps;
	}

	return true;
}

bool WrtcConnection::SetCandidate(Candidate * pCan) {
	if( _ices.size() <= 0 )
		FATAL("We got a candidate but dont yet have any ice objects to save it to!" );
	FOR_VECTOR(_ices, i) {
		// Give the candidate to each Ice object that is of the same protocol
		if( pCan->IsIpv6() == _ices[i]->IsIpv6())
			_ices[i]->AddCandidate(pCan);
	}

	return true;
}

bool WrtcConnection::SetControlled(bool isControlled) {
	_isControlled = isControlled;
	
	return true;
}


bool WrtcConnection::SpawnIceProtocols() {
	//First make sure the sig protocol is valid
	if (_pSig == NULL) {
		FATAL("Signaling protocol is NULL!");
		return false;
	}

	INFO("Spawning ICE objects using these resolved IPs for STUN server: %s", STR(_stunDomain) );
	FOR_VECTOR( _stunIps, i ) {
		INFO( "     %s", STR(_stunIps[i]));
	}
	INFO("Spawning ICE objects using these resolved IPs for TURN server: %s", STR(_turnDomain) );
	FOR_VECTOR( _turnIps, i ) {
		INFO( "     %s", STR(_turnIps[i]));
	}

	// For UDP stuns, use all valid interfaces
#ifdef ENABLE_ICE_UDP
	vector<string> ipAddrs;
	if (!GetInterfaceIps(ipAddrs)) {
		FATAL("Failed to get local IP list");
		return false;
	}

	// We now call this at start time, so we should have stun/turn set
	if( _stunIps.size() == 0 ) {
		FATAL("Trying to spawn ICE objects without a STUN server");
		return false;
	}
	if( _turnIps.size() == 0 ) {
		FATAL("Trying to spawn ICE objects without a TURN server");
		return false;
	}

	// Create the ice UDP objects, enough so that each stun and turn IP is used on each interface
	// Filter for ipv6 vs ipv4

	//Split out ipv6 ip's from ipv4 ones so that we can give the right stun/turn ips to each of the sockets
	vector<string> ipv4Stuns, ipv6Stuns, ipv4Turns, ipv6Turns;
	FOR_VECTOR(_stunIps, i ) {
		if( SocketAddress::isV6(_stunIps[i]) ) 
			ipv6Stuns.push_back(_stunIps[i]);
		else
			ipv4Stuns.push_back(_stunIps[i]);
	}
	FOR_VECTOR(_turnIps, i ) {
		if( SocketAddress::isV6(_turnIps[i]) ) 
			ipv6Turns.push_back(_turnIps[i]);
		else
			ipv4Turns.push_back(_turnIps[i]);
	}
	
	//we may not get both IPv4 and IPv6 addresses, so let's use what we do have even though protocols wont match
	if( ipv6Stuns.size() == 0 ) ipv6Stuns = ipv4Stuns;
	if( ipv4Stuns.size() == 0 ) ipv4Stuns = ipv6Stuns;
	if( ipv6Turns.size() == 0 ) ipv6Turns = ipv4Turns;
	if( ipv4Turns.size() == 0 ) ipv4Turns = ipv6Turns;

	// loop through the local addr vector creating Ice UDP objects
	FOR_VECTOR(ipAddrs, i) {
		// Use only matching protocol (ip4/ip6) ip's
		vector<string> *stunIps, *turnIps;
		if( SocketAddress::isV6(ipAddrs[i]) ) {
			stunIps = &ipv6Stuns;
			turnIps = &ipv6Turns;
		}
		else {
			stunIps = &ipv4Stuns;
			turnIps = &ipv4Turns;
		}

		// We want to loop over the stun/turn ips and use each one at least once.
		// Since we have to send them as a pair, we have to do this annoying loop
		// until we get to the end of whichever one is longer.
		bool moreAddrs(true);
		int s(0), t(0);
		int smax = stunIps->size() - 1;
		int tmax = turnIps->size() - 1;
		
		while (moreAddrs) {
			INFO("Creating ICE Object with local socket: %s, Stun: %s, Turn: %s", STR(ipAddrs[i]), STR((*stunIps)[s]), STR((*turnIps)[t]) );
			BaseIceProtocol *pIceUdp = new IceUdpProtocol();
			pIceUdp->Initialize(ipAddrs[i], this, _pSig);
			pIceUdp->SetStunServer( (*stunIps)[s] );
			pIceUdp->SetTurnServer( _turnUser, _turnCreds, (*turnIps)[t] );
			_ices.push_back(pIceUdp);

			// only increment to max values so we dont blow past the end of our vectors
			moreAddrs = false;
			if( smax > s ) {
				s++;
				moreAddrs = true;
			}
			if( tmax > t ) {
				t++; 
				moreAddrs = true;
			}
		}	
	}
#endif


#ifdef ENABLE_ICE_TCP
	// The TCP socket doesnt take a bind IP, so lets just use a dummy
	string ip = "0.0.0.0";	

	// Spawn TCP ICE for each turn IP (ice TCP doesnt connect to stun servers)
	FOR_VECTOR(_turnIps, i ) {
		BaseIceProtocol *pIceTcp = new IceTcpProtocol();
		pIceTcp->Initialize(ip, this, _pSig);
		// ICE TCP doesnt use stun, just does turn, so setting the stun is good for completeness but
		// is otherwise not used
		pIceTcp->SetStunServer( _turnIps[i] );
		pIceTcp->SetTurnServer( _turnUser, _turnCreds, _turnIps[i] );
		_ices.push_back(pIceTcp);
	}
#endif

	return true;
}

//
// GetInterfaceIps is used to find the network interfaces on this machine
// - we have a WIN32 version and an ELSE version
//
#ifdef WIN32
bool WrtcConnection::GetInterfaceIps(vector<string> & ips) {
	// WINDOZE VERSION
	FATAL("IPV4 ONLY FOR WINDOWS");
	struct addrinfo hints, *res;
	struct in_addr addr;
	uint32_t err;
	char hostname[64];
	err = gethostname(hostname, 64);
	if (err) {
		FATAL("Network interface error: %"PRIu32, err);
		return false;
	}
//#ifdef WEBRTC_DEBUG
	DEBUG("GetInterfaceIps: hostname = %s", hostname);
//#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_INET;		// just IPv4 for now!

	if ((err = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
		FATAL("Network interface error %"PRIu32, err);
		return false;
	}
	// loop through all addresses returned
	while (res) {

		addr.S_un = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.S_un;
		char dst[32];
		if (inet_ntop(AF_INET, (void *)&addr, dst, 32)) {
			string newip = std::string(dst);
//#ifdef WEBRTC_DEBUG
			DEBUG("GetInterfaceIps: Add Interface: %s", STR(newip));
//#endif
			ips.push_back(newip);
		}else {
			WARN("Failed to add interface: 0x%x", (uint32_t)addr.s_addr);
		}

		res = res->ai_next;
	}

	freeaddrinfo(res);
	return true;
}
#else
// LINUX VERSION
bool WrtcConnection::GetInterfaceIps(vector<string> & ips) {
	// Reset ip cache
	ips.clear();

	struct ifaddrs *localIfList = NULL;
	struct ifaddrs *currIface = NULL;
	void *networkAdressSrc = NULL;
	string address;

	// Retrieve all available network addresses/interfaces
	if (getifaddrs(&localIfList) == -1) {
		FATAL("Network interface error: 0x%x", errno);
		return false;
	}

	INFO("Getting all local address/interfaces so we can create ICE objects for each");

	//Loop over the interfaces and pull out the valid local IP's (ipv4 and ipv6)
	for (currIface = localIfList; currIface != NULL; currIface = currIface->ifa_next) {
		// Skip the interface with null address
		if (!currIface->ifa_addr) {
			continue;
		}

		// examine for IPv4
		if (currIface->ifa_addr->sa_family == AF_INET) {
			// Hey this is a valid IPv4 Address
			networkAdressSrc = &((struct sockaddr_in *)currIface->ifa_addr)->sin_addr;
			char dst[INET_ADDRSTRLEN]; //Destination address buffer
			inet_ntop(AF_INET, networkAdressSrc, dst, INET_ADDRSTRLEN);
			address = dst;
        	}
		// examine for IPv6
		else if (currIface->ifa_addr->sa_family == AF_INET6) {
			// Hey this is a valid IPv6 Address
			networkAdressSrc = &((struct sockaddr_in6 *)currIface->ifa_addr)->sin6_addr;
			char dst[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, networkAdressSrc, dst, INET6_ADDRSTRLEN);
			address = dst;
		}

		// Only use non-loopback, non-local-link, and non-empty ip's
		if( address.size() == 0 ) {
			// to help with log clarity, silently skip empty addresses
			continue;
		}
		else if ( ( currIface->ifa_name[0] == 'l' && currIface->ifa_name[1] == 'o' ) ||
				( address.find( "fe80::" ) != std::string::npos )) {
				INFO("Skipping: %s", STR(address));
				continue;
		} else {
			//INFO("VALID:  %s", STR(address));
			// Valid address, so construct a candidate with it
			ips.push_back(address);
		}
	}

	//Cleanup
	if (localIfList != NULL) {
		freeifaddrs(localIfList);
        }

	//remove duplicates
	sort( ips.begin(), ips.end() );
	ips.erase( unique( ips.begin(), ips.end() ), ips.end() );

	INFO("List of local IPs that we will create ICE objects for:");
	for( size_t i = 0; i < ips.size(); i++ )
		INFO("    %s", STR(ips[i]));

	return true;
}
#endif


//
// Timer calls, (into Timer Class further below)
//
void WrtcConnection::StartFastTimer() {
	if (_pSlowTimer) {
		delete _pSlowTimer;
		_pSlowTimer = NULL;
	}
	if (!_pFastTimer) {
		_pFastTimer = WrtcTimer::CreateFastTimer(100, this);
	}
}
void WrtcConnection::StartSlowTimer() {
	if (_pFastTimer) {
		delete _pFastTimer;
		_pFastTimer = NULL;
	}
	if (!_pSlowTimer) {
		// Slow tick is now once per second, but recognizes STUN_REFRESH_RATE_SEC
		_pSlowTimer = WrtcTimer::CreateSlowTimer(1, this);
	}
}

//
// Timer Class stuff
//
WrtcConnection::WrtcTimer
::WrtcTimer(WrtcConnection * pWrtc) {
	_pWrtc = pWrtc;
}
WrtcConnection::WrtcTimer
::~WrtcTimer() {

}
WrtcConnection::WrtcTimer * WrtcConnection::WrtcTimer
::CreateFastTimer(uint32_t ms, WrtcConnection * pWrtc) {
	WrtcTimer * pTimer = new WrtcTimer(pWrtc);
	pTimer->EnqueueForHighGranularityTimeEvent(ms);
	
	return pTimer;
}

WrtcConnection::WrtcTimer * WrtcConnection::WrtcTimer
::CreateSlowTimer(uint32_t secs, WrtcConnection * pWrtc) {
	WrtcTimer * pTimer = new WrtcTimer(pWrtc);
	pTimer->EnqueueForTimeEvent(secs);
	
	return pTimer;
}

// the timer override
bool WrtcConnection::WrtcTimer::TimePeriodElapsed() {
	return _pWrtc->Tick();
}

//
// BaseProtocolOverrides
//

bool WrtcConnection::AllowFarProtocol(uint64_t type) {
	return true;
}

bool WrtcConnection::AllowNearProtocol(uint64_t type) {
	FATAL("This protocol doesn't allow any near protocols");
	return false;
}

bool WrtcConnection::SignalInputData(int32_t recvAmount) {
	FATAL("OPERATION NOT SUPPORTED");
	return false;
}

// SignalInputData
// - this call gets non-stun traffic from below
//
bool WrtcConnection::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	// TODO: handle RTCP
	// double-check RTCP packet
	uint8_t pt = *(GETIBPOINTER(buffer) + 1);
	if (pt == 0xC8 || pt == 0xC9) { // inbound SRTCP
		_rtcpReceiver.ProcessRTCPPacket(buffer);
	}
	else {	// inbound RTP
		//INFO("Please Feed the data to the inbound connection size %d", GETAVAILABLEBYTESCOUNT(buffer));
#ifdef PLAYBACK_SUPPORT
		if (NULL != _pInboundConnectivity) {
			_pInboundConnectivity->FeedData(0, GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
		}
#endif
	}
	return true;
}

bool WrtcConnection::SignalInputData(IOBuffer &buffer) {
	FATAL("OPERATION NOT SUPPORTED");
	return true;
}

bool WrtcConnection::EnqueueForOutbound() {
	if (_pDtls != NULL && (_pDtls->GetType() == PT_INBOUND_DTLS)) {
		if (_useSrtp) {
			InboundDTLSProtocol *pInboundDtls = (InboundDTLSProtocol *) _pDtls;
			if (pInboundDtls->HasSRTP()) {
				if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0) {
					uint8_t firstByte = GETIBPOINTER(_outputBuffer)[0];
					if ((firstByte & 0x80) > 0) { // RTP/RTCP packet
						return pInboundDtls->FeedData(_outputBuffer);
					}
				}
			}
		} else {
			//int32_t ret = _pSctp->SendData(_streamChannelId, GETIBPOINTER(_outputBuffer),
			//		GETAVAILABLEBYTESCOUNT(_outputBuffer));

			int32_t chunk = 15000;
			int32_t buffSize = GETAVAILABLEBYTESCOUNT(_outputBuffer);
			int32_t zeroSendCount = 0;
			while (buffSize > 0) {
				int32_t target = (buffSize > chunk) ? chunk : buffSize;
				int32_t ret = 0;
				if(IsSctpAlive()) {
					ret = _pSctp->SendData(_streamChannelId, GETIBPOINTER(_outputBuffer), target);
				}
				
				// Check first the returned value
				if (ret > 0) {
					// Everything seems ok. Read the buffer, reset the zero count,
					// and decrement th buffsize with the actual sent value
					_outputBuffer.Ignore(ret);
					zeroSendCount = 0;
					buffSize -= ret;
				} else if (ret == 0) {
					// Value returned is zero, handle this by resending
					if (zeroSendCount++ < MAX_ZERO_SEND_THRESHOLD) {
						continue;
					} else {
						// zero send count has reached threshold
						// Use buffsize as error indicator
						WARN("SCTP 0 size sent.");
						buffSize = 0;
						break;
					}
				} else {
					// There was an error in sending, break out
					// Use buffsize as error indicator
					WARN("SCTP send failure");
					buffSize = -1;
					break;
				}
			}

			if (buffSize >= 0) {
				// If there was no error, return
				return true;
			}
		}
	}

	_outputBuffer.IgnoreAll();
	
	if (_pDtls == NULL) {
		// No outbound protocol yet, just return as true
		return true;
	} else {
		WARN("Failure sending data!");
		return false;
	}
}

IOBuffer *WrtcConnection::GetOutputBuffer() {
	return &_outputBuffer;
}

bool WrtcConnection::InitializeDataChannel() {
	DEBUG("Initializing data channel...");
	// Get the bind port and initial best remote port
	uint8_t  ipv6[16];
	uint32_t ip;
	uint16_t port;

	Variant settings;

	string localIpStr = _bestIce->GetHostIpStr();
	if (IpPortStrIsV6(localIpStr)) {
		SplitIpStrV6(localIpStr, ipv6, port);
	} else {
		SplitIpStrV4(localIpStr, ip, port);
	}
	settings["localPort"] = port;

	string remoteIpStr = _bestIce->GetBestCanIpStr();
	if (IpPortStrIsV6(remoteIpStr)) {
		SplitIpStrV6(remoteIpStr, ipv6, port);
	} else {
		SplitIpStrV4(remoteIpStr, ip, port);
	}
	settings["remotePort"] = port;
	settings["sctpPort"] = _pSDP->GetSCTPPort();
	settings["sctpMaxChannels"] = _pSDP->GetSCTPMaxChannels();
	settings[CONF_SSL_CERT] = _customParameters[CONF_SSL_CERT];
	settings[CONF_SSL_KEY] = _customParameters[CONF_SSL_KEY];

	//TODO: We should be spawning dtls and sctp protocols using the factory
	// But for now, let's just instantiate them ourselves
	if (_isControlled) {
		_pDtls = new OutboundDTLSProtocol();
		settings["sctpClient"] = true;
	} else {
		_pDtls = new InboundDTLSProtocol(_useSrtp);
		settings["sctpClient"] = false;
	}

	_pDtls->Initialize(settings);
	_checkHSState = true;

	if(NULL == _pSctp) {
		_pSctp = new SCTPProtocol();
		_sctpAlive = true;
		_pSctp->Initialize(settings);
	}
	
	// Link the protocols together STUN -> DTLS -> SCTP -> WrtcConnection
	_pDtls->SetFarProtocol(_bestIce);

	if(IsSctpAlive()) {
		_pSctp->SetFarProtocol(_pDtls);
		this->SetFarProtocol(_pSctp);
		//TODO: consider refactoring to prevent the need for this
		// Start dtls and sctp
		_pDtls->Start();
		_pSctp->Start(this);
		_dataChannelEstablished = true;
	} else {
		FATAL("Error in data channel initilaize : sctp is not alive : %d",_sctpAlive);
		EnqueueForDelete();
	}
	
	return true;
}

//set sctp protocol tear down status
bool  WrtcConnection::IsSctpAlive() {
	//FINE("WrtcConnection::IsSctpAlive : %d",_sctpAlive);
	return _sctpAlive;
}

void  WrtcConnection::SetSctpAlive(bool sctpalive) {
	INFO("WrtcConnection::SetSctpAlive : %d",sctpalive);
	_sctpAlive = sctpalive;
}

// IAN TODO: start merging this to InitializeDataChannel
bool WrtcConnection::InitializeSrtp() {
	DEBUG("Initializing SRTP...");
	uint32_t ip;
	uint16_t port;
	Variant settings;
	string localIpStr = _bestIce->GetHostIpStr();
	SplitIpStrV4(localIpStr, ip, port);
	settings["localPort"] = port;
	string remoteIpStr = _bestIce->GetBestCanIpStr();
	SplitIpStrV4(remoteIpStr, ip, port);
	settings["remotePort"] = port;
	settings[CONF_SSL_CERT] = _customParameters[CONF_SSL_CERT];
	settings[CONF_SSL_KEY] = _customParameters[CONF_SSL_KEY];
	// Should be spawning dtls protocol using the factory
	// but time is of the essence... let's worry about that later... ian
	_pDtls = new InboundDTLSProtocol(true);

	_pDtls->Initialize(settings);

	// Link the protocols together STUN -> DTLS -> WrtcConnection
	_pDtls->SetFarProtocol(_bestIce);
	this->SetFarProtocol(_pDtls);

	//TODO: consider refactoring to prevent the need for this
	// Start dtls
	_pDtls->Start();
	//	((InboundDTLSProtocol *)_pDtls)->CreateOutSRTPStream(_streamName);
	return true;
}

bool WrtcConnection::SignalDataChannelCreated(const string &name, uint32_t id) {
//#ifdef WEBRTC_DEBUG
	DEBUG("DataChannel created: %s (%"PRIu32")", STR(name), id);
//#endif
	if (name == SCTP_COMMAND_CHANNEL) {
		PROBE_POINT("webrtc", "sess1", "sctp_cmd_channel_created", false);
		// Any form of command or request/response is sent using the 'command' channel
		_commandChannelId = id;
	} else if (name == SCTP_STREAM_CHANNEL) {
		PROBE_POINT("webrtc", "sess1", "sctp_strm_channel_created", false);
		// The data is then sent through the 'stream' command
		_streamChannelId = id;
	} else {
		WARN("Channel name does not match either command or stream channel names!");
	}

	return true;
}

bool WrtcConnection::SignalDataChannelInput(uint32_t id, const uint8_t *pBuffer, const uint32_t size) {
	// Parse the message that was received on this channel and handle accordingly
	if (_commandChannelId == id) {
//#ifdef WEBRTC_DEBUG
		FINE("Message received: (%"PRIu32") %.*s", size, size, pBuffer);
//#endif
		string msgString = string((char *) pBuffer, size);
		Variant msg;
		uint32_t start = 0;
		if (!Variant::DeserializeFromJSON(msgString, msg, start)) {
			FATAL("Unable to parse JSON string!");
			return false;
		}
		
		return HandleDataChannelMessage(msg);
	} else if (_streamChannelId == id) {
		//TODO
		FATAL("Inbound FMP4 data. Operation not yet supported!");
		return false;
	} else {
		WARN("Unknown channel ID: %"PRIu32, id);
		return false;
	}
	
	return true;
}

bool WrtcConnection::SignalDataChannelReady(uint32_t id) {
	FINE("SignalDataChannelReady: %"PRIu32, id);
	return true;
}

bool WrtcConnection::SignalDataChannelClosed(uint32_t id) {
//#ifdef WEBRTC_DEBUG
	FINE("SignalDataChannelClosed: %"PRIu32, id);
//#endif
	return true;
}

bool WrtcConnection::PushMetaData(Variant &metadata) {
	Variant meta;

	meta["type"] = "metadata";
	meta["payload"]= metadata;

	string data = "";
	meta.SerializeToJSON(data, true);

	return SendCommandData(data);
}

bool WrtcConnection::SetupPlaylist(string const &streamName) {
	Variant message;
	return SetupPlaylist(streamName, message);
}

bool WrtcConnection::SetupPlaylist(string const &streamName, Variant &config, bool appendRandomChars) {
	Variant message;
	message["header"] = "pullStream";
	message["payload"]["command"] = message["header"];
	//do not save to pushpull config
	config["serializeToConfig"] = "0";
	config["localStreamName"] = (appendRandomChars ? generateRandomString(8) + ";*" : "") + streamName;
	config["keepAlive"] = "0";
	config["uri"] =
		format("rtmp://localhost/vod/%s", STR(streamName));
	if (config.HasKeyChain(V_MAP, false, 1, "callback")) {
		config["_callback"] = config["callback"];
		config.RemoveKey("callback");
	}
	message["payload"]["parameters"] = config;

	ClientApplicationManager::BroadcastMessageToAllApplications(message);
	return (message.HasKeyChain(V_BOOL, true, 3, "payload", "parameters", "ok")
		|| (bool)message["payload"]["parameters"]["ok"]);

}

bool WrtcConnection::SetupLazyPull(string const &streamName) {
	BaseClientApplication *pApp = GetApplication();
	if (pApp) {
		StreamMetadataResolver *pSMR = pApp->GetStreamMetadataResolver();
		if (pSMR) {
			Metadata metadata;
			return pSMR->ResolveMetadata(streamName, metadata, true);
		}
	}
	return false;
}

bool WrtcConnection::HandleDataChannelMessage(Variant &msg) {
	string errMsg = "";
	string response = "";

	// Check the message type
	if (msg.HasKey("type")) {
		if (msg["type"] == "fmp4Request") {
			// Sanity check, make sure that no SRTP stream exists
			if (_useSrtp) {
				// Nothing to do here
				return true;
			}
			
			// Prepare the response
			msg["type"] = "fmp4Response";

			// Check if this is a full MP4 for iOS devices instead
			if (msg.HasKeyChain(V_BOOL, false, 2, "payload", "fullMp4")) {
				_fullMp4 = (bool) msg["payload"]["fullMp4"];
			}

			if (msg.HasKeyChain(V_STRING, false, 2, "payload", "name")) {
				// Get the requested stream name
				string streamName = msg["payload"]["name"];
				bool useSrtp = false;
				if (msg.HasKeyChain(V_BOOL, false, 2, "payload", "useSrtp")) {
					useSrtp = (bool) msg["payload"]["useSrtp"];
				}

				FINE("WebRTC request: %s", STR(streamName));

				string rsn = lowerCase(streamName);
				_isPlaylist = (rsn.rfind(".lst") == rsn.length() - 4);
				bool isLazyPullVod = _isPlaylist ? false : 
					(rsn.rfind(".vod") == rsn.length() - 4);
				bool streamCanBeSent = true;
				uint8_t retries = 10;
				if (_isPlaylist) {
					if (!SetupPlaylist(streamName)) {
						streamCanBeSent = false;
					} else {
						//wait until requested stream is set up
						while (retries-- > 0 && 
							!SetStreamName(streamName, useSrtp)) {
						}
						streamCanBeSent = (retries > 0);
					}
				} else if (isLazyPullVod) {
					streamCanBeSent = false;
					if (SetupLazyPull(streamName)) {
						//wait until requested stream is set up
						while (retries-- > 0 &&
							!SetStreamName(streamName, useSrtp)) {
						}
						streamCanBeSent = (retries > 0);
					}
				} else {
					streamCanBeSent = SendStreamAsMp4(streamName);
				}
				
				// Send out the outbound FMP4
				if (streamCanBeSent) {
					msg["payload"]["description"] = 
						"Outbound FMP4 stream created.";
					msg["payload"]["status"] = "SUCCESS";
					
					msg.SerializeToJSON(response, true);
					return SendCommandData(response);
				} else {
					errMsg = "Requested stream could not be sent!";
					WARN("%s", STR(errMsg));
				}
			} else {
				errMsg = "Missing name field!";
				WARN("%s", STR(errMsg));
			}

			// If this point was reached, then something went wrong
			msg["payload"]["description"] = errMsg;
			msg["payload"]["status"] = "FAIL";
			msg.SerializeToJSON(response, true);
			return SendCommandData(response);
		} else if (msg["type"] =="command" ) {
			FINE("Command received: %s - [%s]", STR(msg["payload"]["command"]), STR(msg["payload"]["argument"]));
			msg["payload"]["instanceStreamName"] = _streamName;
			msg["payload"]["baseStreamName"] = GetBaseStreamName(_streamName);
			GetEventLogger()->LogWebRTCCommandReceived(msg);
			string command = lowerCase((string)msg["payload"]["command"]);
			string argument = (string)msg["payload"]["argument"];
			_cmdReceived.assign(argument);
			if (command == "stop") {
				INFO("Received Stop Command: %s", STR(argument));
				// We tear down this session if we received this command from the player
				EnqueueForDelete();
			} else if (command == "pause") {
				HandlePauseCommand();
			} else if (command == "resume") {
				HandleResumeCommand();
			} else if (command == "ping") {
				if (_pOutNetMP4Stream) {
					((OutNetFMP4Stream*)_pOutNetMP4Stream)->SignalPing();
				}
			}
			
			return true;
		}
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
		else if (msg["type"] == "heartbeat") {
			ProcessHeartbeat();
			return true;
		}
#endif // WRTC_CAPAB_HAS_HEARTBEAT
		else {
			// invalid request
			WARN("Invalid message type!");
		}
	} else {
		WARN("Invalid message format!");
	}

	FINEST("Message received: %s", STR(msg.ToString()));
	return false;
}

bool WrtcConnection::SendStreamAsMp4(string &streamName) {
	// Get the application
	BaseClientApplication *pApp = NULL;
	if ((pApp = GetApplication()) == NULL) {
		FATAL("Unable to get the application");
		return false;
	}

	uint64_t outputStreamType = ST_OUT_NET_FMP4;
	if (_fullMp4) {
		outputStreamType = ST_OUT_NET_MP4;
	}

	// Get the private stream name
	string privateStreamName = pApp->GetStreamNameByAlias(streamName, false);
	if (privateStreamName == "") {
		FATAL("Stream name alias value not found: %s", STR(streamName));
		return false;
	}

	// Get the streams manager
	StreamsManager *pSM = NULL;
	if ((pSM = pApp->GetStreamsManager()) == NULL) {
		FATAL("Unable to get the streams manager");
		return false;
	}

	map<uint32_t, BaseStream *> outStreams = pSM->FindByTypeByName(outputStreamType, privateStreamName, false, false);
	if (!outStreams.empty()) {
		if (_fullMp4) {
			_pOutNetMP4Stream = (OutNetMP4Stream *) MAP_VAL(outStreams.begin());
		} else {
			_pOutNetMP4Stream = (OutNetFMP4Stream *) MAP_VAL(outStreams.begin());
		}
	}

	if (_pOutNetMP4Stream == NULL) {
		//there is no output stream with this name. Create one
		if (_fullMp4) {
			_pOutNetMP4Stream = OutNetMP4Stream::GetInstance(pApp, privateStreamName);
		} else {
			_pOutNetMP4Stream = OutNetFMP4Stream::GetInstance(pApp, privateStreamName, true);
		}

		if (_pOutNetMP4Stream == NULL) {
			FATAL("Unable to create the output stream");
			return 0;
		}

		BaseInStream *pInStream = NULL;
		map<uint32_t, BaseStream *> inStreams = pSM->FindByTypeByName(ST_IN_NET, privateStreamName, true, false);
		if (inStreams.size() != 0) {
			pInStream = (BaseInStream *) MAP_VAL(inStreams.begin());
		}

		// Link the inbound stream to this output stream
		if ((pInStream != NULL) && (pInStream->IsCompatibleWithType(outputStreamType))) {
			if (!pInStream->Link(_pOutNetMP4Stream)) {
				FATAL("Unable to link the in/out streams");
				_pOutNetMP4Stream->EnqueueForDelete();
				_pOutNetMP4Stream = NULL; // b2: don't delete it later
				return false;
			}
		} else {
			// Either we can't find the input stream or it is incompatible with outFMP4
			if (!pInStream) {
				FATAL("Input stream (%s) not found!", STR(privateStreamName));
			} else {
				FATAL("Input stream (%s) is not compatible with %s!",
						STR(*pInStream), STR(tagToString(outputStreamType)));
			}

			return false;
		}
	}

	((OutNetFMP4Stream *)_pOutNetMP4Stream)->SetWrtcConnection(this);

	// Register the underlying protocol (this one)
	if (_fullMp4) {
		((OutNetMP4Stream *)_pOutNetMP4Stream)->RegisterOutputProtocol(GetId(), NULL);
	} else {
		((OutNetFMP4Stream *)_pOutNetMP4Stream)->RegisterOutputProtocol(GetId(), NULL);
	}

	//it means we already attached the outstream to the wrtcconnection protocol but not the full stack
	if(_isStreamAttached){
		if (!_fullMp4) {
			((OutNetFMP4Stream *)_pOutNetMP4Stream)->SetOutStreamStackEstablished();
		}
	}

	_isStreamAttached = true;
	PROBE_POINT("webrtc", "sess1", "instream_outstream_linked", false);

	return true;
}

bool WrtcConnection::SendCommandData(string &message) {
	// Make sure SCTP layer has been setup
	if(!IsSctpAlive()) {
		return false;
	}

	// Send data to the command channel
	_outbandBuffer.IgnoreAll();
	_outbandBuffer.ReadFromString(message);

	if (_pSctp->SendData(_commandChannelId, GETIBPOINTER(_outbandBuffer), 
			GETAVAILABLEBYTESCOUNT(_outbandBuffer), false) > 0) {
//#ifdef WEBRTC_DEBUG
		// Moving the log into this as it will spam if metadata are being sent out
		FINE("Sent out: %s", STR(message));
//#endif

		PROBE_POINT("webrtc", "sess1", "sct_cmd_send", false);
		PROBE_FLUSH("webrtc", "sess1", false);

		return true;
	}

	FATAL("Error sending data over command channel!");
	return false;	
}

bool WrtcConnection::InitializeCertificate() {
	if (_pCertificate != NULL) {
		// If this has been instantiated already, use this
		return true;
	}
	
	string cert = "";
	string key = "";
	
	// Get the certificate and key
	if (_customParameters.HasKey(CONF_SSL_CERT)) {
		cert = (string) _customParameters[CONF_SSL_CERT];
	}

	if (_customParameters.HasKey(CONF_SSL_KEY)) {
		key = (string) _customParameters[CONF_SSL_KEY];
	}
	
	//TODO: For now, we require a physical certificate
	if ((key == "") || (cert == "")) {
		FATAL("Certificate and key should be provided on the config file!");
		return false;
	}
	
	_pCertificate = X509Certificate::GetInstance(cert, key);
	
	return true;
}

bool WrtcConnection::SendOutOfBandData(const IOBuffer &buffer, void *pUserData) {
	if (pUserData == NULL) {
		// skip data if there is streamChannel is not configured
		if (_streamChannelId == 0xFFFFFFFF) {
			// store moov; send later when there is streamChannel
			if (GETAVAILABLEBYTESCOUNT(_moovBuffer) == 0) {
				_moovBuffer.ReadFromInputBuffer(buffer, GETAVAILABLEBYTESCOUNT(buffer));
			}
			return true;
		}

		// send buffered moov first if not yet sent
		if (GETAVAILABLEBYTESCOUNT(_moovBuffer) > 0) {
			_outputBuffer.ReadFromInputBuffer(_moovBuffer, GETAVAILABLEBYTESCOUNT(_moovBuffer));
			_moovBuffer.IgnoreAll();
			EnqueueForOutbound();
		}
		// Read the buffer and send it out
		_outputBuffer.ReadFromInputBuffer(buffer, GETAVAILABLEBYTESCOUNT(buffer));

		return EnqueueForOutbound();
	} else {
		// pUserData has content, probably metadata
		PushMetaData((Variant &) *((Variant *) pUserData));
	}

	return true;
}

void WrtcConnection::SignalOutOfBandDataEnd(void *pUserData) {
	FINE("Outbound stream has been detached.");

	// Stream has been disconnected
	_isStreamAttached = false;
	_pOutNetMP4Stream = NULL;

	EnqueueForDelete();
}

void WrtcConnection::SignalEvent(string eventName, Variant &args) {
	if (eventName == "srtpInitEvent") {
		if (args.HasKeyChain(V_BOOL, false, 1, "result") && _pOutboundConnectivity != NULL) {
			if (((bool)args.GetValue("result", false)) == true) {
				_pOutboundConnectivity->Enable(true);
				INFO("WrtcConnection: Enable the already created Outbound Connection");
			}
			else {

				_rtcpReceiver.SetOutNetRTPStream(NULL);

				INFO("WrtcConnection: Destroying Outbound Connection and Outbound Stream");

				// destroy outboundConnectivity and Outbound Stream
				_pOutboundConnectivity->SetOutStream(NULL);
				_pOutStream->SetConnectivity(NULL);

				delete _pOutboundConnectivity;
				delete _pOutStream;

				_pOutNetMP4Stream = NULL;
				_pOutStream = NULL;
			}
		}

#ifdef PLAYBACK_SUPPORT
		if (args.HasKeyChain(V_BOOL, false, 1, "result") && _pInboundConnectivity != NULL && _pTalkDownStream != NULL) {
			if (((bool)args.GetValue("result", false)) == true) {
				_pTalkDownStream->Enable(true);
				if(_is2wayAudioSupported) {
#ifdef RTMSG
					notifyLedManager(1);
#endif
				}
			}
			else {

				// destroy inboundConnectivity and Outbound Stream
				delete _pInboundConnectivity;
				delete _pTalkDownStream;

				_pInboundConnectivity = NULL;
				_pTalkDownStream = NULL;
			}
		}
#endif
	}
	else if (eventName == "updatePendingStream") {
		BaseClientApplication *pApp = GetApplication();
		if (pApp == NULL) {
			FATAL("Unable to get application");
			return;
		}
		StreamsManager *pSM = pApp->GetStreamsManager();
		if (pSM == NULL) {
			FATAL("Unable to get streams manager");
			return;
		}
		if (_pPendingInStream == NULL) {
			if (!args.HasKeyChain(_V_NUMERIC, false, 1, "streamID")) {
				FATAL("Invalid update stream callback");
				return;
			}
			uint32_t streamID = args.GetValue("streamID", false);
			_pPendingInStream = (BaseInStream *) pSM->FindByUniqueId(streamID);
			if (_pPendingInStream == NULL) { // for some reason, the stream does not exist anymore
				FATAL("Unable to retrieve stream");
				return;
			}

			_pendingFlag |= PENDING_STREAM_REGISTERED;
		}

		StreamCapabilities *pCaps = _pPendingInStream->GetCapabilities();
		if (pCaps == NULL) {
			return;
		}

		if (!(_pendingFlag & PENDING_HAS_VCODEC) && pCaps->GetVideoCodec() != NULL) {
			_pendingFlag |= PENDING_HAS_VCODEC;
		}
		if (!(_pendingFlag & PENDING_HAS_ACODEC) && pCaps->GetAudioCodec() != NULL) {
			_pendingFlag |= PENDING_HAS_ACODEC;
		}
		uint32_t pendingFlagCheck = _pendingFlag & PENDING_COMPLETE;
		if ((pendingFlagCheck == PENDING_COMPLETE)
			|| (_pPendingInStream->GetType() == ST_IN_NET_RTP && 
				pendingFlagCheck == (PENDING_STREAM_REGISTERED | PENDING_HAS_VCODEC)
				)
			) {
			if (_waiting) {
				string streamName = args.GetValue("streamName", false);
				CreateOutStream(streamName, _pPendingInStream, pSM);
				_waiting = false;
				_pendingFlag = 0;
				_pPendingInStream = NULL;
				Start();
			}
		}
	}
}

void WrtcConnection::Stop() {
	if (_pOutNetMP4Stream != NULL) {
		_pOutNetMP4Stream->SignalDetachedFromInStream();
		_pOutNetMP4Stream = NULL;
	}
}

bool WrtcConnection::HasAudio() {
	return _mediaContext.hasAudio;
}
bool WrtcConnection::HasVideo() {
	return _mediaContext.hasVideo;
}
void WrtcConnection::HasAudio(bool value) {
	_mediaContext.hasAudio = value;
}
void WrtcConnection::HasVideo(bool value) {
	_mediaContext.hasVideo = value;
}

void WrtcConnection::UnlinkSignal(WrtcSigProtocol *pSig, bool external) {
	if (pSig != _pSig)
		return;
	if (external && _pSig != NULL) {
		_pSig->UnlinkConnection(this, false);
	}
	_pSig = NULL;
}

string WrtcConnection::GetBaseStreamName(string const &streamName) {
	size_t pos = streamName.rfind(";*");
	if (pos != string::npos) {
		return streamName.substr(pos + 2);
	}
	return streamName;
}

bool WrtcConnection::IsSameStream(string const &localStreamName, 
		string const &streamName) { 
	if (localStreamName == streamName) {
		return true;
	}
	//this is used to handle playlists using uuids on their names
	bool isSame = false;
	string baseName = GetBaseStreamName(streamName);
	if (baseName != streamName) {
		isSame = (localStreamName == baseName);
	}
	return isSame;
}

bool WrtcConnection::SendWebRTCCommand(Variant& settings) {
	FINEST("WrtcConnection::SendWebRTCCommand()");
	string command = settings["command"];
	string localStreamName = settings["localstreamname"];
	string argument = settings["argument"];
	string isBase64 = settings["base64"];

	string payload = "{\"command\":\"" + command + "\",\"argument\":\"" + argument + "\",\"base64\":" + isBase64 + "}";

	BaseOutStream *pOutStream = GetOutboundStream();
	if (pOutStream != NULL && IsSameStream(localStreamName, pOutStream->GetName())) {
		DEBUG("Sending command through data channel...");

		// Log the message only if client is connected to RMS
		if (payload.find(TOGGLE_COMMAND) != string::npos ) {
			INFO("StreamTeardown: toggling the Audio.");
		}

		return SendCommandData(payload);
	}

	return true;
}

bool WrtcConnection::HandlePauseCommand() {
	INFO("Stream playback paused");
	BaseOutStream *pOutStream = GetOutboundStream();
	if (pOutStream != NULL)
		pOutStream->SignalPause();
	return true;
}

bool WrtcConnection::HandleResumeCommand() {
	INFO("Stream playback resumed");
	BaseOutStream *pOutStream = GetOutboundStream();
	if (pOutStream != NULL)
		pOutStream->SignalResume();
	return true;
}

uint32_t WrtcConnection::GetCommandChannelId() {
	return _commandChannelId;
}

BaseOutStream *WrtcConnection::GetOutboundStream() {
	return _useSrtp ? _pOutStream : _pOutNetMP4Stream;
}

void WrtcConnection::SetPeerCapabilities(string capabilities) {
	// Reset values here
	_capabPeer.hasHeartbeat = false;
	//TODO: include other features

	// Check for the features
#ifdef WRTC_CAPAB_HAS_HEARTBEAT	
	if (capabilities.find(WRTC_CAPAB_STR_HAS_HEARTBEAT) != string::npos) {
		INFO("Peer has support for heartbeat mechanism.");
		_capabPeer.hasHeartbeat = true;
	}
#endif // WRTC_CAPAB_HAS_HEARTBEAT
}

Capabilities WrtcConnection::GetCapabilities(bool isRms) {
	if (isRms) {
		return _capabRms;
	} else {
		return _capabPeer;
	}
}

void WrtcConnection::SetRmsCapabilities() {
	// Reset values here
	_capabRms.hasHeartbeat = false;
	//TODO: include other features
	
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
	_capabRms.hasHeartbeat = true;
#endif // WRTC_CAPAB_HAS_HEARTBEAT
}

#ifdef WRTC_CAPAB_HAS_HEARTBEAT
bool WrtcConnection::SendHeartbeat() {
	Variant meta;

	meta["type"] = "heartbeat";
	meta["payload"]= "";

	string data = "";
	meta.SerializeToJSON(data, true);

	return SendCommandData(data);
}

void WrtcConnection::ProcessHeartbeat() {
	_hbNoAckCounter = 0;
}
#endif // WRTC_CAPAB_HAS_HEARTBEAT

void WrtcConnection::RemoveIceInstance(BaseIceProtocol *pIce) {
	// Remove all instances of this ice protocol from the lists being managed
	FOR_VECTOR_ITERATOR(BaseIceProtocol *, _ices, iterator) {
		if (VECTOR_VAL(iterator)->GetId() == pIce->GetId()) {
			_ices.erase(iterator);
			break;
		}
	}
}

#ifdef RTMSG
void WrtcConnection::notifyLedManager(int led_stat) {

	BaseClientApplication *pApp = GetApplication();
        if (pApp == NULL) {
                FATAL("failed in notifying the Led Manager as unable to get the application");
                return;
        }
	rtMessage_Create(&m);
	rtMessage_SetInt32(m, "audio_status", led_stat);

	WrtcAppProtocolHandler *handler = (WrtcAppProtocolHandler *) (pApp->GetProtocolHandler(PT_WRTC_SIG));
	if(handler) {
		rtConnection connectionSend = handler->GetRtSendMessageTag();
		err = rtConnection_SendMessage(connectionSend, m, "RDKC.TWOWAYAUDIO");

		if (err != RT_OK) {
			FINE("Error sending msg via rtmessage to led manager");
		}
	}
	else {
                FATAL("failed in notifying the Led Manager as unable to get the sig protocol handler");
	}
	rtMessage_Release(m);
}
#endif

bool WrtcConnection::SetPeerState(PeerState peerState) {
	if (_pSig != NULL) {
		_pSig->SetPeerState(peerState);
		return true;
	}
	else {
		return false;
	}
}

string WrtcConnection::GetPeerState() {
	if (_pSig != NULL) {
		return _pSig->GetPeerState();
	}
	else {
		return NULL;
	}
}

bool WrtcConnection::ResolveDomainName(string domainName, vector<string> &resolvedIPs) {
	resolvedIPs.clear();
	string domain;
	uint16_t port;
	if (!SplitIpStr(domainName, domain, port)) {
		FATAL("Invalid STUN/TURN server format (%s)!", STR(domainName));
		return false;
	}

	if(!getAllHostByName(domain, resolvedIPs)) {
		FATAL("Could not resolve STUN/TURN server (%s)!", STR(domain));
		return false;
	}

	INFO("List of resolved ips for %s are: ", STR(domain));
	FOR_VECTOR(resolvedIPs, i) {
		INFO(" %s", STR(resolvedIPs[i]) );
		//Update resolutions with port number so we can use them all later
		resolvedIPs[i] = SocketAddress::getIPwithPort(resolvedIPs[i], port);
	}

	return resolvedIPs.size() > 0;
}
#endif // HAS_PROTOCOL_WEBRTC
