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

#include "netio/kqueue/udpcarrier.h"
#include "netio/kqueue/iohandlermanager.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"

UDPCarrier::UDPCarrier(int32_t fd)
: IOHandler(fd, fd, IOHT_UDP_CARRIER) {
	_nearIp = "";
	_nearPort = 0;
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;
	_surviveError = false;

	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierCreated(stats);
}

UDPCarrier::~UDPCarrier() {
	Variant stats;
	GetStats(stats);
	EventLogger::GetDefaultLogger()->LogCarrierClosed(stats);

	CLOSE_SOCKET(_inboundFd);
}

bool UDPCarrier::OnEvent(struct kevent &event) {
	//3. Do the I/O
	if (_pProtocol == NULL) {
		FATAL("Carrier protocol is already invalid!");
		return false;
	}

	switch (event.filter) {
		case EVFILT_READ:
		{
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			if (pInputBuffer == NULL) {
				FATAL("Carrier protocol buffer is already invalid!");
				return false;
			}
			//o_assert(pInputBuffer != NULL);
			if (!pInputBuffer->ReadFromUDPFd(event.ident, _ioAmount, _peerAddress)) {
				FATAL("Unable to read data");
				return _surviveError;
			}
			_rx += _ioAmount;
			ADD_IN_BYTES_MANAGED(_type, _ioAmount);
			return _pProtocol->SignalInputData(_ioAmount, &_peerAddress);
		}
		case EVFILT_WRITE:
		{
			_pProtocol->ReadyForSend();
			return true;
		}
		default:
		{
			ASSERT("Invalid state: %hu", event.filter);
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

UDPCarrier* UDPCarrier::Create(string bindIp, uint16_t bindPort, uint16_t ttl,
		uint16_t tos, string ssmIp) {

	//1. Create the socket
    // Dual stack support: we default to IPv6
	// We assume that we can create an IPv6 socket, otherwise this will fail
	// TODO: handle the use case that this indeed fails
	int sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if ((sock < 0) || (!setFdCloseOnExec(sock))) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
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

    if ( !bindAddress.setIPAddress(bindIp, bindPort) ) {
		FATAL("Unable to bind on address %s:%hu", STR(bindIp), bindPort);
		CLOSE_SOCKET(sock);
		return NULL;
	}

    if ( bindAddress.isMulticast() ) {
		INFO("Subscribe to multicast %s:%"PRIu16, STR(bindAddress.toString()), bindAddress.getSockPort());
		int activateBroadcast = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &activateBroadcast,
				sizeof (activateBroadcast)) != 0) {
			int err = errno;
			FATAL("Unable to activate SO_BROADCAST on the socket: (%d) %s",
					err, strerror(err));
			return NULL;
		}
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
		int err = errno;
		FATAL("Unable to bind on address: udp://%s:%"PRIu16"; Error was: (%d) %s",
				STR(bindAddress.toString()), bindAddress.getSockPort(), err, strerror(err));
		CLOSE_SOCKET(sock);
		return NULL;
	}
	if (bindAddress.isMulticast()) {
		if (!setFdJoinMulticast(sock, bindAddress.getIPv4(), bindAddress.getSockPort(), ssmIp, bindAddress.getFamily())) {
			FATAL("Adding multicast failed");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

	//4. Create the carrier
	UDPCarrier *pResult = new UDPCarrier(sock);
	pResult->_nearAddress = bindAddress;

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
	socklen_t len = sizeof (sockaddr);
	if (getsockname(_inboundFd, (sockaddr *) & _nearAddress, &len) != 0) {
		FATAL("Unable to get peer's address");
		return false;
	}
	_nearIp = format("%s", inet_ntoa(((sockaddr_in *) & _nearAddress)->sin_addr));
	_nearPort = ENTOHS(((sockaddr_in *) & _nearAddress)->sin_port); //----MARKED-SHORT----
	return true;
}

#endif /* NET_KQUEUE */
