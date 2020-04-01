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
#include "protocols/rpc/inboundrpcprotocol.h"
#include "protocols/rpc/rpcserializer.h"

InboundRPCProtocol::InboundRPCProtocol()
: BaseRPCProtocol(PT_INBOUND_RPC) {
}

InboundRPCProtocol::~InboundRPCProtocol() {
}

bool InboundRPCProtocol::Initialize(Variant &parameters) {
	if ((!parameters.HasKeyChain(V_STRING, false, 1, RPC_PROTOCOL_INPUT_SERIALIZER))
			|| (!parameters.HasKeyChain(V_STRING, false, 1, RPC_PROTOCOL_OUTPUT_SERIALIZER))) {
		FATAL("Invalid "RPC_PROTOCOL_INPUT_SERIALIZER" and/or "RPC_PROTOCOL_OUTPUT_SERIALIZER" settings on the RPC acceptor");
		return false;
	}
	string inputSerializer = (string) parameters.GetValue(
			RPC_PROTOCOL_INPUT_SERIALIZER, false);
	string outputSerializer = (string) parameters.GetValue(
			RPC_PROTOCOL_OUTPUT_SERIALIZER, false);

	_pInputSerializer = BaseRPCSerializer::GetSerializer(inputSerializer);
	if (_pInputSerializer == NULL) {
		FATAL("Invalid "RPC_PROTOCOL_INPUT_SERIALIZER" setting on the RPC acceptor");
		return false;
	}

	_pOutputSerializer = BaseRPCSerializer::GetSerializer(outputSerializer);
	if (_pOutputSerializer == NULL) {
		FATAL("Invalid "RPC_PROTOCOL_OUTPUT_SERIALIZER" setting on the RPC acceptor");
		return false;
	}

	GetCustomParameters() = parameters;
	return true;
}

bool InboundRPCProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP)
			|| (type == PT_INBOUND_SSL)
			|| (type == PT_INBOUND_HTTP2);
}

#endif /* HAS_PROTOCOL_RPC */
