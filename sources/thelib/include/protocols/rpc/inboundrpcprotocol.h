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
#ifndef _INBOUNDRPCPROTOCOL_H
#define	_INBOUNDRPCPROTOCOL_H

#include "protocols/rpc/baserpcprotocol.h"

class InboundRPCProtocol
: public BaseRPCProtocol {
public:
	InboundRPCProtocol();
	virtual ~InboundRPCProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
};

#endif	/* _INBOUNDRPCPROTOCOL_H */
#endif /* HAS_PROTOCOL_RPC */
