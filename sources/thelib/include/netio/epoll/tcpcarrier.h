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


#ifdef NET_EPOLL
#ifndef _TCPCARRIER_H
#define	_TCPCARRIER_H

#include "netio/epoll/iohandler.h"
#include "utils/misc/socketaddress.h"

class TCPCarrier
: public IOHandler {
private:
	bool _writeDataEnabled;
	bool _enableWriteDataCalled;
	SocketAddress _farAddress;
	string _farIp;
	uint16_t _farPort;
	SocketAddress _nearAddress;
	string _nearIp;
	uint16_t _nearPort;
	int32_t _sendBufferSize;
	int32_t _recvBufferSize;
	uint64_t _rx;
	uint64_t _tx;
	int32_t _ioAmount;
	int _lastRecvError;
	int _lastSendError;
public:
	TCPCarrier(int32_t fd);
	virtual ~TCPCarrier();
	virtual bool OnEvent(struct epoll_event &event);
	virtual bool SignalOutputData();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);

	SocketAddress &GetFarEndpointAddress();
	string GetFarEndpointAddressIp();
	uint16_t GetFarEndpointPort();
	SocketAddress &GetNearEndpointAddress();
	string GetNearEndpointAddressIp();
	uint16_t GetNearEndpointPort();
private:
	bool GetEndpointsInfo();
};


#endif	/* _TCPCARRIER_H */
#endif /* NET_EPOLL */
