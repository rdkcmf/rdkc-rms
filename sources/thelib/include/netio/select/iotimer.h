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


#ifdef NET_SELECT
#ifndef _IOTIMER_H
#define	_IOTIMER_H

#include "netio/select/iohandler.h"

class DLLEXP IOTimer
: public IOHandler {
private:
	static int32_t _idGenerator;
public:
	IOTimer();
	virtual ~IOTimer();

	virtual bool SignalOutputData();
	virtual bool OnEvent(select_event &event);
	bool EnqueueForTimeEvent(uint32_t seconds);
	bool EnqueueForHighGranularityTimeEvent(uint32_t milliseconds);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
};

#endif	/* _TIMERIO_H */
#endif /* NET_SELECT */


