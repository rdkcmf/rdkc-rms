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


#include "application/baseclientapplication.h"
#include "protocols/rpc/baserpcappprotocolhandler.h"
#include "protocols/rpc/rpcserializer.h"
#include "eventlogger/sinktypes.h"
#include "eventlogger/rpcsink.h"

RpcSink::RpcSink()
: BaseEventSink(SINK_RPC) {
	_pHandler = NULL;
}

RpcSink::~RpcSink() {

}

bool RpcSink::Initialize(Variant & config, Variant &customData) {
	if (!BaseEventSink::Initialize(config, customData)) {
		FATAL("Unable to initialize event sink");
		return false;
	}
	//1. Validations
	if (!config.HasKeyChain(V_STRING, false, 1, "url")) {
		WARN("RPC URI not specified");
		return false;
	}

	if (!config.HasKeyChain(V_STRING, false, 1, "serializerType")) {
		WARN("RPC Serializer type not specified");
		return false;
	}

	string typeFromFile = (string) config.GetValue("serializerType", false);

	if (typeFromFile == "JSON")
		_serializerType = RPC_PROTOCOL_SERIALIZER_JSONVAR;
	else if (typeFromFile == "XMLRPC")
		_serializerType = RPC_PROTOCOL_SERIALIZER_XML;
	else if (typeFromFile == "XML")
		_serializerType = RPC_PROTOCOL_SERIALIZER_XMLVAR;

	_uri = (string) config.GetValue("url", false);

	return true;
}

void RpcSink::Log(Variant & event) {
	if (_pHandler == NULL)
		return;

	if (CanLogEvent(event["type"])) {

		if (event.HasKeyChain(V_STRING, false, 2, "payload", "protocolType")) {
			if (event["payload"]["protocolType"] == tagToString(PT_OUTBOUND_RPC))
				return;
		}

		//Excluded json serializer case in forcing variant type to V_TYPED_MAP
		//which is not supported by JSON
		if (event != V_TYPED_MAP && _serializerType != RPC_PROTOCOL_SERIALIZER_JSONVAR) {
			if ((!event.HasKeyChain(V_STRING, true, 1, "type"))
					|| (!event.HasKey("payload", true))) {
				WARN("Invalid event structure");
				return;
			}
			event.SetTypeName((string) event["type"]);
		}

		_pHandler->Send(_uri, event, _serializerType);
	}
}

void RpcSink::SetRpcProtocolHandler(BaseRPCAppProtocolHandler *pHandler) {
	_pHandler = pHandler;
}

uint32_t RpcSink::GetSerializerType(string type) {
	if (type == "XMLRPC") {
		return VARIANT_RPC_SERIALIZER_XMLRPC;
	}
	if (type == "XML") {
		return VARIANT_RPC_SERIALIZER_XMLVAR;
	}
	if (type == "JSON") {
		return VARIANT_RPC_SERIALIZER_JSON;
	}
	FATAL("Invalid RPC serializer type: %s", STR(type));
	return 0;
}

