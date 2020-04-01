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



#ifndef _TIMERSMANAGER_H
#define	_TIMERSMANAGER_H

#include "platform/platform.h"

struct TimerEvent {
	uint32_t period;
	uint64_t triggerTime;
	uint32_t id;
	void *pUserData;
	operator string();
};

typedef bool (*ProcessTimerEvent)(TimerEvent &event);

class DLLEXP TimersManager {
private:
	ProcessTimerEvent _processTimerEvent;
	map<uint64_t, map<uint32_t, TimerEvent *> > _timers;
	uint64_t _lastTime;
	uint64_t _currentTime;
	bool _processResult;
	bool _processing;
public:
	TimersManager(ProcessTimerEvent processTimerEvent);
	virtual ~TimersManager();
	void AddTimer(TimerEvent &timerEvent);
	void RemoveTimer(uint32_t eventTimerId);
	int32_t TimeElapsed();
private:
	string DumpTimers();
};

#endif	/* _TIMERSMANAGER_H */


