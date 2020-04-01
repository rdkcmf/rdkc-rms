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
#ifndef _UDPCARRIER_H
#define	_UDPCARRIER_H

#include "netio/iocp/iohandler.h"

class UDPCarrier
: public IOHandler {
private:
	string _nearIp;
	uint16_t _nearPort;
	uint64_t _rx;
	uint64_t _tx;
	Variant _parameters;
	int32_t _ioAmount;
	iocp_event_udp_read *_pReadEvent;
private:
	UDPCarrier(SOCKET fd);
	bool Setup(Variant &settings);
	bool GetEndpointsInfo();
	bool GetFarEndpointsInfo();
public:
	bool _surviveError;
	virtual void SetSurviveError(bool set = true) {_surviveError = set;};
	virtual ~UDPCarrier();
	virtual void CancelIO();
	virtual bool OnEvent(iocp_event &event);
	virtual bool SignalOutputData();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	iocp_event_udp_read *GetReadEvent();

	Variant &GetParameters();
	void SetParameters(Variant parameters);
	bool StartAccept();

	string GetFarEndpointAddress();
	uint16_t GetFarEndpointPort();
	string GetNearEndpointAddress();
	uint16_t GetNearEndpointPort();

	static UDPCarrier* Create(string bindIp, uint16_t bindPort, uint16_t ttl,
			uint16_t tos, string ssmIp);
	static UDPCarrier* Create(string bindIp, uint16_t bindPort,
			BaseProtocol *pProtocol, uint16_t ttl, uint16_t tos, string ssmIp);
};


#endif	/* _UDPCARRIER_H */
#endif /* NET_IOCP */

