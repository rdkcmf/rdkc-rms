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


#ifdef NET_IOCP
#ifndef _IOTIMER_H
#define	_IOTIMER_H

#include "netio/iocp/iohandler.h"

class IOTimer
: public IOHandler {
private:
	static int32_t _idGenerator;
#ifdef HAS_IOCP_TIMER
	iocp_event_timer_elapsed * _pTimerEvent;
	PTP_TIMER _pTimer;

#ifdef TIMER_PERFORMANCE_TEST
	LARGE_INTEGER startCounter;
	int32_t _tick;
#endif /* TIMER_PERFORMANCE_TEST */

#endif /* HAS_IOCP_TIMER */
public:
	IOTimer();
	virtual ~IOTimer();
	virtual void CancelIO();
	virtual bool SignalOutputData();
	virtual bool OnEvent(iocp_event &event);
	bool EnqueueForTimeEvent(uint32_t seconds);
	bool EnqueueForHighGranularityTimeEvent(uint32_t milliseconds);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
#ifdef HAS_IOCP_TIMER
	iocp_event_timer_elapsed * GetTimerEvent();
	PTP_TIMER GetPTTimer();
#endif /* HAS_IOCP_TIMER */
};
#ifdef HAS_IOCP_TIMER
void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance,
		PVOID Context, PTP_TIMER Timer);
#endif /* HAS_IOCP_TIMER */
#endif /* _TIMERIO_H */
#endif /* NET_IOCP */


