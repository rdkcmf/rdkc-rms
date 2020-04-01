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




#ifdef HAS_UDS
#ifdef NET_KQUEUE
#include "netio/kqueue/udsacceptor.h"
#include "netio/kqueue/iohandlermanager.h"
#include "protocols/protocolfactorymanager.h"
//#include "protocols/tcpprotocol.h"
#include "netio/kqueue/udscarrier.h"
#include "application/baseclientapplication.h"

UDSAcceptor::UDSAcceptor(string pathname, Variant parameters,
		vector<uint64_t>/*&*/ protocolChain)
: IOHandler(0, 0, IOHT_UDS_ACCEPTOR) {
//	INFO("[UDS] Creating UDS Acceptor");
	_pApplication = NULL;
	memset(&_address, 0, sizeof (sockaddr_un));
	_abstractPad = 1;
	_protocolChain = protocolChain;
	_parameters = parameters;
	_enabled = false;
	_acceptedCount = 0;
	_droppedCount = 0;
	_pathname = pathname;
	if (_parameters.HasKeyChain(V_BOOL, false, 1, "abstract")) {
		_abstractPad = (bool)_parameters["abstract"] ? 1 : 0;
	}
	MakeAddr(pathname, _address, _socketLength);
//	INFO("[UDS] Namespace set to %sabstract", _abstractPad==0 ? "non-" : "");
}

UDSAcceptor::~UDSAcceptor() {
	CLOSE_SOCKET(_inboundFd);
}

bool UDSAcceptor::Bind() {
	INFO("[UDS] Creating socket");
	_inboundFd = _outboundFd = (int) socket(AF_LOCAL, SOCK_STREAM, 0); //NOINHERIT
	if (_inboundFd < 0) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
		return false;
	}

	INFO("[UDS] Binding socket");
	if (bind(_inboundFd, (sockaddr *) &_address, _socketLength) != 0) {
		int err = errno;
		FATAL("Unable to bind on address: %s; Error was: (%d) %s",
				_address.sun_path + _abstractPad,
				err,
				strerror(err));
		return false;
	}

	INFO("[UDS] Listening...");
	if (listen(_inboundFd, 100) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

	_enabled = true;
	INFO("[UDS] Returning true");
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

bool UDSAcceptor::OnEvent(struct kevent &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool UDSAcceptor::OnConnectionAvailable(struct kevent &event) {
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

bool UDSAcceptor::MakeAddr(string name, struct sockaddr_un &pAddr, 
		socklen_t &pSockLen) {
//	INFO("[UDS] Making address");
	const char *c_name = STR(name);
	uint32_t nameLen = name.size();
	uint32_t limit = (uint32_t) sizeof(pAddr.sun_path) - _abstractPad;
	if (nameLen >= limit) {
		FATAL("Name too long. Length=%"PRIu32" Limit=%"PRIu32, nameLen, limit);
        	return false;
	}
//	INFO("[UDS] Setting namespace");
	if (_abstractPad == 1) {
//		INFO("[UDS] Setting beginning null char for abstract namespace");
		pAddr.sun_path[0] = '\0';  /* abstract namespace */
	}
	memcpy(pAddr.sun_path + _abstractPad, c_name, nameLen + _abstractPad);
//	INFO("[UDS] Setting socket family");
	pAddr.sun_family = AF_LOCAL;
//	INFO("[UDS] Setting addr length");
	pSockLen = _abstractPad + nameLen + offsetof(struct sockaddr_un, sun_path);
	pAddr.sun_len = pSockLen;
//	INFO("[UDS] Returning");
    return true;
}

string UDSAcceptor::ToString() {
	return format("AUDS(%"PRId32"; Path=%s)", _inboundFd, STR(_pathname));
}
#endif /* NET_KQUEUE */
#endif /* HAS_UDS */

