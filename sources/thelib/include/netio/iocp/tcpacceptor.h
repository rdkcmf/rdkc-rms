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
#ifndef _TCPACCEPTOR_H
#define	_TCPACCEPTOR_H

#include "netio/iocp/iohandler.h"

class BaseClientApplication;
class SocketAddress;

class DLLEXP TCPAcceptor
: public IOHandler {
private:
	iocp_event_accept *_pEvent;
	SocketAddress _address;
	vector<uint64_t> _protocolChain;
	BaseClientApplication *_pApplication;
	Variant _parameters;
	bool _enabled;
	uint32_t _acceptedCount;
	uint32_t _droppedCount;
	string _ipAddress;
	uint16_t _port;
public:
	TCPAcceptor(string ipAddress, uint16_t port, Variant parameters,
			vector<uint64_t>/*&*/ protocolChain);
	virtual ~TCPAcceptor();
	virtual void CancelIO();
	iocp_event_accept *GetEvent();
	bool Bind();
	void SetApplication(BaseClientApplication *pApplication);
	bool StartAccept();
	virtual bool SignalOutputData();
	virtual bool OnEvent(iocp_event &event);
	virtual bool OnConnectionAvailable(iocp_event &event);
	bool Accept();
	bool Drop();
	Variant & GetParameters();
	BaseClientApplication *GetApplication();
	vector<uint64_t> &GetProtocolChain();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	bool Enable();
	void Enable(bool enabled);
	inline uint16_t GetDynamicPort() { return (uint16_t)_parameters[CONF_PORT]; }
	uint16_t getAddressFamily();
};


#endif	/* _TCPACCEPTOR_H */
#endif /* NET_IOCP */
