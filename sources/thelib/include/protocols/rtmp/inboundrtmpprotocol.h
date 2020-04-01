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
#ifndef _INBOUNDRTMPPROTOCOL_H
#define	_INBOUNDRTMPPROTOCOL_H

#include "protocols/rtmp/basertmpprotocol.h"

class DLLEXP InboundRTMPProtocol
: public BaseRTMPProtocol {
private:
	RC4_KEY*_pKeyIn;
	RC4_KEY*_pKeyOut;
	uint8_t *_pOutputBuffer;
	uint32_t _currentFPVersion;
	uint8_t _usedScheme;
public:
	InboundRTMPProtocol();
	virtual ~InboundRTMPProtocol();
protected:
	virtual bool PerformHandshake(IOBuffer &buffer);
private:
	bool ValidateClient(IOBuffer &inputBuffer);
	bool ValidateClientScheme(IOBuffer &inputBuffer, uint8_t scheme);
	bool PerformHandshake(IOBuffer &buffer, bool encrypted);
	bool PerformSimpleHandshake(IOBuffer &buffer);
	bool PerformComplexHandshake(IOBuffer &buffer, bool encrypted);
};

#endif	/* _INBOUNDRTMPPROTOCOL_H */
#endif /* HAS_PROTOCOL_RTMP */

