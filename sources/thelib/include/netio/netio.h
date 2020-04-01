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



#ifndef _NETIO_H
#define	_NETIO_H

#include "netio/iohandlertype.h"
#include "netio/fdstats.h"

#ifdef NET_KQUEUE
#include "netio/kqueue/iohandler.h"
#include "netio/kqueue/iohandlermanager.h"
#include "netio/kqueue/iohandlermanagertoken.h"
#include "netio/kqueue/iotimer.h"
#include "netio/kqueue/tcpacceptor.h"
#include "netio/kqueue/tcpcarrier.h"
#include "netio/kqueue/udpcarrier.h"
#include "netio/kqueue/tcpconnector.h"
#ifdef HAS_UDS
#include "netio/kqueue/udscarrier.h"
#include "netio/kqueue/udsacceptor.h"
#endif	/* HAS_UDS */
#ifdef HAS_KQUEUE_TIMERS
#define NETWORK_REACTOR "kqueue with EVFILT_TIMER support"
#else /* HAS_KQUEUE_TIMERS */
#define NETWORK_REACTOR "kqueue without EVFILT_TIMER support"
#endif /* HAS_KQUEUE_TIMERS */
#endif

#ifdef NET_EPOLL
#include "netio/epoll/iohandler.h"
#include "netio/epoll/iohandlermanager.h"
#include "netio/epoll/iohandlermanagertoken.h"
#include "netio/epoll/iotimer.h"
#include "netio/epoll/tcpacceptor.h"
#include "netio/epoll/tcpcarrier.h"
#include "netio/epoll/udpcarrier.h"
#include "netio/epoll/tcpconnector.h"
#ifdef HAS_UDS
#include "netio/epoll/udscarrier.h"
#include "netio/epoll/udsacceptor.h"
#endif	/* HAS_UDS */
#ifdef HAS_EPOLL_TIMERS
#define NETWORK_REACTOR "epoll with timerfd_XXXX support"
#else /* HAS_EPOLL_TIMERS */
#define NETWORK_REACTOR "epoll without timerfd_XXXX support"
#endif /* HAS_EPOLL_TIMERS */
#endif

#ifdef NET_SELECT
#include "netio/select/iohandler.h"
#include "netio/select/iohandlermanager.h"
#include "netio/select/iotimer.h"
#include "netio/select/tcpacceptor.h"
#include "netio/select/tcpcarrier.h"
#include "netio/select/udpcarrier.h"
#include "netio/select/tcpconnector.h"
#define NETWORK_REACTOR "select"
#endif

#ifdef NET_IOCP
#include "netio/iocp/iohandler.h"
#include "netio/iocp/iohandlermanager.h"
#include "netio/iocp/iotimer.h"
#include "netio/iocp/tcpacceptor.h"
#include "netio/iocp/tcpcarrier.h"
#include "netio/iocp/udpcarrier.h"
#include "netio/iocp/tcpconnector.h"
#ifdef HAS_IOCP_TIMER
#define NETWORK_REACTOR "iocp with native timers"
#else /* HAS_IOCP_TIMER */
#define NETWORK_REACTOR "iocp without native timers"
#endif /* HAS_IOCP_TIMER */
#endif

#endif	/* _NETIO_H */


