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
#ifndef _INBOUNDRTMPSDISCRIMINATORPROTOCOL_H
#define	_INBOUNDRTMPSDISCRIMINATORPROTOCOL_H

#include "protocols/baseprotocol.h"

class InboundRTMPSDiscriminatorProtocol
: public BaseProtocol {
public:
	InboundRTMPSDiscriminatorProtocol();
	virtual ~InboundRTMPSDiscriminatorProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
private:
#ifdef HAS_PROTOCOL_HTTP
	bool BindHTTP(IOBuffer &buffer);
#endif /* HAS_PROTOCOL_HTTP */
	bool BindSSL(IOBuffer &buffer);
};


#endif	/* _INBOUNDRTMPSDISCRIMINATORPROTOCOL_H */
#endif /* HAS_PROTOCOL_RTMP */
