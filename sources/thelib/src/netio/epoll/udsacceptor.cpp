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


#include "protocols/rtmp/amf0serializer.h"


#ifdef HAS_UDS
#ifdef NET_EPOLL
#include "netio/epoll/udsacceptor.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/protocolfactorymanager.h"
//#include "protocols/tcpprotocol.h"
#include "netio/epoll/udscarrier.h"
#include "application/baseclientapplication.h"

UDSAcceptor::UDSAcceptor(string pathname, Variant parameters,
		vector<uint64_t>/*&*/ protocolChain)
: IOHandler(0, 0, IOHT_UDS_ACCEPTOR) {
	_pApplication = NULL;
	memset(&_address, 0, sizeof (sockaddr_in));
	MakeAddr(STR(pathname), &_address, &_socketLength);
	
	_protocolChain = protocolChain;
	_parameters = parameters;
	_enabled = false;
	_acceptedCount = 0;
	_droppedCount = 0;
	_pathname = pathname;
}

UDSAcceptor::~UDSAcceptor() {
	CLOSE_SOCKET(_inboundFd);
}

bool UDSAcceptor::Bind() {
	_inboundFd = _outboundFd = (int) socket(AF_LOCAL, SOCK_STREAM, PF_UNIX); //NOINHERIT
	if (_inboundFd < 0) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
		return false;
	}

	if (bind(_inboundFd, (sockaddr *) & _address, _socketLength) != 0) {
		int err = errno;
		FATAL("Unable to bind on address: %s; Error was: (%d) %s",
				_address.sun_path + 1,
				err,
				strerror(err));
		return false;
	}

	if (listen(_inboundFd, 100) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

	_enabled = true;
	return true;
}

void UDSAcceptor::SetApplication(BaseClientApplication *pApplication) {
	o_assert(_pApplication == NULL);
	_pApplication = pApplication;
}

bool UDSAcceptor::StartAccept() {
	return IOHandlerManager::EnableAcceptConnections(this);
}

bool UDSAcceptor::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool UDSAcceptor::OnEvent(struct epoll_event &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool UDSAcceptor::OnConnectionAvailable(struct epoll_event &event) {
	return Accept();
}

bool UDSAcceptor::Accept() {
//	sockaddr address;
//	memset(&address, 0, sizeof (sockaddr));
//	socklen_t len = sizeof (sockaddr);
	int32_t fd;

	//1. Accept the connection
	fd = accept(_inboundFd, NULL, NULL);
	if (fd < 0) {
		int err = errno;
		FATAL("Unable to accept client connection: (%d) %s", err, strerror(err));
		return false;
	}
	if (!_enabled) {
		CLOSE_SOCKET(fd);
		_droppedCount++;
		WARN("Acceptor is not enabled. Client dropped");
		return true;
	}
	/*
	if (!setFdOptions(fd, false)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(fd);
		return false;
	}
	*/
	//4. Create the chain
	BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(_protocolChain, _parameters);
	if (pProtocol == NULL) {
		FATAL("Unable to create protocol chain");
		CLOSE_SOCKET(fd);
		return false;
	}

	//5. Create the carrier and bind it
//	TCPCarrier *pTCPCarrier = new TCPCarrier(fd);
//	pTCPCarrier->SetProtocol(pProtocol->GetFarEndpoint());
//	pProtocol->GetFarEndpoint()->SetIOHandler(pTCPCarrier);
//	if (_parameters.HasKeyChain(V_STRING, false, 1, "witnessFile"))
//		pProtocol->GetNearEndpoint()->SetWitnessFile((string) _parameters.GetValue("witnessFile",false));

	UDSCarrier * pCarrier = new UDSCarrier(fd);
	pCarrier->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pCarrier);

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
	return true;
}

bool UDSAcceptor::Drop() {
//	sockaddr address;
//	memset(&address, 0, sizeof (sockaddr));
//	socklen_t len = sizeof (sockaddr);


	//1. Accept the connection
	int32_t fd = accept(_inboundFd, NULL, NULL);
	if (fd < 0) {
		int err = errno;
		if (err != EWOULDBLOCK)
			WARN("Accept failed. Error code was: (%d) %s", err, strerror(err));
		return false;
	}

	//2. Drop it now
	CLOSE_SOCKET(fd);
	_droppedCount++;

	INFO("Client explicitly dropped");
	return true;
}

Variant & UDSAcceptor::GetParameters() {
	return _parameters;
}

BaseClientApplication *UDSAcceptor::GetApplication() {
	return _pApplication;
}

vector<uint64_t> &UDSAcceptor::GetProtocolChain() {
	return _protocolChain;
}

void UDSAcceptor::GetStats(Variant &info, uint32_t namespaceId) {
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

bool UDSAcceptor::Enable() {
	return _enabled;
}

void UDSAcceptor::Enable(bool enabled) {
	_enabled = enabled;
}

bool UDSAcceptor::IsAlive() {
	//TODO: Implement this. It must return true
	//if this acceptor is operational
	NYI;
	return true;
}

bool UDSAcceptor::MakeAddr(const char* name, struct sockaddr_un* pAddr, 
		socklen_t* pSockLen) {
    uint32_t nameLen = strlen(name);
    if (nameLen >= (uint32_t) sizeof(pAddr->sun_path) -1) {
		FATAL("Name too long");
        return false;
	}
    pAddr->sun_path[0] = '\0';  /* abstract namespace */
    strcpy(pAddr->sun_path + 1, name);
    pAddr->sun_family = AF_LOCAL;
    *pSockLen = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);
    return true;
}

string UDSAcceptor::ToString() {
	return format("AUDS(%"PRId32"; Path=%s)", _inboundFd, STR(_pathname));
}
#endif /* NET_EPOLL */
#endif /* HAS_UDS */

