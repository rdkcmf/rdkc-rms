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


#ifdef NET_EPOLL
#include "netio/epoll/iotimer.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/baseprotocol.h"

#ifdef HAS_EPOLL_TIMERS
#include <sys/timerfd.h>
#else /* HAS_EPOLL_TIMERS */
int32_t IOTimer::_idGenerator;
#endif /* HAS_EPOLL_TIMERS */

IOTimer::IOTimer()
: IOHandler(0, 0, IOHT_TIMER) {
#ifdef HAS_EPOLL_TIMERS
	_count = 0;
	_inboundFd = _outboundFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (_inboundFd < 0) {
		int err = errno;
		ASSERT("timerfd_create failed with error (%d) %s", err, strerror(err));
	}
#else /* HAS_EPOLL_TIMERS */
	_inboundFd = _outboundFd = ++_idGenerator;
#endif /* HAS_EPOLL_TIMERS */
}

IOTimer::~IOTimer() {
	IOHandlerManager::DisableTimer(this, true);
#ifdef HAS_EPOLL_TIMERS
	CLOSE_SOCKET(_inboundFd);
#endif /* HAS_EPOLL_TIMERS */
}

bool IOTimer::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool IOTimer::OnEvent(struct epoll_event &/*ignored*/) {
#ifdef HAS_EPOLL_TIMERS
	if (read(_inboundFd, &_count, 8) != 8) {
		FATAL("Timer failed!");
		return false;
	}
#endif /* HAS_EPOLL_TIMERS */
	if (!_pProtocol->IsEnqueueForDelete()) {
		if (!_pProtocol->TimePeriodElapsed()) {
			FATAL("Unable to handle TimeElapsed event");
			IOHandlerManager::EnqueueForDelete(this);
			return false;
		}
	}
	return true;
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

#endif /* NET_EPOLL */

