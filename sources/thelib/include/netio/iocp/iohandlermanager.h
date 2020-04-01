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
#ifndef _IOHANDLERMANAGER_H
#define	_IOHANDLERMANAGER_H

#include "common.h"
#include "netio/fdstats.h"
#include "netio/iocp/iocpevent.h"

class IOHandler;

class DLLEXP IOHandlerManager {
private:
	static FdStats _fdStats;
	static HANDLE _iocpHandle;
	static map<uint32_t, IOHandler *> _activeIOHandlers;
	static map<uint32_t, IOHandler *> _deadIOHandlers;
	static OVERLAPPED_ENTRY *_pDetectedEvents;
	static bool _active;
	static int32_t _nextWaitPeriod;
#ifndef HAS_IOCP_TIMER
	static TimersManager *_pTimersManager;
	static iocp_event _dummyEvent;
#endif /* HAS_IOCP_TIMER */
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
	static SOCKET CreateRawUDPSocket();
	static void CloseRawUDPSocket(SOCKET socket);
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
	static bool RegisterIOCPFD(SOCKET fd);
#ifdef HAS_IOCP_TIMER
	static BOOL PostCompletedEvent(iocp_event *pEvent);
#else /* HAS_IOCP_TIMER */
private:
	static bool ProcessTimer(TimerEvent &event);
#endif /* HAS_IOCP_TIMER */
};
#ifdef HAS_IOCP_TIMER
void CALLBACK TimerCallback(LPVOID lpArgToCompletionRoutine,
		DWORD dwTimerLowValue,	DWORD dwTimerHighValue);
#endif /* HAS_IOCP_TIMER */
#endif	/* _IOHANDLERMANAGER_H */
#endif /* NET_IOCP */


