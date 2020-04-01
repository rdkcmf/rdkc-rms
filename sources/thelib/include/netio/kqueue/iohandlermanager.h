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
#ifndef _IOHANDLERMANAGER_H
#define	_IOHANDLERMANAGER_H

#include "common.h"
#include "netio/kqueue/iohandlermanagertoken.h"
#include "netio/fdstats.h"

class IOHandler;

class IOHandlerManager {
private:
	static vector<IOHandlerManagerToken *> _tokensVector1;
	static vector<IOHandlerManagerToken *> _tokensVector2;
	static vector<IOHandlerManagerToken *> *_pAvailableTokens;
	static vector<IOHandlerManagerToken *> *_pRecycledTokens;
	static map<uint32_t, IOHandler *> _activeIOHandlers;
	static map<uint32_t, IOHandler *> _deadIOHandlers;
	static int32_t _kq;
	static struct kevent *_pDetectedEvents;
	static struct kevent *_pPendingEvents;
	static int32_t _pendingEventsCount;
	static int32_t _eventsSize;
	static FdStats _fdStats;
#ifndef HAS_KQUEUE_TIMERS
	static struct timespec _timeout;
	static TimersManager *_pTimersManager;
	static struct kevent _dummy;
#endif /* HAS_KQUEUE_TIMERS */
private:
	static void SetupToken(IOHandler *pIOHandler);
	static void FreeToken(IOHandler *pIOHandler);
	static bool RegisterEvent(int32_t fd, int16_t filter,
			uint16_t flags, uint32_t fflags, uint32_t data,
			IOHandlerManagerToken *pToken, bool ignoreError = false);
public:
	static map<uint32_t, IOHandler *> & GetActiveHandlers();
	static map<uint32_t, IOHandler *> & GetDeadHandlers();
	static FdStats &GetStats(bool updateSpeeds);
	static void Initialize();
	static void Start();
	static void SignalShutdown();
	static void ShutdownIOHandlers();
	static void Shutdown();
	static void RegisterIOHandler(IOHandler *pIOHandler);
	static void UnRegisterIOHandler(IOHandler *pIOHandler);
	static int CreateRawUDPSocket();
	static void CloseRawUDPSocket(int socket);
#ifdef GLOBALLY_ACCOUNT_BYTES
	static void AddInBytesManaged(IOHandlerType type, uint64_t bytes);
	static void AddOutBytesManaged(IOHandlerType type, uint64_t bytes);
	static void AddInBytesRawUdp(uint64_t bytes);
	static void AddOutBytesRawUdp(uint64_t bytes);
#endif /* GLOBALLY_ACCOUNT_BYTES */
	static bool EnableReadData(IOHandler *pIOHandler);
	static bool DisableReadData(IOHandler *pIOHandler, bool ignoreError = false);
	static bool EnableWriteData(IOHandler *pIOHandler);
	static bool DisableWriteData(IOHandler *pIOHandler, bool ignoreError = false);
	static bool EnableAcceptConnections(IOHandler *pIOHandler);
	static bool DisableAcceptConnections(IOHandler *pIOHandler, bool ignoreError = false);
	static bool EnableTimer(IOHandler *pIOHandler, uint32_t seconds);
	static bool EnableHighGranularityTimer(IOHandler *pIOHandler, uint32_t milliseconds);
	static bool DisableTimer(IOHandler *pIOHandler, bool ignoreError = false);
	static bool Pulse();
	static void EnqueueForDelete(IOHandler *pIOHandler);
	static uint32_t DeleteDeadHandlers();
private:
#ifndef HAS_KQUEUE_TIMERS
	static bool ProcessTimer(TimerEvent &event);
#endif /* HAS_KQUEUE_TIMERS */
	static inline void ResizeEvents();
};


#endif	/* _IOHANDLERMANAGER_H */
#endif /* NET_KQUEUE */


