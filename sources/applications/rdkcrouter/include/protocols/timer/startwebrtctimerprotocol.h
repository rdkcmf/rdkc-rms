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


#ifndef _StartWebRTCTIMERPROTOCOL_H
#define	_StartWebRTCTIMERPROTOCOL_H

#include "protocols/timer/basetimerprotocol.h"
#include "streaming/streamstatus.h"
#include "application/baseclientapplication.h"

namespace app_rdkcrouter {

    class StartWebRTCTimerProtocol
    : public BaseTimerProtocol {
    private:
    	map<uint32_t, Variant> _webrtcRequests;
        uint32_t _backoffTime;
    public:
        StartWebRTCTimerProtocol();
        virtual ~StartWebRTCTimerProtocol();

        virtual bool TimePeriodElapsed();
        
        void EnqueueOperation(Variant &request, OperationType operationType);
    private:
        bool DoRequests();
        uint32_t GetbackoffTime() { return _backoffTime;};
    };
}

#endif	/* _StartWebRTCTIMERPROTOCOL_H */

