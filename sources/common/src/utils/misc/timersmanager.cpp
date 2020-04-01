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



#include "utils/misc/timersmanager.h"
#include "utils/logging/logging.h"

TimerEvent::operator string() {
	return format("id: %4"PRIu32"; period: %6"PRIu32"; nextRun: %"PRIu64,
			id, period, triggerTime);
}

TimersManager::TimersManager(ProcessTimerEvent processTimerEvent) {
	_processTimerEvent = processTimerEvent;
	_lastTime = 0;
	_currentTime = 0;
	_processResult = false;
	_processing = false;
}

TimersManager::~TimersManager() {

	map<uint64_t, map<uint32_t, TimerEvent *> >::iterator i;
	for (i = _timers.begin(); i != _timers.end(); i++) {

		FOR_MAP(MAP_VAL(i), uint32_t, TimerEvent *, j) {
			delete MAP_VAL(j);
		}
	}
	_timers.clear();
}

void TimersManager::AddTimer(TimerEvent &evt) {
	TimerEvent *pTemp = new TimerEvent;
	memcpy(pTemp, &evt, sizeof (TimerEvent));
	GETMILLISECONDS(_currentTime);
	pTemp->triggerTime = _currentTime + pTemp->period;
	_timers[pTemp->triggerTime][pTemp->id] = pTemp;
}

void TimersManager::RemoveTimer(uint32_t eventTimerId) {
	map<uint64_t, map<uint32_t, TimerEvent *> >::iterator i;
	for (i = _timers.begin(); i != _timers.end(); i++) {
		map<uint32_t, TimerEvent *>::iterator found = MAP_VAL(i).find(eventTimerId);
		if (found != MAP_VAL(i).end()) {
			TimerEvent *pTemp = MAP_VAL(found);
			if (pTemp != NULL) {
				delete pTemp;
			}
			if (_processing) {
				MAP_VAL(i)[eventTimerId] = NULL;
			} else {
				MAP_VAL(i).erase(found);
				if (MAP_VAL(i).size() == 0)
					_timers.erase(MAP_KEY(i));
			}
			return;
		}
	}
}

int32_t TimersManager::TimeElapsed() {
	_processing = true;
	int32_t result = 1000;
	GETMILLISECONDS(_currentTime);
	if (_lastTime > _currentTime) {
		WARN("Clock skew detected. Re-adjusting the timers");
		_lastTime = _currentTime;
		map<uint64_t, map<uint32_t, TimerEvent *> > timers;
		map<uint64_t, map<uint32_t, TimerEvent *> >::iterator i;
		for (i = _timers.begin(); i != _timers.end(); i++) {

			FOR_MAP(MAP_VAL(i), uint32_t, TimerEvent *, j) {
				TimerEvent *pTemp = MAP_VAL(j);
				if (pTemp != NULL) {
					pTemp->triggerTime = _currentTime + pTemp->period;
					timers[pTemp->triggerTime][pTemp->id] = pTemp;
				}
			}
		}
		_timers = timers;
		return 1;
	}
	_lastTime = _currentTime;

	_processResult = false;
	map<uint64_t, map<uint32_t, TimerEvent *> >::iterator i = _timers.begin();
	while (i != _timers.end()) {
		if (MAP_KEY(i) > _currentTime) {
			result = (int32_t) (MAP_KEY(i) - _currentTime);
			break;
		}

		FOR_MAP(MAP_VAL(i), uint32_t, TimerEvent *, j) {
			TimerEvent *pTemp = MAP_VAL(j);
			if (pTemp != NULL) {
				_processResult = _processTimerEvent(*pTemp);
				if (_processResult) {
					pTemp->triggerTime += pTemp->period;
					_timers[pTemp->triggerTime][pTemp->id] = pTemp;
				} else {
					pTemp = MAP_VAL(j);
					MAP_VAL(i)[MAP_KEY(j)] = NULL;
					if (pTemp != NULL) {
						delete pTemp;
					}
				}
			}
		}
		_timers.erase(i);
		i = _timers.begin();
	}
	_processing = false;
	return result;
}

string TimersManager::DumpTimers() {
	string result = "";
	map<uint64_t, map<uint32_t, TimerEvent *> >::iterator i;
	for (i = _timers.begin(); i != _timers.end(); i++) {
		result += format("%"PRIu64"\n", MAP_KEY(i));

		FOR_MAP(MAP_VAL(i), uint32_t, TimerEvent *, j) {
			if (MAP_VAL(j) != NULL)
				result += "\t" + MAP_VAL(j)->operator string() + "\n";
			else
				result += format("\tid: %4"PRIu32"; NULL\n", MAP_KEY(j));
		}
		if (MAP_VAL(i).size() == 0)
			result += "\n";
	}
	return result;
}
