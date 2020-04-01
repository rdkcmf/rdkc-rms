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
#include "netio/iocp/tcpcarrier.h"
#include "netio/iocp/iohandlermanager.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"
#include "utils/misc/socketaddress.h"

TCPCarrier::TCPCarrier(SOCKET fd, bool registerIOCPFD)
: IOHandler(fd, fd, IOHT_TCP_CARRIER) {
	_pReadEvent = iocp_event_tcp_read::GetInstance(this);
	_pWriteEvent = iocp_event_tcp_write::GetInstance(this);
	if (registerIOCPFD)
		IOHandlerManager::RegisterIOCPFD(GetInboundFd());
	IOHandlerManager::EnableReadData(this);
	//memset(&_farAddress, 0, sizeof (sockaddr_in6));
	_farIp = "";
	_farPort = 0;
	//memset(&_nearAddress, 0, sizeof (sockaddr_in6));
	_nearIp = "";
	_nearPort = 0;
	GetEndpointsInfo();
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;

	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierCreated(stats);
}

TCPCarrier::~TCPCarrier() {
	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierClosed(stats);

	CLOSE_SOCKET(_inboundFd);
	_pReadEvent->Release();
	_pReadEvent = NULL;
	_pWriteEvent->Release();
	_pWriteEvent = NULL;
}

void TCPCarrier::CancelIO() {
	CancelIoEx((HANDLE) _inboundFd, _pReadEvent);
	CancelIoEx((HANDLE) _inboundFd, _pWriteEvent);
}

bool TCPCarrier::OnEvent(iocp_event &event) {
	DWORD temp = 0;
	BOOL result = GetOverlappedResult((HANDLE) _inboundFd, &event, &temp, true);
	if (!result) {
		DWORD err = GetLastError();
		FATAL("Unable to read/write data from/to connection: %s. GetLastError returned: %"PRIu32,
				(_pProtocol != NULL) ? STR(*_pProtocol) : "", err);
		return false;
	}
	switch (event.type) {
		case iocp_event_type_read:
		{
			if (_pReadEvent->transferedBytes == 0) {
				FATAL("Unable to read data from connection: %s. EOF encountered",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "");
				return false;
			}
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			o_assert(pInputBuffer != NULL);
			if (!pInputBuffer->ReadFromBuffer((uint8_t *) _pReadEvent->inputBuffer.buf, _pReadEvent->transferedBytes)) {
				FATAL("Unable to read data from connection: %s. IOBuffer failed",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "");
				return false;
			}
			_ioAmount = _pReadEvent->transferedBytes;
			_rx += _ioAmount;
			ADD_IN_BYTES_MANAGED(_type, _ioAmount);
			if (!_pProtocol->SignalInputData(_ioAmount)) {
				FATAL("Unable to read data from connection: %s. Signaling upper protocols failed",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "");
				return false;
			}

			_pReadEvent->transferedBytes = 0;
			_pReadEvent->flags = 0;
			if (WSARecv(_inboundFd, &_pReadEvent->inputBuffer, 1, &_pReadEvent->transferedBytes, &_pReadEvent->flags, _pReadEvent, NULL) != 0) {
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING) {
					FATAL("Unable to read data from connection: %s. WSAGetLastError returned: %"PRIu32,
							(_pProtocol != NULL) ? STR(*_pProtocol) : "", err);
					_pReadEvent->isValid = false;
					return false;
				}
			}
			_pReadEvent->pendingOperation = true;
			return true;
		}
		case iocp_event_type_write:
		{
			if (_pWriteEvent->transferedBytes == 0) {
				FATAL("Unable to write data on connection: %s. EOF encountered",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "");
				return false;
			}
			_tx += _pWriteEvent->transferedBytes;
			ADD_OUT_BYTES_MANAGED(_type, _pWriteEvent->transferedBytes);
			_pWriteEvent->outputBuffer.Ignore(_pWriteEvent->transferedBytes);
			if (GETAVAILABLEBYTESCOUNT(_pWriteEvent->outputBuffer) > 0) {
				_pWriteEvent->wsaBuf.buf = (CHAR *) GETIBPOINTER(_pWriteEvent->outputBuffer);
				_pWriteEvent->wsaBuf.len = GETAVAILABLEBYTESCOUNT(_pWriteEvent->outputBuffer);
				if (WSASend(_outboundFd, &_pWriteEvent->wsaBuf, 1, &_pWriteEvent->transferedBytes, 0, _pWriteEvent, NULL) != 0) {
					DWORD err = WSAGetLastError();
					if (err != WSA_IO_PENDING) {
						FATAL("Unable to write data on connection: %s. WSAGetLastError returned: %"PRIu32,
								(_pProtocol != NULL) ? STR(*_pProtocol) : "", err);
						_pWriteEvent->isValid = false;
						return false;
					}
				}
				_pWriteEvent->pendingOperation = true;
				return true;
			} else {
				if (!SignalOutputData()) {
					FATAL("Unable to write data on connection: %s. Signaling upper protocols failed",
							(_pProtocol != NULL) ? STR(*_pProtocol) : "");
					return false;
				}
				return true;
			}
		}
		default:
		{
			FATAL("Unable to read/write data from/to connection: %s. Invalid event type: %d",
					(_pProtocol != NULL) ? STR(*_pProtocol) : "", event.type);
			return false;
		}
	}
}

bool TCPCarrier::SignalOutputData() {
	//1. test if we have the event
	if (_pWriteEvent == NULL)
		return false;

	//2. Bail out gracefully if we have an outstanding operation
	if (_pWriteEvent->pendingOperation)
		return true;

	//3. Get the output buffer from the connection
	//and bail out if there is no data/buffer
	IOBuffer *pOutputBuffer = _pProtocol->GetOutputBuffer();
	if (pOutputBuffer == NULL) {
		_pProtocol->ReadyForSend();
		return true;
	}

	//4. Transfer the bytes from the connection's buffer on the event's buffer
	//this will also take care of the _sendLimit on the connection's buffer.
	//The event's buffer never ever has a send limit, because IOCP is
	//enqueue-and-wait-for-completion mode. So whatever we put on the event's
	//buffer it will be guaranteed to be send completely
	//After the copy attempt, we see if we actually have something to send
	_pWriteEvent->outputBuffer.ReadFromInputBufferWithIgnore(*pOutputBuffer);
	if (GETAVAILABLEBYTESCOUNT(_pWriteEvent->outputBuffer) == 0) {
		_pProtocol->ReadyForSend();
		return true;
	}

	//5. Setup the low level/windows specific member variables, of course, using
	//the newly fed and independent event's buffer
	_pWriteEvent->wsaBuf.buf = (CHAR *) GETIBPOINTER(_pWriteEvent->outputBuffer);
	_pWriteEvent->wsaBuf.len = GETAVAILABLEBYTESCOUNT(_pWriteEvent->outputBuffer);

	//6. Do the damage
	if (WSASend(_outboundFd, &_pWriteEvent->wsaBuf, 1, &_pWriteEvent->transferedBytes, 0, _pWriteEvent, NULL) != 0) {
		DWORD err = WSAGetLastError();
		if (err != WSA_IO_PENDING) {
			FATAL("Unable to call WSASend. Error was: %"PRIu32, err);
			_pWriteEvent->isValid = false;
			return false;
		}
	}

	//7. We are now in pending operation mode
	_pWriteEvent->pendingOperation = true;

	//8. Done
	return true;
}

iocp_event_tcp_read *TCPCarrier::GetReadEvent() {
	return _pReadEvent;
}

void TCPCarrier::GetStats(Variant &info, uint32_t namespaceId) {
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
	if (!GetEndpointsInfo()) {
		FATAL("Unable to get endpoints info");
		info = "unable to get endpoints info";
		return;
	}
	info["type"] = "IOHT_TCP_CARRIER";
	info["farIP"] = _farIp;
	info["farPort"] = _farPort;
	info["nearIP"] = _nearIp;
	info["nearPort"] = _nearPort;
	info["rx"] = _rx;
	info["tx"] = _tx;
}

SocketAddress &TCPCarrier::GetFarEndpointAddress() {
	if ((_farIp == "") || (_farPort == 0))
		GetEndpointsInfo();
	return _farAddress;
}

string TCPCarrier::GetFarEndpointAddressIp() {
	if (_farIp != "")
		return _farIp;
	GetEndpointsInfo();
	return _farIp;
}

uint16_t TCPCarrier::GetFarEndpointPort() {
	if (_farPort != 0)
		return _farPort;
	GetEndpointsInfo();
	return _farPort;
}

SocketAddress &TCPCarrier::GetNearEndpointAddress() {
	if ((_nearIp == "") || (_nearPort == 0))
		GetEndpointsInfo();
	return _nearAddress;
}

string TCPCarrier::GetNearEndpointAddressIp() {
	if (_nearIp != "")
		return _nearIp;
	GetEndpointsInfo();
	return _nearIp;
}

uint16_t TCPCarrier::GetNearEndpointPort() {
	if (_nearPort != 0)
		return _nearPort;
	GetEndpointsInfo();
	return _nearPort;
}

bool TCPCarrier::GetEndpointsInfo() {
	if ((_farIp != "")
			&& (_farPort != 0)
			&& (_nearIp != "")
			&& (_nearPort != 0)) {
		return true;
	}
	socklen_t len = _farAddress.getLength();
	if (getpeername(_inboundFd, _farAddress.getSockAddr(), &len) != 0) {
		DWORD err = WSAGetLastError();
		FATAL("Unable to get peer's address. Error was: %"PRIu32, err);
		return false;
	}

	_farIp = _farAddress.getIPv4();
	_farPort = _farAddress.getSockPort(); //----MARKED-SHORT----
	
	len = _nearAddress.getLength();
	if (getsockname(_inboundFd, _nearAddress.getSockAddr(), &len) != 0) {
		FATAL("Unable to get peer's address");
		return false;
	}

	_nearIp = _nearAddress.getIPv4();
	_nearPort = _nearAddress.getSockPort(); //----MARKED-SHORT----

	return true;
}

#endif /* NET_IOCP */
