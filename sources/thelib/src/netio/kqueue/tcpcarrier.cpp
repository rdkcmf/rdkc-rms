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
#include "netio/kqueue/tcpcarrier.h"
#include "netio/kqueue/iohandlermanager.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"

#define ENABLE_WRITE_DATA \
if (!_writeDataEnabled) { \
    _writeDataEnabled = true; \
    IOHandlerManager::EnableWriteData(this); \
} \
_enableWriteDataCalled=true;

#define DISABLE_WRITE_DATA \
if (_writeDataEnabled) { \
	_enableWriteDataCalled=false; \
	_pProtocol->ReadyForSend(); \
	if(!_enableWriteDataCalled) { \
		if(_pProtocol->GetOutputBuffer()==NULL) {\
			_writeDataEnabled = false; \
			IOHandlerManager::DisableWriteData(this); \
		} \
	} \
}

TCPCarrier::TCPCarrier(int32_t fd)
: IOHandler(fd, fd, IOHT_TCP_CARRIER) {
	IOHandlerManager::EnableReadData(this);
	_writeDataEnabled = false;
	_enableWriteDataCalled = false;
//	memset(&_farAddress, 0, sizeof (sockaddr_in));
	_farIp = "";
	_farPort = 0;
//	memset(&_nearAddress, 0, sizeof (sockaddr_in));
	_nearIp = "";
	_nearPort = 0;
	GetEndpointsInfo();
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;
	_lastRecvError = 0;
	_lastSendError = 0;

	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierCreated(stats);
}

TCPCarrier::~TCPCarrier() {
	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierClosed(stats);

	CLOSE_SOCKET(_inboundFd);
}

bool TCPCarrier::OnEvent(struct kevent &event) {
	//3. Do the I/O
	switch (event.filter) {
		case EVFILT_READ:
		{
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			o_assert(pInputBuffer != NULL);
			if (!pInputBuffer->ReadFromTCPFd(event.ident, event.data, _ioAmount,
					_lastRecvError)) {
				FATAL("Unable to read data from connection: %s. Error was (%d): %s",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "", _lastRecvError,
						strerror(_lastRecvError));
				return false;
			}
			_rx += _ioAmount;
			ADD_IN_BYTES_MANAGED(_type, _ioAmount);
			if (!_pProtocol->SignalInputData(_ioAmount)) {
				FATAL("Unable to read data from connection: %s. Signaling upper protocols failed",
						(_pProtocol != NULL) ? STR(*_pProtocol) : "");
				return false;
			}
			return true;
		}
		case EVFILT_WRITE:
		{
			IOBuffer *pOutputBuffer = NULL;
			if ((pOutputBuffer = _pProtocol->GetOutputBuffer()) != NULL) {
				if (!pOutputBuffer->WriteToTCPFd(event.ident, event.data,
						_ioAmount, _lastSendError)) {
					FATAL("Unable to write data on connection: %s. Error was (%d): %s",
							(_pProtocol != NULL) ? STR(*_pProtocol) : "", _lastSendError,
							strerror(_lastSendError));
					IOHandlerManager::EnqueueForDelete(this);
					return false;
				}
				_tx += _ioAmount;
				ADD_OUT_BYTES_MANAGED(_type, _ioAmount);
				if (GETAVAILABLEBYTESCOUNT(*pOutputBuffer) == 0) {
					DISABLE_WRITE_DATA;
				}
			} else {
				DISABLE_WRITE_DATA;
			}
			return true;
		}
		default:
		{
			FATAL("Unable to read/write data from/to connection: %s. Invalid event type: %d",
					(_pProtocol != NULL) ? STR(*_pProtocol) : "", event.filter);
			return false;
		}
	}
}

bool TCPCarrier::SignalOutputData() {
	ENABLE_WRITE_DATA;
	return true;
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
	socklen_t len = _farAddress.getLength();;
	// if (getpeername(_inboundFd, (sockaddr *) & _farAddress, &len) != 0) {
	if (getpeername(_inboundFd, (sockaddr *) _farAddress.getSockAddr(), &len) != 0) {		
		FATAL("Unable to get peer's address");
		return false;
	}
	// _farIp = format("%s", inet_ntoa(((sockaddr_in *) & _farAddress)->sin_addr));
	// _farPort = ENTOHS(((sockaddr_in *) & _farAddress)->sin_port); //----MARKED-SHORT----
	_farIp = _farAddress.getIPv4();
	_farPort = _farAddress.getSockPort();


	// if (getsockname(_inboundFd, (sockaddr *) & _nearAddress, &len) != 0) {
	if (getsockname(_inboundFd, _nearAddress.getSockAddr(), &len) != 0) {		
		FATAL("Unable to get peer's address");
		return false;
	}
	// _nearIp = format("%s", inet_ntoa(((sockaddr_in *) & _nearAddress)->sin_addr));
	// _nearPort = ENTOHS(((sockaddr_in *) & _nearAddress)->sin_port); //----MARKED-SHORT----
	_nearIp = _nearAddress.getIPv4();
	_nearPort = _nearAddress.getSockPort();
	return true;
}

#endif /* NET_KQUEUE */

