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


#ifdef NET_SELECT
#ifndef _TCPCONNECTOR_H
#define	_TCPCONNECTOR_H


#include "netio/select/iohandler.h"
#include "protocols/baseprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/select/iohandlermanager.h"
#include "netio/select/tcpcarrier.h"
#include "utils/misc/socketaddress.h"

template<class T>
class DLLEXP TCPConnector
: public IOHandler {
private:
	string _ip;
	uint16_t _port;
	vector<uint64_t> _protocolChain;
	bool _closeSocket;
	Variant _customParameters;
	bool _success;
public:

	TCPConnector(int32_t fd, string ip, uint16_t port,
			vector<uint64_t>& protocolChain, const Variant& customParameters)
	: IOHandler(fd, fd, IOHT_TCP_CONNECTOR) {
		_ip = ip;
		_port = port;
		_protocolChain = protocolChain;
		_closeSocket = true;
		_customParameters = customParameters;
		_success = false;
	}

	virtual ~TCPConnector() {
		if (!_success) {
			T::SignalProtocolCreated(NULL, _customParameters);
		}
		if (_closeSocket) {
			CLOSE_SOCKET(_inboundFd);
		}
	}

	virtual bool SignalOutputData() {
		ASSERT("Operation not supported");
		return false;
	}

	virtual bool OnEvent(select_event &event) {
		IOHandlerManager::EnqueueForDelete(this);
		WARN("THIS IS NOT COMPLETELY IMPLEMENTED");

		BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(
				_protocolChain, _customParameters);

		if (pProtocol == NULL) {
			FATAL("Unable to create protocol chain");
			_closeSocket = true;
			return false;
		}

		TCPCarrier *pTCPCarrier = new TCPCarrier(_inboundFd);
		pTCPCarrier->SetProtocol(pProtocol->GetFarEndpoint());
		pProtocol->GetFarEndpoint()->SetIOHandler(pTCPCarrier);
		if (_customParameters.HasKeyChain(V_STRING, false, 1, "witnessFile"))
			pProtocol->GetNearEndpoint()->SetWitnessFile((string) _customParameters.GetValue("witnessFile", false));
		else if (_customParameters.HasKeyChain(V_STRING, false, 3, "customParameters", "externalStreamConfig", "_witnessFile"))
			pProtocol->GetNearEndpoint()->SetWitnessFile((string)
				(_customParameters.GetValue("customParameters", false)
				.GetValue("externalStreamConfig", false)
				.GetValue("_witnessFile", false))
				);
		else if (_customParameters.HasKeyChain(V_STRING, false, 3, "customParameters", "localStreamConfig", "_witnessFile"))
			pProtocol->GetNearEndpoint()->SetWitnessFile((string)
				(_customParameters.GetValue("customParameters", false)
				.GetValue("localStreamConfig", false)
				.GetValue("_witnessFile", false))
				);

		if (!T::SignalProtocolCreated(pProtocol, _customParameters)) {
			FATAL("Unable to signal protocol created");
			delete pProtocol;
			_closeSocket = true;
			return false;
		}
		_success = true;

		INFO("Outbound connection established: %s", STR(*pProtocol));

		_closeSocket = false;
		return true;
	}

	static bool Connect(string ip, uint16_t port,
			vector<uint64_t>& protocolChain, Variant customParameters) {

		// Dual stack support: we default to IPv6
		// We assume that we can create an IPv6 socket, otherwise this will fail
		// TODO: handle the use case that this indeed fails
		int32_t fd = (int32_t) socket(AF_INET6, SOCK_STREAM, 0);
		if ((fd < 0) || (!setFdCloseOnExec(fd))) {
			int err = LASTSOCKETERROR;
			T::SignalProtocolCreated(NULL, customParameters);
			FATAL("Unable to create fd: %d", err);
			return 0;
		}

		if (!setFdOptions(fd, false)) {
			CLOSE_SOCKET(fd);
			T::SignalProtocolCreated(NULL, customParameters);
			FATAL("Unable to set socket options");
			return false;
		}

		SocketAddress sendAddress;

		// Set if we support built-in dual stack or not
		bool dualstack = true;
		if (!setFdIPv6Only(fd, false)) {
			dualstack = false;
		}
		sendAddress.setIPv6Support(true, dualstack);

		if (!sendAddress.setIPAddress(ip, port)) {
			FATAL("Unable to set address %s:%hu", STR(ip), port);
			CLOSE_SOCKET(fd);
			return false;
		}

		TCPConnector<T> *pTCPConnector = new TCPConnector(fd, sendAddress.getIPv4(), sendAddress.getSockPort(),
				protocolChain, customParameters);

		if (!pTCPConnector->Connect()) {
			IOHandlerManager::EnqueueForDelete(pTCPConnector);
			FATAL("Unable to connect");
			return false;
		}

		return true;
	}

	bool Connect() {
		SocketAddress address;

		if (!address.setIPAddress(_ip, _port)) {
			FATAL("Unable to translate string %s to a valid IP address", STR(_ip));
			return 0;
		}

		if (!IOHandlerManager::EnableWriteData(this)) {
			FATAL("Unable to enable reading data");
			return false;
		}

		if (connect(_inboundFd, address.getSockAddr(), address.getLength()) != 0) {
			int err = LASTSOCKETERROR;
			if (err != SOCKERROR_EINPROGRESS) {
				FATAL("Unable to connect to %s:%hu %d", STR(_ip), _port, err);
				_closeSocket = true;
				return false;
			}
		}

		_closeSocket = false;
		return true;
	}

	virtual void GetStats(Variant &info, uint32_t namespaceId = 0) {

	}
};


#endif	/* _TCPCONNECTOR_H */
#endif /* NET_SELECT */
