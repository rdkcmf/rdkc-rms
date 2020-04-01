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
#ifndef _UDS_ACCEPTOR_H
#define	_UDS_ACCEPTOR_H


#include "netio/kqueue/iohandler.h"

class BaseClientApplication;

class UDSAcceptor
: public IOHandler {
private:
	sockaddr_un _address;
	vector<uint64_t> _protocolChain;
	BaseClientApplication *_pApplication;
	Variant _parameters;
	bool _enabled;
	uint8_t _abstractPad;
	uint32_t _acceptedCount;
	uint32_t _droppedCount;
	string _pathname;
	socklen_t _socketLength;
public:
	UDSAcceptor(string pathname, Variant parameters,
			vector<uint64_t>/*&*/ protocolChain);
	virtual ~UDSAcceptor();
	bool Bind();
	void SetApplication(BaseClientApplication *pApplication);
	bool StartAccept();
	virtual bool SignalOutputData();
	virtual bool OnEvent(struct kevent &event);
	virtual bool OnConnectionAvailable(struct kevent &event);
	bool Accept();
	bool Drop();
	Variant & GetParameters();
	BaseClientApplication *GetApplication();
	vector<uint64_t> &GetProtocolChain();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	bool Enable();
	void Enable(bool enabled);

	string ToString();
private:
	bool MakeAddr(string name, struct sockaddr_un &pAddr, socklen_t &pSockLen);
	bool IsAlive();
};


#endif	/* _UDS_ACCEPTOR_H */
#endif /* NET_KQUEUE */
