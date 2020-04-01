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


#include "protocols/timer/startwebrtctimerprotocol.h"
#include "application/baseclientapplication.h"
#include "application/originapplication.h"
#include "application/rdkcrouter.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "mediaformats/readers/streammetadataresolver.h"
using namespace app_rdkcrouter;

StartWebRTCTimerProtocol::StartWebRTCTimerProtocol() {
    _backoffTime = 0;
}

StartWebRTCTimerProtocol::~StartWebRTCTimerProtocol() {
}

bool StartWebRTCTimerProtocol::TimePeriodElapsed() {
        return DoRequests();
}

void StartWebRTCTimerProtocol::EnqueueOperation(Variant &request,
        OperationType operationType) {
    switch (operationType) {
		case OPERATION_TYPE_WEBRTC:
		{
			_webrtcRequests[(uint32_t)request["configId"]] = request;
            break;
		}
        default:
        {
            WARN("Invalid connection type: %d", operationType);
            return;
        }
    }
}

bool StartWebRTCTimerProtocol::DoRequests() {
    static uint32_t _requestcounter = 0;
    static uint32_t _maxrequestcounter = 2;
    SRAND();
    
    //keep clearing incoming requests until backoff is reset
    if( _backoffTime > 0 ) {
        _backoffTime--;
        return true;
    }

    //1. Do the pull requests
    map<uint32_t, Variant> temp = _webrtcRequests;
	_webrtcRequests.clear();

	FOR_MAP(temp, uint32_t, Variant, i) {
		MAP_VAL(i)["reSpawned"] = true;
		// Based on testing, we can safely remove the stopwebrtc and/or removeconfig
		// Duplicate would no longer happen since the saveconfig already returns
		// immediately if there is a presence of config ID
        _requestcounter++;

        //check if we reached max request counter then randomize back off time and number of requests
        if ( _requestcounter >= _maxrequestcounter) {
            _backoffTime = (rand()%10 + 1);
            _maxrequestcounter = (rand()%2 + 1);
            _requestcounter = 0;
            INFO("StartWebRTCTimerProtocol::_backoffTime : %d : _maxrequestcounter : %d",_backoffTime,_maxrequestcounter);
        }
        
		((OriginApplication *)GetApplication())->StartWebRTC(MAP_VAL(i));
    }

    //Done
    return true;
}

