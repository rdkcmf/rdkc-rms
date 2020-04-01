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


#ifdef HAS_PROTOCOL_EXTERNAL
#include "protocols/external/externaljobtimerprotocol.h"
#include "protocols/external/externalmoduleapi.h"

ExternalJobTimerProtocol::ExternalJobTimerProtocol(protocol_factory_t *pFactory,
		void *pUserData)
: BaseTimerProtocol() {
	memset(&_timerInterface, 0, sizeof (_timerInterface));
	TimerIC *pTimerIC = new TimerIC();
	BaseIC *pFactoryIC = (BaseIC *) pFactory->context.pOpaque;
	pTimerIC->pAPI = pFactoryIC->pAPI;
	pTimerIC->pModule = pFactoryIC->pModule;
	pTimerIC->pApp = pFactoryIC->pApp;
	pTimerIC->pTimer = this;
	_timerInterface.context.pOpaque = pTimerIC;
	_timerInterface.context.pUserData = pUserData;
	_timerInterface.uniqueId = GetId();
	_timerInterface.pFactory = pFactory;
}

ExternalJobTimerProtocol::~ExternalJobTimerProtocol() {
	_timerInterface.pFactory->events.job.timerClosed(&_timerInterface);
	delete (TimerIC *) _timerInterface.context.pOpaque;
	_timerInterface.context.pOpaque = NULL;
}

bool ExternalJobTimerProtocol::TimePeriodElapsed() {
	_timerInterface.pFactory->events.job.timerPulse(&_timerInterface);
	return true;
}

jobtimer_t *ExternalJobTimerProtocol::GetInterface() {
	return &_timerInterface;
}

#endif /* HAS_PROTOCOL_EXTERNAL */
