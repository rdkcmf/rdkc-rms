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

#ifdef HAS_PROTOCOL_WEBRTC
#ifndef __ICETCPPROTOCOL_H
#define __ICETCPPROTOCOL_H

#include "protocols/wrtc/baseiceprotocol.h"
#include "protocols/passthrough/passthroughprotocol.h"

class WrtcConnection;
class WrtcSigProtocol;
class Candidate;
class StunMsg;

class DLLEXP IceTcpProtocol
	: public BaseIceProtocol {
public:
	IceTcpProtocol();
	virtual ~IceTcpProtocol();

	virtual void Start(time_t ms);
	virtual bool SignalInputData(IOBuffer &buffer);
	
	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters);
	bool SignalPassThroughProtocolCreated(PassThroughProtocol *pProtocol,
			Variant &parameters);
private:
	PassThroughProtocol *_senderProtocol;

	virtual bool SendToV4(uint32_t ip, uint16_t port, uint8_t * pData, int len);
	virtual bool SendTo(string ip, uint16_t port, uint8_t * pData, int len);
 };

#endif // __ICETCPPROTOCOL_H
#endif //HAS_PROTOCOL_WEBRTC
 
 
