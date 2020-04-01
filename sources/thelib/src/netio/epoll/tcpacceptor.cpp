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



#ifdef NET_EPOLL
#include "netio/epoll/tcpacceptor.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/tcpprotocol.h"
#include "netio/epoll/tcpcarrier.h"
#include "application/baseclientapplication.h"
#include "utils/misc/socketaddress.h"

TCPAcceptor::TCPAcceptor(string ipAddress, uint16_t port, Variant parameters,
		vector<uint64_t>/*&*/ protocolChain)
: IOHandler(0, 0, IOHT_ACCEPTOR) {
	_pApplication = NULL;

	o_assert (_address.setIPAddress(ipAddress, port)==true);

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
}

bool TCPAcceptor::Bind() {
	_inboundFd = _outboundFd = (int) socket(_address.getSockAddr()->sa_family, SOCK_STREAM, 0); //NOINHERIT
	if (_inboundFd < 0) {
		int err = errno;
		FATAL("Unable to create socket: (%d) %s", err, strerror(err));
		return false;
	}

	if (!setFdOptions(_inboundFd, false)) {
		FATAL("Unable to set socket options");
		return false;
	}

	if (bind(_inboundFd, _address.getSockAddr(), _address.getLength()) != 0) {
		int err = errno;
		FATAL("Unable to bind on address: tcp://%s:%hu; Error was: (%d) %s",
				STR(_address.toString()),
				_address.getSockPort(),
				err,
				strerror(err));
		return false;
	}

	if (_port == 0) {
		socklen_t tempSize = _address.getLength();
		if (getsockname(_inboundFd, _address.getSockAddr(), &tempSize) != 0) {
			FATAL("Unable to extract the random port");
			return false;
		}
		_parameters[CONF_PORT] = (uint16_t) _address.getSockPort();
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
	return IOHandlerManager::EnableAcceptConnections(this);
}

bool TCPAcceptor::SignalOutputData() {
	ASSERT("Operation not supported");
	return false;
}

bool TCPAcceptor::OnEvent(struct epoll_event &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool TCPAcceptor::OnConnectionAvailable(struct epoll_event &event) {
	if (_pApplication == NULL)
		return Accept();
	return _pApplication->AcceptTCPConnection(this);
}

bool TCPAcceptor::Accept() {
	SocketAddress address;
	socklen_t len = address.getLength(); // returns ipv6 size by default...
	                                     // should do sizeof (sockaddr_storage) instead

	int32_t fd;

	//1. Accept the connection
	fd = accept(_inboundFd, address.getSockAddr(), &len);
	if ((fd < 0) || (!setFdCloseOnExec(fd))) {
		int err = errno;
		FATAL("Unable to accept client connection: (%d) %s", err, strerror(err));
		return false;
	}
	if (!_enabled) {
		CLOSE_SOCKET(fd);
		_droppedCount++;
		WARN("Acceptor is not enabled. Client dropped: %s:%"PRIu16" -> %s:%"PRIu16,
				STR(address.toString()),
				address.getSockPort(),
				STR(_ipAddress),
				_port);
		return true;
	}

	if (!setFdOptions(fd, false)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(fd);
		return false;
	}

	//4. Create the chain
	BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(_protocolChain, _parameters);
	if (pProtocol == NULL) {
		FATAL("Unable to create protocol chain");
		CLOSE_SOCKET(fd);
		return false;
	}

	//5. Create the carrier and bind it
	TCPCarrier *pTCPCarrier = new TCPCarrier(fd);
	pTCPCarrier->SetProtocol(pProtocol->GetFarEndpoint());
	pProtocol->GetFarEndpoint()->SetIOHandler(pTCPCarrier);
	if (_parameters.HasKeyChain(V_STRING, false, 1, "witnessFile"))
		pProtocol->GetNearEndpoint()->SetWitnessFile((string) _parameters.GetValue("witnessFile",false));

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

bool TCPAcceptor::Drop() {
	sockaddr address;
	memset(&address, 0, sizeof (sockaddr));
	socklen_t len = sizeof (sockaddr);


	//1. Accept the connection
	int32_t fd = accept(_inboundFd, &address, &len);
	if ((fd < 0) || (!setFdCloseOnExec(fd))) {
		int err = errno;
		if (err != EWOULDBLOCK)
			WARN("Accept failed. Error code was: (%d) %s", err, strerror(err));
		return false;
	}

	//2. Drop it now
	CLOSE_SOCKET(fd);
	_droppedCount++;

	INFO("Client explicitly dropped: %s:%"PRIu16" -> %s:%"PRIu16,
			inet_ntoa(((sockaddr_in *) & address)->sin_addr),
			ENTOHS(((sockaddr_in *) & address)->sin_port),
			STR(_ipAddress),
			_port);
	return true;
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

bool TCPAcceptor::IsAlive() {
	//TODO: Implement this. It must return true
	//if this acceptor is operational
	NYI;
	return true;
}

#endif /* NET_EPOLL */
