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
*/

#include "protocols/http/httpmssliveingest.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"

HTTPMssLiveIngest* HTTPMssLiveIngest::_pMssLiveIngestProtocol = NULL;

HTTPMssLiveIngest::HTTPMssLiveIngest()
: BaseProtocol(PT_HTTP_MSS_LIVEINGEST) {
	_available = 0;
    _moreChunkedData = true;
}

HTTPMssLiveIngest::~HTTPMssLiveIngest() {
    _pMssLiveIngestProtocol = NULL;
}

bool HTTPMssLiveIngest::AllowFarProtocol(uint64_t type) {
	return (type == PT_OUTBOUND_HTTP2);
}

bool HTTPMssLiveIngest::AllowNearProtocol(uint64_t type) {
	return false;
}

bool HTTPMssLiveIngest::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool HTTPMssLiveIngest::SignalInputData(IOBuffer &buffer) {
    return true;
}

bool HTTPMssLiveIngest::SendFragments(uint8_t *pBuffer, uint32_t size) {
	return _outputBuffer.ReadFromBuffer(pBuffer, size)
			&& EnqueueForOutbound();
}

bool HTTPMssLiveIngest::Initialize(Variant &parameters) {
	_customParameters = parameters;
	return true;
}

HTTPInterface* HTTPMssLiveIngest::GetHTTPInterface() {
	return this;
}

IOBuffer * HTTPMssLiveIngest::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

bool HTTPMssLiveIngest::SignalRequestBegin(HttpMethod method, const char *pUri,
		const uint32_t uriLength, const char *pHttpVersion,
		const uint32_t httpVersionLength) {
	return true;
}

bool HTTPMssLiveIngest::SignalResponseBegin(const char *pHttpVersion,
		const uint32_t httpVersionLength, uint32_t code,
		const char *pDescription, const uint32_t descriptionLength) {
	switch (code) {
		case 200:
        case 400: // Initial response of Smooth Streaming server if the body is not yet sent
			return true;
			break;
		case 404:
			FATAL("Not found");
		default:
			FATAL("HTTP error occured");
			return false;
	}
}

bool HTTPMssLiveIngest::SignalHeader(const char *pName, const uint32_t nameLength,
		const char *pValue, const uint32_t valueLength) {
	return true;
}

bool HTTPMssLiveIngest::SignalHeadersEnd() {
	return true;
}

bool HTTPMssLiveIngest::SignalBodyData(HttpBodyType bodyType, uint32_t length,
		uint32_t consumed, uint32_t available, uint32_t remaining) {
	_available = available;
	return true;
}

bool HTTPMssLiveIngest::SignalBodyEnd() {
	return true;
}

bool HTTPMssLiveIngest::SignalRequestTransferComplete() {
	return true;
}

bool HTTPMssLiveIngest::SignalResponseTransferComplete() {
	return true;
}

bool HTTPMssLiveIngest::WriteFirstLine(IOBuffer &outputBuffer) {
    if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP2) {
		outputBuffer.ReadFromBuffer((const uint8_t *) "POST ", 5);
		outputBuffer.ReadFromString((string) _customParameters["publishingPoint"]);
		outputBuffer.ReadFromBuffer((const uint8_t *) " HTTP/1.1\r\n", 11);
	}

	return true;
}

bool HTTPMssLiveIngest::WriteTargetHost(IOBuffer &outputBuffer) {
	if (!_customParameters.HasKeyChain(V_STRING, false, 1, "httpHost")) {
		FATAL("No host name detected on the output HTTP protocol");
		return false;
	}
	outputBuffer.ReadFromString((string) _customParameters.GetValue("httpHost", false));
	return true;
}

bool HTTPMssLiveIngest::WriteCustomHeaders(IOBuffer &outputBuffer) {
    if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP2) {
		outputBuffer.ReadFromString("Accept: */*\r\n");
		outputBuffer.ReadFromString("Connection: close\r\n");
        outputBuffer.ReadFromString("Icy-MetaData: 1\r\n");
	}
	return true;
}

HttpBodyType HTTPMssLiveIngest::GetOutputBodyType() {
	return HTTP_TRANSFER_CHUNKED;
}

int64_t HTTPMssLiveIngest::GetContentLenth() {
	NYIA;
	return 0;
}

bool HTTPMssLiveIngest::NeedsNewOutboundRequest() {
	return _moreChunkedData;
}

bool HTTPMssLiveIngest::HasMoreChunkedData() {
	return _moreChunkedData;
}

bool HTTPMssLiveIngest::SendRequest() {
	//1. Do we have a protocol?
	if (_pFarProtocol == NULL) {
		FATAL("This protocol is not linked");
		return false;
	}
#ifdef HAS_PROTOCOL_HTTP2
	if (_pFarProtocol->GetType() != PT_OUTBOUND_HTTP2) {
		FATAL("Far protocol is not PT_OUTBOUND_HTTP2");
		return false;
	}
#endif /* HAS_PROTOCOL_HTTP2 */

	return EnqueueForOutbound();
}

string HTTPMssLiveIngest::GetHTTPDescription(uint32_t returnCode) {
	switch (returnCode) {
		case 200:
			return "OK";
		case 404:
			return "Not Found";
		case 500:
			return "Internal Server Error";
		default:
			return "";
	}
}

bool HTTPMssLiveIngest::Connect(string ip, uint16_t port,
		Variant customParameters) {

	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			customParameters[CONF_PROTOCOL]);
	if (chain.size() == 0) {
		FATAL("Unable to obtain protocol chain from settings: %s",
				STR(customParameters[CONF_PROTOCOL]));
		return false;
	}
	if (!TCPConnector<HTTPMssLiveIngest>::Connect(ip, port, chain,
			customParameters)) {
		FATAL("Unable to connect to %s:%hu", STR(ip), port);
		return false;
	}
    
    return true;
}

bool HTTPMssLiveIngest::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant customParameters) {

	if (pProtocol == NULL) {
		FATAL("Connection failed:\n%s", STR(customParameters.ToString()));
        return false;
	}

	//2. Trigger processing, including handshake
	_pMssLiveIngestProtocol = (HTTPMssLiveIngest *) pProtocol;
	_pMssLiveIngestProtocol->SetOutboundConnectParameters(customParameters);

	return _pMssLiveIngestProtocol->SendRequest();
}

HTTPMssLiveIngest* HTTPMssLiveIngest::GetInstance() {
    if (NULL == _pMssLiveIngestProtocol) {
        FATAL("HTTPMssLiveIngestProtocol does not have an instance");
        return NULL;
    }
    return _pMssLiveIngestProtocol;
}

