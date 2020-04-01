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



#ifndef _BASETIMERPROTOCOL_H
#define	_BASETIMERPROTOCOL_H

#include "protocols/baseprotocol.h"

class IOTimer;

class DLLEXP BaseTimerProtocol
: public BaseProtocol {
private:
	IOTimer *_pTimer;
	uint32_t _milliseconds;
protected:
	string _name;
public:
	BaseTimerProtocol();
	virtual ~BaseTimerProtocol();

	string GetName();
	uint32_t GetTimerPeriodInMilliseconds();
	double GetTimerPeriodInSeconds();

	virtual IOHandler *GetIOHandler();
	virtual void SetIOHandler(IOHandler *pIOHandler);

	virtual bool EnqueueForTimeEvent(uint32_t seconds);
	virtual bool EnqueueForHighGranularityTimeEvent(uint32_t milliseconds);

	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
};

#endif	/* _BASETIMERPROTOCOL_H */


