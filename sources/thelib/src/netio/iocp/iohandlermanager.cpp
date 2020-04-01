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
#include "Mswsock.h"
#include "netio/iocp/iohandlermanager.h"
#include "netio/iocp/iohandler.h"
#include "netio/iocp/tcpacceptor.h"
#include "netio/iocp/tcpcarrier.h"
#include "netio/iocp/udpcarrier.h"
#include "netio/iocp/iotimer.h"
#include "application/baseclientapplication.h"

#define MAX_OVERLAPPED_ENTRIES 1024

//int ____allocs = 0;

FdStats IOHandlerManager::_fdStats;
HANDLE IOHandlerManager::_iocpHandle = NULL;
map<uint32_t, IOHandler *> IOHandlerManager::_activeIOHandlers;
map<uint32_t, IOHandler *> IOHandlerManager::_deadIOHandlers;
OVERLAPPED_ENTRY *IOHandlerManager::_pDetectedEvents = NULL;
bool IOHandlerManager::_active = false;
int32_t IOHandlerManager::_nextWaitPeriod = 1000;
#ifndef HAS_IOCP_TIMER
iocp_event IOHandlerManager::_dummyEvent(iocp_event_type_timer, NULL);
TimersManager *IOHandlerManager::_pTimersManager = NULL;
#endif /* HAS_IOCP_TIMER */

map<uint32_t, IOHandler *> & IOHandlerManager::GetActiveHandlers() {
	return _activeIOHandlers;
}

map<uint32_t, IOHandler *> & IOHandlerManager::GetDeadHandlers() {
	return _deadIOHandlers;
}

FdStats &IOHandlerManager::GetStats(bool updateSpeeds) {
#ifdef GLOBALLY_ACCOUNT_BYTES
	if (updateSpeeds)
		_fdStats.UpdateSpeeds();
#endif /* GLOBALLY_ACCOUNT_BYTES */
	return _fdStats;
}

void IOHandlerManager::Initialize() {
	_fdStats.Reset();
#ifndef HAS_IOCP_TIMER
	_pTimersManager = new TimersManager(ProcessTimer);
#endif /* HAS_IOCP_TIMER */
	_pDetectedEvents = new OVERLAPPED_ENTRY[MAX_OVERLAPPED_ENTRIES];
	_iocpHandle = CreateIoCompletionPort(
			INVALID_HANDLE_VALUE,
			NULL,
			NULL,
			16);
	if (_iocpHandle == NULL) {
		ASSERT("Unable to open IOCP");
	}
}

void IOHandlerManager::Start() {
	_active = true;
}

void IOHandlerManager::SignalShutdown() {
	_active = false;
}

void IOHandlerManager::ShutdownIOHandlers() {

	FOR_MAP(_activeIOHandlers, uint32_t, IOHandler *, i) {
		EnqueueForDelete(MAP_VAL(i));
	}
	_active = true;
	for (int i = 0; i < 3; i++) {
		//FINEST("Pulse...");
		Pulse();
		Sleep(1000);
	}
	_active = false;
}

void IOHandlerManager::Shutdown() {
#ifndef HAS_IOCP_TIMER
	delete _pTimersManager;
	_pTimersManager = NULL;
#endif /* HAS_IOCP_TIMER */
	if (_activeIOHandlers.size() != 0 || _deadIOHandlers.size() != 0) {
		FATAL("Incomplete shutdown!");
	}
	CloseHandle(_iocpHandle);
	delete[] _pDetectedEvents;
	_pDetectedEvents = NULL;
}

void IOHandlerManager::RegisterIOHandler(IOHandler* pIOHandler) {
	if (MAP_HAS1(_activeIOHandlers, pIOHandler->GetId())) {
		ASSERT("IOHandler already registered");
	}
	size_t before = _activeIOHandlers.size();
	_activeIOHandlers[pIOHandler->GetId()] = pIOHandler;
	_fdStats.RegisterManaged(pIOHandler->GetType());
	DEBUG("Handlers count changed: %"PRIz"u->%"PRIz"u %s", before, before + 1,
			STR(IOHandler::IOHTToString(pIOHandler->GetType())));
}

void IOHandlerManager::UnRegisterIOHandler(IOHandler *pIOHandler) {
	if (MAP_HAS1(_activeIOHandlers, pIOHandler->GetId())) {
		_fdStats.UnRegisterManaged(pIOHandler->GetType());
		size_t before = _activeIOHandlers.size();
		_activeIOHandlers.erase(pIOHandler->GetId());
		DEBUG("Handlers count changed: %"PRIz"u->%"PRIz"u %s", before, before - 1,
				STR(IOHandler::IOHTToString(pIOHandler->GetType())));
	}
}

SOCKET IOHandlerManager::CreateRawUDPSocket() {
	SOCKET result = socket(AF_INET, SOCK_DGRAM, 0);
	if ((((int) result) >= 0)&&(setFdCloseOnExec((int) result))) {
		_fdStats.RegisterRawUdp();
	} else {
		int err = LASTSOCKETERROR;
		FATAL("Unable to create raw udp socket. Error code was: %d", err);
	}
	return result;
}

void IOHandlerManager::CloseRawUDPSocket(SOCKET socket) {
	if (socket > 0) {
		_fdStats.UnRegisterRawUdp();
	}
	CLOSE_SOCKET(socket);
}
#ifdef GLOBALLY_ACCOUNT_BYTES

void IOHandlerManager::AddInBytesManaged(IOHandlerType type, uint64_t bytes) {
	_fdStats.AddInBytesManaged(type, bytes);
}

void IOHandlerManager::AddOutBytesManaged(IOHandlerType type, uint64_t bytes) {
	_fdStats.AddOutBytesManaged(type, bytes);
}

void IOHandlerManager::AddInBytesRawUdp(uint64_t bytes) {
	_fdStats.AddInBytesRawUdp(bytes);
}

void IOHandlerManager::AddOutBytesRawUdp(uint64_t bytes) {
	_fdStats.AddOutBytesRawUdp(bytes);
}

#endif /* GLOBALLY_ACCOUNT_BYTES */

bool IOHandlerManager::EnableReadData(IOHandler *pIOHandler) {
	IOHandlerType type = pIOHandler->GetType();

	switch (type) {
		case IOHT_TCP_CARRIER:
		{
			TCPCarrier *pCarrier = (TCPCarrier *) pIOHandler;
			iocp_event_tcp_read *pEvent = pCarrier->GetReadEvent();
			if (WSARecv(pCarrier->GetInboundFd(), &pEvent->inputBuffer, 1, &pEvent->transferedBytes, &pEvent->flags, pEvent, NULL) != 0) {
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING) {
					FATAL("Unable to call WSARecv. Error was: %"PRIu32, err);
					pEvent->isValid = false;
					return false;
				}
			}
			pEvent->pendingOperation = true;
			return true;
		}

		case IOHT_UDP_CARRIER:
		{
			UDPCarrier *pCarrier = (UDPCarrier *) pIOHandler;
			iocp_event_udp_read *pEvent = pCarrier->GetReadEvent();
			if (WSARecvFrom( pCarrier->GetInboundFd(), 
							&pEvent->inputBuffer, 1, 
							&pEvent->transferedBytes, 
							&pEvent->flags, 
							//(sockaddr *)& pEvent->sourceAddress,
							pEvent->sourceAddress.getSockAddr(),
							&pEvent->sourceLen, pEvent, NULL)) {
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING) {
					FATAL("Unable to call WSARecvFrom. Error was: %"PRIu32" : %s", err, STR(wsaError2String(err)));
					pEvent->isValid = false;
					return false;
				}
			}
			pEvent->pendingOperation = true;
			return true;
		}
		default:
		{
			FATAL("Invalid handler type", type);
			return false;
		}
	}
}

bool IOHandlerManager::DisableReadData(IOHandler *pIOHandler, bool ignoreError) {
	//NYIR;
	return true;
}

bool IOHandlerManager::EnableWriteData(IOHandler *pIOHandler) {
	//NYIR;
	return true;
}

bool IOHandlerManager::DisableWriteData(IOHandler *pIOHandler, bool ignoreError) {
	//NYIR;
	return true;
}

bool IOHandlerManager::EnableAcceptConnections(IOHandler *pIOHandler) {
	TCPAcceptor *pAcceptor = (TCPAcceptor *) pIOHandler;
	iocp_event_accept *pEvent = pAcceptor->GetEvent();
	uint16_t family = pAcceptor->getAddressFamily();
	pEvent->acceptedSock = socket(pAcceptor->getAddressFamily(), SOCK_STREAM, 0); //NOINHERIT
	if (pEvent->acceptedSock == INVALID_SOCKET) {
		FATAL("Unable to create socket");
		return false;
	}

	if (AcceptEx(
			pEvent->listenSock,				//sListenSocket
			pEvent->acceptedSock,			//sAcceptSocket
			pEvent->pBuffer,				//lpOutputBuffer
			0,								//dwReceiveDataLength
			sizeof (sockaddr_in6) + 16, //dwLocalAddressLength
			sizeof (sockaddr_in6) + 16, //dwRemoteAddressLength
			&pEvent->transferedBytes,		//lpdwBytesReceived
			pEvent							//lpOverlapped
			) == FALSE) {
		DWORD err = WSAGetLastError();
		switch (err) {
			case ERROR_IO_PENDING:
			{
				pEvent->pendingOperation = true;
				return true;
			}
			case WSAECONNRESET:
			{
				NYIA;
				return false;
			}
			default:
			{
				pEvent->pendingOperation = false;
				FATAL("Unable to call AcceptEx function. Error was: %"PRIu32" (%s)", err,STR(wsaError2String(err)));
				return false;
			}
		}
	} else {
		pEvent->pendingOperation = false;
		return pAcceptor->OnEvent(*pEvent);
	}
}

bool IOHandlerManager::DisableAcceptConnections(IOHandler *pIOHandler, bool ignoreError) {
	//NYIR;
	return true;
}

bool IOHandlerManager::EnableTimer(IOHandler *pIOHandler, uint32_t seconds) {
#ifdef HAS_IOCP_TIMER
	return EnableHighGranularityTimer(pIOHandler, seconds * 1000);
#else /* HAS_IOCP_TIMER */
	TimerEvent timerEvent = {0, 0, 0, 0};
	timerEvent.id = pIOHandler->GetId();
	timerEvent.period = seconds * 1000;
	timerEvent.pUserData = pIOHandler;
	_pTimersManager->AddTimer(timerEvent);
	return true;
#endif /* HAS_IOCP_TIMER */
}

bool IOHandlerManager::EnableHighGranularityTimer(IOHandler *pIOHandler, uint32_t milliseconds) {
#ifdef HAS_IOCP_TIMER
	if (pIOHandler->GetType() != IOHT_TIMER)
		return false;
	ULARGE_INTEGER dueTime;
	FILETIME fDueTime;
	dueTime.QuadPart = -((LONGLONG) milliseconds * 10 * 1000); // 100-nanosec granularity
	fDueTime.dwHighDateTime = dueTime.HighPart;
	fDueTime.dwLowDateTime = dueTime.LowPart;
	PTP_TIMER pTPTimer = ((IOTimer *) pIOHandler)->GetPTTimer();
	if (pTPTimer == NULL)
		return false;
	SetThreadpoolTimer(pTPTimer, &fDueTime, milliseconds, 0);
	return true;
#else /* HAS_IOCP_TIMER */
	TimerEvent timerEvent = {0, 0, 0, 0};
	timerEvent.id = pIOHandler->GetId();
	timerEvent.period = milliseconds;
	timerEvent.pUserData = pIOHandler;
	_pTimersManager->AddTimer(timerEvent);
	return true;
#endif /* HAS_IOCP_TIMER */
}

bool IOHandlerManager::DisableTimer(IOHandler *pIOHandler, bool ignoreError) {
	if (pIOHandler->GetType() != IOHT_TIMER)
		return true;
#ifdef HAS_IOCP_TIMER
	if (pIOHandler->GetType() == IOHT_TIMER) {
		PTP_TIMER pTPTimer = ((IOTimer *) pIOHandler)->GetPTTimer();
		if (pTPTimer == NULL)
			return false;
		SetThreadpoolTimer(pTPTimer, NULL, 0, 0);
	}
#else /* HAS_IOCP_TIMER */
	_pTimersManager->RemoveTimer(pIOHandler->GetId());
#endif /* HAS_IOCP_TIMER */
	return true;
}

void IOHandlerManager::EnqueueForDelete(IOHandler *pIOHandler) {
	pIOHandler->CancelIO();
	DisableAcceptConnections(pIOHandler, true);
	DisableReadData(pIOHandler, true);
	DisableWriteData(pIOHandler, true);
	DisableTimer(pIOHandler, true);
	if (!MAP_HAS1(_deadIOHandlers, pIOHandler->GetId()))
		_deadIOHandlers[pIOHandler->GetId()] = pIOHandler;
}

uint32_t IOHandlerManager::DeleteDeadHandlers() {
	uint32_t result = 0;
	while (_deadIOHandlers.size() > 0) {
		IOHandler *pIOHandler = MAP_VAL(_deadIOHandlers.begin());
		_deadIOHandlers.erase(pIOHandler->GetId());
		delete pIOHandler;
		result++;
	}
	return result;
}

bool IOHandlerManager::Pulse() {
	if (!_active)
		return false;
#ifndef HAS_IOCP_TIMER
	if (_pTimersManager != NULL)
		_nextWaitPeriod = _pTimersManager->TimeElapsed();
#endif /* HAS_IOCP_TIMER */
	ULONG detectedCount = 0;
	if (!GetQueuedCompletionStatusEx(_iocpHandle, _pDetectedEvents, MAX_OVERLAPPED_ENTRIES, &detectedCount, _nextWaitPeriod, false)) {
		DWORD err = GetLastError();
		if (err != WAIT_TIMEOUT) {
			FATAL("GetQueuedCompletionStatus failed with error code: %"PRIu32, err);
			return false;
		}
	} else {
		//FINEST("detectedCount: %"PRIu32,(uint32_t)detectedCount);
		vector<iocp_event *> releasedEvents;
		for (ULONG i = 0; i < detectedCount; i++) {
			iocp_event *pEvent = (iocp_event *) _pDetectedEvents[i].lpOverlapped;
			pEvent->pendingOperation = false;
			if (pEvent->isValid) {
				pEvent->transferedBytes = _pDetectedEvents[i].dwNumberOfBytesTransferred;
				if (!pEvent->pIOHandler->OnEvent(*pEvent)) {
					// Check the event type.  IF the event is an ACCEPTOR, then
					// we will need to respawn the acceptor!
					if (pEvent->pIOHandler->GetType() == IOHT_ACCEPTOR) {
						// Safe to assume that the pIOHandler class is a TCPAcceptor
						TCPAcceptor *pDeadAcceptor = (TCPAcceptor*)pEvent->pIOHandler;
						Variant params = pDeadAcceptor->GetParameters();
						FATAL("Unable to handle TCP Accept Event: Protocol(%s), IP(%s), Port(%d)",
							  STR(params["protocol"]),
							  STR(params["ip"]),
							  int(params["port"]));

						// Now for the messy part -- we need to enqueue the respawning of the acceptor.
						// To do this, we need access to the JobsTimer class which belongs to the 
						// OriginApplication

						// Get a pointer to the application before sending the event for deletion.
						BaseClientApplication* pApplication = pDeadAcceptor->GetApplication();

						// Enqueue the event for deletion!
						EnqueueForDelete(pEvent->pIOHandler);

						// Now, enqueue the acceptor for respawning.
						Variant message;
						message["header"] = "restartService";
						message["payload"] = params;
						pApplication->SignalApplicationMessage(message);
//						pApplication->RestartService(params);

					} else {
						EnqueueForDelete(pEvent->pIOHandler);
					}

					continue;
				}
			} else {
				//FINEST("Orphan OS. Delete it");
				pEvent->Release();
			}
		}
	}
	return true;
}

bool IOHandlerManager::RegisterIOCPFD(SOCKET fd) {
	if (CreateIoCompletionPort((HANDLE) fd, _iocpHandle, (u_long) 0, 0) == NULL) {
		FATAL("Unable to register handle to IOCP");
		return false;
	}
	return true;
}
#ifdef HAS_IOCP_TIMER

BOOL IOHandlerManager::PostCompletedEvent(iocp_event *pEvent) {
	if (pEvent->pendingOperation)
		return 0;
	pEvent->pendingOperation = true;
	return PostQueuedCompletionStatus(_iocpHandle, 0, NULL, (LPOVERLAPPED) pEvent);
}

void CALLBACK TimerCallback(LPVOID lpArgToCompletionRoutine,
		DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);
	iocp_event_timer_elapsed * pEvent = (iocp_event_timer_elapsed *) lpArgToCompletionRoutine;
	IOHandlerManager::PostCompletedEvent(pEvent);
}
#else /* HAS_IOCP_TIMER */

bool IOHandlerManager::ProcessTimer(TimerEvent &event) {
	if (event.pUserData == NULL)
		return false;
	if (!((IOHandler *) event.pUserData)->OnEvent(_dummyEvent)) {
		FATAL("Unable to handle event");
		if (event.pUserData != NULL)
			EnqueueForDelete((IOHandler *) event.pUserData);
		return false;
	}
	return true;
}
#endif /* HAS_IOCP_TIMER */
#endif /* NET_IOCP */
