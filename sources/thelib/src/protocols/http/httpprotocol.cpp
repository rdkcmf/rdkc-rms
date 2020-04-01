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



#ifdef HAS_PROTOCOL_HTTP2
#include "protocols/http/httpprotocol.h"
#include "protocols/tcpprotocol.h"

#define HTTP_MAX_URI_LENGTH 1024
#define HTTP_MAX_STATUS_CODE_DESCRIPTION_LENGTH 256
#define HTTP_MAX_HEADERS_SIZE 2048
#define MAX_HTTP_BUFFER_LENGTH (1024*1024*16)

//#define _DEBUG_HTTP
#ifdef _DEBUG_HTTP
#define DEBUG_HTTP(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	FINEST("%s: %s",GetStateString(_state),STR(__msg__)); \
} while(0)
#define MOVE_STATE(newState) \
do { \
	WARN("%15s %50s -> %s",__func__,GetStateString(_state),GetStateString((newState))); \
	_state=(newState); \
}while(0)
#else /* _DEBUG_HTTP */
#define DEBUG_HTTP(...)
#define MOVE_STATE(newState) _state=(newState)
#endif /* _DEBUG_HTTP */

HTTPProtocol::HTTPProtocol(uint64_t protocolType)
: BaseProtocol(protocolType) {
	_disconnectAfterTransfer = false;
	_decodedBytesCount = 0;
	_contentLength = 0;
	_consumedLength = 0;
	_transferType = HTTP_TRANSFER_UNKNOWN;
	_method = HTTP_METHOD_UNKNOWN;
	_keepAlive = false;
	_pInterface = NULL;
	_state = STATE_UNKNOWN;
	if (_type == PT_INBOUND_HTTP2) {
		MOVE_STATE(STATE_INPUT_START);
	} else if (_type == PT_OUTBOUND_HTTP2) {
		MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD);
	} else {
		ASSERT("Invalid protocol type detected");
	}
	_outputBodyType = HTTP_TRANSFER_UNKNOWN;
}

HTTPProtocol::~HTTPProtocol() {

}

IOBuffer * HTTPProtocol::GetOutputBuffer() {
	if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
		FATAL("No near protocol or is not HTTP compatible");
		EnqueueForDelete();
		return NULL;
	}

	while (true) {
		switch (_state) {
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_BODY_CONNECTION_CLOSE">
			case STATE_INPUT_BODY_CONNECTION_CLOSE:
			{
				IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
				if (pBuffer != NULL) {
					WARN("Discarding all outbound data because the input data is never ending (HTTP header \"Connection: close\" is present)");
					pBuffer->IgnoreAll();
				}
				return NULL;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_*">
			case STATE_INPUT_START:
			case STATE_INPUT_REQUEST:
			case STATE_INPUT_RESPONSE:
			case STATE_INPUT_HEADER_BEGIN:
			case STATE_INPUT_HEADER:
			case STATE_INPUT_END_CRLF:
			case STATE_INPUT_BODY_CONTENT_LENGTH:
			case STATE_INPUT_BODY_CHUNKED_LENGTH:
			case STATE_INPUT_BODY_CHUNKED_PAYLOAD:
			case STATE_INPUT_BODY_CHUNKED_PAYLOAD_END:
			case STATE_INPUT_END:
			{
				DEBUG_HTTP("Sending data postponed");
				return NULL;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_BUILD">
			case STATE_OUTPUT_HEADERS_BUILD:
			{
				switch (_type) {
						// <editor-fold defaultstate="collapsed" desc="PT_INBOUND_HTTP2">
					case PT_INBOUND_HTTP2:
					{
						if (!_pInterface->WriteFirstLine(_outputHeaders)) {
							FATAL("Unable to write the first HTTP line");
							EnqueueForDelete();
							return NULL;
						}

						_outputHeaders.ReadFromBuffer((const uint8_t *) HTTP_HEADERS_SERVER": "HTTP_HEADERS_SERVER_US"\r\n",
								HTTP_HEADERS_SERVER_LEN + 2 + HTTP_HEADERS_SERVER_US_LEN + 2);

						if (!_pInterface->WriteCustomHeaders(_outputHeaders)) {
							FATAL("Unable to write the custom HTTP headers");
							EnqueueForDelete();
							return NULL;
						}

						_outputBodyType = _pInterface->GetOutputBodyType();
						switch (_outputBodyType) {
							case HTTP_TRANSFER_UNKNOWN:
							{
								FATAL("Invalid body type detected");
								EnqueueForDelete();
								return NULL;
							}
							case HTTP_TRANSFER_CONTENT_LENGTH:
							{
								int64_t bufferLength = _pInterface->GetContentLenth();
								IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
								if ((bufferLength < 0) && (pBuffer != NULL)) {
									bufferLength = GETAVAILABLEBYTESCOUNT(*pBuffer);
								}
								if ((bufferLength > 0) && (pBuffer != NULL)) {
									SETIBSENDLIMIT(*pBuffer, (uint32_t) bufferLength);
								}
								if (bufferLength < 0)
									bufferLength = 0;
								_outputHeaders.ReadFromString(format("Content-Length: %"PRId64"\r\n\r\n", bufferLength));
								break;
							}
							case HTTP_TRANSFER_CHUNKED:
							{
								_outputHeaders.ReadFromBuffer((const uint8_t *) "Transfer-Encoding: chunked\r\n", 28);
								break;
							}
							case HTTP_TRANSFER_CONNECTION_CLOSE:
							{
								IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
								if (pBuffer != NULL) {
									SETIBSENDLIMIT(*pBuffer, (uint32_t) 0xffffffff);
								}
								_outputHeaders.ReadFromBuffer((const uint8_t *) "Connection: close\r\n", 19);
								break;
							}
							default:
							{
								FATAL("Invalid body type returned by HTTPInterface::GetOutputBodyType: %d", _outputBodyType);
								EnqueueForDelete();
								return NULL;
							}
						}
						break;
					}
						// </editor-fold>
						// <editor-fold defaultstate="collapsed" desc="PT_OUTBOUND_HTTP2">
					case PT_OUTBOUND_HTTP2:
					{
						if (!_pInterface->WriteFirstLine(_outputHeaders)) {
							FATAL("Unable to write the first HTTP line");
							EnqueueForDelete();
							return NULL;
						}

						_outputHeaders.ReadFromBuffer((const uint8_t *) "Host: ", 6);
						if (!_pInterface->WriteTargetHost(_outputHeaders)) {
							FATAL("Unable to write the first HTTP line");
							EnqueueForDelete();
							return NULL;
						}
						_outputHeaders.ReadFromBuffer((const uint8_t *) "\r\n", 2);

						_outputHeaders.ReadFromBuffer((const uint8_t *) HTTP_HEADERS_USER_AGENT": "HTTP_HEADERS_SERVER_US"\r\n",
								HTTP_HEADERS_USER_AGENT_LEN + 2 + HTTP_HEADERS_SERVER_US_LEN + 2);

						if (!_pInterface->WriteCustomHeaders(_outputHeaders)) {
							FATAL("Unable to write the custom HTTP headers");
							EnqueueForDelete();
							return NULL;
						}

						_outputBodyType = _pInterface->GetOutputBodyType();
						switch (_outputBodyType) {
							case HTTP_TRANSFER_UNKNOWN:
							{
								FATAL("Invalid body type detected");
								EnqueueForDelete();
								return NULL;
							}
							case HTTP_TRANSFER_CONTENT_LENGTH:
							{
								int64_t bufferLength = _pInterface->GetContentLenth();
								IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
								if ((bufferLength < 0) && (pBuffer != NULL)) {
									bufferLength = GETAVAILABLEBYTESCOUNT(*pBuffer);
								}
								if ((bufferLength > 0) && (pBuffer != NULL)) {
									SETIBSENDLIMIT(*pBuffer, (uint32_t) bufferLength);
								}
								if (bufferLength < 0)
									bufferLength = 0;
								_outputHeaders.ReadFromString(format("Content-Length: %"PRId64"\r\n\r\n", bufferLength));
								break;
							}
							case HTTP_TRANSFER_CHUNKED:
							{
								_outputHeaders.ReadFromBuffer((const uint8_t *) "Transfer-Encoding: chunked\r\n", 28);
								break;
							}
							case HTTP_TRANSFER_CONNECTION_CLOSE:
							{
								IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
								if (pBuffer != NULL) {
									SETIBSENDLIMIT(*pBuffer, 0xffffffff);
								}
								_outputHeaders.ReadFromBuffer((const uint8_t *) "Connection: close\r\n", 19);
								break;
							}
							default:
							{
								FATAL("Invalid body type returned by HTTPInterface::GetOutputBodyType: %d", _outputBodyType);
								EnqueueForDelete();
								return NULL;
							}
						}
						break;
					}
						// </editor-fold>
						// <editor-fold defaultstate="collapsed" desc="default">
					default:
					{
						FATAL("Invalid HTTP protocol type");
						EnqueueForDelete();
						return NULL;
					}
						// </editor-fold>
				}
				MOVE_STATE(STATE_OUTPUT_HEADERS_SEND);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT">
			case STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT:
			{
				IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
				uint32_t bufferLength = 0;
				if (pBuffer != NULL) {
					bufferLength = GETAVAILABLEBYTESCOUNT(*pBuffer);
				}
				if (bufferLength == 0) {
					if (_pInterface->HasMoreChunkedData()) {
						DEBUG_HTTP("The output buffer is empty but there should be more data");
						return NULL;
					}
				}
				_outputHeaders.ReadFromRepeat(0, 12);
				sprintf((char *) (GETIBPOINTER(_outputHeaders) + GETAVAILABLEBYTESCOUNT(_outputHeaders) - 12), "\r\n%08"PRIx32"\r", bufferLength);
				GETIBPOINTER(_outputHeaders)[GETAVAILABLEBYTESCOUNT(_outputHeaders) - 1] = '\n';
				if (pBuffer != NULL) {
					SETIBSENDLIMIT(*pBuffer, bufferLength);
				}
				MOVE_STATE(STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT">
			case STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT:
			{
				_outputHeaders.ReadFromBuffer((const uint8_t *) "\r\n0\r\n\r\n", 7);
				MOVE_STATE(STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_SEND_*">
			case STATE_OUTPUT_HEADERS_SEND:
			case STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT:
			case STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT:
			{
				return &_outputHeaders;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_BODY_SEND">
			case STATE_OUTPUT_BODY_SEND:
			{
				IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
				if (pBuffer == NULL || (GETIBSENDLIMIT(*pBuffer) == 0))
					return NULL;
				return _pNearProtocol->GetOutputBuffer();
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="default">
			default:
			{
				FATAL("Invalid HTTP state");
				EnqueueForDelete();
				return NULL;
			}
				// </editor-fold>
		}
	}
}

void HTTPProtocol::ReadyForSend() {
	if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
		FATAL("No near protocol or is not HTTP compatible");
		EnqueueForDelete();
		return;
	}
	switch (_state) {
			// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_*">
		case STATE_INPUT_START:
		case STATE_INPUT_REQUEST:
		case STATE_INPUT_RESPONSE:
		case STATE_INPUT_HEADER_BEGIN:
		case STATE_INPUT_HEADER:
		case STATE_INPUT_END_CRLF:
		case STATE_INPUT_BODY_CONTENT_LENGTH:
		case STATE_INPUT_BODY_CHUNKED_LENGTH:
		case STATE_INPUT_BODY_CHUNKED_PAYLOAD:
		case STATE_INPUT_BODY_CHUNKED_PAYLOAD_END:
		case STATE_INPUT_END:
		{
			BaseProtocol::ReadyForSend();
			return;
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_SEND">
		case STATE_OUTPUT_HEADERS_SEND:
		{
			switch (_outputBodyType) {
				case HTTP_TRANSFER_UNKNOWN:
				{
					FATAL("Invalid body type detected");
					EnqueueForDelete();
					return;
				}
				case HTTP_TRANSFER_CONTENT_LENGTH:
				{
					MOVE_STATE(STATE_OUTPUT_BODY_SEND);
					break;
				}
				case HTTP_TRANSFER_CHUNKED:
				{
					MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT);
					break;
				}
				case HTTP_TRANSFER_CONNECTION_CLOSE:
				{
					MOVE_STATE(STATE_OUTPUT_BODY_SEND);
					break;
				}
				default:
				{
					FATAL("Invalid HTTP transfer type");
					EnqueueForDelete();
					return;
				}
			}
			EnqueueForOutbound();
			return;
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT">
		case STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT:
		{
			MOVE_STATE(STATE_OUTPUT_BODY_SEND);
			EnqueueForOutbound();
			return;
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT">
		case STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT:
		{
			MOVE_STATE(STATE_INPUT_START);
			if (_type == PT_INBOUND_HTTP2) {
				if (!_pInterface->SignalResponseTransferComplete()) {
					FATAL("Unable to signal HTTP response transfer completed");
					EnqueueForDelete();
					return;
				}
			} else {
				if (!_pInterface->SignalRequestTransferComplete()) {
					FATAL("Unable to signal HTTP request transfer completed");
					EnqueueForDelete();
					return;
				}
			}
			return;
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_BODY_SEND">
		case STATE_OUTPUT_BODY_SEND:
		{
			switch (_outputBodyType) {
				case HTTP_TRANSFER_UNKNOWN:
				{
					FATAL("Invalid body type detected");
					EnqueueForDelete();
					return;
				}
				case HTTP_TRANSFER_CONTENT_LENGTH:
				{
					BaseProtocol::ReadyForSend();
					MOVE_STATE(STATE_INPUT_START);
					if (_type == PT_INBOUND_HTTP2) {
						if (!_pInterface->SignalResponseTransferComplete()) {
							FATAL("Unable to signal HTTP response transfer completed");
							EnqueueForDelete();
							return;
						}
					} else {
						if (!_pInterface->SignalRequestTransferComplete()) {
							FATAL("Unable to signal HTTP request transfer completed");
							EnqueueForDelete();
							return;
						}
					}
					EnqueueForOutbound();
					return;
				}
				case HTTP_TRANSFER_CHUNKED:
				{
					BaseProtocol::ReadyForSend();
					IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
					uint32_t bufferLength = 0;
					if (pBuffer != NULL) {
						bufferLength = GETAVAILABLEBYTESCOUNT(*pBuffer);
					}
					if (bufferLength == 0) {
						if (_pInterface->HasMoreChunkedData()) {
							DEBUG_HTTP("The output buffer is empty but there should be more data");
							MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT);
						} else {
							MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT);
						}
					} else {
						MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT);
					}
					EnqueueForOutbound();
					return;
				}
				case HTTP_TRANSFER_CONNECTION_CLOSE:
				{
					BaseProtocol::ReadyForSend();
					EnqueueForOutbound();
					return;
				}
				default:
				{
					FATAL("Invalid HTTP transfer type");
					EnqueueForDelete();
					return;
				}
			}
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT">
		case STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT:
		{
			return;
		}
			// </editor-fold>
			// <editor-fold defaultstate="collapsed" desc="default">
		default:
		{
			FATAL("Invalid HTTP state: %s", GetStateString(_state));
			EnqueueForDelete();
			return;
		}
			// </editor-fold>
	}
}

IOBuffer * HTTPProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool HTTPProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_TCP
			|| type == PT_INBOUND_SSL
			|| type == PT_OUTBOUND_SSL;
}

bool HTTPProtocol::AllowNearProtocol(uint64_t type) {
	//we can tunnel any kind of data using HTTP
	return true;
}

uint64_t HTTPProtocol::GetDecodedBytesCount() {
	return _decodedBytesCount;
}

bool HTTPProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool HTTPProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool HTTPProtocol::SignalInputData(IOBuffer &buffer) {
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
	if (length == 0)
		return true;
	while (true) {
		switch (_state) {
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_START">
			case STATE_INPUT_START:
			{
				if (length == 0) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				_transferType = HTTP_TRANSFER_UNKNOWN;
				_keepAlive = false;
				_contentLength = 0;
				_consumedLength = 0;
				_method = HTTP_METHOD_UNKNOWN;
				switch (pBuffer[0]) {
					case '\r':
					case '\n':
					case '\t':
					case ' ':
					{
						_decodedBytesCount++;
						buffer.Ignore(1);
						pBuffer = GETIBPOINTER(buffer);
						length = GETAVAILABLEBYTESCOUNT(buffer);
						break;
					}
					case 'G':
					case 'P':
					{
						if (_type != PT_INBOUND_HTTP2) {
							FATAL("Invalid HTTP request: not GET nor POST");
							return false;
						}
						MOVE_STATE(STATE_INPUT_REQUEST);
						break;
					}
					case 'H':
					{
						if (_type != PT_OUTBOUND_HTTP2) {
							FATAL("Invalid HTTP response");
							return false;
						}
						MOVE_STATE(STATE_INPUT_RESPONSE);
						break;
					}
					default:
					{
						FATAL("Invalid HTTP request: not GET/POST nor response");
						return false;
					}
				}
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_REQUEST">
			case STATE_INPUT_REQUEST:
			{
				if (length < 16) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				uint32_t uriStart = 0;
				uint32_t uriEnd = 0;
				//GET /asdsa HTTP/1.1xx
				if ((pBuffer[0] == 'G')
						&& (pBuffer[1] == 'E')
						&& (pBuffer[2] == 'T')
						&& (pBuffer[3] == ' ')
						&& (pBuffer[4] == '/')
						) {
					uriStart = 4;
				} else if ((pBuffer[0] == 'P')
						&& (pBuffer[1] == 'O')
						&& (pBuffer[2] == 'S')
						&& (pBuffer[3] == 'T')
						&& (pBuffer[4] == ' ')
						&& (pBuffer[5] == '/')
						) {
					if (length < 17) {
						DEBUG_HTTP("Not enough data");
						return true;
					}
					uriStart = 5;
				} else {
					FATAL("Invalid HTTP request. Supported methods are GET and POST");
					return false;
				}
				for (uint32_t i = uriStart; i <= length - 11; i++) {
					if ((i - uriStart) >= HTTP_MAX_URI_LENGTH) {
						FATAL("Invalid HTTP request. URI bigger than %"PRIu32" byes", HTTP_MAX_URI_LENGTH);
						return false;
					}

					if (pBuffer[i] != ' ')
						continue;
					uriEnd = i;
					break;
				}
				//DEBUG_HTTP("uriEnd: %"PRIu32"; length: %"PRIu32, uriEnd, length);
				if (((uriEnd + 11) > length) || (uriEnd == 0)) {
					if (length >= HTTP_MAX_HEADERS_SIZE) {
						FATAL("Bogus HTTP request. First line is too long");
						return false;
					}
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if ((pBuffer[uriEnd] != ' ')
						|| (pBuffer[uriEnd + 1] != 'H')
						|| (pBuffer[uriEnd + 2] != 'T')
						|| (pBuffer[uriEnd + 3] != 'T')
						|| (pBuffer[uriEnd + 4] != 'P')
						|| (pBuffer[uriEnd + 5] != '/')
						|| (pBuffer[uriEnd + 6] != '1')
						|| (pBuffer[uriEnd + 7] != '.')
						|| (pBuffer[uriEnd + 8] != '1')
						|| (pBuffer[uriEnd + 9] != 0x0d)
						|| (pBuffer[uriEnd + 10] != 0x0a)
						) {
					FATAL("Invalid HTTP request.Requested HTTP version not supported");
					return false;
				}
				pBuffer[uriEnd] = 0;
				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (!_pInterface->SignalRequestBegin(
						uriStart == 4 ? HTTP_METHOD_REQUEST_GET : HTTP_METHOD_REQUEST_POST,
						(char *) (pBuffer + uriStart),
						uriEnd - uriStart,
						HTTP_VERSION_1_1,
						HTTP_VERSION_1_1_LEN)) {
					FATAL("Unable to call next protocol in the stack");
					return false;
				}
				//DEBUG_HTTP("_headers:\n%s", STR(_headers.ToString()));
				_decodedBytesCount += (uriEnd + 11);
				buffer.Ignore(uriEnd + 11);
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				MOVE_STATE(STATE_INPUT_HEADER_BEGIN);
				_method = uriStart == 4 ? HTTP_METHOD_REQUEST_GET : HTTP_METHOD_REQUEST_POST;
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_RESPONSE">
			case STATE_INPUT_RESPONSE:
			{
				//				string a = string((const char *) pBuffer, length);
				//				FINEST("\n`%s`\n", STR(a));
				if (length < 16) {
					//"HTTP/1.1 xxx Y\r\n"
					DEBUG_HTTP("Not enough data");
					return true;
				}

				if ((pBuffer[0] != 'H')
						|| (pBuffer[1] != 'T')
						|| (pBuffer[2] != 'T')
						|| (pBuffer[3] != 'P')
						|| (pBuffer[4] != '/')
						|| (pBuffer[5] != '1')
						|| (pBuffer[6] != '.')
						|| ((pBuffer[7] != '1') && (pBuffer[7] != '0'))
						|| (pBuffer[8] != ' ')
						|| (pBuffer[9] < '0')
						|| (pBuffer[9] > '9')
						|| (pBuffer[10] < '0')
						|| (pBuffer[10] > '9')
						|| (pBuffer[11] < '0')
						|| (pBuffer[11] > '9')
						|| (pBuffer[12] != ' ')
						) {
					FATAL("Invalid HTTP Response. Incorrect first line");
					return false;
				}

				uint32_t statusCodeDescLen = 0;
				for (uint32_t i = 13; i <= length - 2; i++) {
					if ((i - 13) >= HTTP_MAX_STATUS_CODE_DESCRIPTION_LENGTH) {
						FATAL("Invalid HTTP response error code description. Error code description bigger than %"PRIu32" byes", HTTP_MAX_STATUS_CODE_DESCRIPTION_LENGTH);
						return false;
					}
					if ((pBuffer[i + 1] == '\r')
							&& (pBuffer[i + 2] == '\n')) {
						statusCodeDescLen = i - 13 + 1;
						break;
					}
				}

				if (statusCodeDescLen == 0) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				pBuffer[8] = 0;
				pBuffer[12] = 0;
				pBuffer[13 + statusCodeDescLen] = 0;

				uint32_t statusCode = (uint32_t) atoi((const char *) (pBuffer + 9));

				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (!_pInterface->SignalResponseBegin(
						(char *) pBuffer,
						8,
						statusCode,
						(char *) (pBuffer + 13),
						statusCodeDescLen)) {
					FATAL("Unable to call next protocol in the stack");
					return false;
				}

				_decodedBytesCount += (statusCodeDescLen + 15);
				buffer.Ignore(statusCodeDescLen + 15);
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				_method = HTTP_METHOD_RESPONSE;
				MOVE_STATE(STATE_INPUT_HEADER_BEGIN);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_HEADER_BEGIN">
			case STATE_INPUT_HEADER_BEGIN:
			{
				if (length < 2) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if ((pBuffer[0] == 0x0d) && (pBuffer[1] == 0x0a)) {
					MOVE_STATE(STATE_INPUT_END_CRLF);
				} else {
					MOVE_STATE(STATE_INPUT_HEADER);
				}
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_HEADER">
			case STATE_INPUT_HEADER:
			{
				//				string a((char *) pBuffer, length);
				//				DEBUG_HTTP("--------------\n`%s`", STR(a));
				if (length < 2) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				uint32_t colonPos = 0;
				uint32_t valStartPos = 0;
				uint32_t crlfPos = 0;
				for (uint32_t i = 0; i < length - 1; i++) {
					if (i >= HTTP_MAX_HEADERS_SIZE) {
						FATAL("Invalid HTTP request. Header longer than %"PRIu32, HTTP_MAX_HEADERS_SIZE);
						return false;
					}
					if ((pBuffer[i] == 0x0d) && (pBuffer[i + 1] == 0x0a)) {
						crlfPos = i;
						break;
					}
					if (colonPos == 0) {
						if (pBuffer[i] != ':')
							continue;
						colonPos = i;
						continue;
					}
					if (valStartPos == 0) {
						if ((pBuffer[i] == ' ') || (pBuffer[i] == '\t'))
							continue;
						valStartPos = i;
					}
				}
				if (crlfPos == 0) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if (colonPos == 0) {
					FATAL("Invalid header line detected");
					return false;
				}
				pBuffer[colonPos] = 0;
				pBuffer[crlfPos] = 0;

				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (!_pInterface->SignalHeader(
						(const char *) pBuffer,
						colonPos,
						(valStartPos == 0) ? NULL : (char *) (pBuffer + valStartPos),
						(valStartPos == 0) ? 0 : crlfPos - valStartPos)) {
					FATAL("Unable to signal new http headers on upper protocol: %s", STR(*this));
					return false;
				}

				if ((colonPos == 14)
						&& ((pBuffer[0] == 'c') || (pBuffer[0] == 'C'))
						&& ((pBuffer[1] == 'o') || (pBuffer[1] == 'O'))
						&& ((pBuffer[2] == 'n') || (pBuffer[2] == 'N'))
						&& ((pBuffer[3] == 't') || (pBuffer[3] == 'T'))
						&& ((pBuffer[4] == 'e') || (pBuffer[4] == 'E'))
						&& ((pBuffer[5] == 'n') || (pBuffer[5] == 'N'))
						&& ((pBuffer[6] == 't') || (pBuffer[6] == 'T'))
						&& (pBuffer[7] == '-')
						&& ((pBuffer[8] == 'l') || (pBuffer[8] == 'L'))
						&& ((pBuffer[9] == 'e') || (pBuffer[9] == 'E'))
						&& ((pBuffer[10] == 'n') || (pBuffer[10] == 'N'))
						&& ((pBuffer[11] == 'g') || (pBuffer[11] == 'G'))
						&& ((pBuffer[12] == 't') || (pBuffer[12] == 'T'))
						&& ((pBuffer[13] == 'h') || (pBuffer[13] == 'H'))
						) {
					if (_transferType == HTTP_TRANSFER_CHUNKED) {
						FATAL("Invalid HTTP message. The connection was previously marked as chunked transport");
						return false;
					}
					if (_contentLength != 0) {
						FATAL("Invalid HTTP message. The connection was previously declared as transporting %"PRIu32" bytes", _contentLength);
						return false;
					}
					_contentLength = atoi((char *) (pBuffer + valStartPos));
					_transferType = HTTP_TRANSFER_CONTENT_LENGTH;
				} else if ((colonPos == 17)
						&& ((crlfPos - valStartPos) >= 7)
						&& ((pBuffer[0] == 't') || (pBuffer[0] == 'T'))
						&& ((pBuffer[1] == 'r') || (pBuffer[1] == 'R'))
						&& ((pBuffer[2] == 'a') || (pBuffer[2] == 'A'))
						&& ((pBuffer[3] == 'n') || (pBuffer[3] == 'N'))
						&& ((pBuffer[4] == 's') || (pBuffer[4] == 'S'))
						&& ((pBuffer[5] == 'f') || (pBuffer[5] == 'F'))
						&& ((pBuffer[6] == 'e') || (pBuffer[6] == 'E'))
						&& ((pBuffer[7] == 'r') || (pBuffer[7] == 'R'))
						&& (pBuffer[8] == '-')
						&& ((pBuffer[9] == 'e') || (pBuffer[9] == 'E'))
						&& ((pBuffer[10] == 'n') || (pBuffer[10] == 'N'))
						&& ((pBuffer[11] == 'c') || (pBuffer[11] == 'C'))
						&& ((pBuffer[12] == 'o') || (pBuffer[12] == 'O'))
						&& ((pBuffer[13] == 'd') || (pBuffer[13] == 'D'))
						&& ((pBuffer[14] == 'i') || (pBuffer[14] == 'I'))
						&& ((pBuffer[15] == 'n') || (pBuffer[15] == 'N'))
						&& ((pBuffer[16] == 'g') || (pBuffer[16] == 'G'))
						&& ((pBuffer[valStartPos + 0] == 'c') || (pBuffer[valStartPos + 0] == 'C'))
						&& ((pBuffer[valStartPos + 1] == 'h') || (pBuffer[valStartPos + 1] == 'H'))
						&& ((pBuffer[valStartPos + 2] == 'u') || (pBuffer[valStartPos + 2] == 'U'))
						&& ((pBuffer[valStartPos + 3] == 'n') || (pBuffer[valStartPos + 3] == 'N'))
						&& ((pBuffer[valStartPos + 4] == 'k') || (pBuffer[valStartPos + 4] == 'K'))
						&& ((pBuffer[valStartPos + 5] == 'e') || (pBuffer[valStartPos + 5] == 'E'))
						&& ((pBuffer[valStartPos + 6] == 'd') || (pBuffer[valStartPos + 6] == 'D'))
						) {
					if (_transferType == HTTP_TRANSFER_CONTENT_LENGTH) {
						FATAL("Invalid HTTP message. The connection was previously marked as content-length transport");
						return false;
					}
					_transferType = HTTP_TRANSFER_CHUNKED;
				} else if ((colonPos == 10)
						&& ((crlfPos - valStartPos) >= 10)
						&& ((pBuffer[0] == 'c') || (pBuffer[0] == 'C'))
						&& ((pBuffer[1] == 'o') || (pBuffer[1] == 'O'))
						&& ((pBuffer[2] == 'n') || (pBuffer[2] == 'N'))
						&& ((pBuffer[3] == 'n') || (pBuffer[3] == 'N'))
						&& ((pBuffer[4] == 'e') || (pBuffer[4] == 'E'))
						&& ((pBuffer[5] == 'c') || (pBuffer[5] == 'C'))
						&& ((pBuffer[6] == 't') || (pBuffer[6] == 'T'))
						&& ((pBuffer[7] == 'i') || (pBuffer[7] == 'I'))
						&& ((pBuffer[8] == 'o') || (pBuffer[8] == 'O'))
						&& ((pBuffer[9] == 'n') || (pBuffer[9] == 'N'))
						&& ((pBuffer[valStartPos + 0] == 'k') || (pBuffer[valStartPos + 0] == 'K'))
						&& ((pBuffer[valStartPos + 1] == 'e') || (pBuffer[valStartPos + 1] == 'E'))
						&& ((pBuffer[valStartPos + 2] == 'e') || (pBuffer[valStartPos + 2] == 'E'))
						&& ((pBuffer[valStartPos + 3] == 'p') || (pBuffer[valStartPos + 3] == 'P'))
						&& (pBuffer[valStartPos + 4] == '-')
						&& ((pBuffer[valStartPos + 5] == 'a') || (pBuffer[valStartPos + 5] == 'A'))
						&& ((pBuffer[valStartPos + 6] == 'l') || (pBuffer[valStartPos + 6] == 'L'))
						&& ((pBuffer[valStartPos + 7] == 'i') || (pBuffer[valStartPos + 7] == 'I'))
						&& ((pBuffer[valStartPos + 8] == 'v') || (pBuffer[valStartPos + 8] == 'V'))
						&& ((pBuffer[valStartPos + 9] == 'e') || (pBuffer[valStartPos + 9] == 'E'))
						) {
					if (_transferType == HTTP_TRANSFER_CONNECTION_CLOSE) {
						FATAL("Invalid HTTP message. The connection was previously marked as close");
						return false;
					}
					_keepAlive = true;
				} else if ((colonPos == 10)
						&& ((crlfPos - valStartPos) >= 5)
						&& ((pBuffer[0] == 'c') || (pBuffer[0] == 'C'))
						&& ((pBuffer[1] == 'o') || (pBuffer[1] == 'O'))
						&& ((pBuffer[2] == 'n') || (pBuffer[2] == 'N'))
						&& ((pBuffer[3] == 'n') || (pBuffer[3] == 'N'))
						&& ((pBuffer[4] == 'e') || (pBuffer[4] == 'E'))
						&& ((pBuffer[5] == 'c') || (pBuffer[5] == 'C'))
						&& ((pBuffer[6] == 't') || (pBuffer[6] == 'T'))
						&& ((pBuffer[7] == 'i') || (pBuffer[7] == 'I'))
						&& ((pBuffer[8] == 'o') || (pBuffer[8] == 'O'))
						&& ((pBuffer[9] == 'n') || (pBuffer[9] == 'N'))
						&& ((pBuffer[valStartPos + 0] == 'c') || (pBuffer[valStartPos + 0] == 'C'))
						&& ((pBuffer[valStartPos + 1] == 'l') || (pBuffer[valStartPos + 1] == 'L'))
						&& ((pBuffer[valStartPos + 2] == 'o') || (pBuffer[valStartPos + 2] == 'O'))
						&& ((pBuffer[valStartPos + 3] == 's') || (pBuffer[valStartPos + 3] == 'S'))
						&& ((pBuffer[valStartPos + 4] == 'e') || (pBuffer[valStartPos + 4] == 'E'))
						) {
					if (_keepAlive) {
						FATAL("Invalid HTTP message. The connection was previously marked as keep-alive");
						return false;
					}
					if (_transferType == HTTP_TRANSFER_UNKNOWN) {
						if ((_method == HTTP_METHOD_REQUEST_GET) || (_method == HTTP_METHOD_REQUEST_POST)) {
							_transferType = HTTP_TRANSFER_CONTENT_LENGTH;
							_contentLength = 0;
						} else {
							_transferType = HTTP_TRANSFER_CONNECTION_CLOSE;
						}
					}
				}
				_decodedBytesCount += (crlfPos + 2);
				buffer.Ignore(crlfPos + 2);
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				MOVE_STATE(STATE_INPUT_HEADER_BEGIN);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_END_CRLF">
			case STATE_INPUT_END_CRLF:
			{
				switch (_transferType) {
					case HTTP_TRANSFER_UNKNOWN:
					{
#if 1
						switch (_method) {
							case HTTP_METHOD_RESPONSE:
							{
								_transferType = HTTP_TRANSFER_CONNECTION_CLOSE;
								_contentLength = 0;
								break;
							}
							case HTTP_METHOD_REQUEST_GET:
							case HTTP_METHOD_REQUEST_POST:
							{
								_transferType = HTTP_TRANSFER_CONTENT_LENGTH;
								_contentLength = 0;
								break;
							}
							case HTTP_METHOD_UNKNOWN:
							default:
							{
								FATAL("Invalid HTTP message. Unable to determine the HTTP method of transport");
								return false;
							}
						}
						break;
#else
						FATAL("Invalid HTTP message. Unable to determine the HTTP method of transport");
						return false;
#endif
					}
					case HTTP_TRANSFER_CONTENT_LENGTH:
					{
						if (length < 2) {
							DEBUG_HTTP("Not enough data");
							return true;
						}
						_decodedBytesCount += (2);
						buffer.Ignore(2);
						pBuffer = GETIBPOINTER(buffer);
						length = GETAVAILABLEBYTESCOUNT(buffer);
						MOVE_STATE(STATE_INPUT_BODY_CONTENT_LENGTH);
						if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
							FATAL("No near protocol or is not HTTP compatible");
							return false;
						}
						if (!_pInterface->SignalHeadersEnd()) {
							FATAL("Unable to signal end of http headers section on upper protocol: %s", STR(*this));
							return false;
						}
						break;
					}
					case HTTP_TRANSFER_CHUNKED:
					{
						if (length < 2) {
							DEBUG_HTTP("Not enough data");
							return true;
						}
						_decodedBytesCount += (2);
						buffer.Ignore(2);
						pBuffer = GETIBPOINTER(buffer);
						length = GETAVAILABLEBYTESCOUNT(buffer);
						MOVE_STATE(STATE_INPUT_BODY_CHUNKED_LENGTH);
						if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
							FATAL("No near protocol or is not HTTP compatible");
							return false;
						}
						if (!_pInterface->SignalHeadersEnd()) {
							FATAL("Unable to signal end of http headers section on upper protocol: %s", STR(*this));
							return false;
						}
						break;
					}
					case HTTP_TRANSFER_CONNECTION_CLOSE:
					{
						if (length < 2) {
							DEBUG_HTTP("Not enough data");
							return true;
						}
						_decodedBytesCount += (2);
						buffer.Ignore(2);
						pBuffer = GETIBPOINTER(buffer);
						length = GETAVAILABLEBYTESCOUNT(buffer);
						MOVE_STATE(STATE_INPUT_BODY_CONNECTION_CLOSE);
						if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
							FATAL("No near protocol or is not HTTP compatible");
							return false;
						}
						if (!_pInterface->SignalHeadersEnd()) {
							FATAL("Unable to signal end of http headers section on upper protocol: %s", STR(*this));
							return false;
						}
						break;
					}
					default:
					{
						FATAL("Invalid HTTP message. Unable to determine the transfer type");
						return false;
					}
				}
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_BODY_CONTENT_LENGTH, STATE_INPUT_BODY_CHUNKED_PAYLOAD">
			case STATE_INPUT_BODY_CONTENT_LENGTH:
			case STATE_INPUT_BODY_CHUNKED_PAYLOAD:
			{
				if (_contentLength == 0) {
					pBuffer = GETIBPOINTER(buffer);
					length = GETAVAILABLEBYTESCOUNT(buffer);
					MOVE_STATE(STATE_INPUT_END);
					break;
				}
				uint32_t available = _contentLength - _consumedLength;
				available = available <= length ? available : length;
				uint32_t remaining = _contentLength - available - _consumedLength;
				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (!_pInterface->SignalBodyData(_transferType,
						_contentLength, _consumedLength, available,
						remaining)) {
					FATAL("Unable to signal begin of http body section on upper protocol: %s", STR(*this));
					return false;
				}
				uint32_t beforeCall = 0;
				uint32_t afterCall = 0;
				if (available > 0) {
					beforeCall = length;
					if (_pNearProtocol != NULL) {
						if (!_pNearProtocol->SignalInputData(buffer)) {
							FATAL("Unable to call next protocol in the chain");
							return false;
						}
					} else {
						uint32_t ignoreLength = (length > (_contentLength - _consumedLength))
								? (_contentLength - _consumedLength)
								: length;
						_decodedBytesCount += ignoreLength;
						buffer.Ignore(ignoreLength);
					}
					afterCall = GETAVAILABLEBYTESCOUNT(buffer);
					if (beforeCall < afterCall) {
						FATAL("HTTP request handled badly. Next protocol in the chain appended bytes into the input buffer");
						return false;
					}
					_consumedLength += (beforeCall - afterCall);
					_decodedBytesCount += (beforeCall - afterCall);
					if (_consumedLength > _contentLength) {
						FATAL("HTTP request handled badly. Next protocol in the chain consumed more than the content length");
						return false;
					}
				}
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				if ((beforeCall == afterCall) && (_contentLength != 0)) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if (_consumedLength == _contentLength) {
					MOVE_STATE((_state == STATE_INPUT_BODY_CONTENT_LENGTH) ? STATE_INPUT_END : STATE_INPUT_BODY_CHUNKED_PAYLOAD_END);
				}
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_BODY_CONNECTION_CLOSE">
			case STATE_INPUT_BODY_CONNECTION_CLOSE:
			{
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);

				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (!_pInterface->SignalBodyData(_transferType, 0,
						_consumedLength, length, 0)) {
					FATAL("Unable to signal begin of http body section on upper protocol: %s", STR(*this));
					return false;
				}
				uint32_t beforeCall = 0;
				uint32_t afterCall = 0;
				if (length > 0) {
					beforeCall = length;
					if (_pNearProtocol != NULL) {
						if (!_pNearProtocol->SignalInputData(buffer)) {
							FATAL("Unable to call next protocol in the chain");
							return false;
						}
					} else {
						_decodedBytesCount += length;
						buffer.Ignore(length);
					}
					afterCall = GETAVAILABLEBYTESCOUNT(buffer);
					if (beforeCall < afterCall) {
						FATAL("HTTP request handled badly. Next protocol in the chain appended bytes into the input buffer");
						return false;
					}
					_consumedLength += (beforeCall - afterCall);
					_decodedBytesCount += (beforeCall - afterCall);
				} else {
					DEBUG_HTTP("No data");
					return true;
				}
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				if ((beforeCall == afterCall) && (_contentLength != 0)) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_BODY_CHUNKED_PAYLOAD_END">
			case STATE_INPUT_BODY_CHUNKED_PAYLOAD_END:
			{
				if (length < 2) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if ((pBuffer[0] != 0x0d) || (pBuffer[1] != 0x0a)) {
					FATAL("HTTP chunk not ending with 0x0d 0x0a");
					return false;
				}
				buffer.Ignore(2);
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				_decodedBytesCount += 2;
				MOVE_STATE(STATE_INPUT_BODY_CHUNKED_LENGTH);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_BODY_CHUNKED_LENGTH">
			case STATE_INPUT_BODY_CHUNKED_LENGTH:
			{
				if (length < 2) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				bool leadingWhiteSpace = true;
				int32_t crlfPos = -1;
				uint32_t value = 0;
				for (uint32_t i = 0; i < length - 1; i++) {
					if (i >= 10) {
						FATAL("Invalid chunked content. Chunk size can't be bigger than 8 characters or the size contains chunk-extension");
						return false;
					}

					if ((pBuffer[i] == 0x0d) && (pBuffer[i + 1] == 0x0a)) {
						crlfPos = (int32_t) i;
						break;
					}

					if (leadingWhiteSpace) {
						if ((pBuffer[i] == ' ') || (pBuffer[i] == '\t') || (pBuffer[i] == '0')) {
							continue;
						}
						leadingWhiteSpace = false;
					}

					if (('0' <= pBuffer[i]) && (pBuffer[i] <= '9')) {
						value = (value << 4) | (pBuffer[i]-(uint8_t) '0');
					} else if (('a' <= pBuffer[i]) && (pBuffer[i] <= 'f')) {
						value = (value << 4) | (pBuffer[i]-(uint8_t) 'a' + 10);
					} else if (('A' <= pBuffer[i]) && (pBuffer[i] <= 'F')) {
						value = (value << 4) | (pBuffer[i]-(uint8_t) 'A' + 10);
					} else {
						FATAL("Invalid chunked content. Chunk size contains forbidden characters");
						return false;
					}
				}
				if (crlfPos == -1) {
					DEBUG_HTTP("Not enough data");
					return true;
				}
				if (crlfPos == 0) {
					FATAL("Invalid chunked content. Chunk size is empty");
					return false;
				}
				_contentLength = value;
				_decodedBytesCount += (crlfPos + 2);
				_consumedLength = 0;
				buffer.Ignore(crlfPos + 2);
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				MOVE_STATE(STATE_INPUT_BODY_CHUNKED_PAYLOAD);
				break;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_INPUT_END">
			case STATE_INPUT_END:
			{
				if (_type == PT_OUTBOUND_HTTP2) {
					if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
						FATAL("No near protocol or is not HTTP compatible");
						return false;
					}
					if (!_pInterface->NeedsNewOutboundRequest()) {
						DEBUG_HTTP("No more outbound requests");
						EnqueueForDelete();
						return true;
					}
				}
				pBuffer = GETIBPOINTER(buffer);
				length = GETAVAILABLEBYTESCOUNT(buffer);
				MOVE_STATE(STATE_OUTPUT_HEADERS_BUILD);
				if ((_pNearProtocol == NULL) || ((_pInterface = _pNearProtocol->GetHTTPInterface()) == NULL)) {
					FATAL("No near protocol or is not HTTP compatible");
					return false;
				}
				if (_method == HTTP_METHOD_RESPONSE) {
					if (!_pInterface->SignalResponseTransferComplete()) {
						FATAL("Unable to signal HTTP response transfer complete on upper protocol: %s", STR(*this));
						return false;
					}
				} else {
					if (!_pInterface->SignalRequestTransferComplete()) {
						FATAL("Unable to signal HTTP request transfer complete on upper protocol: %s", STR(*this));
						return false;
					}
				}
				if (!EnqueueForOutbound()) {
					FATAL("Unable send output data from HTTP protocol");
					return false;
				}
				return true;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="STATE_OUTPUT_*">
			case STATE_OUTPUT_HEADERS_BUILD:
			case STATE_OUTPUT_HEADERS_SEND:
			case STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT:
			case STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT:
			case STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT:
			case STATE_OUTPUT_BODY_SEND:
			{
				if (length >= MAX_HTTP_BUFFER_LENGTH) {
					FATAL("waited too long for outputting data and the HTTP input buffer grew too big: %"PRIu32" >= %"PRIu32,
							length, MAX_HTTP_BUFFER_LENGTH);
					return false;
				}
				DEBUG_HTTP("Not parsing any HTTP data yet. Data needs to be send out first");
				return true;
			}
				// </editor-fold>
				// <editor-fold defaultstate="collapsed" desc="default">
			default:
			{
				FATAL("Invalid HTTP request. Invalid state: %"PRIu32, _state);
				return false;
			}
				// </editor-fold>
		}
	}
}
#endif /* HAS_PROTOCOL_HTTP2 */
