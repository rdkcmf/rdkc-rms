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


#include "common.h"
#include "protocols/timer/heartbeattimerprotocol.h"
#include "application/lminterfaceapplication.h"
#include "application/lmcontroller.h"

using namespace app_lminterface;

HeartbeatTimerProtocol::HeartbeatTimerProtocol(LMController *pController)
: BaseTimerProtocol() {
	_pController = pController;
}

HeartbeatTimerProtocol::~HeartbeatTimerProtocol() {
}

bool HeartbeatTimerProtocol::TimePeriodElapsed() {
	o_assert(_pController);
	_pController->SendHeartBeat();
	return true;
}

