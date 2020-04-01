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
#include "protocols/rpc/outboundrpcprotocol.h"
#include "protocols/rpc/rpcserializer.h"
#include "protocols/rtmp/header_le_ba.h"

OutboundRPCProtocol::OutboundRPCProtocol()
: BaseRPCProtocol(PT_OUTBOUND_RPC) {
}

OutboundRPCProtocol::~OutboundRPCProtocol() {
}

bool OutboundRPCProtocol::Initialize(Variant& variant) {
	if (_pOutputSerializer != NULL)
		delete _pOutputSerializer;
	string sType = (string) variant.GetValue("serializerType", false);
	_pOutputSerializer = BaseRPCSerializer::GetSerializer(sType);
	_customParameters = variant;
	if (_pOutputSerializer == NULL) {
		FATAL("Incorrect serializer type: %s", STR(sType));
		return false;
	}
	return true;
}

bool OutboundRPCProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP)
			|| (type == PT_OUTBOUND_SSL)
			|| (type == PT_OUTBOUND_HTTP2);
}

bool OutboundRPCProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

HTTPInterface *OutboundRPCProtocol::GetHTTPInterface() {
	return this;
}

bool OutboundRPCProtocol::SignalRequestBegin(HttpMethod method, const char *pUri,
		const uint32_t uriLength, const char *pHttpVersion,
		const uint32_t httpVersionLength) {
	NYIR;
}

bool OutboundRPCProtocol::SignalResponseBegin(const char *pHttpVersion,
		const uint32_t httpVersionLength, uint32_t code,
		const char *pDescription, const uint32_t descriptionLength) {
	//FINEST("SignalResponseBegin: `%s` %"PRIu32" `%s`", pHttpVersion, code, pDescription);
	return true;
}

bool OutboundRPCProtocol::SignalHeader(const char *pName, const uint32_t nameLength,
		const char *pValue, const uint32_t valueLength) {
	//FINEST("Header: `%s`: `%s`", pName, pValue);
	return true;
}

bool OutboundRPCProtocol::SignalHeadersEnd() {
	//FINEST("-------------------");
	return true;
}

bool OutboundRPCProtocol::SignalBodyData(HttpBodyType bodyType, uint32_t length,
		uint32_t consumed, uint32_t available, uint32_t remaining) {
	//	string str = format("%s length: %"PRIu32"; consumed: %"PRIu32"; available: %"PRIu32"; remaining %"PRIu32,
	//			HttpBodyTypeToString(bodyType),
	//			length,
	//			consumed,
	//			available,
	//			remaining);
	//	FINEST("%s", STR(str));
	_available = available;
	return true;
}

bool OutboundRPCProtocol::SignalBodyEnd() {
	NYIR;
}

bool OutboundRPCProtocol::SignalRequestTransferComplete() {
	return true;
}

bool OutboundRPCProtocol::SignalResponseTransferComplete() {
	return true;
}

HttpBodyType OutboundRPCProtocol::GetOutputBodyType() {
	return HTTP_TRANSFER_CONTENT_LENGTH;
}

int64_t OutboundRPCProtocol::GetContentLenth() {
	return -1;
}

bool OutboundRPCProtocol::NeedsNewOutboundRequest() {
	return false;
}

bool OutboundRPCProtocol::HasMoreChunkedData() {
	return false;
}

bool OutboundRPCProtocol::WriteFirstLine(IOBuffer &outputBuffer) {
	outputBuffer.ReadFromBuffer((const uint8_t *) "POST ", 5);
	outputBuffer.ReadFromString((string) _customParameters["document"]);
	outputBuffer.ReadFromBuffer((const uint8_t *) " HTTP/1.1\r\n", 11);
	return true;
}

bool OutboundRPCProtocol::WriteTargetHost(IOBuffer &outputBuffer) {
	Variant &params = GetCustomParameters();
	if (!params.HasKeyChain(V_STRING, false, 1, "host")) {
		FATAL("No host name detected on the output HTTP RPC protocol");
		return false;
	}
	outputBuffer.ReadFromString((string) params.GetValue("host", false));
	return true;
}

bool OutboundRPCProtocol::WriteCustomHeaders(IOBuffer &outputBuffer) {
	if (_pOutputSerializer == NULL) {
		FATAL("No RPC serializer found");
		return false;
	}

	outputBuffer.ReadFromBuffer((const uint8_t *) "Content-Type: ", 14);
	outputBuffer.ReadFromString(_pOutputSerializer->GetHTTPContentType());
	outputBuffer.ReadFromBuffer((const uint8_t *) "\r\n", 2);

	return true;
}

bool OutboundRPCProtocol::Send(Variant &variant) {
	//1. Do we have a protocol?
	if (_pFarProtocol == NULL) {
		FATAL("This protocol is not linked");
		return false;
	}

	if (0) {

	}
#ifdef HAS_PROTOCOL_HTTP2
	else if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP2) {
		if (!SerializeOutput(variant["payload"])) {
			FATAL("Unable to serialize parameters");
			return false;
		}
	}
#endif /* HAS_PROTOCOL_HTTP */
	else {
		if (!SerializeOutput(variant)) {
			FATAL("Unable to serialize parameters");
			return false;
		}
	}
	return EnqueueForOutbound();
}
#endif /* HAS_PROTOCOL_RPC */
