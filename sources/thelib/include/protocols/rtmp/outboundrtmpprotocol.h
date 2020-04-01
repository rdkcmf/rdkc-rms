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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _OUTBOUNDRTMPPROTOCOL_H
#define	_OUTBOUNDRTMPPROTOCOL_H

#include "protocols/rtmp/basertmpprotocol.h"

class DHWrapper;

class DLLEXP OutboundRTMPProtocol
: public BaseRTMPProtocol {
private:
	uint8_t *_pClientPublicKey;
	uint8_t *_pOutputBuffer;
	uint8_t *_pClientDigest;
	RC4_KEY* _pKeyIn;
	RC4_KEY* _pKeyOut;
	DHWrapper *_pDHWrapper;
	uint8_t _usedScheme;
	bool _encrypted;
public:
	OutboundRTMPProtocol();
	virtual ~OutboundRTMPProtocol();
protected:
	virtual bool PerformHandshake(IOBuffer &buffer);
public:
	static bool Connect(string ip, uint16_t port, Variant customParameters);
	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters);
private:
	bool PerformHandshakeStage1(bool encrypted);
	bool VerifyServer(IOBuffer &inputBuffer);
	bool PerformHandshakeStage2(IOBuffer &inputBuffer, bool encrypted);
};

#endif	/* _OUTBOUNDRTMPPROTOCOL_H */

#endif /* HAS_PROTOCOL_RTMP */

