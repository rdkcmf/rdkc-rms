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
#include "protocols/ws/outboundwsprotocol.h"

OutboundWSProtocol::OutboundWSProtocol()
: BaseWSProtocol(PT_OUTBOUND_WS) {
	_wsHandshakeRequestSent = false;
}

OutboundWSProtocol::~OutboundWSProtocol() {
}

bool OutboundWSProtocol::EnqueueForOutbound() {
	if (!_wsHandshakeRequestSent) {
		_nonce = b64(md5(format("%s_%"PRIu32, STR(Version::GetBanner()), GetId()), false));
		_outputBuffer.ReadFromString(format(""
				"GET %s HTTP/1.1\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Host: %s:%"PRIu16"\r\n"
				"Sec-WebSocket-Version: 13\r\n"
				"Sec-WebSocket-Key: %s\r\n"
				HTTP_HEADERS_USER_AGENT": "HTTP_HEADERS_SERVER_US"\r\n"
				"\r\n",
				STR(GetCustomParameters()["document"]),
				STR(GetCustomParameters()["host"]),
				(uint16_t) GetCustomParameters()["port"],
				STR(_nonce)
				));
		_wsHandshakeRequestSent = true;
		return BaseWSProtocol::EnqueueForOutbound();
	}
	IOBuffer *pBuffer = _pNearProtocol == NULL ? NULL : _pNearProtocol->GetOutputBuffer();
	if (pBuffer == NULL)
		return BaseWSProtocol::EnqueueForOutbound();
	else
		return BaseWSProtocol::SendOutOfBandData(*pBuffer, NULL)
		&& pBuffer->IgnoreAll();
}

#define DIRTY_WS_HANDSHAKE

bool OutboundWSProtocol::ProcessWSHandshake(IOBuffer &buffer) {
#ifdef DIRTY_WS_HANDSHAKE
	//get the buffer and the length
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	size_t length = GETAVAILABLEBYTESCOUNT(buffer);

	//a minimum of characters to accomodate this: "Sec-WebSocket-Accept: a\r\n\r\n"
	if (length < 27)
		return true;

	//don't accept handshakes bigger than 1k
	if (length >= 1024)
		return false;

	//see if we have a headers section end in sight
	uint8_t *pHeadersEnd = NULL;
	if ((pHeadersEnd = (uint8_t *) memmem(pBuffer, length, "\r\n\r\n", 4)) == NULL) //handshake not completed yet
		return true;

	//see if the connection was accepted or not
	if (memmem(pBuffer, length, "Sec-WebSocket-Accept: ", 22) == NULL) {//not accepted
		FATAL("WebSocket client handshake failed");
		EnqueueForDelete();
		return false;
	}

	//ignore the headers section
	if (!buffer.Ignore((uint32_t) (pHeadersEnd - pBuffer + 4))) {
		FATAL("Unable to ignore WebSockets headers");
		return false;
	}
	buffer.MoveData();
#else /* DIRTY_WS_HANDSHAKE */
#error HTTPResponse must be implemented
#endif /* DIRTY_WS_HANDSHAKE */

	//set the handshake complete flag
	_wsHandshakeComplete = true;

	//done, but call EnqueueForOutbound again so all the upper out data is now piped
	return OutboundWSProtocol::EnqueueForOutbound();
}
#endif /* HAS_PROTOCOL_WS */
