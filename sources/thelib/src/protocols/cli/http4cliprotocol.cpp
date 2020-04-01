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


#include "protocols/cli/http4cliprotocol.h"
#include "protocols/http/inboundhttpprotocol.h"

HTTP4CLIProtocol::HTTP4CLIProtocol()
: BaseProtocol(PT_HTTP_4_CLI) {

}

HTTP4CLIProtocol::~HTTP4CLIProtocol() {
}

bool HTTP4CLIProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool HTTP4CLIProtocol::EnqueueForOutbound() {
	//1. Empty our local buffer
	_localOutputBuffer.IgnoreAll();

	//2. Get the HTTP protocol
	InboundHTTPProtocol *pHTTP = (InboundHTTPProtocol *) GetFarProtocol();

	//3. Prepare the HTTP headers
	//pHTTP->SetOutboundHeader(HTTP_HEADERS_CONTENT_TYPE, "application/json");
	pHTTP->SetOutboundHeader(HTTP_HEADERS_CONTENT_TYPE, "text/plain");
	pHTTP->SetOutboundHeader(HTTP_HEADERS_CONNECTION, HTTP_HEADERS_CONNECTION_CLOSE);


	//4. Get the buffer from PT_INBOUND_JSONCLI
	IOBuffer *pBuffer = GetNearProtocol()->GetOutputBuffer();
	if (pBuffer == NULL)
		return true;

	//5. Put the data inside the local buffer and empty the buffer from
	//the PT_INBOUND_JSONCLI
	_localOutputBuffer.ReadFromBuffer(GETIBPOINTER(*pBuffer),
			GETAVAILABLEBYTESCOUNT(*pBuffer));
	pBuffer->IgnoreAll();

	//6. Trigger EnqueueForOutbound down the stack
	return pHTTP->EnqueueForOutbound();
}

IOBuffer * HTTP4CLIProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_localOutputBuffer) != 0)
		return &_localOutputBuffer;
	return NULL;
}

bool HTTP4CLIProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_INBOUND_HTTP;
}

bool HTTP4CLIProtocol::AllowNearProtocol(uint64_t type) {
	return type == PT_INBOUND_JSONCLI;
}

bool HTTP4CLIProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool HTTP4CLIProtocol::SignalInputData(IOBuffer &buffer) {
	//1. Get the HTTP protocol. We are sure is a PT_INBOUND_HTTP
	//because we return true inside AllowFarProtocol only when type == PT_INBOUND_HTTP
	InboundHTTPProtocol *pHTTP = (InboundHTTPProtocol *) GetFarProtocol();

	//2. Get the request headers
	Variant headers = pHTTP->GetHeaders();

	//3. Populate the input buffer for the next protocol in the stack (PT_INBOUND_JSONCLI)
	//with the data we just found out inside the headers
	URI uri;
	string dummy = "http://localhost" + (string) headers[HTTP_FIRST_LINE][HTTP_URL];
	//FINEST("dummy: %s", STR(dummy));
	if (!URI::FromString(dummy, false, uri)) {
		FATAL("Invalid request");
		return false;
	}
	string fullCommand = uri.document();
	fullCommand += " ";
	if (uri.parameters().MapSize() != 0) {
		fullCommand += unb64(MAP_VAL(uri.parameters().begin()));
	}
	fullCommand += "\n";
	_localInputBuffer.ReadFromString(fullCommand);

	//4. Call the next protocol with the new buffer
	return GetNearProtocol()->SignalInputData(_localInputBuffer);
}
