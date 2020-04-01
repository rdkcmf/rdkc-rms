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
#include "netio/iocp/tcpacceptor.h"
#include "netio/iocp/iohandlermanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/tcpprotocol.h"
#include "netio/iocp/tcpcarrier.h"
#include "application/baseclientapplication.h"
#include "utils/misc/socketaddress.h"
#include <Mswsock.h>

TCPAcceptor::TCPAcceptor(string ipAddress, uint16_t port, Variant parameters,
		vector<uint64_t>/*&*/ protocolChain)
: IOHandler(0, 0, IOHT_ACCEPTOR) {
	_pEvent = iocp_event_accept::GetInstance(this);
	_pApplication = NULL;

	INFO("TCPAcceptor setting address: %s ", STR(ipAddress));
	_address.setIPAddress(ipAddress, port);
	_protocolChain = protocolChain;
	_parameters = parameters;
	_enabled = false;
	_acceptedCount = 0;
	_droppedCount = 0;
	_ipAddress = ipAddress;
	_port = port;
}

TCPAcceptor::~TCPAcceptor() {
	CLOSE_SOCKET(_inboundFd);
	_pEvent->listenSock = (SOCKET) - 1;
	_pEvent->Release();
	_pEvent = NULL;
}

void TCPAcceptor::CancelIO() {
	CancelIoEx((HANDLE) _inboundFd, _pEvent);
}

iocp_event_accept *TCPAcceptor::GetEvent() {
	return _pEvent;
}

bool TCPAcceptor::Bind() {
	//_pEvent->listenSock=_inboundFd = _outboundFd = (int) WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	_pEvent->listenSock = _inboundFd = _outboundFd = (int) socket(_address.getFamily(), SOCK_STREAM, 0); //NOINHERIT
	if (_inboundFd == INVALID_SOCKET) {
		int err = LASTSOCKETERROR;
		FATAL("Unable to create socket: %d (%s)", err, STR(wsaError2String(err)));
		return false;
	}
	if (!setFdOptions(_inboundFd, false)) {
		FATAL("Unable to set socket options");
		return false;
	}

	if (bind(_inboundFd, _address.getSockAddr(), _address.getLength()) != 0) {
		int err = LASTSOCKETERROR;
		FATAL("Unable to bind on address: tcp://%s:%hu; Error was: %d (%s)",
				STR(_address.toString()),
				_address.getSockPort(),
				err,
				STR(wsaError2String(err)));
		return false;
	}

	if (_port == 0) {
		socklen_t tempSize = _address.getLength();
		if (getsockname(_inboundFd, _address.getSockAddr(), &tempSize) != 0) {
			FATAL("Unable to extract the random port");
			return false;
		}
		_parameters[CONF_PORT] = (uint16_t)_address.getSockPort();
	}

	if (listen(_inboundFd, 100) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

	_enabled = true;
	return true;
}

void TCPAcceptor::SetApplication(BaseClientApplication *pApplication) {
	o_assert(_pApplication == NULL);
	_pApplication = pApplication;
}

bool TCPAcceptor::StartAccept() {
	if (!IOHandlerManager::RegisterIOCPFD(_inboundFd)) {
		FATAL("Unable to register IOCP Fd");
		return false;
	}
	return IOHandlerManager::EnableAcceptConnections(this);
}

bool TCPAcceptor::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool TCPAcceptor::OnEvent(iocp_event &event) {
	return OnConnectionAvailable(event);
}

bool TCPAcceptor::OnConnectionAvailable(iocp_event &event) {
	if (_pApplication == NULL)
		return Accept();
	return _pApplication->AcceptTCPConnection(this);
}

bool TCPAcceptor::Accept() {
	DWORD temp = 0;
	BOOL result = GetOverlappedResult((HANDLE) _inboundFd, _pEvent, &temp, true);
	if (!result) {
		DWORD err = GetLastError();
		FATAL("Acceptor failed. Error code was: %"PRIu32, err);
		return false;
	}

	sockaddr *pLocalAddress = NULL;
	sockaddr *pRemoteAddress = NULL;
	int dummy = 0;
	GetAcceptExSockaddrs(
			_pEvent->pBuffer,			// lpOutputBuffer,
			0,							//dwReceiveDataLength,
			sizeof (sockaddr_in6) + 16, //dwLocalAddressLength,
			sizeof (sockaddr_in6) + 16, //dwRemoteAddressLength,
			&pLocalAddress,				// LocalSockaddr,
			&dummy,						//LocalSockaddrLength,
			&pRemoteAddress,			//RemoteSockaddr,
			&dummy						// RemoteSockaddrLength
			);
	//FINEST("HANDLE: %"PRIz"u", sizeof (HANDLE));
	//FINEST("SOCKET: %"PRIz"u", sizeof (SOCKET));
	//FINEST("int: %"PRIz"u", sizeof (int));
	//FINEST("_pEvent->listenSock: %"PRIz"u", sizeof (_pEvent->listenSock));
	//int one=_pEvent->listenSock;
	/*string str=format("sizeof(one): %"PRIz"u; sizeof(HANDLE): %"PRIz"u; one: %d; ",sizeof(one),sizeof(HANDLE),one);
	for(uint32_t i=0;i<sizeof(int);i++){
		str+=format("%02"PRIx8,((uint8_t *)&one)[sizeof(int)-1-i]);
	}
	FINEST("%s",STR(str));*/
	if (setsockopt(_pEvent->acceptedSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *) &_pEvent->listenSock, sizeof (_pEvent->listenSock)) != 0) {
		CLOSE_SOCKET(_pEvent->acceptedSock);
		_droppedCount++;
		DWORD err = WSAGetLastError();
		FATAL("Unable to set SO_UPDATE_ACCEPT_CONTEXT. Client dropped: %s:%"PRIu16" -> %s:%"PRIu16"; error: %"PRIu32,
				inet_ntoa(((sockaddr_in *) pLocalAddress)->sin_addr),
				ENTOHS(((sockaddr_in *) pLocalAddress)->sin_port),
				STR(_ipAddress),
				_port,
				err);
		return false;
	}

	if (!_enabled) {
		CLOSE_SOCKET(_pEvent->acceptedSock);
		_droppedCount++;
		WARN("Acceptor is not enabled. Client dropped: %s:%"PRIu16" -> %s:%"PRIu16,
				inet_ntoa(((sockaddr_in *) pLocalAddress)->sin_addr),
				ENTOHS(((sockaddr_in *) pLocalAddress)->sin_port),
				STR(_ipAddress),
				_port);
		return true;
	}

	if (!setFdOptions(_pEvent->acceptedSock, false)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(_pEvent->acceptedSock);
		return false;
	}

	//4. Create the chain
	BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(
			_protocolChain, _parameters);
	if (pProtocol == NULL) {
		FATAL("Unable to create protocol chain");
		CLOSE_SOCKET(_pEvent->acceptedSock);
		return false;
	}

	//5. Create the carrier and bind it
	TCPCarrier *pTCPCarrier = new TCPCarrier(_pEvent->acceptedSock, true);
	pTCPCarrier->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pTCPCarrier);
	if (_parameters.HasKeyChain(V_STRING, false, 1, "witnessFile"))
		pProtocol->GetNearEndpoint()->SetWitnessFile((string) _parameters.GetValue("witnessFile", false));

	//6. Register the protocol stack with an application
	if (_pApplication != NULL) {
		pProtocol = pProtocol->GetNearEndpoint();
		pProtocol->SetApplication(_pApplication);

		//		EventLogger *pEvtLog = _pApplication->GetEventLogger();
		//		if (pEvtLog != NULL) {
		//			pEvtLog->LogInboundConnectionStart(_ipAddress, _port, STR(*(pProtocol->GetFarEndpoint())));
		//			pTCPCarrier->SetEventLogger(pEvtLog);
		//		}
	}

	if (pProtocol->GetNearEndpoint()->GetOutputBuffer() != NULL)
		pProtocol->GetNearEndpoint()->EnqueueForOutbound();

	_acceptedCount++;

	INFO("Inbound connection accepted: %s", STR(*(pProtocol->GetNearEndpoint())));

	//7. Done
	return IOHandlerManager::EnableAcceptConnections(this);
}

bool TCPAcceptor::Drop() {
	NYIR;
}

Variant & TCPAcceptor::GetParameters() {
	return _parameters;
}

BaseClientApplication *TCPAcceptor::GetApplication() {
	return _pApplication;
}

vector<uint64_t> &TCPAcceptor::GetProtocolChain() {
	return _protocolChain;
}

void TCPAcceptor::GetStats(Variant &info, uint32_t namespaceId) {
	info = _parameters;
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
	info["enabled"] = (bool)_enabled;
	info["acceptedConnectionsCount"] = _acceptedCount;
	info["droppedConnectionsCount"] = _droppedCount;
	if (_pApplication != NULL) {
		info["appId"] = (((uint64_t) namespaceId) << 32) | _pApplication->GetId();
		info["appName"] = _pApplication->GetName();
	} else {
		info["appId"] = (((uint64_t) namespaceId) << 32);
		info["appName"] = "";
	}
}

bool TCPAcceptor::Enable() {
	return _enabled;
}

void TCPAcceptor::Enable(bool enabled) {
	_enabled = enabled;
}

uint16_t TCPAcceptor::getAddressFamily() {
	return _address.getFamily();
}

#endif /* NET_IOCP */
