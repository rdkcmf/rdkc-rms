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
*/

#include "application/originapplication.h"
#include "protocols/protocolmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/protocolfactory.h"
#include "protocols/baseprotocol.h"
#include "protocols/tcpprotocol.h"
#include "protocols/dev/v4l2deviceprotocol.h"
#include "protocols/dev/v4l2deviceprotocolhandler.h"
#include "protocols/nmea/inboundnmeaprotocol.h"
#include "protocols/nmea/nmeaappprotocolhandler.h"
#include "protocols/metadata/jsonmetadataappprotocolhandler.h"
#include "protocols/metadata/jsonmetadataprotocol.h"
#include "protocols/timer/clusteringtimerprotocol.h"
#include "protocols/timer/jobstimerprotocol.h"
#include "protocols/timer/jobstimerappprotocolhandler.h"
#include "protocols/timer/startwebrtctimerprotocol.h"
#include "protocols/timer/keepalivetimerprotocol.h"
#include "protocols/rtmp/rtmpappprotocolhandler.h"
#include "protocols/cli/cliappprotocolhandler.h"
#include "protocols/cli/inboundjsoncliprotocol.h"
#ifdef HAS_PROTOCOL_ASCIICLI
#include "protocols/cli/inboundasciicliprotocol.h"
#endif /* HAS_PROTOCOL_ASCIICLI */
#include "protocols/rpc/baserpcappprotocolhandler.h"
#include "protocols/rtp/rtpappprotocolhandler.h"
#include "protocols/rtp/rtspappprotocolhandler.h"
#include "protocols/rtp/rtspprotocol.h"
#include "protocols/rtmp/streaming/outfilertmpflvstream.h"
#include "protocols/ts/tsappprotocolhandler.h"
#include "protocols/variant/originvariantappprotocolhandler.h"
#include "protocols/liveflv/liveflvappprotocolhandler.h"
#include "protocols/rtmp/streaming/innetrtmpstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/drm/drmappprotocolhandler.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include "streaming/hls/outfilehlsstream.h"
#include "streaming/hls/hlsplaylist.h"
#include "streaming/hds/hdsmanifest.h"
#include "streaming/hds/outfilehdsstream.h"
#include "streaming/mss/mssmanifest.h"
#include "streaming/mss/msspublishingpoint.h"
#include "streaming/mss/outfilemssstream.h"
#include "streaming/mss/mssfragment.h"
#include "streaming/dash/dashmanifest.h"
#include "streaming/dash/outfiledashstream.h"
#include "protocols/external/externalmoduleapi.h"
#include "recording/ts/outfilets.h"
#include "recording/flv/outfileflv.h"
#include "recording/mp4/outfilemp4.h"
#include "protocols/passthrough/passthroughappprotocolhandler.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "eventlogger/sinktypes.h"
#include "eventlogger/eventlogger.h"
#include "eventlogger/rpcsink.h"
#include "protocols/timer/processwatchdogtimerprotocol.h"
#include "protocols/timer/customtimer.h"
#include "protocols/timer/drmtimerprotocol.h"
#include "protocols/http/httpappprotocolhandler.h"
#include "protocols/drm/drmdefines.h"
#include "protocols/vmf/outvmfappprotocolhandler.h"
#include "protocols/wrtc/wrtcappprotocolhandler.h"
#include "protocols/variant/statsvariantappprotocolhandler.h"
#include "protocols/rtp/sdpappprotocolhandler.h"
#include "protocols/ws/basewsappprotocolhandler.h"

using namespace app_rdkcrouter;
#ifdef HAS_PROTOCOL_MSS
using namespace MicrosoftSmoothStream;
#endif
#ifdef HAS_PROTOCOL_DASH
using namespace MpegDash;
#endif

#define PULL_PUSH_CONFIG_FILE_VER_INITIAL 1
#define PULL_PUSH_CONFIG_FILE_VER_FORBID_STREAM_NAMES_DUPLICATES 2
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_ISHLS_SETTING 3
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_TTLTOS_SETTING 4
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_WIDTHHEIGHT_SETTING 5
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_RTCP_DETECTION_INTERVAL 6
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_TCURL 7
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_FORCETCP_ON_PUSH 8
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_HDS 9
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_CLEANUP_DESTINATION 10
#define PULL_PUSH_CONFIG_FILE_VER_MISSING_RECORD 11
#define PULL_PUSH_CONFIG_FILE_VER_WRONG_HDS_CHUNK_LENGTH 12
#define PULL_PUSH_CONFIG_FILE_VER_CHANGE_VARIANT_INDEXER 13
#define PULL_PUSH_CONFIG_FILE_VER_REMOVE_ISXXX_BOOL_VALUES 14
#define PULL_PUSH_CONFIG_FILE_VER_RENAME_ROLLING_LIMIT 15
#define PULL_PUSH_CONFIG_FILE_VER_ADD_SSMIP_TO_PULL 16
#define PULL_PUSH_CONFIG_FILE_VER_ADD_RTMP_ABSOLUTE_TIMESTAMPS 17
#define PULL_PUSH_CONFIG_FILE_VER_ADD_HTTP_PROXY 18
#define PULL_PUSH_CONFIG_FILE_VER_ADD_RANGE_START_END 19
#define PULL_PUSH_CONFIG_FILE_VER_ADD_PROCESS_GROUPNAME 20
#define PULL_PUSH_CONFIG_FILE_VER_ADD_RECORD_WAITFORIDR 21
#define PULL_PUSH_CONFIG_FILE_VER_ADD_SEND_RENEW_STREAM 22
#define PULL_PUSH_CONFIG_FILE_VER_ADD_VERIMATRIX_DRM 23
#define PULL_PUSH_CONFIG_FILE_VER_ADD_MSS 24
#define PULL_PUSH_CONFIG_FILE_VER_ADD_DASH 25
#define PULL_PUSH_CONFIG_FILE_VER_ADD_WEBRTC 26
#define PULL_PUSH_CONFIG_FILE_VER_ADD_METADATA 27
#define PULL_PUSH_CONFIG_FILE_VER_CURRENT PULL_PUSH_CONFIG_FILE_VER_ADD_METADATA
static string _STREAM_CONFIG_FILE="/opt/usr_config/videoconfig.xml";
#define OPTIMIZED_VIDEO_PROFILE "/tmp/.OptimizedVideoQualityEnabled"

uint32_t OriginApplication::_idGenerator = 0;
map<string, double> OriginApplication::_hlsTimestamps;
map<string, RecordingSession> OriginApplication::_recordSession;

OriginApplication::OriginApplication(Variant &configuration)
: BaseClientApplication(configuration) {
#ifdef HAS_PROTOCOL_EXTERNAL
    _pExternalAPI = NULL;
#endif /* HAS_PROTOCOL_EXTERNAL */
    _pRTMPHandler = NULL;
    _pHTTPHandler = NULL;
    _pCLIHandler = NULL;
    _pRTPHandler = NULL;
    _pRTSPHandler = NULL;
    _pPassThrough = NULL;
    _pTimerHandler = NULL;
    _pTSHandler = NULL;
    _pLiveFLV = NULL;
    _pVariantHandler = NULL;
    _pRPCHandler = NULL;
    _pStatsHandler = NULL;
    _pSDPHandler = NULL;
#ifdef HAS_PROTOCOL_DRM
    _pDRMHandler = NULL;
    _drmTimerProtocolId = 0;
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_PROTOCOL_RAWMEDIA
	_pRawMediaHandler = NULL;
#endif /* HAS_PROTOCOL_RAWMEDIA */
#ifdef HAS_PROTOCOL_WS
	_pWS = NULL;
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_API
	_pApiHandler = NULL;
#endif /* HAS_PROTOCOL_API */

    _retryTimerProtocolId = 0;
    _startwebrtcTimerProtocolId = 0;
    _keepAliveTimerProtocolId = 0;
    _clusteringTimerProtocolId = 0;
    _processWatchdogTimerProtocolId = 0;
    _configurations["push"].IsArray(true);
    _configurations["pull"].IsArray(true);
    _configurations["hls"].IsArray(true);
    _configurations["hds"].IsArray(true);
    _configurations["mss"].IsArray(true);
    _configurations["dash"].IsArray(true);
    _configurations["record"].IsArray(true);
    _configurations["process"].IsArray(true);
    _configurations["webrtc"].IsArray(true);
	_configurations["metalistener"].IsArray(true);
    _isServer = false;
    _connectionsCountLimit = 0;
    _hardConnectionLimitCount = 0;
    _inBandwidthLimit = 0;
    _outBandwidthLimit = 0;
    _hardLimitIn = 0;
    _hardLimitOut = 0;
    _pushSetup.IsArray(false);
    _hlsVersion = 2;
#ifdef HAS_PROTOCOL_WEBRTC
	_pWrtcAppProtocolHandler = NULL;
#endif // HAS_PROTOCOL_WEBRTC
#ifdef HAS_PROTOCOL_METADATA
	_pJsonMetadataHandler = NULL;
#endif // HAS_PROTOCOL_METADATA
}

OriginApplication::~OriginApplication() {
    UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
    UnRegisterAppProtocolHandler(PT_OUTBOUND_RTMP);
    UnRegisterAppProtocolHandler(PT_INBOUND_RTMPS_DISC);
    if (_pRTMPHandler != NULL) {
        delete _pRTMPHandler;
        _pRTMPHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_INBOUND_HTTP_FOR_RTMP);
    UnRegisterAppProtocolHandler(PT_OUTBOUND_HTTP_FOR_RTMP);
    UnRegisterAppProtocolHandler(PT_HTTP_ADAPTOR);
    UnRegisterAppProtocolHandler(PT_OUTBOUND_HTTP2);
    UnRegisterAppProtocolHandler(PT_HTTP_MSS_LIVEINGEST);
	UnRegisterAppProtocolHandler(PT_HTTP_MEDIA_RECV);
    if (_pHTTPHandler != NULL) {
        delete _pHTTPHandler;
        _pHTTPHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_INBOUND_JSONCLI);
#ifdef HAS_PROTOCOL_ASCIICLI
    UnRegisterAppProtocolHandler(PT_INBOUND_ASCIICLI);
#endif /* HAS_PROTOCOL_ASCIICLI */
    if (_pCLIHandler != NULL) {
        delete _pCLIHandler;
        _pCLIHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_INBOUND_RTP);
    if (_pRTPHandler != NULL) {
        delete _pRTPHandler;
        _pRTPHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_RTSP);
    if (_pRTSPHandler != NULL) {
        delete _pRTSPHandler;
        _pRTSPHandler = NULL;
    }

	UnRegisterAppProtocolHandler(PT_SDP);
	if (_pSDPHandler != NULL) {
		delete _pSDPHandler;
		_pSDPHandler = NULL;
	}

	UnRegisterAppProtocolHandler(PT_PASSTHROUGH);
	if (_pPassThrough != NULL) {
		delete _pPassThrough;
		_pPassThrough = NULL;
	}

    UnRegisterAppProtocolHandler(PT_INBOUND_LIVE_FLV);
    if (_pLiveFLV != NULL) {
        delete _pLiveFLV;
        _pLiveFLV = NULL;
    }

    for (size_t i = 0; i < _pRPCSinks.size(); i++)
        _pRPCSinks[i]->SetRpcProtocolHandler(NULL);
    UnRegisterAppProtocolHandler(PT_OUTBOUND_RPC);
    if (_pRPCHandler != NULL) {
        delete _pRPCHandler;
        _pRPCHandler = NULL;
    }

	UnRegisterAppProtocolHandler(PT_JSON_VAR);
	if (_pStatsHandler != NULL) {
		delete _pStatsHandler;
		_pStatsHandler = NULL;
	}

#ifdef HAS_PROTOCOL_DRM
    UnRegisterAppProtocolHandler(PT_DRM);
    if (_pDRMHandler != NULL) {
        delete _pDRMHandler;
        _pDRMHandler = NULL;
    }
#endif /* HAS_PROTOCOL_DRM */

    BaseProtocol *pProtocol = ProtocolManager::GetProtocol(_retryTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }

    pProtocol = ProtocolManager::GetProtocol(_startwebrtcTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }

    pProtocol = ProtocolManager::GetProtocol(_keepAliveTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }

    pProtocol = ProtocolManager::GetProtocol(_clusteringTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }

    pProtocol = ProtocolManager::GetProtocol(_processWatchdogTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }

#ifdef HAS_PROTOCOL_DRM
    pProtocol = ProtocolManager::GetProtocol(_drmTimerProtocolId);
    if (pProtocol != NULL) {
        pProtocol->EnqueueForDelete();
    }
#endif /* HAS_PROTOCOL_DRM */

    UnRegisterAppProtocolHandler(PT_TIMER);
    if (_pTimerHandler != NULL) {
        delete _pTimerHandler;
        _pTimerHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_INBOUND_TS);
    if (_pTSHandler != NULL) {
        delete _pTSHandler;
        _pTSHandler = NULL;
    }

    UnRegisterAppProtocolHandler(PT_BIN_VAR);
    if (_pVariantHandler != NULL) {
        delete _pVariantHandler;
        _pVariantHandler = NULL;
    }
#ifdef HAS_PROTOCOL_EXTERNAL
    ProtocolFactory::UnRegisterExternalModuleProtocolHandlers(this);

    if (_pExternalAPI != NULL) {
        delete _pExternalAPI;
        _pExternalAPI = NULL;
    }
#endif /* HAS_PROTOCOL_EXTERNAL */
#ifdef HAS_PROTOCOL_WS
	UnRegisterAppProtocolHandler(PT_INBOUND_WS);
	UnRegisterAppProtocolHandler(PT_WS_METADATA);
#ifdef HAS_PROTOCOL_WS_FMP4
	UnRegisterAppProtocolHandler(PT_INBOUND_WS_FMP4);
#endif /* HAS_PROTOCOL_WS_FMP4 */
	if (_pWS != NULL) {
		delete _pWS;
		_pWS = NULL;
	}
#endif /* HAS_PROTOCOL_WS */

#ifdef HAS_PROTOCOL_WEBRTC
	UnRegisterAppProtocolHandler(PT_WRTC_CNX);
	UnRegisterAppProtocolHandler(PT_WRTC_SIG);
	if (_pWrtcAppProtocolHandler != NULL) {
        delete _pWrtcAppProtocolHandler;
        _pWrtcAppProtocolHandler = NULL;
    }
#endif /* HAS_PROTOCOL_WEBRTC */

#ifdef HAS_PROTOCOL_METADATA
	UnRegisterAppProtocolHandler(PT_JSONMETADATA);
	if (_pJsonMetadataHandler != NULL) {
		delete _pJsonMetadataHandler;
		_pJsonMetadataHandler = NULL;
	}
#endif /* HAS_PROTOCOL_METADATA */

#ifdef HAS_PROTOCOL_API
	UnRegisterAppProtocolHandler(PT_API_INTEGRATION);
	if (_pApiHandler != NULL) {
        delete _pApiHandler;
        _pApiHandler = NULL;
    }
#endif /* HAS_PROTOCOL_API */
}

bool OriginApplication::Initialize() {
    if (_configuration["metaFileGenerator"] != V_BOOL) {
        _configuration["metaFileGenerator"] = (bool)false;
    }

    _isServer = !((bool)_configuration["metaFileGenerator"]);

    if (!_isServer)
        return InitializeForMetaFileGenerator();
    else
        return InitializeForServing();
}

bool OriginApplication::IsServer() {
    return _isServer;
}

bool OriginApplication::AcceptTCPConnection(TCPAcceptor *pTCPAcceptor) {
    if (DropConnection(pTCPAcceptor))
        return true;
    //1. Get the parameters
    Variant &params = pTCPAcceptor->GetParameters();

    //2. If this is not an RTMP connection, then don't cluster it
    if ((params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMP)
            && (params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMPS)) {
        return BaseClientApplication::AcceptTCPConnection(pTCPAcceptor);
    }

    //3. If this connection landed on non-clustering
    //acceptor, let the cluster manage it
    bool clustering = false;
    if (params.HasKeyChain(V_BOOL, false, 1, "clustering"))
        clustering = (bool)params.GetValue("clustering", false);
    if (clustering)
        return BaseClientApplication::AcceptTCPConnection(pTCPAcceptor);

    //4. let the cluster kick in
    return _pVariantHandler->AcceptTCPConnection(pTCPAcceptor);
}

void OriginApplication::RegisterProtocol(BaseProtocol *pProtocol) {
    //1. Execute normal register
    BaseClientApplication::RegisterProtocol(pProtocol);

    //2. get the connection type (push/pull) and the stream config
    Variant streamConfig;
    OperationType operationType = GetOperationType(pProtocol, streamConfig);

    //3. If this is a push/pull connection, change the status
    if (operationType != OPERATION_TYPE_STANDARD) {
        SetStreamStatus(streamConfig, SS_CONNECTED, 0);
    }
}

void OriginApplication::UnRegisterProtocol(BaseProtocol *pProtocol) {
    //1. get the connection type (push/pull) and the stream config
    Variant streamConfig;
    OperationType operationType = GetOperationType(pProtocol, streamConfig);
	if ((operationType == OPERATION_TYPE_WEBRTC) &&
		(pProtocol->GetType() == PT_WRTC_CNX)) {
		operationType = OPERATION_TYPE_STANDARD;
	}
    //2. Execute normal unregister
    BaseClientApplication::UnRegisterProtocol(pProtocol);

    //3. If this is a push/pull connection and keepAlive is set, enqueue the operation again
	if (pProtocol->GetType() == PT_WRTC_SIG) { 
		// Get the WRTC parameters and its configID.  Since this is a WRTC_SIG connection
		// we should have a configID.
		Variant currWrtcParams = pProtocol->GetCustomParameters();
		uint32_t currWrtcConfigID = currWrtcParams["configId"];

		// CHECK the list of active protocols if this soon to be deleted WRTCSig protocol is
		// the last of its kind.  IF this is the last of its kind, then we can... or rather
		// _should_ safely assume that the RRS has died.
		map<uint32_t, BaseProtocol *> activeProtocols = ProtocolManager::GetActiveProtocols();
		int nErsConnections = 0;
		FOR_MAP(activeProtocols, uint32_t, BaseProtocol*, i) {
			// We are looking for ALL WRTC connections that share a configID with the 
			// protocol to be unregistered __AND__ is not enqueued for deletion.
			BaseProtocol* pProt = i->second;
			Variant checkParams = pProt->GetCustomParameters();
			uint32_t checkConfigId = checkParams["configId"];
			if (   (pProt->GetType() == PT_WRTC_SIG)
				&& (checkConfigId == currWrtcConfigID)
				&& (!pProt->IsEnqueueForDelete()) ) {
					++nErsConnections;
			}
		}
		// IF we have at least 1 active WRTC connection, then we can assume that the
		// RRS is still alive
		if (nErsConnections == 0) {
			FOR_MAP(_configurations["webrtc"], string, Variant, i) {
				// weed out a blank config that's coming in for some reason
				uint32_t configId = (uint32_t)MAP_VAL(i)["configId"];
				if (!configId) break;
				if (configId == uint32_t(currWrtcParams["configId"])) {
					if (bool(MAP_VAL(i)["keepAlive"]) == true) {
						currWrtcParams["keepAlive"] = true;
						operationType = OPERATION_TYPE_WEBRTC;
						EnqueueTimedOperation(currWrtcParams, operationType);
					}
				}
			}
		}
	} else if (operationType != OPERATION_TYPE_STANDARD) {
        SetStreamStatus(streamConfig, SS_DISCONNECTED, 0);
		// Check if keepAlive is set or this is an active lazy pull
        if ((bool)streamConfig["keepAlive"] ||
				(streamConfig.HasKey("_isLazyPull") && 
					IsLazyPullActive(streamConfig["localStreamName"]))) {
            EnqueueTimedOperation(streamConfig, operationType);
        } else {
            if ((streamConfig.HasKeyChain(V_BOOL, true, 1, "forceReconnect"))
                    && ((bool)streamConfig["forceReconnect"])) {
                EnqueueTimedOperation(streamConfig, operationType);
            } else {
                RemoveFromConfigStatus(operationType, streamConfig);
            }
        }
	} 
    //4. Make sure there is no trace of failed lazy pulls in pull configuration
    Variant &protoParams = pProtocol->GetCustomParameters();
    if (protoParams.HasKeyChain(V_STRING, false, 1, "streamName")) {
        string streamName = (string) protoParams.GetValue("streamName", false);
        if (_configurations.HasKey("pull", false)) {
            uint32_t configId = 0;

            FOR_MAP(_configurations["pull"], string, Variant, i) {
                if ((string) MAP_VAL(i).GetValue("localStreamName", false) == streamName &&
                        MAP_VAL(i).HasKeyChain(_V_NUMERIC, false, 2, "_callback", "protocolID") &&
						MAP_VAL(i).HasKeyChain(V_STRING, false, 1, "_isVod") &&
						((string)MAP_VAL(i)["_isVod"] == "1") &&
                        (uint32_t) MAP_VAL(i)["_callback"]["protocolID"] == pProtocol->GetId()) {
                    configId = (uint32_t) MAP_VAL(i).GetValue("configId", false);
                    break;
                }
            }

            if (configId != 0)
                RemoveConfigId(configId, false);
        }
    }
}

bool OriginApplication::PushLocalStream(Variant &streamConfig) {
    if (!IsActive(streamConfig)) {
        return true;
    }
    streamConfig["operationType"] = (uint8_t) OPERATION_TYPE_PUSH;
    SaveConfig(streamConfig, OPERATION_TYPE_PUSH);
    SetStreamStatus(streamConfig, SS_CONNECTING, 0);
    string localStreamName = streamConfig["localStreamName"];
    string targetUri = streamConfig["targetUri"];
    string targetStreamName = streamConfig["targetStreamName"];

    _pushSetup[localStreamName][targetUri][targetStreamName] = streamConfig;

	StreamsManager *pStreamsManager = GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pStreamsManager->FindByTypeByName(
			ST_IN_NET, streamConfig["localStreamName"], true, false);
	if (streams.size() == 0) {
		streams = pStreamsManager->FindByTypeByName(
			ST_IN_DEVICE, streamConfig["localStreamName"], true, false);
		if (streams.size() == 0) {
			WARN("Input stream %s not available yet", STR(streamConfig["localStreamName"]));
			SetStreamStatus(streamConfig, SS_WAITFORSTREAM, 0);
			return true;
		}
	}

    if (!BaseClientApplication::PushLocalStream(streamConfig)) {
        FATAL("Unable to push the stream");
        return false;
    }

    streamConfig["targetUri"] = targetUri;
    RemoveFromPushSetup(streamConfig);
    return true;
}

bool OriginApplication::PullExternalStream(Variant &streamConfig) {
	if (!IsActive(streamConfig)) {
		return true;
	}
	streamConfig["operationType"] = (uint8_t)OPERATION_TYPE_PULL;
	SaveConfig(streamConfig, OPERATION_TYPE_PULL);
    SetStreamStatus(streamConfig, SS_CONNECTING, 0);
    return BaseClientApplication::PullExternalStream(streamConfig);
}

void OriginApplication::SignalEventSinkCreated(BaseEventSink *pSink) {
    if (pSink->GetSinkType() == SINK_RPC)
        ADD_VECTOR_END(_pRPCSinks, (RpcSink *) pSink);
}

void OriginApplication::EventLogEntry(BaseEventSink *pEventSink, Variant &event) {
    //TODO: we need to find a way to maintain this function for vbrick
    //The thing is that the event needs to be re-arranged with the vbrick style
    //code executed on "if 1" is the generic one. Put it on "if 0" for vbrick style
#if 1
    BaseClientApplication::EventLogEntry(pEventSink, event);
#else
    if (pEventSink->GetSinkType() != SINK_RPC) {
        BaseClientApplication::EventLogEntry(pEventSink, event);
        return;
    }

    uint64_t type = (uint64_t) event["payload"]["typeNumeric"];

    //FINEST("%s", STR(event.ToString()));
    if ((event["type"] != "inStreamCodecsUpdated")
            && (event["type"] != "inStreamClosed")
            && (event["type"] != "outStreamCodecsUpdated")
            && (event["type"] != "outStreamClosed")
            ) {
        if ((type != ST_OUT_NET_PASSTHROUGH) && (type != ST_IN_NET_PASSTHROUGH))
            return;
    }

    Variant call;

    call.SetTypeName("MultiprotocolSAPEvent");
    if ((event["type"] == "inStreamCodecsUpdated")
            || (event["type"] == "outStreamCodecsUpdated")
            || (event["type"] == "outStreamCreated")
            || (event["type"] == "inStreamCreated")) {
        call["object"]["event"] = "start";
    } else {
        call["object"]["event"] = "stop";
    }

    if (TAG_KIND_OF(type, ST_IN)) {
        call["object"]["direction"] = "in";
    } else {
        call["object"]["direction"] = "out";
    }
    call["object"]["stream"] = event["payload"]["name"];
    call["object"]["appName"] = event["payload"]["appName"];
    call["object"]["audio-type"] = event["payload"]["audio"]["codec"];
    call["object"]["video-type"] = event["payload"]["video"]["codec"];
    call["object"]["rate"] = event["payload"]["bandwidth"];
    call["object"]["ip"] = event["payload"]["ip"];
    call["object"]["port"] = event["payload"]["port"];
    call["object"]["uniqueId"] = event["payload"]["uniqueId"];
    if (event.HasKeyChain(V_MAP, true, 2, "payload", "hlsSettings")) {
        if ((bool)event["payload"]["hlsSettings"]["createMasterPlaylist"])
            call["object"]["stream"] = (string) event["payload"]["hlsSettings"]["groupName"];
        else
            call["object"]["stream"] = "HLS/" + (string) event["payload"]["name"];
    }
    if (event.HasKeyChain(V_MAP, true, 2, "payload", "hdsSettings")) {
        call["object"]["stream"] = (string) event["payload"]["hdsSettings"]["groupName"]
                + "/" + (string) event["payload"]["name"];
    }

    bool send = false;
    if ((type == ST_IN_NET_PASSTHROUGH) || (type == ST_OUT_NET_PASSTHROUGH)) {
        call["object"]["protocol"] = "passThroughTs";
        send = true;
    } else if ((type == ST_IN_NET_RTMP)
            || (type == ST_OUT_NET_RTMP)) {
        call["object"]["protocol"] = "rtmp";
        send = true;
    } else if ((type == ST_IN_NET_TS) || (type == ST_OUT_NET_TS)) {
        call["object"]["protocol"] = "ts";
        send = true;
    } else if ((type == ST_IN_NET_RTP) || (type == ST_OUT_NET_RTP)) {
        call["object"]["protocol"] = "rtsp";
        send = true;
    } else if (type == ST_IN_NET_LIVEFLV) {
        call["object"]["protocol"] = "liveflv";
        send = true;
    } else if (type == ST_OUT_FILE_HLS) {
        call["object"]["protocol"] = "hls";
        send = true;
    } else if (type == ST_OUT_FILE_HDS) {
        call["object"]["protocol"] = "hds";
        send = true;
    } else if (type == ST_OUT_FILE_MSS) {
        call["object"]["protocol"] = "mss";
        send = true;
    } else if (type == ST_OUT_FILE_DASH) {
        call["object"]["protocol"] = "dash";
        send = true;
    } else {
        WARN("Invalid stream type: %s", STR(event["payload"]["type"]));
    }

    Variant &streamSettings = GetStreamSettings(event["payload"]);
    if (streamSettings == V_MAP) {

        FOR_MAP(streamSettings, string, Variant, i) {
            if ((MAP_KEY(i).length() > 0) && (MAP_KEY(i)[0] == '_'))
                call["object"][MAP_KEY(i)] = MAP_VAL(i);
        }
    } else {
        if ((type == ST_OUT_NET_TS)
                || (type == ST_OUT_NET_RTP)
                || (type == ST_OUT_NET_RTMP)) {
            send = false;
        }
    }

    if (send) {
        BaseClientApplication::EventLogEntry(pEventSink, call);
    }
#endif
}

void OriginApplication::InsertPlaylistItem(string &playlistName, string &localStreamName,
        double insertPoint, double sourceOffset, double duration) {
    _pVariantHandler->InsertPlaylistItem(playlistName, localStreamName, insertPoint, sourceOffset, duration);
}

bool OriginApplication::IsStreamAlreadyCreated(OperationType operationType, Variant &settings) {
    string localStreamName;
    string targetFolder;
    string chunkBaseName;

    // If this is launchprocess, just check the stream name if it already exists
    // return false otherwise
    if (operationType == OPERATION_TYPE_LAUNCHPROCESS) {
        string streamName = (string) settings["rmsTargetStreamName"];
        if (!GetStreamsManager()->StreamNameAvailable(streamName)) {
            return true;
        }

        // Make sure that we did not miss anything
        string arguments;

        FOR_MAP(_configurations["process"], string, Variant, i) {
            arguments = (string) MAP_VAL(i)["arguments"];
            if (arguments.rfind(streamName, (arguments.length() - streamName.length())) != string::npos) {
                return true;
            }
        }

        return false;
    }

    // Populate the necessary values
    if ((operationType == OPERATION_TYPE_MSS)
            || (operationType == OPERATION_TYPE_DASH)) {
        settings["chunkBaseName"] = "dummy";
    }
    if ((operationType == OPERATION_TYPE_HLS)
            || (operationType == OPERATION_TYPE_HDS)
            || (operationType == OPERATION_TYPE_MSS)
            || (operationType == OPERATION_TYPE_DASH)) {
        localStreamName = (string) settings["localStreamName"];
        targetFolder = (string) settings["targetFolder"];
        chunkBaseName = (string) settings["chunkBaseName"];
    }
    //TODO include here other streams, dash, etc

    // Check if same stream was previously created
    if (_pushSetup.HasKeyChain(V_MAP, false, 3, STR(localStreamName), STR(targetFolder),
            STR(chunkBaseName))) {
        return true;
    }

    string key = "";
    if (operationType == OPERATION_TYPE_HLS) {
        key = "hls";
    } else if (operationType == OPERATION_TYPE_HDS) {
        key = "hds";
    } else if (operationType == OPERATION_TYPE_MSS) {
        key = "mss";
    } else if (operationType == OPERATION_TYPE_DASH) {
        key = "dash";
    }
    if (key != "") {

        FOR_MAP(_configurations[key], string, Variant, i) {
            if ((MAP_VAL(i)["localStreamName"] == localStreamName)
                    && (MAP_VAL(i)["targetFolder"] == targetFolder)
                    && (MAP_VAL(i)["chunkBaseName"] == chunkBaseName)) {
                return true;
            }
        }
    }

    // No same stream AND group exists, return false
    return false;
}

#ifdef HAS_PROTOCOL_HLS

bool OriginApplication::CreateHLSStream(Variant &settings) {
    Variant result;
    if (!ComputeHLSPaths(settings, result)) {
        FATAL("Unable to compute HLS paths");
        return false;
    }
    settings["configIds"] = Variant();
    settings["configIds"].IsArray(true);

    FOR_MAP(result, string, Variant, i) {
        /*
         * Check first if we have the scenario where both local stream names and
         * group names are equal to that of being requested
         */
        if (IsStreamAlreadyCreated(OPERATION_TYPE_HLS, MAP_VAL(i))) {
            FATAL("Requested stream (%s) with same group (%s) was already created!",
                    STR(MAP_VAL(i)["localStreamName"]), STR(MAP_VAL(i)["groupName"]));

            return false;
        }

        MAP_VAL(i).RemoveKey("configIds");
        MAP_VAL(i)["hlsVersion"] = _hlsVersion;
        if (!CreateHLSStreamPart(MAP_VAL(i))) {
            FATAL("Unable to create HLS stream");
            return false;
        }
        settings["configIds"].PushToArray(MAP_VAL(i)["configId"]);
    }

    return true;
}

bool OriginApplication::ComputeHLSPaths(Variant &settings, Variant &result) {
    result.Reset();
    result.IsArray(true);
    Variant localStreamNames = settings["localStreamNames"];
    Variant bandwidths = settings["bandwidths"];
    Variant tempSettings = settings;
    tempSettings.RemoveKey("localStreamNames");
    tempSettings.RemoveKey("bandwidths");
    string baseTargetFolder = tempSettings["targetFolder"];
    if (baseTargetFolder[baseTargetFolder.size() - 1] != PATH_SEPARATOR)
        baseTargetFolder += PATH_SEPARATOR;
    string groupName = (string) tempSettings["groupName"];
    string groupTargetFolder = baseTargetFolder + groupName + PATH_SEPARATOR;
    string masterPlaylistPath = baseTargetFolder + groupName + PATH_SEPARATOR + "playlist.m3u8";
    double timestamp = 0;
    GETCLOCKS(timestamp, double);
    timestamp = timestamp / (double) CLOCKS_PER_SECOND * 1000.0;
    if (MAP_HAS1(_hlsTimestamps, groupName)) {
        timestamp = (double) _hlsTimestamps[groupName];
    } else {
        _hlsTimestamps[groupName] = timestamp;
    }
    tempSettings["timestamp"] = timestamp;

    for (uint32_t i = 0; i < localStreamNames.MapSize(); i++) {
        //fix the parameters inside _localStreamName
        string normalizedStreamName = (string) localStreamNames[(uint32_t) i];
        URI temp;
        if (!URI::FromString("http://localhost/" + normalizedStreamName, false, temp)) {
            FATAL("Unable to fix localStreamname: %s", STR(normalizedStreamName));
            return false;
        }
        normalizedStreamName = temp.document();
        Variant parameters = temp.parameters();

        FOR_MAP(parameters, string, Variant, j) {
            normalizedStreamName += "_" + MAP_KEY(j);
            normalizedStreamName += "_" + (string) MAP_VAL(j);
        }

        tempSettings["localStreamName"] = (string) localStreamNames[(uint32_t) i];
        tempSettings["bandwidth"] = (uint32_t) bandwidths[(uint32_t) i];
        tempSettings["groupTargetFolder"] = groupTargetFolder;
        tempSettings["targetFolder"] = groupTargetFolder + normalizedStreamName;
        tempSettings["masterPlaylistPath"] = masterPlaylistPath;
        result.PushToArray(tempSettings);
    }

    return true;
}

bool OriginApplication::CreateHLSStreamPart(Variant &settings) {
    if (!IsActive(settings)) {
        return true;
    }

    if ((settings["drmType"] == DRM_TYPE_VERIMATRIX) &&
            !_configuration.HasKey("drm", false)) {
        FATAL("Encryption with Verimatrix DRM selected but not configured.");
        return false;
    }

    settings["operationType"] = (uint8_t) OPERATION_TYPE_HLS;
    SaveConfig(settings, OPERATION_TYPE_HLS);
    SetStreamStatus(settings, SS_CONNECTING, 0);
    string targetFolder = settings["targetFolder"];
    string chunkBaseName = settings["chunkBaseName"];
    string localStreamName = settings["localStreamName"];
    _pushSetup[localStreamName][targetFolder][chunkBaseName] = settings;

    //1. Get the streams manager
    StreamsManager *pSM = GetStreamsManager();

    //2. Find the in stream
    map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
            ST_IN_NET,
            localStreamName,
            true,
            false);

    BaseInStream *pStream = NULL;

    FOR_MAP(streams, uint32_t, BaseStream *, i) {
        if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_HLS)) {
            pStream = (BaseInStream *) MAP_VAL(i);
            break;
        }
    }
    if (pStream == NULL) {
        WARN("Input stream %s not available yet", STR(localStreamName));
        SetStreamStatus(settings, SS_WAITFORSTREAM, 0);
        return true;
    }

    //3. Create the HLS stream
    OutFileHLSStream *pOutHLS = OutFileHLSStream::GetInstance(
            this,
            localStreamName,
            settings);
    if (pOutHLS == NULL) {
        FATAL("Unable to create HLS stream");
        return false;
    }

    //4. Link them together
    if (!pStream->Link(pOutHLS)) {
        pOutHLS->EnqueueForDelete();
        FATAL("Source stream %s not compatible with HLS streaming", STR(localStreamName));
        return false;
    }

    RemoveFromPushSetup(settings);

    //5.
    return true;
}

void OriginApplication::RemoveHLSGroup(string groupName, bool deleteHLSFiles) {
    vector<uint32_t> configs;
    string groupPath = "";

    FOR_MAP(_configurations["hls"], string, Variant, i) {
        if (MAP_VAL(i)["groupName"] != groupName)
            continue;
        groupPath = (string) MAP_VAL(i)["groupTargetFolder"];
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_HLS;
        ADD_VECTOR_END(configs, (uint32_t) MAP_VAL(i)["configId"]);
    }

    for (uint32_t i = 0; i < configs.size(); i++)
        RemoveConfigId(configs[i], deleteHLSFiles);

    if (deleteHLSFiles) {
        if (groupPath != "") {
            deleteFolder(groupPath, true);
        }
    }
}

#endif /* HAS_PROTOCOL_HLS */

#ifdef HAS_PROTOCOL_HDS

bool OriginApplication::CreateHDSStream(Variant &settings) {
    Variant result;
    if (!ComputeHDSPaths(settings, result)) {
        FATAL("Unable to compute HDS paths");
        return false;
    }

    settings["configIds"] = Variant();
    settings["configIds"].IsArray(true);

    FOR_MAP(result, string, Variant, i) {
        /*
         * Check first if we have the scenario where both local stream names and
         * group names are equal to that of being requested
         */
        if (IsStreamAlreadyCreated(OPERATION_TYPE_HDS, MAP_VAL(i))) {
            FATAL("Requested stream (%s) with same group (%s) was already created!",
                    STR(MAP_VAL(i)["localStreamName"]), STR(MAP_VAL(i)["groupName"]));

            return false;
        }

        MAP_VAL(i).RemoveKey("configIds");
        if (!CreateHDSStreamPart(MAP_VAL(i))) {
            FATAL("Unable to create HDS stream");
            return false;
        }
        settings["configIds"].PushToArray(MAP_VAL(i)["configId"]);
    }

    return true;
}

bool OriginApplication::ComputeHDSPaths(Variant settings, Variant &result) {
    result.Reset();
    result.IsArray(true);
    Variant localStreamNames = settings["localStreamNames"];
    Variant bandwidths = settings["bandwidths"];
    Variant tempSettings = settings;
    tempSettings.RemoveKey("localStreamNames");
    tempSettings.RemoveKey("bandwidths");
    string baseTargetFolder = tempSettings["targetFolder"];
    if (baseTargetFolder[baseTargetFolder.size() - 1] != PATH_SEPARATOR)
        baseTargetFolder += PATH_SEPARATOR;
    string groupTargetFolder = baseTargetFolder + (string) tempSettings["groupName"]
            + PATH_SEPARATOR;

    for (uint32_t i = 0; i < localStreamNames.MapSize(); i++) {
        //fix the parameters inside _localStreamName
        string normalizedStreamName = (string) localStreamNames[(uint32_t) i];
        URI temp;
        if (!URI::FromString("http://localhost/" + normalizedStreamName, false, temp)) {
            FATAL("Unable to fix localStreamname: %s", STR(normalizedStreamName));
            return false;
        }
        normalizedStreamName = temp.document();
        Variant parameters = temp.parameters();

        FOR_MAP(parameters, string, Variant, j) {
            normalizedStreamName += "_" + MAP_KEY(j);
            normalizedStreamName += "_" + (string) MAP_VAL(j);
        }

        tempSettings["localStreamName"] = (string) localStreamNames[(uint32_t) i];
        tempSettings["bandwidth"] = (uint32_t) bandwidths[(uint32_t) i];
        tempSettings["groupTargetFolder"] = groupTargetFolder;
        tempSettings["targetFolder"] = groupTargetFolder + normalizedStreamName;
        result.PushToArray(tempSettings);
    }

    return true;
}

bool OriginApplication::CreateHDSStreamPart(Variant &settings) {
    if (!IsActive(settings)) {
        return true;
    }
    settings["operationType"] = (uint8_t) OPERATION_TYPE_HDS;
    SaveConfig(settings, OPERATION_TYPE_HDS);
    SetStreamStatus(settings, SS_CONNECTING, 0);
    string targetFolder = settings["targetFolder"];
    string chunkBaseName = settings["chunkBaseName"];
    string localStreamName = settings["localStreamName"];
    _pushSetup[localStreamName][targetFolder][chunkBaseName] = settings;
    //
    //1. Get the streams manager
    StreamsManager *pSM = GetStreamsManager();

    //2. Find the in stream
    map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
            ST_IN_NET,
            localStreamName,
            true,
            false);

    BaseInStream *pStream = NULL;

    FOR_MAP(streams, uint32_t, BaseStream *, i) {
        if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_HDS)) {
            pStream = (BaseInStream *) MAP_VAL(i);
            break;
        }
    }
    if (pStream == NULL) {
        WARN("Input stream %s not available yet", STR(localStreamName));
        SetStreamStatus(settings, SS_WAITFORSTREAM, 0);
        return true;
    }

    //3. Create the HDS stream
    OutFileHDSStream *pOutHDS = OutFileHDSStream::GetInstance(
            this,
            localStreamName,
            settings);
    if (pOutHDS == NULL) {
        FATAL("Unable to create HDS stream");
        return false;
    }

    //4. Link them together
    if (!pStream->Link(pOutHDS)) {
        pOutHDS->EnqueueForDelete();
        FATAL("Source stream %s not compatible with HDS streaming", STR(localStreamName));
        return false;
    }

    RemoveFromPushSetup(settings);

    //5.
    return true;
}

void OriginApplication::RemoveHDSGroup(string groupName, bool deleteHDSFiles) {
    vector<uint32_t> configs;
    string groupPath = "";

    FOR_MAP(_configurations["hds"], string, Variant, i) {
        if (MAP_VAL(i)["groupName"] != groupName)
            continue;
        groupPath = (string) MAP_VAL(i)["groupTargetFolder"];
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_HDS;
        ADD_VECTOR_END(configs, (uint32_t) MAP_VAL(i)["configId"]);
    }

    for (uint32_t i = 0; i < configs.size(); i++)
        RemoveConfigId(configs[i], deleteHDSFiles);

    if (deleteHDSFiles) {
        if (groupPath != "") {
            deleteFolder(groupPath, true);
        }
    }
}
#endif /* HAS_PROTOCOL_HDS */

#ifdef HAS_PROTOCOL_MSS

bool OriginApplication::CreateMSSStream(Variant &settings) {
    Variant result;
    if (!ComputeMSSPaths(settings, result)) {
        FATAL("Unable to compute MSS stream paths");
        return false;
    }

    settings["configIds"] = Variant();
    settings["configIds"].IsArray(true);

    FOR_MAP(result, string, Variant, i) {
        /*
         * Check first if we have the scenario where both local stream names and
         * group names are equal to that of being requested
         */
        if (IsStreamAlreadyCreated(OPERATION_TYPE_MSS, MAP_VAL(i))) {
            FATAL("Requested stream (%s) with same group (%s) was already created!",
                    STR(MAP_VAL(i)["localStreamName"]), STR(MAP_VAL(i)["groupName"]));

            return false;
        }

        MAP_VAL(i).RemoveKey("configIds");
        if (!CreateMSSStreamPart(MAP_VAL(i))) {
            FATAL("Unable to create MSS stream");
            return false;
        }
        settings["configIds"].PushToArray(MAP_VAL(i)["configId"]);
    }

    return true;
}

bool OriginApplication::ComputeMSSPaths(Variant settings, Variant &result) {
    result.Reset();
    result.IsArray(true);
    Variant localStreamNames = settings["localStreamNames"];
    Variant bandwidths = settings["bandwidths"];
    Variant tempSettings = settings;
    //tempSettings.RemoveKey("localStreamNames");
    tempSettings.RemoveKey("bandwidths");
    string baseTargetFolder = tempSettings["targetFolder"];
    if (baseTargetFolder[baseTargetFolder.size() - 1] != PATH_SEPARATOR)
        baseTargetFolder += PATH_SEPARATOR;
    string groupTargetFolder = baseTargetFolder + (string) tempSettings["groupName"]
            + PATH_SEPARATOR;
    // used in MSS live ingest
    tempSettings["streamNames"] = localStreamNames;
    if (settings.HasKey("audioBitrates"))
        tempSettings["audioBitrates"] = settings["audioBitrates"];
    if (settings.HasKey("videoBitrates"))
        tempSettings["videoBitrates"] = settings["videoBitrates"];
    tempSettings["bitrates"] = bandwidths;

    for (uint32_t i = 0; i < localStreamNames.MapSize(); i++) {
        //fix the parameters inside _localStreamName
        string normalizedStreamName = (string) localStreamNames[(uint32_t) i];
        URI temp;
        if (!URI::FromString("http://localhost/" + normalizedStreamName, false, temp)) {
            FATAL("Unable to fix localStreamname: %s", STR(normalizedStreamName));
            return false;
        }
        normalizedStreamName = temp.document();
        Variant parameters = temp.parameters();

        FOR_MAP(parameters, string, Variant, j) {
            normalizedStreamName += "_" + MAP_KEY(j);
            normalizedStreamName += "_" + (string) MAP_VAL(j);
        }

        tempSettings["localStreamName"] = (string) localStreamNames[(uint32_t) i];
        tempSettings["bandwidth"] = (uint32_t) bandwidths[(uint32_t) i];
        tempSettings["groupTargetFolder"] = groupTargetFolder;
        tempSettings["targetFolder"] = groupTargetFolder + normalizedStreamName;
        if (settings.HasKey("audioBitrates"))
            tempSettings["audioBitrate"] = (uint32_t) settings["audioBitrates"][(uint32_t) i];
        if (settings.HasKey("videoBitrates"))
            tempSettings["videoBitrate"] = (uint32_t) settings["videoBitrates"][(uint32_t) i];
        result.PushToArray(tempSettings);
    }

    return true;
}

bool OriginApplication::CreateMSSStreamPart(Variant &settings) {
    if (!IsActive(settings)) {
        return true;
    }
    settings["operationType"] = (uint8_t) OPERATION_TYPE_MSS;
	if ((string)settings["ismType"] == "")
		settings["ismType"] = "ismc";
    SaveConfig(settings, OPERATION_TYPE_MSS);
    SetStreamStatus(settings, SS_CONNECTING, 0);
    string targetFolder = settings["targetFolder"];
    string chunkBaseName = settings["chunkBaseName"];
    string localStreamName = settings["localStreamName"];
    _pushSetup[localStreamName][targetFolder][chunkBaseName] = settings;
    //1. Get the streams manager
    StreamsManager *pSM = GetStreamsManager();

    //2. Find the in stream
    map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
            ST_IN_NET,
            localStreamName,
            true,
            false);

    BaseInStream *pStream = NULL;

    FOR_MAP(streams, uint32_t, BaseStream *, i) {
        if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_MSS)) {
            pStream = (BaseInStream *) MAP_VAL(i);
            break;
        }
    }
    if (pStream == NULL) {
        WARN("Input stream %s not available yet", STR(localStreamName));
        SetStreamStatus(settings, SS_WAITFORSTREAM, 0);
        return true;
    }

    //3. Create the MSS stream
    OutFileMSSStream *pOutMSS = OutFileMSSStream::GetInstance(
            this,
            localStreamName,
            settings);
    if (pOutMSS == NULL) {
        FATAL("Unable to create MSS stream");
        return false;
    }

    //4. Link them together
    if (!pStream->Link(pOutMSS)) {
        pOutMSS->EnqueueForDelete();
        FATAL("Source stream %s not compatible with MSS streaming", STR(localStreamName));
        return false;
    }

    RemoveFromPushSetup(settings);

    //5.
    return true;
}

void OriginApplication::RemoveMSSGroup(string groupName, bool deleteMSSFiles) {
    vector<uint32_t> configs;
    string groupPath = "";

    FOR_MAP(_configurations["mss"], string, Variant, i) {
        if (MAP_VAL(i)["groupName"] != groupName)
            continue;
        groupPath = (string) MAP_VAL(i)["groupTargetFolder"];
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_MSS;
        ADD_VECTOR_END(configs, (uint32_t) MAP_VAL(i)["configId"]);
    }

    // cleanup the MSS session variables
    map<string, MicrosoftSmoothStream::SessionInfo>::iterator j;
    bool manifestDeleted = false;
    for (j = MicrosoftSmoothStream::OutFileMSSStream::_sessionInfo.begin(); j != MicrosoftSmoothStream::OutFileMSSStream::_sessionInfo.end();) {
        if (j->first.substr(0, groupName.length()) == groupName) {
            j->second.Reset();
            if (!manifestDeleted && NULL != j->second.pManifest) {
                delete j->second.pManifest;
                if (NULL != j->second.pPublishingPoint) {
                    delete j->second.pPublishingPoint;
                    j->second.pPublishingPoint = NULL;
                }
                j->second.pManifest = NULL;
                manifestDeleted = true;
            } else {
                j->second.pManifest = NULL;
                j->second.pPublishingPoint = NULL;
            }
            MicrosoftSmoothStream::OutFileMSSStream::_sessionInfo.erase(j++);           
        }
        else {
            ++j;
        }
    }

    MicrosoftSmoothStream::OutFileMSSStream::_prevGroupName = "";
    MicrosoftSmoothStream::OutFileMSSStream::_prevManifest.erase(groupName);
    if (MicrosoftSmoothStream::OutFileMSSStream::_prevPublishingPoint.size() > 0)
        MicrosoftSmoothStream::OutFileMSSStream::_prevPublishingPoint.erase(groupName);

    for (uint32_t i = 0; i < configs.size(); i++)
        RemoveConfigId(configs[i], deleteMSSFiles);

    if (deleteMSSFiles) {
        if (groupPath != "") {
            deleteFolder(groupPath, true);
        }
    }
}
#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH

bool OriginApplication::CreateDASHStream(Variant &settings) {
    Variant result;
    if (!ComputeDASHPaths(settings, result)) {
        FATAL("Unable to compute DASH stream paths");
        return false;
    }

    settings["configIds"] = Variant();
    settings["configIds"].IsArray(true);

    FOR_MAP(result, string, Variant, i) {
        /*
         * Check first if we have the scenario where both local stream names and
         * group names are equal to that of being requested
         */
        if (IsStreamAlreadyCreated(OPERATION_TYPE_DASH, MAP_VAL(i))) {
            FATAL("Requested stream (%s) with same group (%s) was already created!",
                    STR(MAP_VAL(i)["localStreamName"]), STR(MAP_VAL(i)["groupName"]));

            return false;
        }

        MAP_VAL(i).RemoveKey("configIds");
        if (!CreateDASHStreamPart(MAP_VAL(i))) {
            FATAL("Unable to create DASH stream");
            return false;
        }
        settings["configIds"].PushToArray(MAP_VAL(i)["configId"]);
    }

    return true;
}

bool OriginApplication::ComputeDASHPaths(Variant settings, Variant &result) {
    result.Reset();
    result.IsArray(true);
    Variant localStreamNames = settings["localStreamNames"];
    Variant bandwidths = settings["bandwidths"];
    Variant tempSettings = settings;
    tempSettings.RemoveKey("localStreamNames");
    tempSettings.RemoveKey("bandwidths");
    string baseTargetFolder = tempSettings["targetFolder"];
    if (baseTargetFolder[baseTargetFolder.size() - 1] != PATH_SEPARATOR)
        baseTargetFolder += PATH_SEPARATOR;
    string groupTargetFolder = baseTargetFolder + (string) tempSettings["groupName"]
            + PATH_SEPARATOR;

    for (uint32_t i = 0; i < localStreamNames.MapSize(); i++) {
        //fix the parameters inside _localStreamName
        string normalizedStreamName = (string) localStreamNames[(uint32_t) i];
        URI temp;
        if (!URI::FromString("http://localhost/" + normalizedStreamName, false, temp)) {
            FATAL("Unable to fix localStreamname: %s", STR(normalizedStreamName));
            return false;
        }
        normalizedStreamName = temp.document();
        Variant parameters = temp.parameters();

        FOR_MAP(parameters, string, Variant, j) {
            normalizedStreamName += "_" + MAP_KEY(j);
            normalizedStreamName += "_" + (string) MAP_VAL(j);
        }

        tempSettings["localStreamName"] = (string) localStreamNames[(uint32_t) i];
        tempSettings["bandwidth"] = (uint32_t) bandwidths[(uint32_t) i];
        tempSettings["groupTargetFolder"] = groupTargetFolder;
        tempSettings["targetFolder"] = groupTargetFolder + normalizedStreamName;
        result.PushToArray(tempSettings);
    }

    return true;
}

bool OriginApplication::CreateDASHStreamPart(Variant &settings) {
    if (!IsActive(settings)) {
        return true;
    }
    settings["operationType"] = (uint8_t) OPERATION_TYPE_DASH;
    SaveConfig(settings, OPERATION_TYPE_DASH);
    SetStreamStatus(settings, SS_CONNECTING, 0);
    string targetFolder = settings["targetFolder"];
    string chunkBaseName = settings["chunkBaseName"];
    string localStreamName = settings["localStreamName"];
    _pushSetup[localStreamName][targetFolder][chunkBaseName] = settings;
    //1. Get the streams manager
    StreamsManager *pSM = GetStreamsManager();

    //2. Find the in stream
    map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
            ST_IN_NET,
            localStreamName,
            true,
            false);

    BaseInStream *pStream = NULL;

    FOR_MAP(streams, uint32_t, BaseStream *, i) {
        if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_DASH)) {
            pStream = (BaseInStream *) MAP_VAL(i);
            break;
        }
    }
    if (pStream == NULL) {
        WARN("Input stream %s not available yet", STR(localStreamName));
        SetStreamStatus(settings, SS_WAITFORSTREAM, 0);
        return true;
    }

    //3. Create the DASH stream
    OutFileDASHStream *pOutDASH = OutFileDASHStream::GetInstance(
            this,
            localStreamName,
            settings);
    if (pOutDASH == NULL) {
        FATAL("Unable to create DASH stream");
        return false;
    }

    //4. Set the application
    pOutDASH->SetApplication(this);

    //5. Link them together
    if (!pStream->Link(pOutDASH)) {
        pOutDASH->EnqueueForDelete();
        FATAL("Source stream %s not compatible with DASH streaming", STR(localStreamName));
        return false;
    }

    RemoveFromPushSetup(settings);

    //6.
    return true;
}

void OriginApplication::RemoveDASHGroup(string groupName, bool deleteDASHFiles) {
    vector<uint32_t> configs;
    string groupPath = "";

    FOR_MAP(_configurations["dash"], string, Variant, i) {
        if (MAP_VAL(i)["groupName"] != groupName)
            continue;
        groupPath = (string) MAP_VAL(i)["groupTargetFolder"];
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_DASH;
        ADD_VECTOR_END(configs, (uint32_t) MAP_VAL(i)["configId"]);
    }

    // cleanup the DASH session variables
    map<string, MpegDash::SessionInfo>::iterator j;
    bool manifestDeleted = false;
    for (j = MpegDash::OutFileDASHStream::_sessionInfo.begin(); j != MpegDash::OutFileDASHStream::_sessionInfo.end();) {
        if (j->first.substr(0, groupName.length()) == groupName ) {
            j->second.Reset();
            if (!manifestDeleted && NULL != j->second.pManifest) {
                delete j->second.pManifest;
                j->second.pManifest = NULL;
                manifestDeleted = true;
            } else {
                j->second.pManifest = NULL;
            }
            MpegDash::OutFileDASHStream::_sessionInfo.erase(j++);
        } else {
            ++j;
        }
    }

    MpegDash::OutFileDASHStream::_prevGroupName = "";
    MpegDash::OutFileDASHStream::_prevManifest.erase(groupName);

    for (uint32_t i = 0; i < configs.size(); i++)
        RemoveConfigId(configs[i], deleteDASHFiles);

    if (deleteDASHFiles) {
        if (groupPath != "") {
            deleteFolder(groupPath, true);
        }
    }
}
#endif /* HAS_PROTOCOL_DASH */

void OriginApplication::RemoveProcessGroup(string groupName) {
    vector<uint32_t> configs;

    FOR_MAP(_configurations["process"], string, Variant, i) {
        if (MAP_VAL(i)["groupName"] != groupName)
            continue;
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_LAUNCHPROCESS;
        ADD_VECTOR_END(configs, (uint32_t) MAP_VAL(i)["configId"]);
    }

    for (uint32_t i = 0; i < configs.size(); i++)
		RemoveConfigId(configs[i], false);
}

void OriginApplication::RemoveConfig(Variant streamConfig,
        OperationType operationType, bool deleteHlsHdsFiles) {
    switch (operationType) {
        case OPERATION_TYPE_PULL:
        {
            _configurations["pull"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            break;
        }
        case OPERATION_TYPE_RECORD:
        {
            _configurations["record"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            break;
        }
        case OPERATION_TYPE_PUSH:
        {
            _configurations["push"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            break;
        }
        case OPERATION_TYPE_LAUNCHPROCESS:
        {
            ProcessWatchdogTimerProtocol * pWatchDog = NULL;
            if ((_processWatchdogTimerProtocolId != 0)
                    && ((pWatchDog =
                    (ProcessWatchdogTimerProtocol *) ProtocolManager::GetProtocol(
                    _processWatchdogTimerProtocolId)) != NULL))
                pWatchDog->UnWatch(streamConfig);
            _configurations["process"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            break;
        }
#ifdef HAS_PROTOCOL_HLS
        case OPERATION_TYPE_HLS:
        {
            _configurations["hls"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            HLSPlaylist::RemoveFromMasterPlaylist(streamConfig);
            if (deleteHlsHdsFiles) {
                deleteFolder(streamConfig["targetFolder"], true);
            }
            break;
        }
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
        case OPERATION_TYPE_HDS:
        {
            _configurations["hds"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            HDSManifest::RemoveMediaAndSave(streamConfig);
            if (deleteHlsHdsFiles) {
                deleteFolder(streamConfig["targetFolder"], true);
            }
            break;
        }
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
        case OPERATION_TYPE_MSS:
        {
            _configurations["mss"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            break;
        }
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        case OPERATION_TYPE_DASH:
        {
            _configurations["dash"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            if (deleteHlsHdsFiles) {
                deleteFolder(streamConfig["targetFolder"], true);
            }
            break;
        }
#endif /* HAS_PROTOCOL_DASH */
        case OPERATION_TYPE_WEBRTC:
        {
            _configurations["webrtc"].RemoveAt(streamConfig["configId"]);
            streamConfig["keepAlive"] = (bool)false;
            RemoveFromPushSetup(streamConfig);
            break;
        }
#ifdef HAS_PROTOCOL_METADATA
		case OPERATION_TYPE_METADATA:
		{
			_configurations["metalistener"].RemoveAt(streamConfig["configId"]);
			streamConfig["keepAlive"] = (bool)false;
			map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();
			IOHandler* pHandler = (IOHandler*)services[streamConfig["acceptor"]["id"]];
			((TCPAcceptor*)pHandler)->Enable(false);
			IOHandlerManager::EnqueueForDelete(pHandler);
		}
#endif /* HAS_PROTOCOL_METADATA */
        default:
        {
            break;
        }
    }
    if (!SerializePullPushConfig(_configurations))
        WARN("Unable to serialize pushPull setup");
}

void OriginApplication::RemoveConfigId(uint32_t configId, bool deleteHLSFiles) {
    OperationType operationType = OPERATION_TYPE_STANDARD;
    Variant config;
    string operationTypeString = "";
    if (_configurations["pull"].HasIndex(configId)) {
        config = _configurations["pull"][configId];
        operationType = OPERATION_TYPE_PULL;
        operationTypeString = "pull";
    } else if (_configurations["push"].HasIndex(configId)) {
        config = _configurations["push"][configId];
        operationType = OPERATION_TYPE_PUSH;
        operationTypeString = "push";
    } else if (_configurations["hls"].HasIndex(configId)) {
        config = _configurations["hls"][configId];
        operationType = OPERATION_TYPE_HLS;
        operationTypeString = "hls";
    } else if (_configurations["hds"].HasIndex(configId)) {
        config = _configurations["hds"][configId];
        operationType = OPERATION_TYPE_HDS;
        operationTypeString = "hds";
    } else if (_configurations["mss"].HasIndex(configId)) {
        config = _configurations["mss"][configId];
        operationType = OPERATION_TYPE_MSS;
        operationTypeString = "mss";
    } else if (_configurations["dash"].HasIndex(configId)) {
        config = _configurations["dash"][configId];
        operationType = OPERATION_TYPE_DASH;
        operationTypeString = "dash";
    } else if (_configurations["record"].HasIndex(configId)) {
        config = _configurations["record"][configId];
        operationType = OPERATION_TYPE_RECORD;
        operationTypeString = "record";
    } else if (_configurations["process"].HasIndex(configId)) {
        config = _configurations["process"][configId];
        operationType = OPERATION_TYPE_LAUNCHPROCESS;
        operationTypeString = "process";
    } else if (_configurations["webrtc"].HasIndex(configId)) {
        config = _configurations["webrtc"][configId];
        operationType = OPERATION_TYPE_WEBRTC;
        operationTypeString = "webrtc";
	} else if (_configurations["metalistener"].HasIndex(configId)) {
		config = _configurations["metalistener"][configId];
		operationType = OPERATION_TYPE_METADATA;
		operationTypeString = "metalistener";
	}

    // if one of the conditions is true, then configId is present and must be removed
    if (config == V_NULL && operationTypeString != "") {
        _configurations[operationTypeString].RemoveAt(configId);
        return;
    }

    RemoveConfig(config, operationType, deleteHLSFiles);
}

bool OriginApplication::GetConfig(Variant &config, uint32_t configId) {
    bool result = true;
    string operationTypeString = "";
    if (_configurations["pull"].HasIndex(configId)) {
        config = _configurations["pull"][configId];
        operationTypeString = "pull";
    } else if (_configurations["push"].HasIndex(configId)) {
        config = _configurations["push"][configId];
        operationTypeString = "push";
    } else if (_configurations["hls"].HasIndex(configId)) {
        config = _configurations["hls"][configId];
        operationTypeString = "hls";
    } else if (_configurations["hds"].HasIndex(configId)) {
        config = _configurations["hds"][configId];
        operationTypeString = "hds";
    } else if (_configurations["mss"].HasIndex(configId)) {
        config = _configurations["mss"][configId];
        operationTypeString = "mss";
    } else if (_configurations["dash"].HasIndex(configId)) {
        config = _configurations["dash"][configId];
        operationTypeString = "dash";
    } else if (_configurations["record"].HasIndex(configId)) {
        config = _configurations["record"][configId];
        operationTypeString = "record";
    } else if (_configurations["process"].HasIndex(configId)) {
        config = _configurations["process"][configId];
        operationTypeString = "process";
	} else if (_configurations["metalistener"].HasIndex(configId)) {
		config = _configurations["metalistener"][configId];
		operationTypeString = "metalistener";
    } else {
        result = false;
    }
    return result;
}

Variant OriginApplication::GetStreamsConfig() {
    return _configurations;
}

void OriginApplication::SignalLinkedStreams(BaseInStream *pInStream, BaseOutStream *pOutStream) {

	Variant &outStreamConfig = pOutStream->GetProtocol()->GetCustomParameters();
	Variant &inStreamConfig = pInStream->GetProtocol()->GetCustomParameters();

//	DEBUG("[DEBUG] Linked %s(%"PRIu32") --> %s(%"PRIu32")", 
//		STR(tagToString(pInStream->GetType())), pInStream->GetUniqueId(), STR(tagToString(pOutStream->GetType())), pOutStream->GetUniqueId());

	if (pInStream->GetProtocol()->GetType() == PT_RTSP) { // this condition should be updated if vod type streams are not just from rtsp anymore
		outStreamConfig["_metadata"] = ((RTSPProtocol *)pInStream->GetProtocol())->GetCompleteSDP();
		outStreamConfig["_metadata2"] = ((RTSPProtocol *)pInStream->GetProtocol())->GetPlayResponse();
		outStreamConfig["_inboundVod"] = (bool)(inStreamConfig["sourceType"] == "vod");
		pOutStream->SetInboundVod((bool)outStreamConfig["_inboundVod"]);
		((BaseOutNetStream*)pOutStream)->SetKeyFrameConsumed(false);
//		DEBUG("[DEBUG] setting outbound meta to inbound sdp");
	}
	//must pass _origId no matter what
	outStreamConfig["_origId"] = (uint32_t)(pInStream->GetUniqueId());

	// if instream has a pending meta to pass to outstream, pass it here.
	if (inStreamConfig.HasKeyChain(V_MAP, false, 1, "_metadata")) {
		outStreamConfig["_metadata"] = inStreamConfig["_metadata"];
//		inStreamConfig.RemoveKey("_metadata");
//		DEBUG("[DEBUG] setting outbound meta to inbound metadata");
	}
	// if instream has a pending 2nd meta to pass to outstream, pass it here.
	if (inStreamConfig.HasKeyChain(V_MAP, false, 1, "_metadata2")) {
		outStreamConfig["_metadata2"] = inStreamConfig["_metadata2"];
	}
	if (inStreamConfig.HasKeyChain(V_BOOL, false, 1, "_inboundVod")) {
		outStreamConfig["_inboundVod"] = inStreamConfig["_inboundVod"];
		pOutStream->SetInboundVod((bool)outStreamConfig["_inboundVod"]);
		((BaseOutNetStream*)pOutStream)->SetKeyFrameConsumed(false);
	}
	if (inStreamConfig.HasKeyChain(_V_NUMERIC, false, 1, "_origId")) {
		outStreamConfig["_origId"] = (uint32_t) inStreamConfig["_origId"];
	}

	StreamsManager *pSM = GetStreamsManager();
	BaseInStream *pLoopbackInStream = NULL;
	if (pSM != NULL) {
		// check if outstream is a "self-pull / self-playback"
		if (outStreamConfig.HasKeyChain(_V_NUMERIC, false, 1, "loopbackId")) {
			uint32_t loopbackInStreamId = (uint32_t) outStreamConfig.GetValue("loopbackId", false);
			pLoopbackInStream = (BaseInStream *)pSM->FindByUniqueId(loopbackInStreamId);
		} else {
			Variant outStats;
			pOutStream->GetStats(outStats);
			if (outStats.HasKeyChain(V_STRING, false, 1, "ip") && ((string)outStats.GetValue("ip", false) == "127.0.0.1")) {
				uint16_t farPort = (uint16_t) outStats.GetValue("farPort", false);
				uint64_t loopbackStreamType = 0;
				switch (pOutStream->GetType()) {
				case ST_OUT_NET_RTMP:
					loopbackStreamType = ST_IN_NET_RTMP;
					break;
				}
				if (loopbackStreamType != 0) {
					map<uint32_t, BaseStream*> streams = pSM->FindByType(loopbackStreamType);
					FOR_MAP(streams, uint32_t, BaseStream *, i) {
						Variant inStats;
						MAP_VAL(i)->GetStats(inStats);
						if ((uint16_t)inStats.GetValue("nearPort", false) == farPort) {
							pLoopbackInStream = (BaseInStream *)MAP_VAL(i);
							break;
						}
					}
					if (pLoopbackInStream != NULL) {
//						DEBUG("[DEBUG] loopback inbound found: %"PRIu32, pLoopbackInStream->GetUniqueId());
						// Save loopback inbound stream id on loopback source outbound to save computing time
						outStreamConfig["loopbackId"] = pLoopbackInStream->GetUniqueId();
					}
					/*
					else {
						DEBUG("[DEBUG] loopback inbound not found");
					}
					*/
				}
			}
		}
		if (pLoopbackInStream != NULL) {
			// Get linked streams
			vector<BaseOutStream *> loopbackOutStreams = pLoopbackInStream->GetOutStreams();
			if (loopbackOutStreams.size() > 0) {
				FOR_VECTOR(loopbackOutStreams, i) {
					// pass metadata to linked streams
					loopbackOutStreams[i]->GetProtocol()->GetCustomParameters()["_metadata"] = outStreamConfig["_metadata"];
					loopbackOutStreams[i]->GetProtocol()->GetCustomParameters()["_metadata2"] = outStreamConfig["_metadata2"];
					// pass inboundvod flag to linked streams
					loopbackOutStreams[i]->GetProtocol()->GetCustomParameters()["_inboundVod"] = outStreamConfig["_inboundVod"];
					loopbackOutStreams[i]->GetProtocol()->GetCustomParameters()["_origId"] = outStreamConfig["_origId"];
					loopbackOutStreams[i]->SetInboundVod((bool)outStreamConfig["_inboundVod"]);
					((BaseOutNetStream*)pOutStream)->SetKeyFrameConsumed(false);
				}
			}
			else {
				// if there are no linked streams, store metadata to be triggered by this functions
				pLoopbackInStream->GetProtocol()->GetCustomParameters()["_metadata"] = outStreamConfig["_metadata"];
				pLoopbackInStream->GetProtocol()->GetCustomParameters()["_metadata2"] = outStreamConfig["_metadata2"];
				// if there are no linked streams, just store the inbound vod flag
				// in the loopback stream then let the loopback do the passing to outstream config
				pLoopbackInStream->GetProtocol()->GetCustomParameters()["_inboundVod"] = outStreamConfig["_inboundVod"];
				// if there are no linked streams, just store the id of the origin stream
				// in the loopback stream then let the loopback do the passing to outstream config
				pLoopbackInStream->GetProtocol()->GetCustomParameters()["_origId"] = outStreamConfig["_origId"];
			}
			outStreamConfig.RemoveKey("_metadata");
			outStreamConfig.RemoveKey("_metadata2");
		}
	}
}

void OriginApplication::SignalUnLinkingStreams(BaseInStream *pStream,
        BaseOutStream *pOutStream) {
    if ((pOutStream == NULL) || (pOutStream->GetProtocol() == NULL))
        return;
    Variant streamConfig;
    OperationType operationType = GetOperationType(pOutStream->GetProtocol(), streamConfig);
    if ((operationType == OPERATION_TYPE_PUSH)
            || (operationType == OPERATION_TYPE_HLS)
            || (operationType == OPERATION_TYPE_HDS)
            || (operationType == OPERATION_TYPE_MSS)
            || (operationType == OPERATION_TYPE_DASH)
            || (operationType == OPERATION_TYPE_RECORD)) {
        pOutStream->EnqueueForDelete();
        RemoveFromConfigStatus(operationType, streamConfig);
    }
}

void OriginApplication::SignalStreamRegistered(BaseStream *pStream, bool registerStreamExpiry) {
	//1. Do the normal routine
	BaseClientApplication::SignalStreamRegistered(pStream, registerStreamExpiry);

#ifdef HAS_RTSP_LAZYPULL
	Variant customParams = pStream->GetProtocol()->GetCustomParameters();
	Variant params;
	if (customParams.HasKeyChain(V_MAP, false, 2, "customParameters", "externalStreamConfig")) {
		params = customParams.GetValue("customParameters", false).GetValue("externalStreamConfig", false);
	} else {
		params = pStream->GetProtocol()->GetCustomParameters();
	}
    if (params.HasKeyChain(V_MAP, false, 1, "_callback")) {
        Variant &callback = params["_callback"];
		if (callback.HasKeyChain(_V_NUMERIC, false, 1, "protocolID")) {
			uint32_t protocolID = callback["protocolID"];
			BaseProtocol *pProtocol = ProtocolManager::GetProtocol(protocolID);
			if (pProtocol != NULL) {
				uint64_t protocolType = pProtocol->GetType();
				switch (protocolType) {
					case PT_RTSP:
					{
						_pRTSPHandler->UpdatePendingStream(protocolID, pStream);
						break;
					}
					case PT_WRTC_CNX:
					{
						Variant data;
						data["streamID"] = pStream->GetUniqueId();
						data["streamName"] = callback["streamName"];
						pProtocol->SignalEvent("updatePendingStream", data);
						break;
					}
					case PT_INBOUND_RTMP:
					{
						if (callback.HasKeyChain(_V_NUMERIC, false, 1, "rtmpStreamID")) {
							uint32_t rtmpStreamId = (uint32_t) callback.GetValue("rtmpStreamID", false);
							BaseRTMPProtocol *pRtmpProtocol = (BaseRTMPProtocol *)pProtocol;
							BaseStream* pRtmpStream = pRtmpProtocol->GetRTMPStream(rtmpStreamId);
							if (pRtmpStream == NULL) {
								if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
									BaseInStream *pInStream = (BaseInStream *)pStream;
									if (pInStream->IsUnlinkedVod()) pInStream->EnqueueForDelete();
								}
							}
						}
					}
				}
			}
			else {
				if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
					BaseInStream *pInStream = (BaseInStream *)pStream;
					if (pInStream->IsUnlinkedVod()) pInStream->EnqueueForDelete();
				}
			}
		}
    }
#endif /* HAS_RTSP_LAZYPULL */
    //2. Set the status on the pushPullConfiguration
    if (pStream->GetProtocol() != NULL) {
        Variant streamConfig;
        OperationType operationType = GetOperationType(
                pStream->GetProtocol()->GetNearEndpoint(), streamConfig);

        //3. If this is a push/pull connection, change the status
        if (operationType != OPERATION_TYPE_STANDARD) {
            SetStreamStatus(streamConfig, SS_STREAMING, pStream->GetUniqueId());
        }
    }

    //2. Is this an inbound stream of type RTMP,RTP,LIVEFLV,TS or PASSTHROUGH?
    uint64_t streamType = pStream->GetType();
    string stream_name = pStream->GetName();

    if (TAG_KIND_OF(streamType, ST_IN_NET)) {
        if (streamType != ST_IN_NET_EXT) {
            if (registerStreamExpiry) {
                INFO("Registering stream expiry for `%s` stream", STR(stream_name));
                RegisterExpireStream(pStream->GetUniqueId());
            }
            else {
                WARN("Avoid registering stream expiry for `%s` stream", STR(stream_name));
            }
        }
    }

    if ((streamType != ST_IN_NET_LIVEFLV)
            && (streamType != ST_IN_NET_RTMP)
            && (streamType != ST_IN_NET_RTP)
            && (streamType != ST_IN_NET_TS)
            && (streamType != ST_IN_NET_PASSTHROUGH)
            && (streamType != ST_IN_NET_EXT)) {
        return;
    }

    _pVariantHandler->SignalInpuStreamAvailable(pStream->GetName());

    if (streamType != ST_IN_NET_PASSTHROUGH) {
        ProcessAutoHLS(pStream->GetName());
        ProcessAutoHDS(pStream->GetName());
#ifdef HAS_PROTOCOL_MSS
        ProcessAutoMSS(pStream->GetName());
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        ProcessAutoDASH(pStream->GetName());
#endif /* HAS_PROTOCOL_DASH */
    }

    //3. Get the stream name and compare it with the list of stream names
    //we have to push forward
    if (_pushSetup != V_MAP)
        return;
    string streamName = pStream->GetName();
    if (!_pushSetup.HasKey(streamName)) {
        //WARN("No push configuration for localStreamName `%s`", STR(streamName));
        return;
    }

    //4. Start pushing routines
    if (_pushSetup.HasKey(streamName)) {

        //5. We need to create a copy of the variant we want to cycle on
        //because inside the for loop we have function calls that may alter
        //the content of the thing we cycle on. When this happens, our
        //iterator is invalidated leading to memory corruption. To be more specific,
        //RecordStream() can alter _pushSetup when executed while we cycle over it
        Variant copy = _pushSetup[streamName];

        FOR_MAP(copy, string, Variant, i) {

            FOR_MAP(MAP_VAL(i), string, Variant, j) {
                OperationType operationType = (OperationType) ((uint8_t) MAP_VAL(j)["operationType"]);
                if (operationType == OPERATION_TYPE_RECORD)
                    RecordStream(MAP_VAL(j));
                else
                    EnqueueTimedOperation(MAP_VAL(j), operationType);
            }
        }
    }
}

bool OriginApplication::OutboundConnectionFailed(Variant &customParameters) {
    //1. get the connection type (push/pull) and the stream config
    Variant streamConfig;
    OperationType operationType = GetOperationType(customParameters, streamConfig);

    //2. If this is a push/pull connection and keepAlive is set, enqueue the operation again
    if (operationType != OPERATION_TYPE_STANDARD) {
        SetStreamStatus(streamConfig, SS_ERROR_CONNECTION_FAILED, 0);
		// Check if keepAlive is set or this is an active lazy pull
		if (operationType == OPERATION_TYPE_PULL) {
			EventLogger *eventLogger = GetEventLogger();
			if (eventLogger != NULL) {
				eventLogger->LogPullStreamFailed(streamConfig);
			}
		}
        if ((bool)streamConfig["keepAlive"] || 
				(streamConfig.HasKey("_isLazyPull") && 
					IsLazyPullActive(streamConfig["localStreamName"]))) {
            EnqueueTimedOperation(streamConfig, operationType);
        }
    }
#ifdef HAS_RTSP_LAZYPULL
    if (customParameters.HasKeyChain(_V_NUMERIC, false, 2, "_callback", "protocolID")) {
        uint32_t protocolID = (uint32_t) customParameters.GetValue("_callback", false).GetValue("protocolID", false);
        BaseProtocol *pProto = ProtocolManager::GetProtocol(protocolID);
		if (pProto != NULL)
			pProto->EnqueueForDelete();
    }
#endif
	if (customParameters.HasKeyChain(V_STRING, false, 1, "_isVod") && 
			(string) customParameters.GetValue("_isVod", false) == "1") {
		RemoveConfig(customParameters, operationType, false);
	}
	if (operationType == OPERATION_TYPE_PULL) {
		if ((bool)streamConfig["keepAlive"] == false) {
			RemoveFromConfigStatus(operationType, streamConfig);
		}
	}
    return true;
}

void OriginApplication::SetStreamStatus(Variant &config, StreamStatus status,
        uint64_t uniqueStreamId) {
    //1. Get the config ID
    if ((config != V_MAP)
            || (!config.HasKey("configId"))
            || (config["configId"] != _V_NUMERIC)
            ) {
		if (!config.HasKey("serializeToConfig") ||
			((bool)config["serializeToConfig"] == true)) {
			WARN("Unable to set pull/push status on config\n%s", STR(config.ToString()));
		}
        return;
    }
    uint32_t configId = config["configId"];

    //2. Check if our local copy of states has any pull/push/hls/hds/mss/dash/record/process section
    if ((!_configurations.HasKey("pull"))
            && (!_configurations.HasKey(_configurations, "push"))
            && (!_configurations.HasKey(_configurations, "hls"))
            && (!_configurations.HasKey(_configurations, "hds"))
            && (!_configurations.HasKey(_configurations, "mss"))
            && (!_configurations.HasKey(_configurations, "dash"))
            && (!_configurations.HasKey(_configurations, "record"))
            && (!_configurations.HasKey(_configurations, "process"))
			&& (!_configurations.HasKey(_configurations, "metalistener"))
            ) {
		WARN("Unable to set pull/push/hls/hds/mss/dash/record/process/metalistener status");
        return;
    }

    //3. Get the key which is either pull, push or HLS
    string key;
    if (_configurations["pull"][configId] == V_NULL) {
        //not pull
        _configurations["pull"].RemoveAt(configId);
        if (_configurations["push"][configId] == V_NULL) {
            //not push
            _configurations["push"].RemoveAt(configId);
            if (_configurations["hls"][configId] == V_NULL) {
                //not hls
                _configurations["hls"].RemoveAt(configId);
                if (_configurations["hds"][configId] == V_NULL) {
                    // not hds
                    _configurations["hds"].RemoveAt(configId);
                    if (_configurations["mss"][configId] == V_NULL) {
                        // not mss
                        _configurations["mss"].RemoveAt(configId);
                        if (_configurations["dash"][configId] == V_NULL) {
                            // not dash
                            _configurations["dash"].RemoveAt(configId);
                            if (_configurations["record"][configId] == V_NULL) {
                                // not record
                                _configurations["record"].RemoveAt(configId);
                                if (_configurations["process"][configId] == V_NULL) {
                                    // not process
                                    _configurations["process"].RemoveAt(configId);
									if (_configurations["metalistener"][configId] == V_NULL) {
										// not metadata listener
										_configurations["metalistener"].RemoveAt(configId);
										WARN("Unable to set pull/push/hls/hds/mss/dash/record/process status");
										return;
									} else {
										// metalistener
										key = "metalistener";
									}
                                } else {
                                    // process
                                    key = "process";
                                }
                            } else {
                                // record
                                key = "record";
                            }
                        } else {
                            //dash
                            key = "dash";
                        }
                    } else {
                        //mss
                        key = "mss";
                    }
                } else {
                    //hds
                    key = "hds";
                }
            } else {
                //hls
                key = "hls";
            }
        } else {
            //push
            key = "push";
        }
    } else {
        //pull
        key = "pull";
    }
    _configurations[key][configId]["status"]["previous"] = _configurations[key][configId]["status"]["current"];
    _configurations[key][configId]["status"]["current"]["code"] = (uint8_t) status;
    _configurations[key][configId]["status"]["current"]["description"] = StreamStatusString(status);
    _configurations[key][configId]["status"]["current"]["timestamp"] = (uint64_t) time(NULL);
    _configurations[key][configId]["status"]["current"]["uniqueStreamId"] = (uint64_t) uniqueStreamId;
}

BaseAppProtocolHandler *OriginApplication::GetProtocolHandler(string &scheme) {
    BaseAppProtocolHandler *pResult = NULL;
    if ((scheme == SCHEME_MPEGTSUDP)
            || (scheme == SCHEME_MPEGTSTCP)
            || (scheme == SCHEME_DEEP_MPEGTSUDP)
            || (scheme == SCHEME_DEEP_MPEGTSTCP)) {
        pResult = BaseClientApplication::GetProtocolHandler(PT_PASSTHROUGH);
    }
#ifdef HAS_PROTOCOL_EXTERNAL
    else if ((pResult = ProtocolFactory::GetProtocolHandler(scheme)) != NULL) {
        return pResult;
    }
#endif /* HAS_PROTOCOL_EXTERNAL */
#ifdef HAS_V4L2
    else if(scheme == "dev") {
        return _pDevHandler;
    }
#endif /* HAS_V4L2 */

#ifdef HAS_PROTOCOL_NMEA
        else if(scheme == SCHEME_NMEA) {
        return _pNMEAHandler;
    }
#endif /* HAS_PROTOCOL_NMEA */
#ifdef HAS_PROTOCOL_METADATA
        else if(scheme == SCHEME_JSONMETA) {
            return _pJsonMetadataHandler;
        }
#endif /* HAS_PROTOCOL_METADATA */
    else {
        pResult = BaseClientApplication::GetProtocolHandler(scheme);
    }
    return pResult;
}

uint32_t OriginApplication::GetConnectionsCountLimit() {
    return _connectionsCountLimit;
}

int64_t OriginApplication::GetConnectionsCount() {
    int64_t connectionsCount = IOHandlerManager::GetStats(false).GetManagedTcp().Current()
            + IOHandlerManager::GetStats(false).GetManagedUdp().Current();

    map<uint64_t, EdgeServer> &edges = _pVariantHandler->GetEdges();

    FOR_MAP(edges, uint64_t, EdgeServer, e) {
#ifdef HAS_WEBSERVER
        if (MAP_VAL(e).IsWebServer())
            continue;
#endif
        connectionsCount += MAP_VAL(e).ConnectionsCount();
    }

    return connectionsCount;
}

void OriginApplication::SetConnectionsCountLimit(uint32_t connectionsCountLimit) {
    if (connectionsCountLimit == 0)
        connectionsCountLimit = _hardConnectionLimitCount;
    if (_hardConnectionLimitCount != 0)
        connectionsCountLimit =
            connectionsCountLimit < _hardConnectionLimitCount ?
            connectionsCountLimit : _hardConnectionLimitCount;
    _connectionsCountLimit = connectionsCountLimit;
    Variant temp = (uint32_t) _connectionsCountLimit;
    if (!temp.SerializeToXmlFile(_configuration["connectionsLimitPersistenceFile"])) {
        WARN("Unable to save connections limit persistence file");
    }
}

void OriginApplication::GetBandwidth(double &in, double &out, double &maxIn, double &maxOut) {
    map<uint64_t, EdgeServer> &edges = _pVariantHandler->GetEdges();
    in = 0;
    out = 0;
    FdStats &stats = IOHandlerManager::GetStats(true);
    in = stats.InSpeed();
    out = stats.OutSpeed();

    FOR_MAP(edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
        if (MAP_VAL(i).IsWebServer())
            continue;
#endif
        in += (double) MAP_VAL(i).IOStats()["grandTotal"]["inSpeed"];
        out += (double) MAP_VAL(i).IOStats()["grandTotal"]["outSpeed"];
    }
    maxIn = _inBandwidthLimit;
    maxOut = _outBandwidthLimit;
}

void OriginApplication::SetBandwidthLimit(double in, double out) {
    if (in <= 0)
        in = _hardLimitIn;
    if (out <= 0)
        out = _hardLimitOut;
    if (_hardLimitIn > 0)
        in = in < _hardLimitIn ? in : _hardLimitIn;
    if (_hardLimitOut > 0)
        out = out < _hardLimitOut ? out : _hardLimitOut;


    _inBandwidthLimit = in;
    _outBandwidthLimit = out;

    Variant temp;
    temp["in"] = (double) _inBandwidthLimit;
    temp["out"] = (double) _outBandwidthLimit;
    if (!temp.SerializeToXmlFile(_configuration["bandwidthLimitPersistenceFile"])) {
        WARN("Unable to save bandwidth limit persistence file");
    }
}

bool OriginApplication::StreamNameAvailable(string streamName,
        BaseProtocol *pProtocol, bool bypassIngestPoints) {

    if (bypassIngestPoints) {
        if (!GetStreamsManager()->StreamNameAvailable(streamName)) {
            return false;
        }
    } else {
        if (!BaseClientApplication::StreamNameAvailable(streamName, pProtocol,
                bypassIngestPoints))
            return false;
    }

    FOR_MAP(_configurations["pull"], string, Variant, i) {
        if (MAP_VAL(i)["localStreamName"] != streamName)
            continue;
        if (pProtocol == NULL)
            return false;

        Variant streamConfig;
        OperationType operationType = GetOperationType(pProtocol, streamConfig);

        if (operationType != OPERATION_TYPE_PULL)
            return false;

        if (streamConfig["configId"] != MAP_VAL(i)["configId"])
            return false;
    }

    return true;
}

bool OriginApplication::RecordStream(Variant &settings) {
    if (!IsActive(settings)) {
        return true;
    }
    //1. Prepare the settings
    settings["operationType"] = (uint8_t) OPERATION_TYPE_RECORD;
    uint64_t preset = (uint64_t) settings["preset"];
	bool dateFolderStructure = ((bool) settings["dateFolderStructure"]) || (preset == 1);
    bool overwrite = (bool)settings["overwrite"];
    uint32_t configId = 0;
    if (settings.HasKey("configId", true))
        configId = (uint32_t) settings["configId"];
    string pathToFile = settings["pathToFile"];
    string firstPathToFile = pathToFile;
    string type = settings["type"];
	string fileName = pathToFile;
	string fileExtension = type;

	// replace extension only when preset is not used (not WatchTower)
//	if (!dateFolderStructure) {
	string localStreamName = settings["localStreamName"];

	if (dateFolderStructure) { //see also "preset" in OutFileMP4::OpenMP4
//		type = "mp4";
		string recordFolder = pathToFile; //for WatchTower, pathToFile is a folder, not a file
		// Remove filename
		//size_t lastSlashPos = fileName.find_last_of(PATH_SEPARATOR);
		//string recordFolder = fileName;
		//if (lastSlashPos != string::npos) {
		//	recordFolder = fileName.substr(0, lastSlashPos);
		//}
		// Append date folder
		time_t now = time(NULL);
		Timestamp tsNow = *localtime(&now);
		char datePart[9];
		char timePart[7];
		strftime(datePart, 9, "%Y%m%d", &tsNow);
		strftime(timePart, 7, "%H%M%S", &tsNow);
		recordFolder += format("%c%s%c%s%c", PATH_SEPARATOR, STR(localStreamName),
				PATH_SEPARATOR, datePart, PATH_SEPARATOR);
		if (!fileExists(recordFolder)) {
			if (!createFolder(recordFolder, true)) {
				FATAL("Unable to create folder %s", STR(recordFolder));
				return false;
			}
		}
		// Append filename in streamname-date-time format
		pathToFile = recordFolder + format("%s-%s-%s.%s", STR(localStreamName),
				datePart, timePart, STR(type));
		// Split up again
		settings["pathToFile"] = recordFolder;
		splitFileName(pathToFile, fileName, fileExtension);

        if (_recordSession[localStreamName].IsFlagSet(type)) {
			return true;
        } 
        
	} else {
		size_t extensionIndex = fileName.rfind(".mp4");
		if (extensionIndex != std::string::npos) {
			fileExtension = ".mp4";
		}
		else {
			extensionIndex = fileName.rfind(".flv");
			if (extensionIndex != std::string::npos) {
				fileExtension = ".flv";
			}
			else {
				extensionIndex = fileName.rfind(".ts");
				if (extensionIndex != std::string::npos) {
					fileExtension = ".ts";
				}
				else {
					fileExtension = "";
				}
			}
		}

		uint32_t length = (uint32_t)fileName.length();
		if (length - (uint32_t)fileExtension.length() != (uint32_t)extensionIndex) {
			fileExtension = ""; //reinitializing fileExtension to empty just in case of fileExtension strings not found at the end of fileName
			fileName = fileName.substr(0, length - fileExtension.length());
			fileExtension = type;
			pathToFile = fileName + "." + type;
			WARN("Incompatible file extension detected:%s The file path will be changed to reflect the right name: %s",
				STR(settings["pathToFile"]), STR(pathToFile));
		}
		//remove . character from fileExtension
		fileExtension.erase(std::remove(fileExtension.begin(), fileExtension.end(), '.'), fileExtension.end());
		settings["pathToFile"] = pathToFile;
	}


	// Check if chunking enabled
    bool chunkingEnabled = (((uint32_t) settings["chunkLength"]) > 0) ? true : false;
    //2. compute computedPathToFile
    /*
     * compute computedPathToFile
     *		overwrite
     *			pathToFile active -> bail out
     *			pathToFile already scheduled with different configId -> bail out
     *			can't write on the pathToFile -> bail out
     *			#computedPathToFile=pathToFile#
     *		non-overwrite
     *			0. index too big -> bail out
     *			1. #computedPathToFile=file_index#
     *			2. computedPathToFile active -> increment index and goto 1
     *			3. computedPathToFile already scheduled with different configId -> increment index and goto 1
     *			4. can't write on the computedPathToFile -> increment index and goto 1
     *			5. done
     * save computedPathToFile in the settings
     */
	string computedPathToFile = "";
	string computedPathToFileWithPart = "";

	if (RecordedFileActive(pathToFile)) {
		FATAL("Record destination already active: %s", STR(pathToFile));
		return false;
    }
	if (configId != 0 && !overwrite) {
		if (RecordedFileScheduled(pathToFile, configId)) {
			FATAL("Record destination already scheduled: %s", STR(pathToFile));
			return false;
		}
	}

    if (overwrite) {
        if (!RecordedFileHasPermissions(pathToFile)) {
            FATAL("Can't record to %s due to permission errors or destination folder not found", STR(pathToFile));
            return false;
        }
        computedPathToFile = pathToFile;
    } else {
        uint32_t index = 0;
        while (true) {
            if (index > 8192) {
                FATAL("Recording failed because none of the %s_[1-8192].%s files were available due to missing folder or permission errors",
                        STR(fileName), STR(fileExtension));
                return false;
            }
            if (index == 0) {
                computedPathToFile = pathToFile;
                if (chunkingEnabled) {
                    if (preset == 1 || dateFolderStructure)
                        computedPathToFileWithPart = format("%s.%s",
                            STR(fileName), STR(fileExtension));
                    else
                        computedPathToFileWithPart = format("%s_part0000.%s",
                            STR(fileName), STR(fileExtension));
                }
            } else {
                computedPathToFile = format("%s_%04"PRIu32".%s", STR(fileName), index, STR(fileExtension));
                if (chunkingEnabled) {
                    computedPathToFileWithPart = format("%s_%04"PRIu32"_part0000.%s",
                            STR(fileName), index, STR(fileExtension));
                }
            }

            if ((!dateFolderStructure) && ((fileExists(computedPathToFile))
                    || (!RecordedFileHasPermissions(computedPathToFile))
                    || (chunkingEnabled
                    && ((fileExists(computedPathToFileWithPart))
                    || (!RecordedFileHasPermissions(computedPathToFileWithPart)))))) {
                index++;
                continue;
            }
            break;
        }
    }
    settings["computedPathToFile"] = computedPathToFile;

    //2. Get the streams manager
    StreamsManager *pSM = GetStreamsManager();

    //3. Find the in stream
    map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
            ST_IN_NET,
            localStreamName,
            true,
            false);

    BaseInStream *pStream = NULL;

    FOR_MAP(streams, uint32_t, BaseStream *, i) {
        if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_TS) ||
                MAP_VAL(i)->IsCompatibleWithType(ST_OUT_FILE_RTMP_FLV)) {
            pStream = (BaseInStream *) MAP_VAL(i);
            break;
        }
    }

    
    settings["pathToFile"] = firstPathToFile;

    //4. If no instream was found, we return
    if (pStream == NULL) {
        WARN("Input stream %s not available yet. Recording scheduled on %s", STR(localStreamName), STR(computedPathToFile));
        SaveConfig(settings, OPERATION_TYPE_RECORD);
        SetStreamStatus(settings, SS_WAITFORSTREAM, 0);
        _pushSetup[localStreamName][pathToFile]["recording"] = settings;
        return true;
    }

	//5. Create the outbound stream
    BaseOutStream *pOutStream = NULL;
    if (type == "ts") {
        //6. Delete the file if it exists
        if (fileExists(computedPathToFile)) {
            if (!deleteFile(computedPathToFile)) {
                FATAL("Unable to delete recorded file %s", STR(computedPathToFile));
                return false;
            }
        }

		SaveConfig(settings, OPERATION_TYPE_RECORD);

		pOutStream = OutFileTS::GetInstance(
				this,
				localStreamName,
				settings);

	} else if (type == "flv") {
        if (fileExists(computedPathToFile)) {
            if (!deleteFile(computedPathToFile)) {
                FATAL("Unable to delete recorded file %s", STR(computedPathToFile));
                return false;
            }
        }

        SaveConfig(settings, OPERATION_TYPE_RECORD);

        pOutStream = OutFileFLV::GetInstance(
                this,
                localStreamName,
                settings);
    } else if (type == "mp4") {
        //6. Delete the file if it exists
        if (fileExists(computedPathToFile)) {
            if (!deleteFile(computedPathToFile)) {
                FATAL("Unable to delete recorded file %s", STR(computedPathToFile));
                return false;
            }
        }
        
        SaveConfig(settings, OPERATION_TYPE_RECORD);
        
		// Add the mp4 writer binary
		settings["mp4BinPath"] = _configuration["mp4BinPath"];
		pOutStream = OutFileMP4::GetInstance(
				this,
				localStreamName,
				settings);

	} else if (type == "vmf") {
		// ToDo: Do I care about file existence?
		//
		// get the settings straight, OutVmfAppProtocolHandler will do more
		// figure VMF Port
		if (_configuration.HasKey("vmfPort")) {
			settings["vmfPort"] = _configuration["vmfPort"];
		} else {
			settings["vmfPort"] = VMF_DEFAULT_PORT;
		}
		settings["inStreamId"] = pStream->GetUniqueId();
		// Do I care about this either??
		SaveConfig(settings, OPERATION_TYPE_RECORD);
		// note: settings["pathToFile"]
		// fill in other stuff so ConnectCompletiion Callback can construct streams
		settings["name"] = GetName();
		settings["instreamPtr"] = (uint64_t)pStream;

		// ConnectToVmf will use TCPConnector to connect and stand up the protocol stack
		bool ok = _pOutVmfAppProtocolHandler->ConnectToVmf(localStreamName, settings);
		return ok;	// we really don't know!
		/*
		if (pOutStream == NULL) {
			FATAL("Can't create VMF chain, not recording metadata");
			return false;
		}
		*/
	} else {
		//13. Incorrect stream type
		FATAL("Invalid output stream type detected wile attempting to do recording: %s", STR(type));
		return false;
	}
        SaveConfig(settings, OPERATION_TYPE_RECORD);


    //14. Link them together
    if (!pStream->Link(pOutStream)) {
        FATAL("Source stream %s not compatible with %s recording", STR(localStreamName), STR(type));
        pOutStream->EnqueueForDelete();
        settings["keepAlive"] = (bool)false;
        RemoveFromConfigStatus(OPERATION_TYPE_RECORD, settings);
        RemoveFromPushSetup(settings);
        return false;
    }

    //15. update the stream status and save it to pullpushconfig.xml
    SetStreamStatus(settings, SS_STREAMING, pOutStream->GetUniqueId());

    //16. Remove it from scheduled pushes setup
    RemoveFromPushSetup(settings);

    _recordSession[localStreamName].SetFlagForType(type);

    //17. Done
    return true;
}

bool OriginApplication::SendWebRTCCommand(Variant & settings) {
	// THEN get a handle to the list of WRTC sessions
	// [WRTCAppProtocolHandler]<>--[WRTCSigProtocol]<>--[WRTCConnection]
	//                                                   < >
	//                                                    |
	//                                [OutNetFMP4Stream]--+
	// 
	// WRTCConnection::SendCommandData() IF the connected stream == targetstreamname

	return _pWrtcAppProtocolHandler->SendWebRTCCommand(settings);
}

bool OriginApplication::LaunchProcess(Variant &settings) {
    settings["operationType"] = (uint8_t) OPERATION_TYPE_LAUNCHPROCESS;
    SaveConfig(settings, OPERATION_TYPE_LAUNCHPROCESS);
    SetStreamStatus(settings, SS_CONNECTING, 0);

    if (_processWatchdogTimerProtocolId == 0) {
        RemoveConfig(settings, OPERATION_TYPE_LAUNCHPROCESS, false);
        return false;
    }

    // Get the timer protocol
    ProcessWatchdogTimerProtocol * pWatchDog =
            (ProcessWatchdogTimerProtocol *) ProtocolManager::GetProtocol(
            _processWatchdogTimerProtocolId);
    if (pWatchDog == NULL) {
        FATAL("Unable to get the timer protocol");
        RemoveConfig(settings, OPERATION_TYPE_LAUNCHPROCESS, false);
        return false;
    }

    if (!pWatchDog->Watch(settings)) {
        FATAL("Unable to start watching process");
        RemoveConfig(settings, OPERATION_TYPE_LAUNCHPROCESS, false);
        return false;
    }

    SetStreamStatus(settings, SS_STREAMING, 0);

    return true;
}

bool OriginApplication::SetCustomTimer(Variant &settings) {
    CustomTimer *pTimer = new CustomTimer();
    pTimer->SetApplication(this);
    if (!pTimer->Initialize(settings)) {
        pTimer->EnqueueForDelete();
        return false;
    }
    return true;
}

bool OriginApplication::CreateIngestPoint(string &privateStreamName, string &publicStreamName) {
    if (!BaseClientApplication::CreateIngestPoint(privateStreamName, publicStreamName))
        return false;
    if (!SerializeIngestPointsConfig()) {
        FATAL("Unable to serialize ingest points config");
        RemoveIngestPoint(privateStreamName, publicStreamName);
        return false;
    }

    _pVariantHandler->CreateIngestPoint(privateStreamName, publicStreamName);

    return true;
}

void OriginApplication::RemoveIngestPoint(string &privateStreamName, string &publicStreamName) {
    BaseClientApplication::RemoveIngestPoint(privateStreamName, publicStreamName);
    if (!SerializeIngestPointsConfig()) {
        WARN("Unable to serialize ingest points config");
    }
}

void OriginApplication::SignalApplicationMessage(Variant &message) {
    if ((!message.HasKeyChain(V_STRING, false, 1, "header"))
            || (!message.HasKey("payload", false)))
        return;
    string header = message["header"];
    Variant &payload = message.GetValue("payload", false);

    if (header == "limits") {
        int32_t connections = 0;
        double inBandwidth = 0.0;
        double outBandwidth = 0.0;

        if (payload.HasKeyChain(_V_NUMERIC, false, 1, "connections"))
            connections = (uint32_t) payload["connections"];

        if (payload.HasKeyChain(_V_NUMERIC, false, 1, "inBandwidth"))
            inBandwidth = (double) payload["inBandwidth"];

        if (payload.HasKeyChain(_V_NUMERIC, false, 1, "outBandwidth"))
            outBandwidth = (double) payload["outBandwidth"];

        SetHardLimits(connections, inBandwidth, outBandwidth);
    } else if (header == "prepareForShutdown") {
        ShutdownEdges();
    } else if (header == "pullStream") {
		_pCLIHandler->ProcessMessage(NULL, message["payload"]);
    } else if (header == "launchProcess") {
        LaunchProcess(message["payload"]);
    } else if (header == "codecUpdated") {
        uint32_t protocolID = payload["protocolID"];
        BaseProtocol *pProto = ProtocolManager::GetProtocol(protocolID);
		if (pProto != NULL) {
			switch (pProto->GetType())
			{
#ifdef HAS_RTSP_LAZYPULL
			case PT_RTSP:
			{
				BaseStream *pStream = GetStreamsManager()->FindByUniqueId(payload["streamID"]);
				_pRTSPHandler->UpdatePendingStream(protocolID, pStream);
				break;
			}
			case PT_WRTC_CNX:
			{
				pProto->SignalEvent("updatePendingStream", payload);
			}
#endif
			}
		}
    } else if (header == "removeConfig") {
        if (payload.HasKeyChain(_V_NUMERIC, false, 1, "configId")) {
            uint32_t configId = (uint32_t) payload.GetValue("configId", false);
            bool hlsFlag = (bool) payload.GetValue("deleteHlsFiles", false);
            RemoveConfigId(configId, hlsFlag);
		} else {
			if (payload.HasKeyChain(V_STRING, false, 1, "localStreamName")) {
				string streamName = (string)payload.GetValue("localStreamName", 
					false);
				FOR_MAP(_configurations["pull"], string, Variant, i) {
					string localStreamName = MAP_VAL(i)["localStreamName"];
					if (localStreamName != streamName) {
						continue;
					}
					string configId = (string)MAP_VAL(i)["configId"];
					message["payload"]["parameters"]["id"] = configId;
					_pCLIHandler->ProcessMessage(NULL, message["payload"]);
					break;
				}
			}
		}
    } else if (header == "statusReport") {
        _pStatsHandler->SignalEdgeStatusReport(payload);
	}
	else if (header == "restartService") {
		EnqueueTimedOperation(payload, OPERATION_TYPE_RESTARTSERVICE);
	} else if (header == "vodRequest") {
		ProcessVodRequest(payload);
	} else {
        WARN("Unhandled message: %s", STR(header));
    }
}

void OriginApplication::ProcessVodRequest(Variant &details) {
	if (!details.HasKeyChain(V_STRING, false, 1, "streamName") && !details.HasKeyChain(V_STRING, false, 1, "action")) {
		return;
	}
	string streamName = (string)details.GetValue("streamName", false);
	string action = (string)details.GetValue("action", false);

	StreamsManager *pSM = GetStreamsManager();
	if (pSM == NULL) {
		FATAL("Unable to get streams manager");
		return;
	}
	BaseInNetStream *pInNetStream = NULL;
	map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
		ST_IN_NET, streamName, true, false);

	FOR_MAP(streams, uint32_t, BaseStream*, i) {
		if (TAG_KIND_OF(MAP_VAL(i)->GetType(), ST_IN)) {
			pInNetStream = (BaseInNetStream *)MAP_VAL(i);
			break;
		}
	}
	
	if (pInNetStream == NULL) {
		WARN("Unable to find inbound stream `%s`", STR(streamName));
		return;
	}

	pInNetStream->SignalLazyPullSubscriptionAction(action);
}

void OriginApplication::SignalServerBootstrapped() {
    if ((!_isServer) || (_delayedProcesses != V_MAP)) {
        // Nothing to do here if this is not a server
        return;
    }

    FOR_MAP(_delayedProcesses, string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!LaunchProcess(MAP_VAL(i))) {
            WARN("process config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
}

string OriginApplication::GetStreamNameByAlias(const string &streamName, bool remove) {
    string result = BaseClientApplication::GetStreamNameByAlias(streamName, remove);
    if (remove && HasExpiredAlias(streamName))
        _pVariantHandler->RemoveStreamAlias(streamName);
    return result;
}

void OriginApplication::ShutdownEdges() {
    // Kill the launched processes first
    ProcessWatchdogTimerProtocol *pWatchDog =
            (ProcessWatchdogTimerProtocol *) ProtocolManager::GetProtocol(_processWatchdogTimerProtocolId);

    if (pWatchDog != NULL) {
        pWatchDog->Reset();
    } else {
        WARN("Unable to get the timer protocol");
    }

    // Now kill the edges
    _pVariantHandler->ShutdownEdges();
}

void OriginApplication::SetHardLimits(uint32_t connections,
        double inBandwidth, double outBandwidth) {
    if (connections > 0)
        _hardConnectionLimitCount = connections;
    if (inBandwidth > 0)
        _hardLimitIn = inBandwidth;
    if (outBandwidth > 0)
        _hardLimitOut = outBandwidth;
    SetConnectionsCountLimit(_hardConnectionLimitCount);
    SetBandwidthLimit(_hardLimitIn, _hardLimitOut);
}

bool OriginApplication::InitializeForServing() {
    if (!BaseClientApplication::Initialize()) {
        FATAL("Unable to initialize application");
        return false;
    }

    //1. Register protocol handlers
    _pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
    RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);
    RegisterAppProtocolHandler(PT_INBOUND_RTMPS_DISC, _pRTMPHandler);

    _pHTTPHandler = new HTTPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_HTTP_FOR_RTMP, _pHTTPHandler);
    RegisterAppProtocolHandler(PT_OUTBOUND_HTTP_FOR_RTMP, _pHTTPHandler);
    RegisterAppProtocolHandler(PT_HTTP_ADAPTOR, _pHTTPHandler);
    RegisterAppProtocolHandler(PT_OUTBOUND_HTTP2, _pHTTPHandler);
    RegisterAppProtocolHandler(PT_HTTP_MSS_LIVEINGEST, _pHTTPHandler);
	RegisterAppProtocolHandler(PT_HTTP_MEDIA_RECV, _pHTTPHandler);
	RegisterAppProtocolHandler(PT_INBOUND_HTTP_4_FMP4, _pHTTPHandler);
	
	_pCLIHandler = new CLIAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_JSONCLI, _pCLIHandler);
#ifdef HAS_PROTOCOL_ASCIICLI
    RegisterAppProtocolHandler(PT_INBOUND_ASCIICLI, _pCLIHandler);
#endif /* HAS_PROTOCOL_ASCIICLI */

    _pRTPHandler = new RTPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_RTP, _pRTPHandler);
    RegisterAppProtocolHandler(PT_RTCP, _pRTPHandler);

    _pRTSPHandler = new RTSPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_RTSP, _pRTSPHandler);

    _pSDPHandler = new SDPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_SDP, _pSDPHandler);

    _pPassThrough = new PassThroughAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_PASSTHROUGH, _pPassThrough);

    _pTimerHandler = new JobsTimerAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_TIMER, _pTimerHandler);

    _pTSHandler = new TSAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_TS, _pTSHandler);

    _pLiveFLV = new LiveFLVAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_LIVE_FLV, _pLiveFLV);

    _pVariantHandler = new OriginVariantAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_BIN_VAR, _pVariantHandler);

    _pRPCHandler = new BaseRPCAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_OUTBOUND_RPC, _pRPCHandler);
        
#ifdef HAS_V4L2
    _pDevHandler = new V4L2DeviceProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_V4L2, _pDevHandler);
#endif /* HAS_V4L2 */

#ifdef HAS_PROTOCOL_NMEA
    _pNMEAHandler = new NMEAAppProtocolHandler( _configuration );
    RegisterAppProtocolHandler(PT_INBOUND_NMEA, _pNMEAHandler);
#endif /* HAS_PROTOCOL_NMEA */

#ifdef HAS_PROTOCOL_METADATA
    _pJsonMetadataHandler = new JsonMetadataAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_JSONMETADATA, _pJsonMetadataHandler);
#endif /* HAS_PROTOCOL_METADATA */
    _pOutVmfAppProtocolHandler = new OutVmfAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_OUTBOUND_VMF, _pOutVmfAppProtocolHandler);

#ifdef HAS_PROTOCOL_WEBRTC
	// WebRTC Protocol Handler (wrtc)
	_pWrtcAppProtocolHandler = new WrtcAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_WRTC_SIG, _pWrtcAppProtocolHandler);
	RegisterAppProtocolHandler(PT_WRTC_CNX, _pWrtcAppProtocolHandler);
#endif

    for (size_t i = 0; i < _pRPCSinks.size(); i++)
        _pRPCSinks[i]->SetRpcProtocolHandler(_pRPCHandler);

#ifdef HAS_PROTOCOL_DRM
    if (_configuration.HasKey("drm", false)) {
        _pDRMHandler = new DRMAppProtocolHandler(_configuration);
        RegisterAppProtocolHandler(PT_DRM, _pDRMHandler);
    }
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_PROTOCOL_RAWMEDIA
	_pRawMediaHandler = new RawMediaProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_RAW_MEDIA, _pRawMediaHandler);
#endif /* HAS_PROTOCOL_RAWMEDIA */
    if (_configuration.HasKeyChain(V_MAP, false, 1, "statusReport")) {
        _pStatsHandler = new StatsVariantAppProtocolHandler(_configuration["statusReport"]);
        _pStatsHandler->SetApplication(this);
        RegisterAppProtocolHandler(PT_JSON_VAR, _pStatsHandler);
    }
#ifdef HAS_PROTOCOL_WS
	_pWS = new BaseWSAppProtocolHandler(_configuration);
	//$b2: ????
	RegisterAppProtocolHandler(PT_INBOUND_WS, _pWS);	//$b2:???
	RegisterAppProtocolHandler(PT_WS_METADATA, _pWS);	//$b2:???
	RegisterAppProtocolHandler(PT_OUTBOUND_WS, _pWS);	//$b2:???
#ifdef HAS_PROTOCOL_WS_FMP4
	RegisterAppProtocolHandler(PT_INBOUND_WS_FMP4, _pWS);
#endif /* HAS_PROTOCOL_WS_FMP4 */
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_API
	_pApiHandler = new ApiProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_API_INTEGRATION, _pApiHandler);
#endif /* HAS_PROTOCOL_API */

    //2. Initialize the keepAlive timer
    BaseTimerProtocol *pProtocol = new JobsTimerProtocol(false);
    _retryTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    pProtocol->EnqueueForTimeEvent(1);

    //2. Initialize the start webrtc timer
    pProtocol = new StartWebRTCTimerProtocol();
    _startwebrtcTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    pProtocol->EnqueueForTimeEvent(1);

    uint32_t streamsExpireTimer = 0;
    if (_configuration.HasKeyChain(_V_NUMERIC, true, 1, "streamsExpireTimer")) {
        streamsExpireTimer = (uint32_t) _configuration["streamsExpireTimer"];
    }
    if (streamsExpireTimer != 0) {
        pProtocol = new KeepAliveTimerProtocol();
        _keepAliveTimerProtocolId = pProtocol->GetId();
        pProtocol->SetApplication(this);
        pProtocol->EnqueueForTimeEvent(streamsExpireTimer);
    } else {
        _keepAliveTimerProtocolId = 0;
    }

    pProtocol = new ClusteringTimerProtocol();
    _clusteringTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    pProtocol->EnqueueForTimeEvent(10);

    pProtocol = new ProcessWatchdogTimerProtocol();
    _processWatchdogTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    pProtocol->EnqueueForTimeEvent(10);

#ifdef HAS_PROTOCOL_DRM
    pProtocol = new DRMTimerProtocol();
    _drmTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    uint32_t requestTimer = 1;
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "drm", "requestTimer")) {
        requestTimer = (uint32_t) (_configuration.GetValue("drm", false)
                .GetValue("requestTimer", false));
        if (requestTimer < 1) {
            requestTimer = 1;
        }
    }
    pProtocol->EnqueueForTimeEvent(requestTimer);
#endif /* HAS_PROTOCOL_DRM */

#ifdef HAS_PROTOCOL_EXTERNAL
    _pExternalAPI = new ExternalModuleAPI(this);

    if (!ProtocolFactory::FinishInitModules(_pExternalAPI, this)) {
        FATAL("Unable to finish modules initializations");
        return false;
    }
#endif /* HAS_PROTOCOL_EXTERNAL */

    //3. Persistence files
    if ((_configuration["pushPullPersistenceFile"] != V_STRING)
            || (_configuration["pushPullPersistenceFile"] == "")) {
        _configuration["pushPullPersistenceFile"] = "./pushPullSetup.xml";
    }
    if ((_configuration["authPersistenceFile"] != V_STRING)
            || (_configuration["authPersistenceFile"] == "")) {
        _configuration["authPersistenceFile"] = "./auth.xml";
    }
    if ((_configuration["connectionsLimitPersistenceFile"] != V_STRING)
            || (_configuration["connectionsLimitPersistenceFile"] == "")) {
        _configuration["connectionsLimitPersistenceFile"] = "./connlimits.xml";
    }
    if ((_configuration["bandwidthLimitPersistenceFile"] != V_STRING)
            || (_configuration["bandwidthLimitPersistenceFile"] == "")) {
        _configuration["bandwidthLimitPersistenceFile"] = "./bandwidthlimits.xml";
    }
    if ((_configuration["ingestPointsPersistenceFile"] != V_STRING)
            || (_configuration["ingestPointsPersistenceFile"] == "")) {
        _configuration["ingestPointsPersistenceFile"] = "./ingestpoints.xml";
    }

    if ((_configuration["videoconfigPersistenceFile"] != V_STRING)
            || (_configuration["videoconfigPersistenceFile"] == "")) {
        _configuration["videoconfigPersistenceFile"] = "./videoconfig.xml";
    }

    if ((_configuration["rfcvideoconfigPersistenceFile"] != V_STRING)
            || (_configuration["rfcvideoconfigPersistenceFile"] == "")) {
        _configuration["rfcvideoconfigPersistenceFile"] = _STREAM_CONFIG_FILE;
    }



    _hlsVersion = (_configuration.HasKeyChain(_V_NUMERIC, false, 1, "hlsVersion") 
            ? (uint32_t) _configuration["hlsVersion"] : 2);
    //4. Process initial ingest points persistence file
    if (!ProcessIngestPointsList()) {
        WARN("Unable to process ingest points list");
    }

    //5. Process initial pushPull list
    if (!ProcessPushPullList()) {
        WARN("Unable to process pushPull list");
    }

    //6. Test to see if we have authPersistenceFile
    if (!ProcessAuthPersistenceFile()) {
        WARN("Unable to process auth persistence file");
    }

    //7. Test to see if we have connectionsLimitPersistenceFile
#ifdef HAS_LICENSE
    _hardConnectionLimitCount = License::GetLicenseInstance()->GetConnectionLimit();
#endif /* HAS_LICENSE */
    if (_hardConnectionLimitCount != 0)
        SetConnectionsCountLimit(_hardConnectionLimitCount);

    if (!ProcessConnectionsLimitPersistenceFile()) {
        WARN("Unable to process connections limits persistence file");
    }

    //8. Enforce hard limits on the in/out bitrates
    if (_configuration.HasKeyChain(_V_NUMERIC, true, 2, "hardLimits", "in")) {
        _inBandwidthLimit = _hardLimitIn = (double) _configuration["hardLimits"]["in"];
    }
    if (_configuration.HasKeyChain(_V_NUMERIC, true, 2, "hardLimits", "out")) {
        _outBandwidthLimit = _hardLimitOut = (double) _configuration["hardLimits"]["out"];
    }

    //9. Read the persisted values for bandwidth limits
#ifdef HAS_LICENSE
    _hardLimitIn = License::GetLicenseInstance()->GetInboundBandwidthLimit();
    _hardLimitOut = License::GetLicenseInstance()->GetOutboundBandwidthLimit();
#endif /* HAS_LICENSE */
    if ((_hardLimitIn > 0) || (_hardLimitOut > 0))
        SetBandwidthLimit(_hardLimitIn, _hardLimitOut);

    if (!ProcessBandwidthLimitPersistenceFile()) {
        WARN("Unable to process bandwidth limits persistence file");
    }

    //10. Read templates
    ReadAutoHLSTemplate();
    ReadAutoHDSTemplate();
#ifdef HAS_PROTOCOL_MSS
    ReadAutoMSSTemplate();
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
    ReadAutoDASHTemplate();
#endif /* HAS_PROTOCOL_DASH */

    //11. Done
    return true;
}

bool OriginApplication::InitializeForMetaFileGenerator() {
    if (!BaseClientApplication::Initialize()) {
        FATAL("Unable to initialize application");
        return false;
    }

    if (GetStreamMetadataResolver() == NULL) {
        FATAL("No streams metadata resolver found");
        return false;
    }

    if ((_configuration["scanInterval"] != _V_NUMERIC)
            || ((int32_t) _configuration["scanInterval"] <= 0)) {
        _configuration["scanInterval"] = (int32_t) 1;
    }

    _pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
    RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);

    _pTimerHandler = new JobsTimerAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_TIMER, _pTimerHandler);

    //2. Initialize the keepAlive timer
    JobsTimerProtocol *pProtocol = new JobsTimerProtocol(true);
    _retryTimerProtocolId = pProtocol->GetId();
    pProtocol->SetApplication(this);
    pProtocol->EnqueueForTimeEvent((uint32_t) _configuration["scanInterval"]);



    return true;
}

void OriginApplication::EnqueueTimedOperation(Variant &request,
        OperationType operationType) {
    //1. Is this still active?
    if (!IsActive(request))
        return;

    if (operationType != OPERATION_TYPE_WEBRTC) {
        //1. Get the timer protocol
        JobsTimerProtocol * pTimer =
                (JobsTimerProtocol *) ProtocolManager::GetProtocol(
                _retryTimerProtocolId);
        if (pTimer == NULL) {
            FATAL("Unable to get the timer protocol");
            return;
        }

        //2. Enqueue the operation
        pTimer->EnqueueOperation(request, operationType);
    } else {
        //1. Get the start webrtc timer protocol
        StartWebRTCTimerProtocol * pTimer =
                (StartWebRTCTimerProtocol *) ProtocolManager::GetProtocol(
                _startwebrtcTimerProtocolId);
        if (pTimer == NULL) {
            FATAL("Unable to get the start webrtc timer protocol");
            return;
        }

        //2. Enqueue the operation
        pTimer->EnqueueOperation(request, operationType);
    }

}

void OriginApplication::RegisterExpireStream(uint32_t uniqueId) {
    if (_keepAliveTimerProtocolId == 0)
        return;
    //1. Get the timer protocol
    KeepAliveTimerProtocol * pTimer =
            (KeepAliveTimerProtocol *) ProtocolManager::GetProtocol(
            _keepAliveTimerProtocolId);
    if (pTimer == NULL) {
        FATAL("Unable to get the timer protocol");
        return;
    }

    pTimer->RegisterExpireStream(uniqueId);
}

bool OriginApplication::ProcessIngestPointsList() {
    string filePath = _configuration["ingestPointsPersistenceFile"];

    Variant setup;
    if (!Variant::DeserializeFromXmlFile(filePath, setup)) {
        WARN("Unable to load file %s", STR(filePath));
        return false;
    }

    if (setup != V_MAP) {
        FATAL("Invalid ingest points persistence file");
        return false;
    }

    FOR_MAP(setup, string, Variant, i) {
        if (MAP_VAL(i) != V_STRING)
            continue;
        string privateStreamName = MAP_KEY(i);
        string publicStreamName = MAP_VAL(i);
        if (!CreateIngestPoint(privateStreamName, publicStreamName)) {
            WARN("Unable to create ingest point %s -> %s",
                    STR(privateStreamName),
                    STR(publicStreamName));
        }
    }

    return true;
}

bool OriginApplication::ProcessPushPullList() {
    string filePath = _configuration["pushPullPersistenceFile"];

//If at device boot time is not set correct rms fails to start due to below code
#if 0
    //1. Validate time
    time_t now = getutctime();
    time_t reference = (time_t) getFileModificationDate(filePath);
    if (now < reference) {
        if (reference - now > 86400) {
            ShutdownEdges();
            exit(0);
        }
    }
#endif

    //2. Try to load the file
    Variant setup;
    if (!Variant::DeserializeFromXmlFile(filePath, setup)) {
        WARN("Unable to load file %s", STR(filePath));
        return false;
    }

    //3. Validate
    //TODO: Here we must validate the pushPull setup
    if (setup != V_MAP) {
        WARN("Unable to load file %s", STR(filePath));
        return false;
    }

    if ((setup.HasKeyChain(V_STRING, false, 1, "version"))) {
        setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_INITIAL;
    }

    if ((!setup.HasKeyChain(V_UINT32, false, 1, "version"))) {
        ASSERT("Incompatible pull/push settings version inside file %s.",
                STR(filePath));
    }

    //RDKC-6484 - Disable checksum for set up xml
    /*if (!ValidateConfigChecksum(setup)) {
        string destFile = format("%s.%"PRIu64, STR(filePath), (uint32_t) time(NULL));
        WARN("Checksum failed for the pushPullConfig file %s. It will be moved here: %s",
                STR(filePath),
                STR(destFile));
        if (!moveFile(filePath, destFile)) {
            ASSERT("Unable to move file");
        }
        return true;
    }*/

    if ((uint32_t) setup["version"] != PULL_PUSH_CONFIG_FILE_VER_CURRENT) {
        if (!UpgradePullPushConfigFile(setup)) {
            ASSERT("Unable to upgrade  pull/push settings file: %s from %"PRIu32" to %"PRIu32,
                    STR(filePath),
                    (uint32_t) setup["version"],
                    (uint32_t) PULL_PUSH_CONFIG_FILE_VER_CURRENT
                    );
        }
        if (!Variant::DeserializeFromXmlFile(filePath, setup)) {
            WARN("Unable to load file %s", STR(filePath));
            return false;
        }
    }
    //4. Do the hls
#ifdef HAS_PROTOCOL_HLS

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        MAP_VAL(i)["hlsVersion"] = _hlsVersion;
		//invalidate EXT-X-BYTERANGE if version is less than 4
		bool disableHlsResume = false;
		bool isByteRange = (bool) MAP_VAL(i)["useByteRange"];
		if (isByteRange && _hlsVersion < 4) {
			MAP_VAL(i)["useByteRange"] = false;
			MAP_VAL(i)["fileLength"] = 0;
			disableHlsResume = true;
		}
		//invalidate sample-aes if version is less than 5
		if (MAP_VAL(i)["drmType"] == DRM_TYPE_SAMPLE_AES && _hlsVersion < 5) {
			MAP_VAL(i)["drmType"] = DRM_TYPE_NONE;
			disableHlsResume = true;
		}
		//invalidate startOffset if version is less than 6
		if (((uint32_t)MAP_VAL(i)["startOffset"]) != 0 && _hlsVersion < 6) {
			MAP_VAL(i)["startOffset"] = 0;
			disableHlsResume = true;
		}

		if (disableHlsResume)
			MAP_VAL(i)["hlsResume"] = 0;

        if (!CreateHLSStreamPart(MAP_VAL(i))) {
            WARN("hls config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#endif /* HAS_PROTOCOL_HLS */

    //5. Do the hds
#ifdef HAS_PROTOCOL_HDS

    FOR_MAP(setup["hds"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!CreateHDSStreamPart(MAP_VAL(i))) {
            WARN("hds config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#endif /* HAS_PROTOCOL_HDS */

    //6. Do the mss
#ifdef HAS_PROTOCOL_MSS

    FOR_MAP(setup["mss"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!CreateMSSStreamPart(MAP_VAL(i))) {
            WARN("mss config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#endif /* HAS_PROTOCOL_MSS */

    //7. Do the dash
#ifdef HAS_PROTOCOL_DASH

    FOR_MAP(setup["dash"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!CreateDASHStreamPart(MAP_VAL(i))) {
            WARN("dash config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#endif /* HAS_PROTOCOL_DASH */

    //8. Do the pull

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");

        //update config param with video config params before calling pull
        Variant videoconfigvalue;
        Variant streamconfig = MAP_VAL(i);

        //if rfc based confile file is available update config
        if ( !fileExists(OPTIMIZED_VIDEO_PROFILE) && fileExists(_configuration["rfcvideoconfigPersistenceFile"]) ) {
            INFO("RFC video config file is available: %s", STR(_configuration["rfcvideoconfigPersistenceFile"]));
            if (!Variant::DeserializeFromXmlFile(_configuration["rfcvideoconfigPersistenceFile"], videoconfigvalue)) {
                WARN("Unable to load rfc config file %s", STR(_configuration["rfcvideoconfigPersistenceFile"]));
                
                //if for any reason we fail to load rfc defined video config fallback to default
                if (!Variant::DeserializeFromXmlFile( _configuration["videoconfigPersistenceFile"], videoconfigvalue)) {
                    WARN("Unable to load default video config file after rfc load failure %s", STR( _configuration["videoconfigPersistenceFile"]));
                } else {
                streamconfig["videoconfig"] = videoconfigvalue;
                }
            } else {
                streamconfig["videoconfig"] = videoconfigvalue;
            }
        } else {
            INFO("RFC video config file is not available: Using default video config");
            if (!Variant::DeserializeFromXmlFile( _configuration["videoconfigPersistenceFile"], videoconfigvalue)) {
                WARN("Unable to load default video config file %s", STR( _configuration["videoconfigPersistenceFile"]));
            } else {
                streamconfig["videoconfig"] = videoconfigvalue;
            }
        }

        if (!PullExternalStream(streamconfig)) {
            WARN("pull config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }

    //9. Do the push

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!PushLocalStream(MAP_VAL(i))) {
            WARN("push config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }

    //10. do the recording

    FOR_MAP(setup["record"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!RecordStream(MAP_VAL(i))) {
            WARN("record config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#ifdef HAS_PROTOCOL_WEBRTC
    // 10.1 do WEBRTC
    FOR_MAP(setup["webrtc"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("configId");
        if (!StartWebRTC(MAP_VAL(i))) {
            WARN("WebRTC config failed:\n%s", STR(MAP_VAL(i).ToString()));
        }
    }
#endif // HAS_PROTOCOL_WEBRTC

#ifdef HAS_PROTOCOL_METADATA
	// 11. add metadata listener
	FOR_MAP(setup["metalistener"], string, Variant, i) {
		MAP_VAL(i).RemoveKey("configId");
		TCPAcceptor* pAcceptor = NULL;
		if (AddMetadataListener(MAP_VAL(i), pAcceptor)) {
			WARN("Adding metadata acceptor failed:\n%s", STR(MAP_VAL(i).ToString()));
		} //instantiate metadata acceptor
	}
#endif // HAS_PROTOCOL_METADATA

	//12. do the processes after RMS have fully started
    _delayedProcesses = setup["process"];

    //13. Done
    if (!fileExists(filePath)) {
        File f;
        f.Initialize(filePath, FILE_OPEN_MODE_TRUNCATE);
        f.Close();
    }
    return true;
}

bool OriginApplication::ProcessAuthPersistenceFile() {
    Variant boolValue;
    if (!Variant::DeserializeFromXmlFile(_configuration["authPersistenceFile"], boolValue)) {
        WARN("Unable to load file %s", STR(_configuration["authPersistenceFile"]));
        return false;
    }

    if (boolValue != V_BOOL) {
        WARN("Unable to load file %s", STR(_configuration["authPersistenceFile"]));
        return false;
    }

    _pRTMPHandler->SetAuthentication((bool)boolValue);

    return true;
}

bool OriginApplication::ProcessConnectionsLimitPersistenceFile() {
    Variant value;
    if (!Variant::DeserializeFromXmlFile(_configuration["connectionsLimitPersistenceFile"], value)) {
        WARN("Unable to load file %s", STR(_configuration["connectionsLimitPersistenceFile"]));
        return false;
    }

    if (value != V_UINT32) {
        WARN("Unable to load file %s", STR(_configuration["connectionsLimitPersistenceFile"]));
        return false;
    }

    SetConnectionsCountLimit((uint32_t) value);

    return true;
}

bool OriginApplication::ProcessBandwidthLimitPersistenceFile() {
    Variant value;
    if (!Variant::DeserializeFromXmlFile(_configuration["bandwidthLimitPersistenceFile"], value)) {
        WARN("Unable to load file %s", STR(_configuration["bandwidthLimitPersistenceFile"]));
        return false;
    }

    if ((!value.HasKeyChain(V_DOUBLE, true, 1, "in"))
            || (!value.HasKeyChain(V_DOUBLE, true, 1, "out"))) {
        WARN("Unable to load file %s", STR(_configuration["bandwidthLimitPersistenceFile"]));
        return false;
    }

    SetBandwidthLimit(value["in"], value["out"]);

    return true;
}

void OriginApplication::SaveConfig(Variant &streamConfig, 
		OperationType operationType) {
    if (streamConfig.HasKey("configId"))
        return;
    streamConfig["configId"] = ++_idGenerator;
    string key = "";
    switch (operationType) {
        case OPERATION_TYPE_PULL:
        {
            key = "pull";
            break;
        }
        case OPERATION_TYPE_PUSH:
        {
            key = "push";
            break;
        }
        case OPERATION_TYPE_HLS:
        {
            key = "hls";
            break;
        }
        case OPERATION_TYPE_HDS:
        {
            key = "hds";
            break;
        }
        case OPERATION_TYPE_MSS:
        {
            key = "mss";
            break;
        }
        case OPERATION_TYPE_DASH:
        {
            key = "dash";
            break;
        }
        case OPERATION_TYPE_RECORD:
        {
            key = "record";
            break;
        }
        case OPERATION_TYPE_LAUNCHPROCESS:
        {
            key = "process";
            break;
        }
        case OPERATION_TYPE_WEBRTC:
        {
            key = "webrtc";
            break;
        }
		case OPERATION_TYPE_METADATA:
		{
			key = "metalistener";
			break;
		}
        default:
        {
            WARN("Configuration %s can't be persisted", STR(streamConfig.ToString()));
            return;
        }
    }

    _configurations[key][streamConfig["configId"]] = streamConfig;
	if (!SerializePullPushConfig(_configurations))
        WARN("Unable to save configuration");
}

bool OriginApplication::IsActive(Variant & config) {
    if (!config.HasKey("configId"))
        return true;

    uint32_t configId = (uint32_t) config["configId"];
    if ((!_configurations["pull"].HasIndex(configId))
            && (!_configurations["push"].HasIndex(configId))
            && (!_configurations["hls"].HasIndex(configId))
            && (!_configurations["hds"].HasIndex(configId))
            && (!_configurations["mss"].HasIndex(configId))
            && (!_configurations["dash"].HasIndex(configId))
            && (!_configurations["record"].HasIndex(configId))
            && (!_configurations["process"].HasIndex(configId))
			&& (!_configurations["webrtc"].HasIndex(configId))) {
        return false;
    }

    return true;
}

bool OriginApplication::ValidateConfigChecksum(Variant & setup) {
    if (!setup.HasKeyChain(V_STRING, false, 1, "checksum"))
        return false;
    string readChecksum = setup["checksum"];
    setup.RemoveKey("checksum");
    string content;
    if (!setup.SerializeToBin(content)) {
        FATAL("Unable to compute checksum");
        return false;
    }
    string wantedChecksum = lowerCase(md5(content, true));
    setup["checksum"] = readChecksum;
    if (wantedChecksum != readChecksum) {
        FATAL("Checksum failed");
        return false;
    }
    return true;
}

bool OriginApplication::ApplyChecksum(Variant & setup) {
    setup.RemoveKey("checksum");
    setup["serverVersion"] = Version::GetAll();
    string content;
    if (!setup.SerializeToBin(content)) {
        FATAL("Unable to compute checksum");
        return false;
    }
    setup["checksum"] = lowerCase(md5(content, true));
    return true;
}

bool OriginApplication::SerializePullPushConfig(Variant & config) {
	Variant temp = config;

    vector<string> nonKeepAliveStreams;

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["pull"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
		if (!((bool)(MAP_VAL(i)["serializeToConfig"])) || 
				!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["pull"].RemoveKey(nonKeepAliveStreams[i]);
    }


    nonKeepAliveStreams.clear();

    FOR_MAP(temp["push"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["push"].RemoveKey(nonKeepAliveStreams[i]);
    }


    nonKeepAliveStreams.clear();

    FOR_MAP(temp["hls"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["hls"].RemoveKey(nonKeepAliveStreams[i]);
    }

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["hds"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["hds"].RemoveKey(nonKeepAliveStreams[i]);
    }

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["mss"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["mss"].RemoveKey(nonKeepAliveStreams[i]);
    }

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["dash"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["dash"].RemoveKey(nonKeepAliveStreams[i]);
    }

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["record"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        MAP_VAL(i).RemoveKey("computedPathToFile");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["record"].RemoveKey(nonKeepAliveStreams[i]);
    }

    nonKeepAliveStreams.clear();

    FOR_MAP(temp["process"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"])))
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["process"].RemoveKey(nonKeepAliveStreams[i]);
    }
    nonKeepAliveStreams.clear();

    FOR_MAP(temp["webrtc"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("status");
        if (!((bool)(MAP_VAL(i)["keepAlive"]))) {
            ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
        }
    }
    for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
        temp["webrtc"].RemoveKey(nonKeepAliveStreams[i]);
    }

	FOR_MAP(temp["metalistener"], string, Variant, i) {
		MAP_VAL(i).RemoveKey("status");
		MAP_VAL(i).RemoveKey("localstreamname");
		//if (!((bool)(MAP_VAL(i)["keepAlive"])))
			//ADD_VECTOR_END(nonKeepAliveStreams, MAP_KEY(i));
	}
	//for (vector<string>::size_type i = 0; i < nonKeepAliveStreams.size(); i++) {
		//temp["metalistener"].RemoveKey(nonKeepAliveStreams[i]);
	//}

	//nonKeepAliveStreams.clear();

    // now do it
    temp["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_CURRENT;
    if (!ApplyChecksum(temp)) {
        FATAL("Unable to apply checksum");
        return false;
    }
    return temp.SerializeToXmlFile(_configuration["pushPullPersistenceFile"]);
}

bool OriginApplication::SerializeIngestPointsConfig() {
    Variant temp;
    temp.IsArray(false);
    map<string, string> &ingestPoints = ListIngestPoints();

    FOR_MAP(ingestPoints, string, string, i) {
        temp[MAP_KEY(i)] = MAP_VAL(i);
    }
    return temp.SerializeToXmlFile(_configuration["ingestPointsPersistenceFile"]);
}

bool OriginApplication::DropConnection(TCPAcceptor * pTCPAcceptor) {
    //1. Get the parameters
    Variant &params = pTCPAcceptor->GetParameters();

    //2. If this is a CLI or VARIANT, than continue accepting it
    if ((params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_CLI_JSON)
#ifdef HAS_PROTOCOL_ASCIICLI
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_CLI_ASCII)
#endif /* HAS_PROTOCOL_ASCIICLI */
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_HTTP_CLI_JSON)
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_XML_VARIANT)
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_BIN_VARIANT)
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_HTTP_XML_VARIANT)
            || (params[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_HTTP_BIN_VARIANT)
            )
        return false;

    //3. Ok, this could be dropped. Test the connection counters now
    if (_connectionsCountLimit != 0) {
        int64_t currentCount = GetConnectionsCount();
        if (currentCount > _connectionsCountLimit) {
            INFO("Connection will be dropped. Connections (current/max): %"PRId64"/%"PRIu32,
                    currentCount, _connectionsCountLimit);
            pTCPAcceptor->Drop();
            return true;
        }
    }

    //4. Get the bandwidth limits
    double in = 0, out = 0, maxIn = 0, maxOut = 0;
    GetBandwidth(in, out, maxIn, maxOut);

    //5. Check the bandwidth limits
    if (((maxIn > 0) && (in > maxIn)) || ((maxOut > 0) && (out > maxOut))) {
        INFO("Connection will be dropped. Connections (in/maxIn out/maxOut): %.2f/%.2f %.2f/%.2f",
                in, maxIn, out, maxOut);
        pTCPAcceptor->Drop();
        return true;
    }

    //6. The connection will be accepted
    return false;
}

void OriginApplication::RemoveFromPushSetup(Variant &settings) {
    if ((bool)settings["keepAlive"])
        return;
    string localStreamName;
    string targetUri;
    string targetStreamName;
    switch ((uint8_t) settings["operationType"]) {
        case OPERATION_TYPE_RECORD:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["pathToFile"];
            targetStreamName = "recording";
            break;
        }
        case OPERATION_TYPE_PUSH:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["targetUri"];
            targetStreamName = (string) settings["targetStreamName"];
            break;
        }
        case OPERATION_TYPE_HLS:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["targetFolder"];
            targetStreamName = (string) settings["chunkBaseName"];
            break;
        }
        case OPERATION_TYPE_HDS:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["targetFolder"];
            targetStreamName = (string) settings["chunkBaseName"];
            break;
        }
        case OPERATION_TYPE_MSS:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["targetFolder"];
            targetStreamName = (string) settings["chunkBaseName"];
            break;
        }
        case OPERATION_TYPE_DASH:
        {
            localStreamName = (string) settings["localStreamName"];
            targetUri = (string) settings["targetFolder"];
            targetStreamName = (string) settings["chunkBaseName"];
            break;
        }
        case OPERATION_TYPE_WEBRTC:
        {
        	return;	// nothing to do here, no localstreamname
        }
        default:
        {
            WARN("persistent push not removed:\n%s", STR(settings.ToString()));
            return;
        }
    }

    if (_pushSetup.HasKeyChain(V_MAP, true, 3, STR(localStreamName),
            STR(targetUri), STR(targetStreamName))) {
        _pushSetup[localStreamName][targetUri].RemoveKey(targetStreamName);
        if (_pushSetup[localStreamName][targetUri].MapSize() == 0) {
            _pushSetup[localStreamName].RemoveKey(targetUri);
            if (_pushSetup[localStreamName].MapSize() == 0) {
                _pushSetup.RemoveKey(localStreamName);
            }
        }
    }
}

void OriginApplication::RemoveFromConfigStatus(OperationType operationType, Variant &settings) {
    if ((bool)settings["keepAlive"])
        return;
    string key;
    switch (operationType) {
        case OPERATION_TYPE_PULL:
        {
            key = "pull";
            break;
        }
        case OPERATION_TYPE_PUSH:
        {
            key = "push";
            break;
        }
        case OPERATION_TYPE_HLS:
        {
            key = "hls";
            break;
        }
        case OPERATION_TYPE_HDS:
        {
            key = "hds";
            break;
        }
        case OPERATION_TYPE_MSS:
        {
            key = "mss";
            break;
        }
        case OPERATION_TYPE_DASH:
        {
            key = "dash";
            break;
        }
        case OPERATION_TYPE_RECORD:
        {
            key = "record";
            break;
        }
        case OPERATION_TYPE_LAUNCHPROCESS:
        {
            key = "process";
            break;
        }
        case OPERATION_TYPE_WEBRTC:
        {
            //DEBUG("$b2$ Trying RemoveFromConfigStatus()");
            key = "webrtc";
            break;
        }
		case OPERATION_TYPE_METADATA:
		{
			//DEBUG("$b2$ Trying RemoveFromConfigStatus()");
			key = "metalistener";
			break;
		}
        default:
        {
            key = "";
            break;
        }
    }
    if (key != "") {
        _configurations[key].RemoveAt((uint32_t) settings["configId"]);
        if (!SerializePullPushConfig(_configurations))
            WARN("Unable to save push/pull configuration");
    }
}

bool OriginApplication::RecordedFileActive(string path) {
    //FINEST("** RecordedFileActive on %s", STR(path));
    StreamsManager *pSM = GetStreamsManager();

    map<uint32_t, BaseStream *> allStreams = pSM->FindByType(ST_OUT_FILE_TS);
    map<uint32_t, BaseStream *> flvStreams = pSM->FindByType(ST_OUT_FILE_RTMP_FLV);

    FOR_MAP(flvStreams, uint32_t, BaseStream *, i) {
        allStreams[MAP_KEY(i)] = MAP_VAL(i);
    }

    FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
        Variant &params = MAP_VAL(i)->GetProtocol()->GetCustomParameters();
        if (!params.HasKeyChain(V_STRING, true, 3, "customParameters",
                "recordConfig",
                "pathToFile"))
            continue;
        if (params["customParameters"]["recordConfig"]["pathToFile"] == path) {
            //FINEST("File active: %s", STR(path));
            return true;
        }
    }
    return false;
}

bool OriginApplication::RecordedFileScheduled(string path, uint32_t configId) {
    //FINEST("** RecordedFileScheduled on %s", STR(path));

    FOR_MAP(_pushSetup, string, Variant, i) {
        if (!MAP_VAL(i).HasKey(path))
            continue;
        if ((uint32_t) MAP_VAL(i)[path]["recording"]["configId"] != configId) {
            //FINEST("File found in _pushSetup: %s", STR(path));
            return true;
        }
    }

    FOR_MAP(_configurations["record"], string, Variant, i) {
        if ((uint32_t) MAP_VAL(i)["configId"] == configId)
            continue;
        if (MAP_VAL(i)["pathToFile"] == path) {
            //FINEST("File found in _streamsConfigurations: %s", STR(path));
            return true;
        }
    }

    return false;
}

bool OriginApplication::RecordedFileHasPermissions(string path) {
    //FINEST("** ReordedFileHasPermissions on %s", STR(path));
    File temp;
    temp.SuppressLogErrorsOnInit();
    if (fileExists(path)) {
        //FINEST("File exists: %s", STR(path));
        if (!temp.Initialize(path, FILE_OPEN_MODE_APPEND)) {
            //FINEST("No permissions: %s", STR(path));
            return false;
        }
        temp.Close();
        return true;
    } else {
        if (!temp.Initialize(path, FILE_OPEN_MODE_TRUNCATE)) {
            //FINEST("No permissions: %s", STR(path));
            return false;
        }
        temp.Close();
        if (!deleteFile(path)) {
            //FINEST("Can't delete file: %s", STR(path));
            return false;
        }
        return true;
    }
}

void OriginApplication::ProcessAutoHLS(string streamName) {
    if (_autoHlsTemplate == V_NULL) {
        return;
    }

    _autoHlsTemplate["localStreamNames"].RemoveAllKeys();
    _autoHlsTemplate["localStreamNames"].PushToArray(streamName);
    _autoHlsTemplate["chunkBaseName"] = streamName;
    _autoHlsTemplate["playlistName"] = streamName + ".m3u8";

    // Sanity check
    string targetFolder = normalizePath(_autoHlsTemplate["targetFolder"], "");
    if (targetFolder == "") {
        FATAL("Could not process HLS. Invalid target folder: %s", STR(_autoHlsTemplate["targetFolder"]));
        return;
    }

    if (!CreateHLSStream(_autoHlsTemplate)) {
        FATAL("Unable to create auto HLS for stream %s in folder %s",
                STR(streamName), STR(_autoHlsTemplate["targetFolder"]));
        return;
    }

    INFO("Auto HLS created for stream %s in folder %s",
            STR(streamName), STR(_autoHlsTemplate["targetFolder"]));
}

void OriginApplication::ProcessAutoHDS(string streamName) {
    if (_autoHdsTemplate == V_NULL) {
        return;
    }

    _autoHdsTemplate["localStreamNames"].RemoveAllKeys();
    _autoHdsTemplate["localStreamNames"].PushToArray(streamName);
    _autoHdsTemplate["chunkBaseName"] = streamName;
    _autoHdsTemplate["manifestName"] = streamName + ".f4m";

    // Sanity check
    string targetFolder = normalizePath(_autoHdsTemplate["targetFolder"], "");
    if (targetFolder == "") {
        FATAL("Could not process HDS. Invalid target folder: %s", STR(_autoHdsTemplate["targetFolder"]));
        return;
    }
#ifdef HAS_PROTOCOL_HDS
    if (!CreateHDSStream(_autoHdsTemplate)) {
        FATAL("Unable to create auto HDS for stream %s in folder %s",
                STR(streamName), STR(_autoHdsTemplate["targetFolder"]));
        return;
    }

    INFO("Auto HDS created for stream %s in folder %s",
            STR(streamName), STR(_autoHdsTemplate["targetFolder"]));
#endif
}

#ifdef HAS_PROTOCOL_MSS

void OriginApplication::ProcessAutoMSS(string streamName) {
    if (_autoMssTemplate == V_NULL) {
        return;
    }

    _autoMssTemplate["localStreamNames"].RemoveAllKeys();
    _autoMssTemplate["localStreamNames"].PushToArray(streamName);
    _autoMssTemplate["manifestName"] = format("%s.ismc", STR(streamName));
	_autoMssTemplate["isLive"] = true; //autoMSS will be live
	_autoMssTemplate["isAutoMSS"] = true;

    if (!CreateMSSStream(_autoMssTemplate)) {
        FATAL("Unable to create auto MSS for stream %s in folder %s",
                STR(streamName), STR(_autoMssTemplate["targetFolder"]));
        return;
    }

    INFO("Auto MSS created for stream %s in folder %s",
            STR(streamName), STR(_autoMssTemplate["targetFolder"]));
}

#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH

void OriginApplication::ProcessAutoDASH(string streamName) {
    if (_autoDashTemplate == V_NULL) {
        return;
    }

    _autoDashTemplate["localStreamNames"].RemoveAllKeys();
    _autoDashTemplate["localStreamNames"].PushToArray(streamName);
	_autoDashTemplate["manifestName"] = format("%s.mpd", STR(streamName));
	_autoDashTemplate["dynamicProfile"] = true; //autoDASH will be live 
	_autoDashTemplate["isAutoDASH"] = true;

    if (!CreateDASHStream(_autoDashTemplate)) {
        FATAL("Unable to create auto DASH for stream %s in folder %s",
                STR(streamName), STR(_autoDashTemplate["targetFolder"]));
        return;
    }

    INFO("Auto DASH created for stream %s in folder %s",
            STR(streamName), STR(_autoDashTemplate["targetFolder"]));
}

#endif /* HAS_PROTOCOL_DASH */

void OriginApplication::ReadAutoHLSTemplate() {
    //1. reset the template
    _autoHlsTemplate.Reset();

    //2. Put in the fixed settings
    _autoHlsTemplate["localStreamNames"].IsArray(true);
    _autoHlsTemplate["bandwidths"].PushToArray((uint32_t) 0);
    _autoHlsTemplate["cleanupDestination"] = (bool)true;
    _autoHlsTemplate["createMasterPlaylist"] = (bool)false;
    _autoHlsTemplate["keepAlive"] = (bool)false;
    _autoHlsTemplate["overwriteDestination"] = (bool)true;
    _autoHlsTemplate["playlistType"] = "rolling";

    //3. Read the configurable settings from the config file
    if (!(_configuration.HasKeyChain(V_STRING, false, 2, "autoHLS", "targetFolder"))) {
        _autoHlsTemplate.Reset();
        return;
    } else {
        string targetFolder = (string) (_configuration.GetValue("autoHLS", false)
                .GetValue("targetFolder", false));
        _autoHlsTemplate["targetFolder"] = targetFolder;
    }

    // 4. Read chunkOnIDR
    _autoHlsTemplate["chunkOnIDR"] = (bool)false; //the default value from CLI
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "autoHLS", "chunkOnIDR"))
        _autoHlsTemplate["chunkOnIDR"] = (_configuration.GetValue("autoHLS", false)
            .GetValue("chunkOnIDR", false));

    // 4. Read chunkLength
    uint32_t chunkLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHLS", "chunkLength")) {
        chunkLength = (uint32_t) (_configuration.GetValue("autoHLS", false)
                .GetValue("chunkLength", false));
    }
    if ((chunkLength < 1) || (chunkLength > 3600)) {
        chunkLength = 10;
        WARN("Invalid autoHLS chunkLength. Defaulted to %"PRIu32, chunkLength);
    }
    _autoHlsTemplate["chunkLength"] = (uint64_t) chunkLength;

#ifdef HAS_PROTOCOL_DRM
    // 5. Read drmType (replaces encryptStream)
    string drmType = "";
    if (_configuration.HasKeyChain(V_STRING, false, 2, "autoHLS", "drmType")) {
        drmType = (string) (_configuration.GetValue("autoHLS", false)
                .GetValue("drmType", false));
        if (drmType == DRM_TYPE_NONE)
            drmType = "";
    }
    _autoHlsTemplate["drmType"] = drmType;
#else /* HAS_PROTOCOL_DRM */
    // 5. Read encryptStream (deprecated)
    bool encryptStream = false;
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "autoHLS", "encryptStream")) {
        encryptStream = (bool) (_configuration.GetValue("autoHLS", false)
                .GetValue("encryptStream", false));
    }
    _autoHlsTemplate["encryptStream"] = encryptStream;
#endif /* HAS_PROTOCOL_DRM */

    // 6. Read AESKeyCount
    uint64_t AESKeyCount = 5; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHLS", "AESKeyCount")) {
        AESKeyCount = (uint64_t) (_configuration.GetValue("autoHLS", false)
                .GetValue("AESKeyCount", false));
    }
    if ((AESKeyCount < 1) || (AESKeyCount > 20)) {
        AESKeyCount = 5;
        WARN("Invalid autoHLS AESKeyCount. Defaulted to %"PRIu64, AESKeyCount);
    }
    _autoHlsTemplate["AESKeyCount"] = (uint64_t) AESKeyCount;

    // 7. Read playlistLength
    uint64_t playlistLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHLS", "playlistLength")) {
        playlistLength = (uint64_t) (_configuration.GetValue("autoHLS", false)
                .GetValue("playlistLength", false));
    }
    if ((playlistLength < 5) || (playlistLength > 999999)) {
        playlistLength = 10;
        WARN("Invalid autoHLS playlistLength. Defaulted to %"PRIu64, playlistLength);
    }
    _autoHlsTemplate["playlistLength"] = (uint64_t) playlistLength;

    // 8. Read staleRetentionCount
    uint64_t staleRetentionCount = playlistLength; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHLS", "staleRetentionCount")) {
        staleRetentionCount = (uint64_t) (_configuration.GetValue("autoHLS", false)
                .GetValue("staleRetentionCount", false));
    }
    if ((staleRetentionCount == 0) || (staleRetentionCount > 999999)) {
        staleRetentionCount = playlistLength;
        WARN("Invalid autoHLS playlistLength. Defaulted to %"PRIu64, staleRetentionCount);
    }
    _autoHlsTemplate["staleRetentionCount"] = (uint64_t) staleRetentionCount;

    //9. Read groupName
    string groupName = "autoHLS";
    if (_configuration.HasKeyChain(V_STRING, false, 2, "autoHLS", "groupName")) {
        groupName = (string) (_configuration.GetValue("autoHLS", false)
                .GetValue("groupName", false));
    }
    trim(groupName);
    if (groupName == "") {
        groupName = "autoHLS";
        WARN("Invalid autoHLS groupName. Defaulted to %s", STR(groupName));
    }
    _autoHlsTemplate["groupName"] = groupName;
}

void OriginApplication::ReadAutoHDSTemplate() {
    //1. reset the template
    _autoHdsTemplate.Reset();

    //2. Put in the fixed settings
    _autoHdsTemplate["localStreamNames"].IsArray(true);
    _autoHdsTemplate["bandwidths"].PushToArray((uint32_t) 0);
    _autoHdsTemplate["cleanupDestination"] = (bool)true;
    _autoHdsTemplate["createMasterPlaylist"] = (bool)false;
    _autoHdsTemplate["keepAlive"] = (bool)false;
    _autoHdsTemplate["overwriteDestination"] = (bool)true;
    _autoHdsTemplate["playlistType"] = "rolling";

    //3. Read the configurable settings from the config file
    if (!(_configuration.HasKeyChain(V_STRING, false, 2, "autoHDS", "targetFolder"))) {
        _autoHdsTemplate.Reset();
        return;
    } else {
        string targetFolder = (string) (_configuration.GetValue("autoHDS", false)
                .GetValue("targetFolder", false));
        _autoHdsTemplate["targetFolder"] = targetFolder;
    }

    // 4. Read chunkOnIDR
    _autoHdsTemplate["chunkOnIDR"] = (bool)false; //the default value from CLI
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "autoHDS", "chunkOnIDR"))
        _autoHdsTemplate["chunkOnIDR"] = (_configuration.GetValue("autoHDS", false)
            .GetValue("chunkOnIDR", false));

    // 4. Read chunkLength
    uint32_t chunkLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHDS", "chunkLength")) {
        chunkLength = (uint32_t) (_configuration.GetValue("autoHDS", false)
                .GetValue("chunkLength", false));
    }
    if ((chunkLength < 1) || (chunkLength > 3600)) {
        chunkLength = 10;
        WARN("Invalid autoHDS chunkLength. Defaulted to %"PRIu32, chunkLength);
    }
    _autoHdsTemplate["chunkLength"] = (uint64_t) chunkLength;

    // 7. Read playlistLength
    uint64_t playlistLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHDS", "playlistLength")) {
        playlistLength = (uint64_t) (_configuration.GetValue("autoHDS", false)
                .GetValue("playlistLength", false));
    }
    if ((playlistLength < 5) || (playlistLength > 999999)) {
        playlistLength = 10;
        WARN("Invalid autoHDS playlistLength. Defaulted to %"PRIu64, playlistLength);
    }
    _autoHdsTemplate["playlistLength"] = (uint64_t) playlistLength;

    // 8. Read staleRetentionCount
    uint64_t staleRetentionCount = playlistLength; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoHDS", "staleRetentionCount")) {
        staleRetentionCount = (uint64_t) (_configuration.GetValue("autoHDS", false)
                .GetValue("staleRetentionCount", false));
    }
    if ((staleRetentionCount == 0) || (staleRetentionCount > 999999)) {
        staleRetentionCount = playlistLength;
        WARN("Invalid autoHDS playlistLength. Defaulted to %"PRIu64, staleRetentionCount);
    }
    _autoHdsTemplate["staleRetentionCount"] = (uint64_t) staleRetentionCount;

    //9. Read groupName
    string groupName = "autoHDS";
    if (_configuration.HasKeyChain(V_STRING, false, 2, "autoHDS", "groupName")) {
        groupName = (string) (_configuration.GetValue("autoHDS", false)
                .GetValue("groupName", false));
    }
    trim(groupName);
    if (groupName == "") {
        groupName = "autoHDS";
        WARN("Invalid autoHDS groupName. Defaulted to %s", STR(groupName));
    }
    _autoHdsTemplate["groupName"] = groupName;
}

#ifdef HAS_PROTOCOL_MSS

void OriginApplication::ReadAutoMSSTemplate() {
    //1. reset the template
    _autoMssTemplate.Reset();

    //2. Put in the fixed settings
    _autoMssTemplate["bandwidths"].PushToArray((uint32_t) 0);
    _autoMssTemplate["cleanupDestination"] = (bool) true;
    _autoMssTemplate["keepAlive"] = (bool) false;
    _autoMssTemplate["localStreamNames"].IsArray(true);
    _autoMssTemplate["manifestName"] = (string) "manifest.ismc";
    _autoMssTemplate["overwriteDestination"] = (bool) true;
    _autoMssTemplate["playlistType"] = "rolling";

    //3. Read the configurable settings from the config file
    if (!(_configuration.HasKeyChain(V_STRING, false, 2, "autoMSS", "targetFolder"))) {
        _autoMssTemplate.Reset();
        return;
    } else {
        string targetFolder = (string) (_configuration.GetValue("autoMSS", false)
                .GetValue("targetFolder", false));
        _autoMssTemplate["targetFolder"] = targetFolder;
    }

    _autoMssTemplate["chunkOnIDR"] = (bool)false; //the default value from CLI
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "autoMSS", "chunkOnIDR"))
        _autoMssTemplate["chunkOnIDR"] = (_configuration.GetValue("autoMSS", false)
            .GetValue("chunkOnIDR", false));

    // 4. Read chunkLength
    uint32_t chunkLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoMSS", "chunkLength")) {
        chunkLength = (uint32_t) (_configuration.GetValue("autoMSS", false)
                .GetValue("chunkLength", false));
    }
    if ((chunkLength < 1) || (chunkLength > 3600)) {
        chunkLength = 10;
        WARN("Invalid autoMSS chunkLength. Defaulted to %"PRIu32, chunkLength);
    }
    _autoMssTemplate["chunkLength"] = (uint64_t) chunkLength;

    // 7. Read playlistLength
    uint64_t playlistLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoMSS", "playlistLength")) {
        playlistLength = (uint64_t) (_configuration.GetValue("autoMSS", false)
                .GetValue("playlistLength", false));
    }
    if ((playlistLength < 5) || (playlistLength > 999999)) {
        playlistLength = 10;
        WARN("Invalid autoMSS playlistLength. Defaulted to %"PRIu64, playlistLength);
    }
    _autoMssTemplate["playlistLength"] = (uint64_t) playlistLength;

    // 8. Read staleRetentionCount
    uint64_t staleRetentionCount = playlistLength; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoMSS", "staleRetentionCount")) {
        staleRetentionCount = (uint64_t) (_configuration.GetValue("autoMSS", false)
                .GetValue("staleRetentionCount", false));
    }
    if ((staleRetentionCount == 0) || (staleRetentionCount > 999999)) {
        staleRetentionCount = playlistLength;
        WARN("Invalid autoMSS playlistLength. Defaulted to %"PRIu64, staleRetentionCount);
    }
    _autoMssTemplate["staleRetentionCount"] = (uint64_t) staleRetentionCount;

    //9. Read groupName
    string groupName = "autoMSS";
    if (_configuration.HasKeyChain(V_STRING, false, 2, "autoMSS", "groupName")) {
        groupName = (string) (_configuration.GetValue("autoMSS", false)
                .GetValue("groupName", false));
    }
    trim(groupName);
    if (groupName == "") {
        groupName = "autoMSS";
        WARN("Invalid autoMSS groupName. Defaulted to %s", STR(groupName));
    }
    _autoMssTemplate["groupName"] = groupName;
}

#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH

void OriginApplication::ReadAutoDASHTemplate() {
    //1. reset the template
    _autoDashTemplate.Reset();

    //2. Put in the fixed settings
    _autoDashTemplate["bandwidths"].PushToArray((uint32_t) 0);
    _autoDashTemplate["cleanupDestination"] = (bool) true;
    _autoDashTemplate["keepAlive"] = (bool) false;
    _autoDashTemplate["localStreamNames"].IsArray(true);
    _autoDashTemplate["manifestName"] = (string) "manifest.mpd";
    _autoDashTemplate["overwriteDestination"] = (bool) true;
    _autoDashTemplate["playlistType"] = "rolling";

    //3. Read the configurable settings from the config file
    if (!(_configuration.HasKeyChain(V_STRING, false, 2, "autoDASH", "targetFolder"))) {
        _autoDashTemplate.Reset();
        return;
    } else {
        string targetFolder = (string) (_configuration.GetValue("autoDASH", false)
                .GetValue("targetFolder", false));
        _autoDashTemplate["targetFolder"] = targetFolder;
    }

    _autoDashTemplate["chunkOnIDR"] = (bool)false; //the default value from CLI
    if (_configuration.HasKeyChain(V_BOOL, false, 2, "autoDASH", "chunkOnIDR"))
        _autoDashTemplate["chunkOnIDR"] = (_configuration.GetValue("autoDASH", false)
            .GetValue("chunkOnIDR", false));

    // 4. Read chunkLength
    uint32_t chunkLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoDASH", "chunkLength")) {
        chunkLength = (uint32_t) (_configuration.GetValue("autoDASH", false)
                .GetValue("chunkLength", false));
    }
    if ((chunkLength < 1) || (chunkLength > 3600)) {
        chunkLength = 10;
        WARN("Invalid autoDASH chunkLength. Defaulted to %"PRIu32, chunkLength);
    }
    _autoDashTemplate["chunkLength"] = (uint64_t) chunkLength;

    // 7. Read playlistLength
    uint64_t playlistLength = 10; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoDASH", "playlistLength")) {
        playlistLength = (uint64_t) (_configuration.GetValue("autoDASH", false)
                .GetValue("playlistLength", false));
    }
    if ((playlistLength < 5) || (playlistLength > 999999)) {
        playlistLength = 10;
        WARN("Invalid autoDASH playlistLength. Defaulted to %"PRIu64, playlistLength);
    }
    _autoDashTemplate["playlistLength"] = (uint64_t) playlistLength;

    // 8. Read staleRetentionCount
    uint64_t staleRetentionCount = playlistLength; //the default value from CLI
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 2, "autoDASH", "staleRetentionCount")) {
        staleRetentionCount = (uint64_t) (_configuration.GetValue("autoDASH", false)
                .GetValue("staleRetentionCount", false));
    }
    if ((staleRetentionCount == 0) || (staleRetentionCount > 999999)) {
        staleRetentionCount = playlistLength;
        WARN("Invalid autoDASH playlistLength. Defaulted to %"PRIu64, staleRetentionCount);
    }
    _autoDashTemplate["staleRetentionCount"] = (uint64_t) staleRetentionCount;

    //9. Read groupName
    string groupName = "autoDASH";
    if (_configuration.HasKeyChain(V_STRING, false, 2, "autoDASH", "groupName")) {
        groupName = (string) (_configuration.GetValue("autoDASH", false)
                .GetValue("groupName", false));
    }
    trim(groupName);
    if (groupName == "") {
        groupName = "autoDASH";
        WARN("Invalid autoDASH groupName. Defaulted to %s", STR(groupName));
    }
    _autoDashTemplate["groupName"] = groupName;
}

#endif /* HAS_PROTOCOL_DASH */

bool OriginApplication::UpgradePullPushConfigFile(Variant & setup) {
    string filePath = _configuration["pushPullPersistenceFile"];
    //1. Get the source version
    uint32_t sourceVersion = (uint32_t) setup["version"];

    //2. Upgrade it one step
    switch (sourceVersion) {
        case PULL_PUSH_CONFIG_FILE_VER_INITIAL:
        {
            if (!UpgradePullPushConfigFile_1_2(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_FORBID_STREAM_NAMES_DUPLICATES:
        {
            if (!UpgradePullPushConfigFile_2_3(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_ISHLS_SETTING:
        {
            if (!UpgradePullPushConfigFile_3_4(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_TTLTOS_SETTING:
        {
            if (!UpgradePullPushConfigFile_4_5(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_WIDTHHEIGHT_SETTING:
        {
            if (!UpgradePullPushConfigFile_5_6(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_RTCP_DETECTION_INTERVAL:
        {
            if (!UpgradePullPushConfigFile_6_7(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_TCURL:
        {
            if (!UpgradePullPushConfigFile_7_8(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_FORCETCP_ON_PUSH:
        {
            if (!UpgradePullPushConfigFile_8_9(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_HDS:
        {
            if (!UpgradePullPushConfigFile_9_10(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_CLEANUP_DESTINATION:
        {
            if (!UpgradePullPushConfigFile_10_11(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_MISSING_RECORD:
        {
            if (!UpgradePullPushConfigFile_11_12(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_WRONG_HDS_CHUNK_LENGTH:
        {
            if (!UpgradePullPushConfigFile_12_13(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_CHANGE_VARIANT_INDEXER:
        {
            if (!UpgradePullPushConfigFile_13_14(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_REMOVE_ISXXX_BOOL_VALUES:
        {
            if (!UpgradePullPushConfigFile_14_15(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_RENAME_ROLLING_LIMIT:
        {
            if (!UpgradePullPushConfigFile_15_16(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_SSMIP_TO_PULL:
        {
            if (!UpgradePullPushConfigFile_16_17(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_RTMP_ABSOLUTE_TIMESTAMPS:
        {
            if (!UpgradePullPushConfigFile_17_18(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_HTTP_PROXY:
        {
            if (!UpgradePullPushConfigFile_18_19(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_RANGE_START_END:
        {
            if (!UpgradePullPushConfigFile_19_20(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_PROCESS_GROUPNAME:
        {
            if (!UpgradePullPushConfigFile_20_21(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_RECORD_WAITFORIDR:
        {
            if (!UpgradePullPushConfigFile_21_22(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_SEND_RENEW_STREAM:
        {
            if (!UpgradePullPushConfigFile_22_23(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_VERIMATRIX_DRM:
        {
            if (!UpgradePullPushConfigFile_23_24(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_MSS:
        {
            if (!UpgradePullPushConfigFile_24_25(setup))
                return false;
            break;
        }
        case PULL_PUSH_CONFIG_FILE_VER_ADD_DASH:
        {
            if (!UpgradePullPushConfigFile_25_26(setup))
                return false;
            break;
        }
		case PULL_PUSH_CONFIG_FILE_VER_ADD_WEBRTC:
		{
			if (!UpgradePullPushConfigFile_26_27(setup))
				return false;
			break;
		}
		case PULL_PUSH_CONFIG_FILE_VER_CURRENT:
		{
			return true;
		}
        default:
        {
            return false;
        }
    }

    if (!ApplyChecksum(setup)) {
        FATAL("Unable to apply checksum");
        return false;
    }

    if (!setup.SerializeToXmlFile(filePath)) {
        FATAL("Unable to save file %s", STR(filePath));
        return false;
    }

    INFO("File %s upgraded from %"PRIu32" to %"PRIu32,
            STR(filePath),
            sourceVersion,
            sourceVersion + 1);

    //3. Re-read the file
    setup.Reset();
    if (!Variant::DeserializeFromXmlFile(filePath, setup)) {
        WARN("Unable to load file %s", STR(filePath));
        return false;
    }

    //3. Get the new source version
    sourceVersion = (uint32_t) setup["version"];

    //4. Do we need to upgrade it once more?
    if (sourceVersion < PULL_PUSH_CONFIG_FILE_VER_CURRENT) {
        return UpgradePullPushConfigFile(setup);
    } else {
        return true;
    }
}

bool OriginApplication::UpgradePullPushConfigFile_1_2(Variant & setup) {
    map<string, string> streamNames;

    FOR_MAP(setup["pull"], string, Variant, i) {
        string localStreamName = MAP_VAL(i)["localStreamName"];
        if (MAP_HAS1(streamNames, localStreamName)) {
            FATAL("Duplicate stream name found: %s", STR(localStreamName));
            return false;
        }
        streamNames[localStreamName] = localStreamName;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_FORBID_STREAM_NAMES_DUPLICATES;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_2_3(Variant & setup) {
    map<string, string> streamNames;

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i)["isHls"] = (bool)true;
    }

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["isHls"] = (bool)false;
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["isHls"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_ISHLS_SETTING;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_3_4(Variant & setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["ttl"] = (uint16_t) 0x100;
        MAP_VAL(i)["tos"] = (uint16_t) 0x100;
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["ttl"] = (uint16_t) 0x100;
        MAP_VAL(i)["tos"] = (uint16_t) 0x100;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_TTLTOS_SETTING;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_4_5(Variant & setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["width"] = (uint32_t) 0;
        MAP_VAL(i)["height"] = (uint32_t) 0;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_WIDTHHEIGHT_SETTING;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_5_6(Variant & setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)[CONF_APPLICATION_RTCPDETECTIONINTERVAL] =
                (uint8_t) _configuration[CONF_APPLICATION_RTCPDETECTIONINTERVAL];
    }
    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_RTCP_DETECTION_INTERVAL;
    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_6_7(Variant & setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["tcUrl"] = "";
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["tcUrl"] = "";
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_TCURL;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_7_8(Variant & setup) {

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["forceTcp"] = (bool)false;
    }
    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_FORCETCP_ON_PUSH;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_8_9(Variant & setup) {

    setup["hds"].IsArray(true);

    FOR_MAP(setup["hds"], string, Variant, i) {
        MAP_VAL(i)["isHds"] = (bool)true;
    }

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i)["isHds"] = (bool)false;
    }

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["isHds"] = (bool)false;
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["isHds"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_HDS;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_9_10(Variant & setup) {

    setup["hds"].IsArray(true);

    FOR_MAP(setup["hds"], string, Variant, i) {
        MAP_VAL(i)["cleanupDestination"] = (bool)false;
    }

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i)["cleanupDestination"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_CLEANUP_DESTINATION;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_10_11(Variant & setup) {

    setup["record"].IsArray(true);

    FOR_MAP(setup["record"], string, Variant, i) {
        MAP_VAL(i)["isRecord"] = (bool)true;
    }

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i)["isRecord"] = (bool)false;
    }

    FOR_MAP(setup["hds"], string, Variant, i) {
        MAP_VAL(i)["isRecord"] = (bool)false;
    }

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["isRecord"] = (bool)false;
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["isRecord"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_MISSING_RECORD;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_11_12(Variant &setup) {

    FOR_MAP(setup["hls"], string, Variant, i) {
        uint32_t cl = (uint32_t) MAP_VAL(i)["chunkLength"];
        if (cl < 5 || cl > 3600)
            MAP_VAL(i)["chunkLength"] = (uint32_t) 5;
    }
    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_WRONG_HDS_CHUNK_LENGTH;

    return true;
}

void ChangeVariantIndexer(Variant &variant) {
    if ((variant != V_MAP) && (variant != V_TYPED_MAP))
        return;

    vector<string> dirtyKeys;

    FOR_MAP(variant, string, Variant, i) {
        string key = MAP_KEY(i);
        if (key.find("__index__value__") == 0)
            ADD_VECTOR_END(dirtyKeys, key);
        ChangeVariantIndexer(MAP_VAL(i));
    }

    FOR_VECTOR(dirtyKeys, i) {
        uint32_t newKey = strtoul(dirtyKeys[i].c_str() + 16, NULL, 10);
        Variant val = variant[dirtyKeys[i]];
        variant.RemoveKey(dirtyKeys[i]);
        variant[newKey] = val;
    }
}

bool OriginApplication::UpgradePullPushConfigFile_12_13(Variant &setup) {
    ChangeVariantIndexer(setup);

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_CHANGE_VARIANT_INDEXER;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_13_14(Variant &setup) {
    setup["process"].IsArray(true);

    FOR_MAP(setup["record"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("isHls", false);
        MAP_VAL(i).RemoveKey("isHds", false);
        MAP_VAL(i).RemoveKey("isRecord", false);
        MAP_VAL(i).RemoveKey("isPull", false);
        MAP_VAL(i).RemoveKey("isPush", false);
        MAP_VAL(i).RemoveKey("isProcess", false);
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_RECORD;
    }

    FOR_MAP(setup["hls"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("isHls", false);
        MAP_VAL(i).RemoveKey("isHds", false);
        MAP_VAL(i).RemoveKey("isRecord", false);
        MAP_VAL(i).RemoveKey("isPull", false);
        MAP_VAL(i).RemoveKey("isPush", false);
        MAP_VAL(i).RemoveKey("isProcess", false);
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_HLS;
    }

    FOR_MAP(setup["hds"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("isHls", false);
        MAP_VAL(i).RemoveKey("isHds", false);
        MAP_VAL(i).RemoveKey("isRecord", false);
        MAP_VAL(i).RemoveKey("isPull", false);
        MAP_VAL(i).RemoveKey("isPush", false);
        MAP_VAL(i).RemoveKey("isProcess", false);
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_HDS;
    }

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("isHls", false);
        MAP_VAL(i).RemoveKey("isHds", false);
        MAP_VAL(i).RemoveKey("isRecord", false);
        MAP_VAL(i).RemoveKey("isPull", false);
        MAP_VAL(i).RemoveKey("isPush", false);
        MAP_VAL(i).RemoveKey("isProcess", false);
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_PULL;
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i).RemoveKey("isHls", false);
        MAP_VAL(i).RemoveKey("isHds", false);
        MAP_VAL(i).RemoveKey("isRecord", false);
        MAP_VAL(i).RemoveKey("isPull", false);
        MAP_VAL(i).RemoveKey("isPush", false);
        MAP_VAL(i).RemoveKey("isProcess", false);
        MAP_VAL(i)["operationType"] = (uint8_t) OPERATION_TYPE_PUSH;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_REMOVE_ISXXX_BOOL_VALUES;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_14_15(Variant &setup) {
    if (!setup.HasKeyChain(V_MAP, true, 1, "hds"))
        return true;

    FOR_MAP(setup["hds"], string, Variant, i) {
        Variant rollingLimit = MAP_VAL(i).GetValue("rollingLimit", false);
        MAP_VAL(i).RemoveKey("rollingLimit", false);
        MAP_VAL(i)["playlistLength"] = rollingLimit;

        bool rolling = (bool)MAP_VAL(i).GetValue("rolling", false);
        MAP_VAL(i).RemoveKey("rolling", false);
        if (rolling)
            MAP_VAL(i)["playlistType"] = "rolling";
        else
            MAP_VAL(i)["playlistType"] = "appending";
        MAP_VAL(i)["staleRetentionCount"] = rollingLimit;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_RENAME_ROLLING_LIMIT;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_15_16(Variant &setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["ssmIp"] = "";
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_SSMIP_TO_PULL;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_16_17(Variant &setup) {

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["rtmpAbsoluteTimestamps"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_RTMP_ABSOLUTE_TIMESTAMPS;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_17_18(Variant &setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["httpProxy"] = "";
    }

    FOR_MAP(setup["push"], string, Variant, i) {
        MAP_VAL(i)["httpProxy"] = "";
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_HTTP_PROXY;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_18_19(Variant &setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["rangeStart"] = (int64_t) - 2;
        MAP_VAL(i)["rangeEnd"] = (int64_t) - 1;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_RANGE_START_END;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_19_20(Variant &setup) {

    FOR_MAP(setup["process"], string, Variant, i) {
        MAP_VAL(i)["groupName"] = format("transcoded_group_%s", STR(generateRandomString(8)));
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_PROCESS_GROUPNAME;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_20_21(Variant &setup) {

    FOR_MAP(setup["record"], string, Variant, i) {
        MAP_VAL(i)["waitForIDR"] = (bool) false;
        MAP_VAL(i)["winQtCompat"] = (bool) true;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_RECORD_WAITFORIDR;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_21_22(Variant &setup) {

    FOR_MAP(setup["pull"], string, Variant, i) {
        MAP_VAL(i)["sendRenewStream"] = (bool)false;
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_SEND_RENEW_STREAM;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_22_23(Variant &setup) {

    FOR_MAP(setup["hls"], string, Variant, i) {
        bool encryptStream = (bool) MAP_VAL(i).GetValue("encryptStream", false);
        MAP_VAL(i).RemoveKey("encryptStream", false);
        MAP_VAL(i)["drmType"] = (string) (encryptStream ? DRM_TYPE_RMS : DRM_TYPE_NONE);
    }

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_VERIMATRIX_DRM;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_23_24(Variant &setup) {
    setup["mss"].IsArray(true);

    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_MSS;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_24_25(Variant &setup) {
    setup["dash"].IsArray(true);
    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_DASH;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_25_26(Variant &setup) {
    setup["webrtc"].IsArray(true);
    setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_WEBRTC;

    return true;
}

bool OriginApplication::UpgradePullPushConfigFile_26_27(Variant &setup) {
	setup["metalistener"].IsArray(true);
	setup["version"] = (uint32_t) PULL_PUSH_CONFIG_FILE_VER_ADD_METADATA;
	
	return true;
}

void OriginApplication::GatherStats(Variant &reportInfo) {

    if (_pVariantHandler->GetEdges().size() != 0)
        _pVariantHandler->QueryEdgeStatus(reportInfo);
    reportInfo["stats"]["origin"] = Variant();
    _pRTMPHandler->GetRtmpOutboundStats(reportInfo["stats"]["origin"]);
}

bool OriginApplication::PushMetadata(Variant &settings) {
	//INFO("*b2*: OA::PushMetadata: settings: %s", STR(settings.ToString()));
	// Do I care about this either??
	//SaveConfig(settings, OPERATION_TYPE_RECORD);
	// note: settings["pathToFile"]
	// fill in other stuff so ConnectCompletiion Callback can construct streams
	settings["name"] = GetName();
	string localStreamName = settings["streamName"];
	// ConnectToVmf will use TCPConnector to connect and stand up the protocol stack
	bool ok = _pOutVmfAppProtocolHandler->ConnectToVmf(localStreamName, settings);
	return ok;
}

bool OriginApplication::ShutdownMetadata(Variant &settings) {
	MetadataManager * pMM = GetMetadataManager();
	string streamName = settings["streamName"];
	vector<BaseOutStream *> streams = pMM->RemovePushStreams(streamName);
	FOR_VECTOR(streams, i) {
		BaseOutStream * bos = streams[i];
		bos->EnqueueForDelete();
	}
	return true;
}
#ifdef HAS_PROTOCOL_WEBRTC

bool OriginApplication::StartWebRTC(Variant & settings) {
	bool ok = false;
	bool isRespawned = settings.HasKey("reSpawned", false);
	bool isclientActivated = settings.HasKey("clientActivated", false);

	//UPDATE: use RRS field as identifier to prevent multiple config when the domain
	// is resolved into different IP addresses.
	// Note that "rrs" contains the IP address as well if there is no domain name passed
	string rrs = settings["rrs"];
	
	// Comcast request: resolve again the IP address of RRS
	// Only do this if it is not client activated requests/connections
	if (!isclientActivated) {
		DEBUG("Resolving %s...", STR(rrs));
		string ip = getHostByName(rrs);
		// UPDATE:
		// Sometimes there will be a case where DNS resolution failed when
		// RRS was respawned. In such case, we'll continue and do a retry.
		// Immediate return will only happen during user API requests.
		if (ip == "") {
			if (!isRespawned) {
				FATAL("Unable to resolve RRS: %s", STR(rrs));
				return false;
			} else {
				WARN("Unable to resolve RRS: %s, Retrying...", STR(rrs));
			}
		}
		// Assign the resolved IP to the proper field
		settings["rrsip"] = ip;
	} else {
		// Once the marker was used, remove it
		settings.RemoveKey("clientActivated", false);
	}
	
	// Sanity check/handler in case we missed something
	if (rrs == "") {
		rrs = (string) settings["rrsip"];
		settings["rrs"] = rrs;
	}

	// Check for exact duplicate, exit if it is
	int rrsport = (int) settings["rrsport"];
	string roomid = settings["roomid"];
	if (!isRespawned) {

		FOR_MAP(_configurations["webrtc"], string, Variant, i) {
			if ((rrs == (string) (MAP_VAL(i)["rrs"]))
					&& (rrsport == (int) MAP_VAL(i)["rrsport"])
					&& (roomid == (string) (MAP_VAL(i)["roomid"]))
					) {
				FATAL("Attempted duplicate StartWebRTC(%s:%d, %s)",
						STR(rrs), rrsport, STR(roomid));
				return false;
			}
		}
	}
	// now save our new config
	settings["operationType"] = (uint8_t) OPERATION_TYPE_WEBRTC;
	SaveConfig(settings, OPERATION_TYPE_WEBRTC);
    //SetStreamStatus(settings, SS_CONNECTING, 0);

	settings[CONF_APPLICATION_NAME] = GetName();

	// Get the certificate if available
	if (_configuration.HasKeyChain(V_STRING, false, 2, "webrtc", CONF_SSL_CERT)) {
		settings[CONF_SSL_CERT] = (string) (_configuration.GetValue("webrtc", false)
				.GetValue(CONF_SSL_CERT, false));
	}

	// Get the key if available
	if (_configuration.HasKeyChain(V_STRING, false, 2, "webrtc", CONF_SSL_KEY)) {
		settings[CONF_SSL_KEY] = (string) (_configuration.GetValue("webrtc", false)
				.GetValue(CONF_SSL_KEY, false));
	}

	if (_configuration.HasKeyChain(V_STRING, false, 2, "webrtc", "trustedCerts")) {
		settings["trustedCerts"] = (string)(_configuration.GetValue("webrtc", false)
			.GetValue("trustedCerts", false));
	}
	//settings["serverCert"] = unb64(TEST_CERT);

	// Check if we have a user-set rrsOverSsl
	if (!settings.HasKey("rrsOverSsl", false)) {
		// no user-set rrsOverSsl.  Just use the one in the
		// config.lua
		if (_configuration.HasKeyChain(V_BOOL, false, 2, "webrtc", "rrsOverSsl")) {
			settings["rrsOverSsl"] = (bool) (_configuration.GetValue("webrtc", false)
				.GetValue("rrsOverSsl", false));
		} else {
			// Default to false, do not use ssl
			settings["rrsOverSsl"] = (bool)false;
		}
	}

	if (_pWrtcAppProtocolHandler) {
		ok = _pWrtcAppProtocolHandler->StartWebRTC(settings);
	}

	return ok;
}

bool OriginApplication::StopWebRTC(Variant & settings) {
	bool ok = false;
	string rrsip = settings["rrsip"];
	int rrsport = (int) settings["rrsport"];
	string roomid = settings["roomid"];
	bool isIntentional = !settings.HasKey("keepAlive", false);
	bool isRespawned = settings.HasKey("reSpawned", false);
	INFO("WebRTC Stopping: Addr: %s:%d, Room: %s (blank=any)",
			STR(rrsip), rrsport, STR(roomid));

	//INFO("StopWebRTC: _configurations['webrtc'] %s",
	//		STR(_configurations["webrtc"].ToString()));
	// stash the set to remove so we don't change the map we are iterating over
	vector<uint32_t> toRemove;

    FOR_MAP(_configurations["webrtc"], string, Variant, i) {
    	// weed out a blank config that's coming in for some reason
    	uint32_t configId = (uint32_t)MAP_VAL(i)["configId"];
    	if (!configId) break; //============================>
    	// if we default (empty) or if we match on each parm
    	if (   (!rrsip.size()  || (rrsip == (string)(MAP_VAL(i)["rrsip"])) )
    	    && (!rrsport       || (rrsport == (int) MAP_VAL(i)["rrsport"]) )
    	    && (!roomid.size() || (roomid == (string)(MAP_VAL(i)["roomid"])) )
    		){
			MAP_VAL(i)["keepAlive"] = !isIntentional;
			toRemove.push_back(configId);
    		if (_pWrtcAppProtocolHandler) {
    			ok = _pWrtcAppProtocolHandler->StopWebRTC(configId);
    		}
    	}
    }
    // now remove from our list of configIds
	if (!isRespawned) {
		FOR_VECTOR(toRemove, i) {
			RemoveConfigId(toRemove[i], false);
		}
	}
	return ok;
}
#endif // HAS_PROTOCOL_WEBRTC

#ifdef HAS_PROTOCOL_METADATA
bool OriginApplication::AddMetadataListener(Variant & settings, TCPAcceptor* &pAcceptor) {
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_INBOUND_JSONMETADATA);
	settings[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_JSONMETADATA;

	string ip = settings["ip"];
	int port = (int)settings["port"];
	string localstreamname = settings["localStreamName"];
	
	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();
	
	FOR_MAP(services, uint32_t, IOHandler *, i) {
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pAcceptor = (TCPAcceptor *)MAP_VAL(i);
		if ((pAcceptor->GetParameters()[CONF_IP] == ip)
			 && ((uint16_t)pAcceptor->GetParameters()[CONF_PORT] == port)) {
			FATAL("%s:%u is taken", STR(ip), port);
			return false;
		}
	}
	// check for exact duplicate, exit if it is
	FOR_MAP(_configurations["metalistener"], string, Variant, i) {
		if ((ip == (string)(MAP_VAL(i)["ip"]))
			 && (port == (int)MAP_VAL(i)["port"])
			 && (localstreamname == (string)(MAP_VAL(i)["localStreamName"]))) {
			FATAL("Attempted duplicate metadata listener( %s:%d, %s)",
				STR(ip), port, STR(localstreamname));
			return false;
		}
	}

	pAcceptor = new TCPAcceptor(ip, (uint16_t)port, settings, chain);
	if (!pAcceptor->Bind()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Unable to bind acceptor");
		return false;
	}

	pAcceptor->SetApplication(this);
	if (!pAcceptor->StartAccept()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Unable to start acceptor");
		return false;
	}
	
	Variant stats;
	pAcceptor->GetStats(stats, 0);
	settings["acceptor"] = stats;
	// now save our new config
	settings["operationType"] = (uint8_t)OPERATION_TYPE_METADATA;
	SaveConfig(settings, OPERATION_TYPE_METADATA);
	//SetStreamStatus(settings, SS_CONNECTING, 0);
	
	settings[CONF_APPLICATION_NAME] = GetName();
	
	return true;

}
#endif //HAS_PROTOCOL_METADATA

bool OriginApplication::IsLazyPullActive(string streamName) {
	// Get the streams manager
	StreamsManager *pSM = GetStreamsManager();

	// Find presence of outbound stream
	map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(ST_OUT_NET,
			streamName, true, false);

	// If there is an outbound stream matching this stream name, then we know
	// that we still have a client connected to the lazy pulled stream
	if (streams.size() > 0) {
		return true;
	}

	return false;
}


// Restart TCPAcceptor.
// TODO: Duplicated portions of the CLIAppProcotocolHandler
//       ::ProcessMessageCreateService() method.  Consider
//       refactoring that method to save code.
bool OriginApplication::RestartService(Variant& params) {
	INFO("Restarting TCP listener: Protocol(%s), IP(%s), Port(%d)",
		STR(params["protocol"]),STR(params["ip"]),(int)params["port"]);

	string ip = params["ip"];
	if (getHostByName(ip) != ip) {
		return false;
	}

	int port = params["port"];
	string protocol = params["protocol"];
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(protocol);
	if (chain.size() == 0) {
		FATAL("invalid protocol (%s)", STR(protocol));
		return false;
	}
	if (chain[0] != PT_TCP) {
		FATAL("invalid protocol -- non-TCP (%s)", STR(protocol));
		return false;
	}

	//4. Do we already have a service bound on ip:port?
	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();

	FOR_MAP(services, uint32_t, IOHandler *, i) {
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pAcceptor = (TCPAcceptor *)MAP_VAL(i);
		if ((pAcceptor->GetParameters()[CONF_IP] == ip)
			&& ((uint16_t)pAcceptor->GetParameters()[CONF_PORT] == port)) {
			FATAL("%s:%u is taken", STR(ip), port);
			return false;
		}
	}

	//5. create the configuration
	Variant config;
	config[CONF_IP] = ip;
	config[CONF_PORT] = (uint16_t)port;
	config[CONF_PROTOCOL] = protocol;

	//6. Spawn the service
	TCPAcceptor *pAcceptor = new TCPAcceptor(ip, (uint16_t)port, config, chain);
	if (!pAcceptor->Bind()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Could not bind to port.");
		return false;
	}
	pAcceptor->SetApplication(this);
	if (!pAcceptor->StartAccept()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Could not accept connections.");
		return false;
	}
	return true;
}
