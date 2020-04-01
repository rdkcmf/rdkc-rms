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


#include "eventlogger/eventlogger.h"
#include "eventlogger/logfilesink.h"
#include "eventlogger/rpcsink.h"
#include "eventlogger/sinktypes.h"
#include "eventlogger/consolesink.h"
#include "application/clientapplicationmanager.h"
#include "streaming/basestream.h"
#include "streaming/streamstypes.h"
#include "protocols/timer/basetimerprotocol.h"
#include "streaming/baseoutstream.h"

uint32_t EventLogger::_idGen = 0;
EventLogger EventLogger::_defaultLogger;

#define VALIDATE_LOGGING do { \
	if ((_eventSinkTable.size() == 0) && \
			(_pApplication != NULL) && \
			(_pApplication->IsOrigin())) \
		return; \
	_message.Reset(); \
	} while(0)

class ServerShutdownTimerProtocol
: public BaseTimerProtocol {
private:
    bool _trigered;
public:

    ServerShutdownTimerProtocol() {
        _trigered = false;
    }

    virtual ~ServerShutdownTimerProtocol() {

    }

    virtual bool TimePeriodElapsed() {
        if (_trigered)
            return true;
        _trigered = true;
        IOHandlerManager::SignalShutdown();
        EnqueueForDelete();
        return true;
    }
};

EventLogger::EventLogger() {
    _pApplication = NULL;
    _message.IsArray(false);
    _id = _idGen++;
    _eventId = 1;
}

EventLogger::~EventLogger() {

    FOR_VECTOR(_eventSinkTable, i) {
        delete _eventSinkTable[i];
    }
    _eventSinkTable.clear();
}

EventLogger *EventLogger::GetDefaultLogger() {
    return &_defaultLogger;
}

bool EventLogger::InitializeDefaultLogger(Variant &config) {
    return _defaultLogger.Initialize(config, NULL);
}

void EventLogger::SignalShutdown() {
    Variant message;
    message["header"] = "prepareForShutdown";
    message["payload"] = Variant();
    ClientApplicationManager::BroadcastMessageToAllApplications(message);

    bool _hasRpc = false;
    _defaultLogger.LogServerStopping();
    _hasRpc |= _defaultLogger.EnabledForEvent(SINK_RPC, "serverStopping");

    map<uint32_t, BaseClientApplication *> apps = ClientApplicationManager::GetAllApplications();

    FOR_MAP(apps, uint32_t, BaseClientApplication *, i) {
        if (MAP_VAL(i) == NULL)
            continue;
        EventLogger *pEvtLog = MAP_VAL(i)->GetEventLogger();
        if (pEvtLog == NULL)
            continue;
        pEvtLog->LogServerStopping();
        _hasRpc |= pEvtLog->EnabledForEvent(SINK_RPC, "serverStopping");
    }

    if (!_hasRpc) {
        IOHandlerManager::SignalShutdown();
        return;
    }

    ServerShutdownTimerProtocol *pProtocol = new ServerShutdownTimerProtocol();
    pProtocol->EnqueueForTimeEvent(5);
}

uint32_t EventLogger::GetId() {
    return _id;
}

bool EventLogger::Initialize(Variant &config, BaseClientApplication *pApplication) {
    if (config.HasKey("customData", false)) {
        _customData = config.GetValue("customData", false);
    }

    if ((_customData == V_NULL)&&(this != &_defaultLogger))
        _customData = _defaultLogger._customData;

    _pApplication = pApplication;
    if (_pApplication->IsOrigin()) {
        if (!config.HasKeyChain(V_MAP, false, 1, "sinks")) {
            WARN("no sinks provided for the event logger");
            return true;
        }

        Variant &sinks = config.GetValue("sinks", false);
        if (sinks.MapSize() == 0) {
            WARN("no sinks provided for the event logger");
            return true;
        }

        FOR_MAP(sinks, string, Variant, i) {
            BaseEventSink *pSink = SpawnEventSink(MAP_VAL(i));
            if (pSink == NULL) {
                WARN("Unable to initialize event sink:\n%s", STR(MAP_VAL(i).ToString()));
                continue;
            }
            ADD_VECTOR_END(_eventSinkTable, pSink);
        }
    }
    return true;
}

Variant &EventLogger::GetCustomData() {
    return _customData;
}

void EventLogger::LogStreamCreated(BaseStream *pStream) {
    if (pStream == NULL)
        return;
    VALIDATE_LOGGING;
    if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
        if (!CanLogEvent("inStreamCreated"))
            return;
        _message["type"] = "inStreamCreated";
    } else if (TAG_KIND_OF(pStream->GetType(), ST_OUT)) {
        if (!CanLogEvent("outStreamCreated"))
            return;
        _message["type"] = "outStreamCreated";
    } else {
        if (!CanLogEvent("streamCreated"))
            return;
        _message["type"] = "streamCreated";
    }
    pStream->GetStats(_message["payload"]);
    Log(_message);
}

void EventLogger::LogStreamCodecsUpdated(BaseStream *pStream) {
    if (pStream == NULL)
        return;
    VALIDATE_LOGGING;
    if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
        if (!CanLogEvent("inStreamCodecsUpdated"))
            return;
        _message["type"] = "inStreamCodecsUpdated";
    } else if (TAG_KIND_OF(pStream->GetType(), ST_OUT)) {
        if (!CanLogEvent("outStreamCodecsUpdated"))
            return;
        _message["type"] = "outStreamCodecsUpdated";
    } else {
        if (!CanLogEvent("streamCodecsUpdated"))
            return;
        _message["type"] = "streamCodecsUpdated";
    }
    pStream->GetStats(_message["payload"]);
    Log(_message);
}

void EventLogger::LogStreamClosed(BaseStream *pStream) {
    if (pStream == NULL)
        return;
    VALIDATE_LOGGING;
    if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
        if (!CanLogEvent("inStreamClosed"))
            return;
        _message["type"] = "inStreamClosed";
    } else if (TAG_KIND_OF(pStream->GetType(), ST_OUT)) {
        if (!CanLogEvent("outStreamClosed"))
            return;
        _message["type"] = "outStreamClosed";
    } else {
        if (!CanLogEvent("streamClosed"))
            return;
        _message["type"] = "streamClosed";
    }
    pStream->GetStats(_message["payload"]);
    Log(_message);
}

void EventLogger::LogAudioFeedStopped(BaseStream *pStream) {
    if (pStream == NULL)
        return;
    VALIDATE_LOGGING;
    if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
        if (!CanLogEvent("audioFeedStopped"))
            return;
        _message["type"] = "audioFeedStopped";
    } 
    _message["payload"]["id"] = pStream->GetUniqueId();
    _message["payload"]["localStreamName"] = pStream->GetName();
    Log(_message);
}

void EventLogger::LogVideoFeedStopped(BaseStream *pStream) {
    if (pStream == NULL)
        return;
    VALIDATE_LOGGING;
    if (TAG_KIND_OF(pStream->GetType(), ST_IN)) {
        if (!CanLogEvent("videoFeedStopped"))
            return;
        _message["type"] = "videoFeedStopped";
    } 
    _message["payload"]["id"] = pStream->GetUniqueId();
    _message["payload"]["localStreamName"] = pStream->GetName();
    Log(_message);
}

void EventLogger::LogCLIRequest(Variant &request) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("cliRequest"))
        return;
    _message["type"] = "cliRequest";
    _message["payload"] = request;
    Log(_message);
}

void EventLogger::LogCLIResponse(Variant &response) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("cliResponse"))
        return;
    _message["type"] = "cliResponse";
    _message["payload"] = response;
    Log(_message);
}

void EventLogger::LogApplicationStart(BaseClientApplication *pApplication) {
    VALIDATE_LOGGING;
    if (pApplication == NULL)
        return;
    if (!CanLogEvent("applicationStart"))
        return;
    _message["type"] = "applicationStart";
    _message["payload"]["id"] = pApplication->GetId();
    _message["payload"]["name"] = pApplication->GetName();
    _message["payload"]["config"] = pApplication->GetConfiguration();
    Log(_message);
}

void EventLogger::LogApplicationStop(BaseClientApplication *pApplication) {
    VALIDATE_LOGGING;
    if (pApplication == NULL)
        return;
    if (!CanLogEvent("applicationStop"))
        return;
    _message["type"] = "applicationStop";
    _message["payload"]["id"] = pApplication->GetId();
    _message["payload"]["name"] = pApplication->GetName();
    _message["payload"]["config"] = pApplication->GetConfiguration();
    Log(_message);
}

void EventLogger::LogCarrierCreated(Variant &stats) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("carrierCreated"))
        return;
    _message["type"] = "carrierCreated";
    _message["payload"] = stats;
    Log(_message);
}

void EventLogger::LogCarrierClosed(Variant &stats) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("carrierClosed"))
        return;
    _message["type"] = "carrierClosed";
    _message["payload"] = stats;
    Log(_message);
}

void EventLogger::LogProtocolRegisteredToApplication(BaseProtocol *pProtocol) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("protocolRegisteredToApp"))
        return;
    _message["type"] = "protocolRegisteredToApp";
    _message["payload"]["protocolType"] = tagToString(pProtocol->GetType());
    _message["payload"]["customParameters"] = pProtocol->GetCustomParameters();
    _message["payload"]["id"] = pProtocol->GetId();
    Log(_message);
}

void EventLogger::LogProtocolUnregisteredFromApplication(BaseProtocol *pProtocol) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("protocolUnregisteredFromApp"))
        return;
    _message["type"] = "protocolUnregisteredFromApp";
    _message["payload"]["protocolType"] = tagToString(pProtocol->GetType());
    _message["payload"]["customParameters"] = pProtocol->GetCustomParameters();
    _message["payload"]["id"] = pProtocol->GetId();
    Log(_message);
}

void EventLogger::LogServerStarted() {
    VALIDATE_LOGGING;
    if (!CanLogEvent("serverStarted"))
        return;
    _message["type"] = "serverStarted";
    _message["payload"] = Version::GetAll();
    Log(_message);
}

void EventLogger::LogServerStopping() {
    VALIDATE_LOGGING;
    if (!CanLogEvent("serverStopping"))
        return;
    _message["type"] = "serverStopping";
    _message["payload"] = Version::GetAll();
    Log(_message);
}

void EventLogger::LogProcessStarted(Variant &processInfo) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("processStarted"))
        return;
    _message["type"] = "processStarted";
    _message["payload"] = processInfo;
    Log(_message);
}

void EventLogger::LogProcessStopped(Variant &processInfo) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("processStopped"))
        return;
    _message["type"] = "processStopped";
    _message["payload"] = processInfo;
    Log(_message);
}

void EventLogger::LogTimerCreated(Variant &info) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("timerCreated"))
        return;
    _message["type"] = "timerCreated";
    _message["payload"] = info;
    Log(_message);
}

void EventLogger::LogTimerTriggered(Variant &info) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("timerTriggered"))
        return;
    _message["type"] = "timerTriggered";
    _message["payload"] = info;
    Log(_message);
}

void EventLogger::LogTimerClosed(Variant &info) {
    VALIDATE_LOGGING;
    if (!CanLogEvent("timerClosed"))
        return;
    _message["type"] = "timerClosed";
    _message["payload"] = info;
    Log(_message);
}

#ifdef HAS_PROTOCOL_HLS

void EventLogger::LogHLSChunkCreated(const string filename, double playlistStartTs,
        double segmentStartTs) {
    VALIDATE_LOGGING;
    if (_eventSinkTable.size() == 0)
        return;
    string type = "hlsChunkCreated";
    if (!CanLogEvent(type))
        return;
    _message.Reset();
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    _message["payload"]["playlistStartTs"] = playlistStartTs;
    _message["payload"]["segmentStartTs"] = segmentStartTs;
    Log(_message);
}

void EventLogger::LogHLSChunkClosed(const string filename, double playlistStartTs,
        double segmentStartTs, double segmentDuration) {
    VALIDATE_LOGGING;
    if (_eventSinkTable.size() == 0)
        return;
    string type = "hlsChunkClosed";
    if (!CanLogEvent(type))
        return;
    _message.Reset();
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    _message["payload"]["playlistStartTs"] = playlistStartTs;
    _message["payload"]["segmentStartTs"] = segmentStartTs;
    _message["payload"]["segmentDuration"] = segmentDuration;
    Log(_message);
}

void EventLogger::LogHLSChunkError(string errorMsg) {
    VALIDATE_LOGGING;
    string type = "hlsChunkError";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["error"] = errorMsg;
    Log(_message);
}

void EventLogger::LogHLSChildPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "hlsChildPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

void EventLogger::LogHLSMasterPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "hlsMasterPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

#endif /* HAS_PROTOCOL_HLS */

#ifdef HAS_PROTOCOL_HDS

void EventLogger::LogHDSChunkCreated(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "hdsChunkCreated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    Log(_message);
}

void EventLogger::LogHDSChunkClosed(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "hdsChunkClosed";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "stopTime")) {
        _message["payload"]["stopTime"] = chunkInfo["stopTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "frameCount")) {
        _message["payload"]["frameCount"] = chunkInfo["frameCount"];
    }
    Log(_message);
}

void EventLogger::LogHDSChunkError(string errorMsg) {
    VALIDATE_LOGGING;
    string type = "hdsChunkError";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["error"] = errorMsg;
    Log(_message);
}

void EventLogger::LogHDSChildPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "hdsChildPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

void EventLogger::LogHDSMasterPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "hdsMasterPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

#endif /* HAS_PROTOCOL_HDS */

#ifdef HAS_PROTOCOL_MSS

void EventLogger::LogMSSChunkCreated(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "mssChunkCreated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    Log(_message);
}

void EventLogger::LogMSSChunkClosed(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "mssChunkClosed";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "stopTime")) {
        _message["payload"]["stopTime"] = chunkInfo["stopTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "frameCount")) {
        _message["payload"]["frameCount"] = chunkInfo["frameCount"];
    }
    Log(_message);
}

void EventLogger::LogMSSChunkError(string errorMsg) {
    VALIDATE_LOGGING;
    string type = "mssChunkError";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["error"] = errorMsg;
    Log(_message);
}

void EventLogger::LogMSSPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "mssPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH

void EventLogger::LogDASHChunkCreated(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "dashChunkCreated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    Log(_message);
}

void EventLogger::LogDASHChunkClosed(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "dashChunkClosed";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "stopTime")) {
        _message["payload"]["stopTime"] = chunkInfo["stopTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "frameCount")) {
        _message["payload"]["frameCount"] = chunkInfo["frameCount"];
    }
    Log(_message);
}

void EventLogger::LogDASHChunkError(string errorMsg) {
    VALIDATE_LOGGING;
    string type = "dashChunkError";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["error"] = errorMsg;
    Log(_message);
}

void EventLogger::LogDASHPlaylistUpdated(string filename) {
    VALIDATE_LOGGING;
    string type = "dashPlaylistUpdated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    _message["payload"]["file"] = filename;
    Log(_message);
}

#endif /* HAS_PROTOCOL_DASH */

#ifdef HAS_WEBSERVER
void EventLogger::LogMediaFileDownloaded(Variant &sessionInfo) {
	VALIDATE_LOGGING;
	string type = "mediaFileDownloaded";
	if (!CanLogEvent(type))
		return;
	_message["type"] = type;
	_message["payload"]["clientIP"] = sessionInfo["clientIP"];
	_message["payload"]["mediaFile"] = sessionInfo["mediaFile"];
	_message["payload"]["startTime"] = sessionInfo["startTime"];
	_message["payload"]["stopTime"] = sessionInfo["stopTime"];
	_message["payload"]["elapsed"] = sessionInfo["elapsed"];
	Log(_message);
}

void EventLogger::LogStreamingSessionStarted(Variant &sessionInfo) {
	VALIDATE_LOGGING;
	string type = "streamingSessionStarted";
	if (!CanLogEvent(type))
		return;
	_message["type"] = type;
	_message["payload"]["clientIP"] = sessionInfo["clientIP"];
	_message["payload"]["sessionId"] = sessionInfo["sessionId"];
	_message["payload"]["playlist"] = sessionInfo["playlist"];
	_message["payload"]["startTime"] = sessionInfo["startTime"];
	Log(_message);
}

void EventLogger::LogStreamingSessionEnded(Variant &sessionInfo) {
	VALIDATE_LOGGING;
	string type = "streamingSessionEnded";
	if (!CanLogEvent(type))
		return;
	_message["type"] = type;
	_message["payload"]["clientIP"] = sessionInfo["clientIP"];
	_message["payload"]["sessionId"] = sessionInfo["sessionId"];
	_message["payload"]["playlist"] = sessionInfo["playlist"];
	_message["payload"]["startTime"] = sessionInfo["startTime"];
	_message["payload"]["stopTime"] = sessionInfo["stopTime"];
	Log(_message);
}

#endif /* HAS_WEBSERVER */
// RECORD section

void EventLogger::LogRecordChunkCreated(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    if (_eventSinkTable.size() == 0)
        return;
    string type = "recordChunkCreated";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
	// add user variables to event
	if (chunkInfo.MapSize() > 0) {
		FOR_MAP(chunkInfo, string, Variant, i) {
			if (MAP_KEY(i).substr(0, 1) == "_") {
				_message["payload"][MAP_KEY(i)] = MAP_VAL(i);
			}
		}
	}
    Log(_message);
}

void EventLogger::LogRecordChunkClosed(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "recordChunkClosed";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "file")) {
        _message["payload"]["file"] = chunkInfo["file"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "startTime")) {
        _message["payload"]["startTime"] = chunkInfo["startTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "stopTime")) {
        _message["payload"]["stopTime"] = chunkInfo["stopTime"];
    }
    if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "frameCount")) {
        _message["payload"]["frameCount"] = chunkInfo["frameCount"];
    }
	// add user variables to event
	if (chunkInfo.MapSize() > 0) {
		FOR_MAP(chunkInfo, string, Variant, i) {
			if (MAP_KEY(i).substr(0, 1) == "_") {
				_message["payload"][MAP_KEY(i)] = MAP_VAL(i);
			}
		}
	}
    Log(_message);
}

void EventLogger::LogRecordChunkError(Variant &chunkInfo) {
    VALIDATE_LOGGING;
    string type = "recordChunkError";
    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
	if (!chunkInfo.HasKeyChain(V_MAP, false, 1, "errorMsg")) {
		_message["payload"]["error"] = chunkInfo["errorMsg"];
	}
	// add user variables to event
	if (chunkInfo.MapSize() > 0) {
		FOR_MAP(chunkInfo, string, Variant, i) {
			if (MAP_KEY(i).substr(0, 1) == "_") {
				_message["payload"][MAP_KEY(i)] = MAP_VAL(i);
			}
		}
	}
    Log(_message);
}
// RECORD section end

void EventLogger::LogRTMPPlaylistItemStart(Variant &info) {
    VALIDATE_LOGGING;
	uint32_t index = 0;
	uint32_t itemCount = 0;
	if (info.HasKeyChain(_V_NUMERIC, false, 1, "index")) {
		index = info["index"];
	}
	if (info.HasKeyChain(_V_NUMERIC, false, 1, "itemCount")) {
		itemCount = info["itemCount"];
	}
    string type = "playlistItemStart";
	if (index == 0 && itemCount != 1)
		type = "firstPlaylistItemStart";
	else if (index == itemCount - 1)
		type = "lastPlaylistItemStart";

    if (!CanLogEvent(type))
        return;
    _message["type"] = type;
	if (info.HasKeyChain(V_STRING, false, 1, "playlistName")) {
		_message["payload"]["playlistName"] = info["playlistName"];
	} else {
		_message["payload"]["playlistName"] = "-";
	}
    Log(_message);
}

void EventLogger::LogWebRTCCommandReceived(Variant &commandInfo) {
	VALIDATE_LOGGING;

	string type = "webRtcCommandReceived";
	if (!CanLogEvent(type))
		return;

	_message.Reset();
	_message["type"] = type;
	_message["payload"]["command"] = commandInfo["payload"]["command"];
	_message["payload"]["argument"] = commandInfo["payload"]["argument"];
	_message["payload"]["base64"] = commandInfo["payload"]["base64"];
	_message["payload"]["baseStreamName"] = 
		commandInfo["payload"]["baseStreamName"];
	_message["payload"]["instanceStreamName"] =
		commandInfo["payload"]["instanceStreamName"];

	FINEST("Event to be sent: %s", STR(_message.ToString()));
	Log(_message);
}

void EventLogger::LogWebRtcServiceConnected(Variant &params) {
	VALIDATE_LOGGING;
	string type = "webRtcServiceConnected";
	if (!CanLogEvent(type))
		return;
	_message.Reset();
	_message["type"] = type;
	_message["payload"]["rrsId"] = params["rrsid"];
	_message["payload"]["rrs"] = params["rrs"];
	_message["payload"]["rrsIp"] = params["rrsip"];
	_message["payload"]["rrsPort"] = params["rrsport"];
	_message["payload"]["roomId"] = params["roomid"];
	_message["payload"]["configId"] = params["configId"];
	Log(_message);
}

void EventLogger::LogPullStreamFailed(Variant &details) {
	VALIDATE_LOGGING;

	string type = "pullStreamFailed";
	if (!CanLogEvent(type))
		return;

	_message.Reset();
	_message["type"] = type;
	_message["payload"] = details;

	Log(_message);
}

void EventLogger::LogPlayerJoinedRoom(Variant& payload)
{
    VALIDATE_LOGGING;
    _message["type"] = "playerJoinedRoom";
    _message["payload"]["roomName"] = payload["roomName"];
    _message["payload"]["clientName"] = payload["clientName"];
    _message["payload"]["clientID"] = payload["clientID"];
    Log(_message);
}

void EventLogger::LogOutStreamFirstFrameSent (BaseOutStream* pOutStream)
{
    if (pOutStream == NULL){
        return;
    }
    VALIDATE_LOGGING;
    _message["type"] = "outStreamFirstFrameSent";
    pOutStream->GetStats(_message["payload"]);
    Log(_message);
}

bool EventLogger::EnabledForEvent(uint64_t sinkType, string eventType) {
    for (uint32_t i = 0; i < _eventSinkTable.size(); i++) {
        if (_eventSinkTable[i]->GetSinkType() != sinkType)
            continue;
        if (!_eventSinkTable[i]->CanLogEvent(eventType))
            continue;
        return true;
    }
    return false;
}

bool EventLogger::CanLogEvent(string eventType) {

    if (_pApplication != NULL && _pApplication->IsEdge())
        return true;
    //1. Search the cache
    map<string, bool>::iterator i1 = _canLogEventCache.find(eventType);
    if (i1 != _canLogEventCache.end())
        return MAP_VAL(i1);

    bool result = false;

    //2. query all the sinks, but break on the first true value
    //that means we have to build the message anyway because there is
    //at least one sink needing it

    FOR_VECTOR(_eventSinkTable, i) {
        result |= _eventSinkTable[i]->CanLogEvent(eventType);
        if (result)
            break;
    }

    //3. Store the flag
    _canLogEventCache[eventType] = result;

    //4. Done
    return result;
}

void EventLogger::LogEdgeRelayedEvent(Variant &edgeEvent) {
    if (edgeEvent.HasKeyChain(V_STRING, false, 1, "type")) {
        string eventType = (string) edgeEvent.GetValue("type", false);
#ifndef HAS_WEBSERVER
		if (!CanLogEvent(eventType))
			return;
#else
		if (eventType == "streamingSessionStarted")
			LogStreamingSessionStarted(edgeEvent);
		else if (eventType == "streamingSessionEnded")
			LogStreamingSessionEnded(edgeEvent);
		else if (eventType == "mediaFileDownloaded")
			LogMediaFileDownloaded(edgeEvent);
		else
#endif
        Log(edgeEvent);
    }
}

void EventLogger::Log(Variant &event) {
    if (!event.HasKeyChain(_V_NUMERIC, false, 2, "payload", "PID"))
        event["payload"]["PID"] = GetPid();
    if (_pApplication != NULL && _pApplication->IsEdge()) {
        Variant message;
        message["header"] = "eventLog";
        message["payload"] = event;
        _pApplication->SignalApplicationMessage(message);
    } else {
        event["id"] = (uint64_t) _eventId++;
        event["timestamp"] = (uint64_t) getutctime();
        if (!event.HasKeyChain(_V_NUMERIC, false, 1, "processId"))
            event["processId"] = GetPid();

        FOR_VECTOR(_eventSinkTable, i) {
            event["payload"]["customData"] = _eventSinkTable[i]->GetCustomData();
            if (_pApplication != NULL) {
                _pApplication->EventLogEntry(_eventSinkTable[i], event);
            } else {
                if ((event != V_MAP) && (event != V_TYPED_MAP))
                    continue;
                _eventSinkTable[i]->Log(event);
            }
        }
    }
}

BaseEventSink * EventLogger::SpawnEventSink(Variant &config) {
    BaseEventSink * pResult = NULL;

    if (!config.HasKeyChain(V_STRING, false, 1, "type")) {
        FATAL("No type provided for the events sink");
        return NULL;
    }

    string type = lowerCase((string) config.GetValue("type", false));
    if (type == "file") {
        pResult = new LogFileSink();
    } else if (type == "rpc") {
        pResult = new RpcSink();
    } else if (type == "console") {
        pResult = new ConsoleSink();
    } else {
        FATAL("Event sink type not recognized: %s", STR(type));
        return NULL;
    }

    if (pResult != NULL) {
        if ((&_defaultLogger == this) && (pResult->GetSinkType() == SINK_RPC)) {
            WARN("RPC event sink not supported on global event logger");
            delete pResult;
            pResult = NULL;
        } else {
            if (!pResult->Initialize(config, _customData)) {
                FATAL("Unable to initialize event sink");
                delete pResult;
                pResult = NULL;
            }
        }
    }

    if ((pResult != NULL) && (_pApplication != NULL))
        _pApplication->SignalEventSinkCreated(pResult);

    return pResult;
}
