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
#include "eventlogger/baseeventsink.h"
#include "eventlogger/sinktypes.h"
#include "application/baseclientapplication.h"

BaseEventSink::BaseEventSink(uint64_t sinkType) {
    _sinkType = sinkType;
    _pEvtLogger = NULL;
}

BaseEventSink::~BaseEventSink() {

}

bool BaseEventSink::Initialize(Variant &config, Variant &customData) {
    //1. Check if event filter is present
    if (config.HasKeyChain(V_MAP, false, 1, "enabledEvents")
            && config["enabledEvents"].IsArray()) {
        //1.1 Register all enabled events to local map

        FOR_MAP(config["enabledEvents"], string, Variant, i) {
            ADD_VECTOR_END(_enabledEvents, MAP_VAL(i));
        }

    } else {
        //2. if event filter is not present, set all events as enabled
        ADD_VECTOR_END(_enabledEvents, "inStreamCreated");
        ADD_VECTOR_END(_enabledEvents, "outStreamCreated");
        ADD_VECTOR_END(_enabledEvents, "streamCreated");
        ADD_VECTOR_END(_enabledEvents, "inStreamCodecsUpdated");
        ADD_VECTOR_END(_enabledEvents, "outStreamCodecsUpdated");
        ADD_VECTOR_END(_enabledEvents, "streamCodecsUpdated");
        ADD_VECTOR_END(_enabledEvents, "inStreamClosed");
        ADD_VECTOR_END(_enabledEvents, "outStreamClosed");
        ADD_VECTOR_END(_enabledEvents, "streamClosed");
        ADD_VECTOR_END(_enabledEvents, "cliRequest");
        ADD_VECTOR_END(_enabledEvents, "cliResponse");
        ADD_VECTOR_END(_enabledEvents, "applicationStart");
        ADD_VECTOR_END(_enabledEvents, "applicationStop");
        ADD_VECTOR_END(_enabledEvents, "carrierCreated");
        ADD_VECTOR_END(_enabledEvents, "carrierClosed");
        ADD_VECTOR_END(_enabledEvents, "serverStarted");
        ADD_VECTOR_END(_enabledEvents, "serverStopping");
        ADD_VECTOR_END(_enabledEvents, "protocolRegisteredToApp");
        ADD_VECTOR_END(_enabledEvents, "protocolUnregisteredFromApp");
        ADD_VECTOR_END(_enabledEvents, "processStarted");
        ADD_VECTOR_END(_enabledEvents, "processStopped");
        ADD_VECTOR_END(_enabledEvents, "timerCreated");
        ADD_VECTOR_END(_enabledEvents, "timerTriggered");
        ADD_VECTOR_END(_enabledEvents, "timerClosed");
#ifdef HAS_PROTOCOL_HLS
        ADD_VECTOR_END(_enabledEvents, "hlsChunkCreated");
        ADD_VECTOR_END(_enabledEvents, "hlsChunkClosed");
        ADD_VECTOR_END(_enabledEvents, "hlsChunkError");
        ADD_VECTOR_END(_enabledEvents, "hlsChildPlaylistUpdated");
        ADD_VECTOR_END(_enabledEvents, "hlsMasterPlaylistUpdated");
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
        ADD_VECTOR_END(_enabledEvents, "hdsChunkCreated");
        ADD_VECTOR_END(_enabledEvents, "hdsChunkClosed");
        ADD_VECTOR_END(_enabledEvents, "hdsChunkError");
        ADD_VECTOR_END(_enabledEvents, "hdsChildPlaylistUpdated");
        ADD_VECTOR_END(_enabledEvents, "hdsMasterPlaylistUpdated");
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
        ADD_VECTOR_END(_enabledEvents, "mssChunkCreated");
        ADD_VECTOR_END(_enabledEvents, "mssChunkClosed");
        ADD_VECTOR_END(_enabledEvents, "mssChunkError");
        ADD_VECTOR_END(_enabledEvents, "mssPlaylistUpdated");
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        ADD_VECTOR_END(_enabledEvents, "dashChunkCreated");
        ADD_VECTOR_END(_enabledEvents, "dashChunkClosed");
        ADD_VECTOR_END(_enabledEvents, "dashChunkError");
        ADD_VECTOR_END(_enabledEvents, "dashPlaylistUpdated");
#endif /* HAS_PROTOCOL_DASH */
#ifdef HAS_WEBSERVER
		ADD_VECTOR_END(_enabledEvents, "mediaFileDownloaded");
		ADD_VECTOR_END(_enabledEvents, "streamingSessionStarted");
		ADD_VECTOR_END(_enabledEvents, "streamingSessionEnded");
#endif /* HAS_WEBSERVER */
#ifdef HAS_PROTOCOL_WEBRTC
        ADD_VECTOR_END(_enabledEvents, "webRtcCommandReceived");
		ADD_VECTOR_END(_enabledEvents, "webRtcServiceConnected");
#endif /* HAS_PROTOCOL_WEBRTC */
        // record section
        ADD_VECTOR_END(_enabledEvents, "recordChunkCreated");
        ADD_VECTOR_END(_enabledEvents, "recordChunkClosed");
        ADD_VECTOR_END(_enabledEvents, "recordChunkError");
        // record section end
        // event to indicate that the audio/video feed stopped
        ADD_VECTOR_END(_enabledEvents, "audioFeedStopped");
        ADD_VECTOR_END(_enabledEvents, "videoFeedStopped");
	
		ADD_VECTOR_END(_enabledEvents, "pullStreamFailed");
	}

    if (config.HasKey("customData", false)) {
        _customData = config.GetValue("customData", false);
    }

    if (_customData == V_NULL)
        _customData = customData;

    return true;
}

Variant &BaseEventSink::GetCustomData() {
    return _customData;
}

bool BaseEventSink::CanLogEvent(string eventType) {

    FOR_VECTOR_ITERATOR(string, _enabledEvents, i) {
        if (VECTOR_VAL(i) == eventType)
            return true;
    }
    return false;
}

uint64_t BaseEventSink::GetSinkType() {
    return _sinkType;
}
