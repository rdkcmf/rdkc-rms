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


#include "protocols/timer/customtimer.h"
#include "eventlogger/eventlogger.h"
using namespace app_rdkcrouter;

CustomTimer::CustomTimer() {
	_oneShoot = true;
	_triggerEvents = false;
}

CustomTimer::~CustomTimer() {
	if (_triggerEvents)
		GetEventLogger()->LogTimerClosed(GetCustomParameters()["settings"]);
}

bool CustomTimer::TimePeriodElapsed() {
	uint32_t tc = (uint32_t) GetCustomParameters()["settings"]["triggerCount"];
	GetCustomParameters()["settings"]["triggerCount"] = tc + 1;
	if (_triggerEvents)
		GetEventLogger()->LogTimerTriggered(GetCustomParameters()["settings"]);
	if (_oneShoot)
		EnqueueForDelete();
	return true;
}

bool CustomTimer::Initialize(Variant &settings) {
	settings["timerId"] = GetId();
	settings["triggerCount"] = (uint32_t) 0;
	GetCustomParameters()["settings"] = settings;

	if (settings.HasKeyChain(_V_NUMERIC, false, 1, "value")) {
		_oneShoot = false;
		if (!EnqueueForTimeEvent((uint32_t) settings.GetValue("value", false))) {
			FATAL("Unable to enqueue timer for events");
			_triggerEvents = false;
			EnqueueForDelete();
			return false;
		}
		_triggerEvents = true;
	} else {
		_oneShoot = true;
		int64_t delta = (int64_t) (settings["value"].GetTimeT() - getutctime());
		if (delta <= 0) {
			_triggerEvents = false;
			GetEventLogger()->LogTimerCreated(GetCustomParameters()["settings"]);
			GetCustomParameters()["settings"]["triggerCount"] = 1;
			settings["triggerCount"] = 1;
			GetEventLogger()->LogTimerTriggered(GetCustomParameters()["settings"]);
			GetEventLogger()->LogTimerClosed(GetCustomParameters()["settings"]);
			EnqueueForDelete();
			return true;
		}
		if (!EnqueueForTimeEvent((uint32_t) delta)) {
			FATAL("Unable to enqueue timer for events");
			_triggerEvents = false;
			EnqueueForDelete();
			return false;
		}
		_triggerEvents = true;
	}

	if (_triggerEvents)
		GetEventLogger()->LogTimerCreated(GetCustomParameters()["settings"]);
	return true;
}
