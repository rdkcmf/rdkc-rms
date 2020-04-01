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



#include "protocols/timer/basetimerprotocol.h"
#include "netio/netio.h"

BaseTimerProtocol::BaseTimerProtocol()
: BaseProtocol(PT_TIMER) {
	_pTimer = new IOTimer();
	_pTimer->SetProtocol(this);
	_milliseconds = 0;
}

BaseTimerProtocol::~BaseTimerProtocol() {
	if (_pTimer != NULL) {
		IOTimer *pTimer = _pTimer;
		_pTimer = NULL;
		pTimer->SetProtocol(NULL);
		delete pTimer;
	}
}

string BaseTimerProtocol::GetName() {
	return _name;
}

uint32_t BaseTimerProtocol::GetTimerPeriodInMilliseconds() {
	return _milliseconds;
}

double BaseTimerProtocol::GetTimerPeriodInSeconds() {
	return (double) _milliseconds / 1000.0;
}

IOHandler *BaseTimerProtocol::GetIOHandler() {
	return _pTimer;
}

void BaseTimerProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if (pIOHandler->GetType() != IOHT_TIMER) {
			ASSERT("This protocol accepts only Timer carriers");
		}
	}
	_pTimer = (IOTimer *) pIOHandler;
}

bool BaseTimerProtocol::EnqueueForTimeEvent(uint32_t seconds) {
	if (_pTimer == NULL) {
		ASSERT("BaseTimerProtocol has no timer");
		return false;
	}
	_milliseconds = seconds * 1000;
	return _pTimer->EnqueueForTimeEvent(seconds);
}

bool BaseTimerProtocol::EnqueueForHighGranularityTimeEvent(uint32_t milliseconds) {
	if (_pTimer == NULL) {
		ASSERT("BaseTimerProtocol has no timer");
		return false;
	}
	_milliseconds = milliseconds;
	return _pTimer->EnqueueForHighGranularityTimeEvent(milliseconds);
}

bool BaseTimerProtocol::AllowFarProtocol(uint64_t type) {
	ASSERT("Operation not supported");
	return false;
}

bool BaseTimerProtocol::AllowNearProtocol(uint64_t type) {
	ASSERT("Operation not supported");
	return false;
}

bool BaseTimerProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool BaseTimerProtocol::SignalInputData(IOBuffer &buffer) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}


