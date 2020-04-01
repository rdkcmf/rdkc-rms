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
#ifdef HAS_PROTOCOL_WS
#include "protocols/ws/inboundwsprotocol.h"
#include "protocols/ws/wsinterface.h"


static const uint8_t gEndWSHeaders[] = "\r\n";
static const size_t gEndWSHeadersLength = sizeof (gEndWSHeaders) - 1;

static uint8_t gBase404Reply[] = ""
		" 404 Not found\r\n"
		WS_HEADERS_COMMON
		HTTP_HEADERS_CONTENT_LENGTH": 0\r\n"
		"\r\n"
		;
static const size_t gBase404ReplyLength = sizeof (gBase404Reply) - 1;

static uint8_t gBase101Reply[] = ""
		" 101 Switching Protocols\r\n"
		WS_HEADERS_COMMON
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: "
		;
static const size_t gBase101ReplyLength = sizeof (gBase101Reply) - 1;

InboundWSProtocol::InboundWSProtocol() :
BaseWSProtocol(PT_INBOUND_WS) {
}

InboundWSProtocol::~InboundWSProtocol() {
}

bool InboundWSProtocol::ProcessWSHandshake(IOBuffer &buffer) {
	WSInterface *pInterface = NULL;
	if ((_pNearProtocol == NULL) || ((pInterface = _pNearProtocol->GetWSInterface()) == NULL)) {
		FATAL("Protocol not compatible with WebSocket transport");
		return false;
	}

	//the current HTTP request needs to be in good shape
	if (_httpRequest.GetState() == HS_ERROR) {
		FATAL("Invalid HTTP state");
		return false;
	}

	//we do not accumulate too many pending requests
	if (GETAVAILABLEBYTESCOUNT(buffer) > 1024 * 1024) {
		FATAL("The HTTP requests backlog is too big");
		return false;
	}

	//reset the currently parsed HTTP request if it is completed, and clear
	//the corresponding amount of data from the buffer
	if (_httpRequest.IsComplete()) {
		buffer.Ignore((uint32_t) _httpRequest.BytesCount());
		buffer.MoveData();
		_httpRequest.Clear();
	}

	//parse a request
	if (!_httpRequest.Parse(buffer)) {
		FATAL("Invalid HTTP request");
		return false;
	}
	//FINEST("\n%s", STR(_httpRequest.ToString(buffer)));

	//if the request is not complete, than wait for more data
	if (!_httpRequest.IsComplete()) {
		FINEST("Waiting for more data");
		return true;
	}

	//get the Variant-ed request, easier to work with
	Variant &request = _httpRequest.GetVariant(buffer);
	//FINEST("\n%s", STR(request.ToString()));

	if (!pInterface->SignalInboundConnection(request))
		return Send404(request[HTTP_FIRST_LINE][HTTP_VERSION]);
;
	_outIsBinary = pInterface->IsOutBinary();

	//send the 101 reply
	if ((!request[HTTP_HEADERS].HasKeyChain(V_STRING, false, 1, "Sec-WebSocket-Key"))
			|| (!Send101(request[HTTP_FIRST_LINE][HTTP_VERSION], request[HTTP_HEADERS].GetValue("Sec-WebSocket-Key", false)))) {
		FATAL("Unable to send the 101 reply");
		return false;
	}

	//ignore the handshake bytes from the input stream
	if (!buffer.Ignore((uint32_t) _httpRequest.BytesCount())) {
		FATAL("Unable to ignore the WS handshake");
		return false;
	}

	//set the handshake complete flag
	_wsHandshakeComplete = true;

	//done
	return true;
}

bool InboundWSProtocol::Send404(const string &httpVersion) {
	if ((!_outputBuffer.ReadFromString(httpVersion))
			|| (!_outputBuffer.ReadFromBuffer(gBase404Reply, gBase404ReplyLength))
			|| (!EnqueueForOutbound())
			) {
		FATAL("Unable to build the 404 response");
		return false;
	}
	GracefullyEnqueueForDelete();
	return true;
}

bool InboundWSProtocol::Send101(const string &httpVersion, const string &clientChallange) {
	string clientChallenge = clientChallange + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1((const uint8_t *) clientChallenge.c_str(), clientChallenge.size(), _sha1Digest);
	string serverChallangeResponse = b64(_sha1Digest, SHA_DIGEST_LENGTH);
	return _outputBuffer.ReadFromString(httpVersion)
			&& _outputBuffer.ReadFromBuffer(gBase101Reply, gBase101ReplyLength)
			&& _outputBuffer.ReadFromString(serverChallangeResponse + "\r\n")
			&& _outputBuffer.ReadFromBuffer(gEndWSHeaders, gEndWSHeadersLength)
			&& EnqueueForOutbound()
			;
}

#endif /* HAS_PROTOCOL_WS */
