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


#ifdef NET_KQUEUE
#include "netio/kqueue/iotimer.h"
#include "protocols/baseprotocol.h"
#include "netio/kqueue/iohandlermanager.h"

int32_t IOTimer::_idGenerator;

IOTimer::IOTimer()
: IOHandler(0, 0, IOHT_TIMER) {
	_outboundFd = _inboundFd = ++_idGenerator;
}

IOTimer::~IOTimer() {
	IOHandlerManager::DisableTimer(this, true);
}

bool IOTimer::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool IOTimer::OnEvent(struct kevent &event) {
	switch (event.filter) {
		case EVFILT_TIMER:
		{
			if (!_pProtocol->IsEnqueueForDelete()) {
				if (!_pProtocol->TimePeriodElapsed()) {
					FATAL("Unable to handle TimeElapsed event");
					IOHandlerManager::EnqueueForDelete(this);
					return false;
				}
			}
			return true;
		}
		default:
		{
			ASSERT("Invalid state: %hu", event.filter);

			return false;
		}
	}
}

bool IOTimer::EnqueueForTimeEvent(uint32_t seconds) {
	return IOHandlerManager::EnableTimer(this, seconds);
}

bool IOTimer::EnqueueForHighGranularityTimeEvent(uint32_t milliseconds) {
	return IOHandlerManager::EnableHighGranularityTimer(this, milliseconds);
}

void IOTimer::GetStats(Variant &info, uint32_t namespaceId) {
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
}

#endif /* NET_KQUEUE */

