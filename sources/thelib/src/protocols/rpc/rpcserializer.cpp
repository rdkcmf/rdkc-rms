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
#include "protocols/rpc/rpcserializer.h"

BaseRPCSerializer::BaseRPCSerializer() {
	_httpContentType = "application/octet-stream";
}

BaseRPCSerializer::~BaseRPCSerializer() {
}

string &BaseRPCSerializer::GetHTTPContentType() {
	return _httpContentType;
}

BaseRPCSerializer *BaseRPCSerializer::GetSerializer(string &type) {
	if (type == RPC_PROTOCOL_SERIALIZER_BINVAR) {
		return new VariantRPCSerializer(VARIANT_RPC_SERIALIZER_BIN);
	}
	if (type == RPC_PROTOCOL_SERIALIZER_XML) {
		return new VariantRPCSerializer(VARIANT_RPC_SERIALIZER_XMLRPC);
	}
	if (type == RPC_PROTOCOL_SERIALIZER_XMLVAR) {
		return new VariantRPCSerializer(VARIANT_RPC_SERIALIZER_XMLVAR);
	}
	if (type == RPC_PROTOCOL_SERIALIZER_JSONVAR) {
		return new VariantRPCSerializer(VARIANT_RPC_SERIALIZER_JSON);
	}
	if (type == RPC_PROTOCOL_SERIALIZER_KVP) {
		return new KvpRPCSerializer();
	}
	FATAL("Invalid RPC serializer type: %s", STR(type));
	return NULL;
}

VariantRPCSerializer::VariantRPCSerializer(uint8_t type)
: BaseRPCSerializer() {
	_type = type;
	switch (_type) {
		case VARIANT_RPC_SERIALIZER_BIN:
		{
			_httpContentType = "application/octet-stream";
			break;
		}
		case VARIANT_RPC_SERIALIZER_XMLRPC:
		case VARIANT_RPC_SERIALIZER_XMLVAR:
		{
			_httpContentType = "application/xml";
			break;
		}
		case VARIANT_RPC_SERIALIZER_JSON:
		{
			_httpContentType = "application/json";
			break;
		}
		default:
		{
			ASSERT("Invalid variant RPC serializer type");
			break;
		}
	}
}

VariantRPCSerializer::~VariantRPCSerializer() {
}

bool VariantRPCSerializer::Serialize(Variant &source, IOBuffer &dest) {
	//	NYIR;
	string serial = "";

	//1. Serialize to string
	switch (_type) {
		case VARIANT_RPC_SERIALIZER_BIN:
		{
			if (!source.SerializeToBin(serial)) {
				WARN("Cannot serialize to XML RPC");
				return false;
			}
			break;
		}
		case VARIANT_RPC_SERIALIZER_XMLRPC:
		{
			if (!source.SerializeToXmlRpcRequest(serial, true)) {
				WARN("Cannot serialize to XML RPC");
				return false;
			}
			break;
		}
		case VARIANT_RPC_SERIALIZER_XMLVAR:
		{
			if (!source.SerializeToXml(serial)) {
				WARN("Cannot serialize to XML VAR");
				return false;
			}
			break;
		}
		case VARIANT_RPC_SERIALIZER_JSON:
		{
			if (!source.SerializeToJSON(serial, true)) {
				WARN("Cannot serialize to JSON");
				return false;
			}
			break;
		}
		default:
		{
			FATAL("Invalid RPC serializer type: %"PRId32, (int32_t) _type);
			return false;
		}
	}

	//2. Write string to dest
	if (!dest.ReadFromString(serial)) {
		WARN("Unable to read serialized string");
		return false;
	}

	return true;
}

bool VariantRPCSerializer::Deserialize(IOBuffer &source, Variant &dest) {
	NYIR;
}

KvpRPCSerializer::KvpRPCSerializer()
: BaseRPCSerializer() {
	_httpContentType = "application/text";
}

KvpRPCSerializer::~KvpRPCSerializer() {
}

bool KvpRPCSerializer::Serialize(Variant &source, IOBuffer &dest) {
	NYIR;
}

bool KvpRPCSerializer::Deserialize(IOBuffer &source, Variant &dest) {
	NYIR;
}

#endif /* HAS_PROTOCOL_RPC */
