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


#ifdef HAS_PROTOCOL_RPC
#ifndef _BASERPCAPPPROTOCOLHANDLER_H
#define	_BASERPCAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

class BaseRPCAppProtocolHandler
: public BaseAppProtocolHandler {
private:
	vector<uint64_t> _outboundHttpRpc;
	vector<uint64_t> _outboundHttpsRpc;
	Variant &GetScaffold(string &uriString);
	vector<uint64_t> &GetProtocolChain(bool isSSL = false);
	Variant _urlCache;
public:
	BaseRPCAppProtocolHandler(Variant &configuration);
	virtual ~BaseRPCAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	bool Send(string url, Variant &variant,
			string serializer);
	bool Send(string ip, uint16_t port, Variant &variant,
			string serializer);

	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters);
	virtual void ConnectionFailed(Variant &parameters);
};


#endif	/* _BASERPCAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_RPC */
