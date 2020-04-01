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
#ifndef _IOCPEVENT_H
#define	_IOCPEVENT_H

#include "common.h"
#include "utils/misc/socketaddress.h"

class IOHandler;

typedef enum iocp_event_type {
	iocp_event_type_timer = 0,
	iocp_event_type_read,
	iocp_event_type_write,
	iocp_event_type_accept,
	iocp_event_type_connect,
	iocp_event_type_udp
} iocp_event_type;

//extern int ____allocs;

typedef struct iocp_event : public OVERLAPPED {
	iocp_event_type type;
	bool isValid;
	bool pendingOperation;
	IOHandler *pIOHandler;
	DWORD transferedBytes;

	iocp_event(iocp_event_type _type, IOHandler * _pIOHandler) {
		OVERLAPPED *pTemp = (OVERLAPPED *)this;
		memset(pTemp, 0, sizeof (OVERLAPPED));
		type = _type;
		isValid = true;
		pendingOperation = false;
		pIOHandler = _pIOHandler;
		transferedBytes = 0;
		//____allocs++;
		//FINEST("C: %p %d %d", this, type, ____allocs);
	}

	virtual ~iocp_event() {
		//____allocs--;
		//FINEST("D: %p %d %d", this, type, ____allocs);
	}

	virtual void Release() {
		isValid = false;
		pIOHandler = NULL;
		transferedBytes = 0;
		if (!pendingOperation) {
			delete this;
		}
	}
} iocp_event;

typedef struct iocp_event_accept : public iocp_event {
	SOCKET listenSock;
	SOCKET acceptedSock;
	uint8_t pBuffer[2 * sizeof (sockaddr_in6) + 32];

	iocp_event_accept(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_accept, _pIOHandler) {
		listenSock = (SOCKET) - 1;
		acceptedSock = (SOCKET) - 1;
		memset(pBuffer, 0, sizeof (pBuffer));
	}

	static iocp_event_accept * GetInstance(IOHandler * _pIOHandler) {
		iocp_event_accept *pResult = new iocp_event_accept(_pIOHandler);
		return pResult;
	}

	virtual void Release() {
		CLOSE_SOCKET(listenSock);
		CLOSE_SOCKET(acceptedSock);
		iocp_event::Release();
	}
} iocp_event_accept;

#define MAX_IOCP_EVT_READ_SIZE 16*1024

typedef struct iocp_event_tcp_read : public iocp_event {
	WSABUF inputBuffer;
	DWORD flags;

	iocp_event_tcp_read(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_read, _pIOHandler) {
		inputBuffer.buf = new CHAR[MAX_IOCP_EVT_READ_SIZE];
		inputBuffer.len = MAX_IOCP_EVT_READ_SIZE;
		memset(inputBuffer.buf, 0, MAX_IOCP_EVT_READ_SIZE);
		flags = 0;
	}

	static iocp_event_tcp_read * GetInstance(IOHandler * _pIOHandler) {
		iocp_event_tcp_read *pResult = new iocp_event_tcp_read(_pIOHandler);
		return pResult;
	}

	virtual void Release() {
		if (inputBuffer.buf != NULL) {
			delete[] inputBuffer.buf;
		}
		inputBuffer.buf = NULL;
		inputBuffer.len = 0;
		iocp_event::Release();
	}
} iocp_event_tcp_read;

typedef struct iocp_event_tcp_write : public iocp_event {
	IOBuffer outputBuffer;
	WSABUF wsaBuf;

	iocp_event_tcp_write(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_write, _pIOHandler) {
	}

	static iocp_event_tcp_write * GetInstance(IOHandler * _pIOHandler) {
		iocp_event_tcp_write *pResult = new iocp_event_tcp_write(_pIOHandler);
		return pResult;
	}

	virtual void Release() {
		outputBuffer.IgnoreAll();
		iocp_event::Release();
	}
} iocp_event_tcp_write;

typedef struct iocp_event_udp_read : public iocp_event {
	WSABUF inputBuffer;
	DWORD flags;
	SocketAddress sourceAddress;
	socklen_t sourceLen;

	iocp_event_udp_read(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_udp, _pIOHandler) {
		inputBuffer.buf = new CHAR[MAX_IOCP_EVT_READ_SIZE];
		inputBuffer.len = MAX_IOCP_EVT_READ_SIZE;
		memset(inputBuffer.buf, 0, MAX_IOCP_EVT_READ_SIZE);
		sourceLen = sourceAddress.getLength();
		flags = 0;
	}

	static iocp_event_udp_read * GetInstance(IOHandler * _pIOHandler) {
		iocp_event_udp_read *pResult = new iocp_event_udp_read(_pIOHandler);
		return pResult;
	}

	virtual void Release() {
		if (inputBuffer.buf != NULL) {
			delete[] inputBuffer.buf;
		}
		inputBuffer.buf = NULL;
		inputBuffer.len = 0;
		iocp_event::Release();
	}
} iocp_event_udp_read;

typedef struct iocp_event_connect : public iocp_event {
	uint8_t pBuffer[2 * sizeof (sockaddr_in6) + 32];
	SOCKET connectedSock;
	bool closeSocket;

	iocp_event_connect(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_connect, _pIOHandler) {
		connectedSock = (SOCKET) - 1;
		memset(pBuffer, 0, sizeof (pBuffer));
	}

	static iocp_event_connect * GetInstance(IOHandler * _pIOHandler) {
		iocp_event_connect *pResult = new iocp_event_connect(_pIOHandler);
		return pResult;
	}

	virtual void Release() {
		if (closeSocket) {
			CLOSE_SOCKET(connectedSock);
		}
		iocp_event::Release();
	}
} iocp_event_connect;

#ifdef HAS_IOCP_TIMER
typedef struct iocp_event_timer_elapsed : public iocp_event {

	iocp_event_timer_elapsed(IOHandler * _pIOHandler) : iocp_event(iocp_event_type_timer, _pIOHandler) {
//		hEvent = CreateEvent(NULL, false, false, NULL);
	}

	static iocp_event_timer_elapsed * GetInstance(IOHandler * _pIOHandler) {
		return new iocp_event_timer_elapsed(_pIOHandler);
	}
	virtual void Release() {
		iocp_event::Release();
//		CloseHandle(hEvent);
	}
} iocp_event_timer_elapsed;
#endif /* HAS_IOCP_TIMER */
#endif	/* _IOCPEVENT_H */
#endif /* NET_IOCP */
