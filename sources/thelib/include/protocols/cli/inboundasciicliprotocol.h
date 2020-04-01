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



#ifdef HAS_PROTOCOL_CLI
#ifndef _INBOUNDASCIICLIPROTOCOL_H
#define _INBOUNDASCIICLIPROTOCOL_H

#include "protocols/cli/inboundbasecliprotocol.h"

class DLLEXP InboundASCIICLIProtocol
: public InboundBaseCLIProtocol {
public:
	InboundASCIICLIProtocol();
	virtual ~InboundASCIICLIProtocol();

	virtual bool Initialize(Variant &parameters);
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif /* __clang__ */
	virtual bool SignalInputData(IOBuffer &buffer);
#ifdef __clang__
#pragma clang diagnostic pop
#endif /* __clang__ */
	virtual bool SendMessage(Variant &message);
private:
	bool ParseCommand(string &command);
};


#endif	/* _INBOUNDASCIICLIPROTOCOL_H */
#endif /* HAS_PROTOCOL_CLI */
