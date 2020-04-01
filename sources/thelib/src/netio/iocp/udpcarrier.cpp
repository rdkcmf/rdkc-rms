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

#include "netio/iocp/udpcarrier.h"
#include "netio/iocp/iohandlermanager.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"
#include "utils/misc/socketaddress.h"

UDPCarrier::UDPCarrier(SOCKET fd)
: IOHandler(fd, fd, IOHT_UDP_CARRIER) {
	_nearIp = "";
	_nearPort = 0;
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;
	_surviveError = false;
	_pReadEvent = iocp_event_udp_read::GetInstance(this);
	IOHandlerManager::RegisterIOCPFD(GetInboundFd());

	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierCreated(stats);
}

UDPCarrier::~UDPCarrier() {
	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierClosed(stats);

	CLOSE_SOCKET(_inboundFd);
	_pReadEvent->Release();
	_pReadEvent = NULL;
}

void UDPCarrier::CancelIO() {
	CancelIoEx((HANDLE) _inboundFd, _pReadEvent);
}

bool UDPCarrier::OnEvent(iocp_event &event) {
	//FINEST("%"PRIu32,_inboundFd);
	DWORD temp = 0;
	BOOL result = GetOverlappedResult((HANDLE) _inboundFd, &event, &temp, true);
	if (!result) {
		DWORD err = GetLastError();
		FATAL("UDPCarrier failed. Error code was: %"PRIu32" (%s)", err, STR(wsaError2String(err)) );
		return _surviveError;	// return true if we have _surviveError set
	}
	switch (event.type) {
		case iocp_event_type_udp:
		{
			if (_pReadEvent->transferedBytes == 0) {
				FATAL("EOF encountered");
				return false;
			}
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			o_assert(pInputBuffer != NULL);
			if (!pInputBuffer->ReadFromBuffer((uint8_t *) _pReadEvent->inputBuffer.buf, _pReadEvent->transferedBytes)) {
				FATAL("Unable to read data");
				return false;
			}
			_ioAmount = _pReadEvent->transferedBytes;
			_rx += _ioAmount;
			ADD_IN_BYTES_MANAGED(_type, _ioAmount);
			if (!_pProtocol->SignalInputData(_ioAmount, & _pReadEvent->sourceAddress)) {
				FATAL("Unable to signal upper protocols");
				return false;
			}

			_pReadEvent->transferedBytes = 0;
			_pReadEvent->flags = 0;
			if (WSARecvFrom( _inboundFd, 
							&_pReadEvent->inputBuffer, 1, 
							&_pReadEvent->transferedBytes, 
							&_pReadEvent->flags, 
							//(sockaddr *)& _pReadEvent->sourceAddress,
							_pReadEvent->sourceAddress.getSockAddr(),
							&_pReadEvent->sourceLen, _pReadEvent, NULL)) {

				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING) {
					FATAL("Unable to call WSARecvFrom. Error was: %"PRIu32, err);
					_pReadEvent->isValid = false;
					return _surviveError;	// return true if we have _surviveError set
				}

			}
			_pReadEvent->pendingOperation = true;
			return true;
		}
		default:
		{
			FATAL("Invalid event type: %d", event.type);
			return false;
		}
	}

}

bool UDPCarrier::SignalOutputData() {
	uint8_t* pBuffer = GETIBPOINTER(*_pProtocol->GetOutputBuffer());
	int length = GETAVAILABLEBYTESCOUNT(*_pProtocol->GetOutputBuffer());

	SocketAddress* destInfo = _pProtocol->GetDestInfo();

	if (sendto(GetOutboundFd(), (char *)pBuffer, length, 0, destInfo->getSockAddr(), destInfo->getLength()) != length) {
			int err = LASTSOCKETERROR;
			FATAL("Unable to send data: %d", err);
			return false;
	}
	ADD_OUT_BYTES_MANAGED(IOHT_UDP_CARRIER, length);
	if (NULL != destInfo)
		delete destInfo;

	return true;
}

void UDPCarrier::GetStats(Variant &info, uint32_t namespaceId) {
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
	if (!GetEndpointsInfo()) {
		FATAL("Unable to get endpoints info");
		info = "unable to get endpoints info";
		return;
	}
	info["type"] = "IOHT_UDP_CARRIER";
	info["nearIP"] = _nearIp;
	info["nearPort"] = _nearPort;
	info["rx"] = _rx;
}

Variant &UDPCarrier::GetParameters() {
	return _parameters;
}

void UDPCarrier::SetParameters(Variant parameters) {
	_parameters = parameters;
}

bool UDPCarrier::StartAccept() {
	return IOHandlerManager::EnableReadData(this);
}

string UDPCarrier::GetFarEndpointAddress() {
	ASSERT("Operation not supported");
	return "";
}

uint16_t UDPCarrier::GetFarEndpointPort() {
	ASSERT("Operation not supported");
	return 0;
}

string UDPCarrier::GetNearEndpointAddress() {
	if (_nearIp != "")
		return _nearIp;
	GetEndpointsInfo();
	return _nearIp;
}

uint16_t UDPCarrier::GetNearEndpointPort() {
	if (_nearPort != 0)
		return _nearPort;
	GetEndpointsInfo();
	return _nearPort;
}

iocp_event_udp_read *UDPCarrier::GetReadEvent() {
	return _pReadEvent;
}

UDPCarrier* UDPCarrier::Create(string bindIp, uint16_t bindPort, uint16_t ttl,
		uint16_t tos, string ssmIp) {

	//TODO : to check
	//Another TODO: why create the temporary socket address?
	//1. Create the socket
	SocketAddress tempSock;
	tempSock.setIPAddress(bindIp, bindPort);

	// Dual stack support: we default to IPv6
	// We assume that we can create an IPv6 socket, otherwise this will fail
	// TODO: handle the use case that this indeed fails
	SOCKET sock = socket(AF_INET6, SOCK_DGRAM, 0); //NOINHERIT
	if (((int) sock) < 0) {
		int err = LASTSOCKETERROR;
		FATAL("Unable to create socket: %d (%s)", err, STR(wsaError2String(err)));
		return NULL;
	}

	//2. fd options
	if (!setFdOptions(sock, true)) {
		FATAL("Unable to set fd options");
		CLOSE_SOCKET(sock);
		return NULL;
	}

	if (tos <= 255) {
		if (!setFdTOS(sock, (uint8_t) tos)) {
			FATAL("Unable to set tos");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

	//ULONG testVal1 = EHTONL(inet_addr(bindIp.c_str()));
	//bool isMulticast = ((testVal1 > 0xe0000000) && (testVal1 < 0xefffffff));
	bool isMulticast = tempSock.isMulticast();

	//3. Bind
	if (bindIp == "") {
		bindIp = "0.0.0.0";
		bindPort = 0;
	}

	SocketAddress bindAddress;
	
	// Set if we support built-in dual stack or not
	bool dualstack = true;
	if (!setFdIPv6Only(sock, false)) {
		WARN("Dual stack not supported.");
		dualstack = false;
	}
	bindAddress.setIPv6Support(true, dualstack, true);

	if (isMulticast) {
		// Set address to INADDR_ANY
		if (SocketAddress::isV6(bindIp)) {
			bindAddress.setIPAddress("::", bindPort);
		} else {
			bindAddress.setIPAddress("0.0.0.0", bindPort);
		}
	} else {
		if ( !bindAddress.setIPAddress(bindIp, bindPort ) ) {
			FATAL("Unable to bind on address %s:%hu", STR(bindIp), bindPort);
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

	/*
	bindAddress.sin_family = PF_INET;
	if (isMulticast)
		bindAddress.sin_addr.s_addr = INADDR_ANY;
	else
		bindAddress.sin_addr.s_addr = inet_addr(bindIp.c_str());
	bindAddress.sin_port = EHTONS(bindPort); //----MARKED-SHORT----
	if (bindAddress.sin_addr.s_addr == INADDR_NONE) {
		FATAL("Unable to bind on address %s:%hu", STR(bindIp), bindPort);
		CLOSE_SOCKET(sock);
		return NULL;
	}*/

	if (isMulticast) {
		INFO("Subscribe to multicast %s:%"PRIu16, STR(bindIp), bindPort);
		if (ttl <= 255) {
			if (!setFdMulticastTTL(sock, (uint8_t) ttl, bindAddress.getFamily())) {
				FATAL("Unable to set ttl");
				CLOSE_SOCKET(sock);
				return NULL;
			}
		}
	} else {
		if (ttl <= 255) {
			if (!setFdTTL(sock, (uint8_t) ttl)) {
				FATAL("Unable to set ttl");
				CLOSE_SOCKET(sock);
				return NULL;
			}
		}
	}
	if (bind(sock, bindAddress.getSockAddr(), bindAddress.getLength()) != 0) {
		int err = LASTSOCKETERROR;
		FATAL("Unable to bind on address: udp://%s:%"PRIu16"; Error was: %d (%s)",
				STR(bindAddress.toString()), bindAddress.getSockPort(), err, STR(wsaError2String(err)));
		CLOSE_SOCKET(sock);
		return NULL;
	}
	if (isMulticast) {
		if (!setFdJoinMulticast(sock, bindAddress.getIPv4(), bindAddress.getSockPort(), ssmIp, bindAddress.getFamily())) {
			FATAL("Adding multicast failed");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

	//4. Create the carrier
	UDPCarrier *pResult = new UDPCarrier(sock);

	pResult->_pReadEvent->sourceAddress = bindAddress;

	return pResult;
}

UDPCarrier* UDPCarrier::Create(string bindIp, uint16_t bindPort,
		BaseProtocol *pProtocol, uint16_t ttl, uint16_t tos, string ssmIp) {
	if (pProtocol == NULL) {
		FATAL("Protocol can't be null");
		return NULL;
	}

	UDPCarrier *pResult = Create(bindIp, bindPort, ttl, tos, ssmIp);
	if (pResult == NULL) {
		FATAL("Unable to create UDP carrier");
		return NULL;
	}

	pResult->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pResult);

	return pResult;
}

bool UDPCarrier::Setup(Variant &settings) {
	NYIR;
}

bool UDPCarrier::GetEndpointsInfo() {
	//if (getsockname(_inboundFd, (sockaddr *)& _pReadEvent->sourceAddress, &_pReadEvent->sourceLen) != 0) {
	if (getsockname(_inboundFd, _pReadEvent->sourceAddress.getSockAddr(), &_pReadEvent->sourceLen) != 0) {
		FATAL("Unable to get peer's address");
		return false;
	}
	_nearIp = _pReadEvent->sourceAddress.toString();
	_nearPort = _pReadEvent->sourceAddress.getSockPort(); //----MARKED-SHORT----
//	_nearIp = format("%s", inet_ntoa(((sockaddr_in *) & _pReadEvent->sourceAddress)->sin_addr));
//	_nearPort = ENTOHS(((sockaddr_in *) & _pReadEvent->sourceAddress)->sin_port); //----MARKED-SHORT----
	return true;
}
#endif /* NET_IOCP */
