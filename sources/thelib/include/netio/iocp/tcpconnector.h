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
#ifndef _TCPCONNECTOR_H
#define	_TCPCONNECTOR_H

#include "Mswsock.h"
#include "netio/iocp/iohandler.h"
#include "protocols/baseprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/iocp/iohandlermanager.h"
#include "netio/iocp/tcpcarrier.h"
#include "utils/misc/socketaddress.h"
//#include "eventlogger/eventlogger.h"
#include <Mswsock.h>

template<class T>
class TCPConnector
: public IOHandler {
private:
	string _ip;
	uint16_t _port;
	vector<uint64_t> _protocolChain;
	bool _closeSocket;
	Variant _customParameters;
	bool _success;
	iocp_event_connect *_pEvent;

public:

	TCPConnector(SOCKET fd, string ip, uint16_t port,
			vector<uint64_t>& protocolChain, const Variant& customParameters)
	: IOHandler(fd, fd, IOHT_TCP_CONNECTOR) {
		_ip = ip;
		_port = port;
		_protocolChain = protocolChain;
		_closeSocket = true;
		_customParameters = customParameters;
		_success = false;
		_pEvent = iocp_event_connect::GetInstance(this);
		_pEvent->closeSocket = _closeSocket;
		IOHandlerManager::RegisterIOCPFD(GetInboundFd());
	}

	virtual ~TCPConnector() {
		if (!_success) {
			T::SignalProtocolCreated(NULL, _customParameters);
		}
		if (_closeSocket) {
			CLOSE_SOCKET(_inboundFd);
		}
		if (_pEvent != NULL) {
//			_pEvent->closeSocket = true;
			_pEvent->Release();
			_pEvent = NULL;
		}
	}

	virtual void CancelIO() {
		CancelIoEx((HANDLE) _inboundFd, _pEvent);
	}

	virtual bool SignalOutputData() {
		ASSERT("Operation not supported");
		return false;
	}

	virtual bool OnEvent(iocp_event &event) {
		DWORD temp = 0;
		BOOL result = GetOverlappedResult((HANDLE) _inboundFd, &event, &temp, true);
		if (!result) {
			DWORD err = GetLastError();
			FATAL("TCPConnector failed. Error code was: %" PRIu32" (%s)", err, STR(wsaError2String(err)));
			_pEvent->closeSocket = true;
			return false;
		}

		if (setsockopt(_inboundFd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) != 0) {
			DWORD err = WSAGetLastError();
			WARN("Unable to set SO_UPDATE_ACCEPT_CONTEXT. Error:%" PRIu32" (%s)", err, STR(wsaError2String(err)));
			return true;
		}

		BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(_protocolChain, _customParameters);

		if (pProtocol == NULL) {
			FATAL("Unable to create protocol chain");
			_closeSocket = true;
			_pEvent->closeSocket = _closeSocket;
			return false;
		}

		TCPCarrier *pTCPCarrier = new TCPCarrier(_inboundFd, false); //inbound is already registered to iocp
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

		IOHandlerManager::EnqueueForDelete(this);

		if (!T::SignalProtocolCreated(pProtocol, _customParameters)) {
			FATAL("Unable to signal protocol created");
			delete pProtocol;
			_closeSocket = true;
			_pEvent->closeSocket = _closeSocket;
			return false;
		}
		_success = true;

		INFO("Outbound connection established: %s", STR(*pProtocol));

		_closeSocket = false;
		_pEvent->closeSocket = _closeSocket;

		//Log Outbound Connection Event
		//		EventLogger * pEvtLog = pProtocol->GetEventLogger();
		//		if (pEvtLog != NULL) {
		//			pEvtLog->LogOutboundConnectionStart(pTCPCarrier->GetFarEndpointAddressIp(),
		//				pTCPCarrier->GetFarEndpointPort(), STR(*pProtocol));
		//			pTCPCarrier->SetEventLogger(pEvtLog);
		//		}
		return true;
	}

	static bool Connect(string ip, uint16_t port,
			vector<uint64_t>& protocolChain, Variant customParameters) {
		// Dual stack support: we default to IPv6
		// We assume that we can create an IPv6 socket, otherwise this will fail
		// TODO: handle the use case that this indeed fails
		SOCKET fd = (int32_t) socket(AF_INET6, SOCK_STREAM, 0);
		if ((((int) fd) < 0) || (!setFdCloseOnExec((int) fd))) {
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
		} else {
			//pTCPConnector->OnEvent(*_pEvent);
		}
		return true;
	}

	bool Connect() {
		SocketAddress address;
		if (!address.setIPAddress(_ip, _port)) {
			FATAL("Unable to translate string %s to a valid IP address", STR(_ip));
			return 0;
		}

		SocketAddress anyAddress;
		if (address.getFamily() == AF_INET6) {
			anyAddress.setIPAddress("::", 0);
		} else {
			anyAddress.setIPAddress("0.0.0.0", 0);
		}

		_pEvent->connectedSock = _inboundFd;
		//Bind before ConnectEx
		if (bind(_pEvent->connectedSock, anyAddress.getSockAddr(), anyAddress.getLength()) != 0) {
			int err = LASTSOCKETERROR;
			FATAL("Unable to bind on address: %s:%hu; Error was: %d",
					STR(anyAddress.toString()),
					anyAddress.getSockPort(),
					err);
			return false;
		}

		// Load ConnectEx
		GUID GuidConnectEx = WSAID_CONNECTEX;
		LPFN_CONNECTEX lpfnConnectEx = NULL;
		DWORD dwBytes = 0;

		if (WSAIoctl(_pEvent->connectedSock,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx,
				sizeof (GuidConnectEx),
				&lpfnConnectEx,
				sizeof (lpfnConnectEx),
				&dwBytes,
				NULL,
				NULL
				) != 0) {

			FATAL("Error WSAIoctl");
			return false;
		}

		// Try ConnectEx
		//dwBytes = 0;
		BOOL bRet = lpfnConnectEx(
				_pEvent->connectedSock, //__in      SOCKET s,
				address.getSockAddr(), //__in      const struct sockaddr *name,
				address.getLength(), //__in      int namelen,
				NULL, // __in_opt  PVOID lpSendBuffer,
				0, //__in      DWORD dwSendDataLength,
				(LPDWORD) &_pEvent->transferedBytes, //__out     LPDWORD lpdwBytesSent, // NOTE: No & before merge
				_pEvent //__in      LPOVERLAPPED lpOverlapped
				);
		if (bRet == FALSE) {
			int err = WSAGetLastError();
			switch (err) {
				case ERROR_IO_PENDING:
				{
					_pEvent->pendingOperation = true;
					_closeSocket = false;
					_pEvent->closeSocket = _closeSocket;
					return true;
				}

				default:
				{
					_pEvent->pendingOperation = false;
					FATAL("Unable to connect to %s:%hu (%d)", STR(_ip), _port, err);
					_closeSocket = true;
					_pEvent->closeSocket = _closeSocket;
					return false;
				}
			}
		}

		_closeSocket = false;
		_pEvent->closeSocket = _closeSocket;
		return true;
	}

	virtual void GetStats(Variant &info, uint32_t namespaceId = 0) {
		//NYIA;
	}
};


#endif	/* _TCPCONNECTOR_H */
#endif /* NET_IOCP */


