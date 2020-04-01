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
#ifndef _HTTPPROTOCOL_H
#define	_HTTPPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/http/httpinterface.h"

class IOBuffer;
class IOBuffer;

enum HttpState {
	STATE_UNKNOWN = 0,
	STATE_INPUT_START,
	STATE_INPUT_REQUEST,
	STATE_INPUT_RESPONSE,
	STATE_INPUT_HEADER_BEGIN,
	STATE_INPUT_HEADER,
	STATE_INPUT_END_CRLF,
	STATE_INPUT_BODY_CONTENT_LENGTH,
	STATE_INPUT_BODY_CHUNKED_LENGTH,
	STATE_INPUT_BODY_CHUNKED_PAYLOAD,
	STATE_INPUT_BODY_CHUNKED_PAYLOAD_END,
	STATE_INPUT_BODY_CONNECTION_CLOSE,
	STATE_INPUT_END,
	STATE_OUTPUT_HEADERS_BUILD,
	STATE_OUTPUT_HEADERS_SEND,
	STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT,
	STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT,
	STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT,
	STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT,
	STATE_OUTPUT_BODY_SEND
};

static inline const char *GetStateString(HttpState state) {
	switch (state) {
		case STATE_UNKNOWN:return "STATE_UNKNOWN";
		case STATE_INPUT_START:return "STATE_INPUT_START";
		case STATE_INPUT_REQUEST:return "STATE_INPUT_REQUEST";
		case STATE_INPUT_RESPONSE:return "STATE_INPUT_RESPONSE";
		case STATE_INPUT_HEADER_BEGIN:return "STATE_INPUT_HEADER_BEGIN";
		case STATE_INPUT_HEADER:return "STATE_INPUT_HEADER";
		case STATE_INPUT_END_CRLF:return "STATE_INPUT_END_CRLF";
		case STATE_INPUT_BODY_CONTENT_LENGTH:return "STATE_INPUT_BODY_CONTENT_LENGTH";
		case STATE_INPUT_BODY_CHUNKED_LENGTH:return "STATE_INPUT_BODY_CHUNKED_LENGTH";
		case STATE_INPUT_BODY_CHUNKED_PAYLOAD:return "STATE_INPUT_BODY_CHUNKED_PAYLOAD";
		case STATE_INPUT_BODY_CHUNKED_PAYLOAD_END:return "STATE_INPUT_BODY_CHUNKED_PAYLOAD_END";
		case STATE_INPUT_BODY_CONNECTION_CLOSE:return "STATE_INPUT_BODY_CONNECTION_CLOSE";
		case STATE_INPUT_END:return "STATE_INPUT_END";
		case STATE_OUTPUT_HEADERS_BUILD:return "STATE_OUTPUT_HEADERS_BUILD";
		case STATE_OUTPUT_HEADERS_SEND:return "STATE_OUTPUT_HEADERS_SEND";
		case STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT:return "STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT";
		case STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT:return "STATE_OUTPUT_HEADERS_SEND_CHUNKED_CONTENT";
		case STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT:return "STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT";
		case STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT:return "STATE_OUTPUT_HEADERS_SEND_LAST_CHUNKED_CONTENT";
		case STATE_OUTPUT_BODY_SEND:return "STATE_OUTPUT_BODY_SEND";
		default:return "#(unknown)#";
	}
};

class DLLEXP HTTPProtocol
: public BaseProtocol {
private:
	HttpState _state;
	uint64_t _decodedBytesCount;
	IOBuffer _inputBuffer;
	IOBuffer _outputHeaders;
	HttpBodyType _outputBodyType;
	bool _disconnectAfterTransfer;
	HttpBodyType _transferType;
	uint32_t _contentLength;
	uint32_t _consumedLength;
	uint32_t _method;
	bool _keepAlive;
	HTTPInterface *_pInterface;
public:
	HTTPProtocol(uint64_t protocolType);
	virtual ~HTTPProtocol();

	virtual IOBuffer * GetOutputBuffer();
	virtual void ReadyForSend();
	virtual IOBuffer * GetInputBuffer();
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual uint64_t GetDecodedBytesCount();
	virtual bool Initialize(Variant &parameters);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	//protected:
	//	virtual bool Authenticate() = 0;
};

#endif	/* _HTTPPROTOCOL_H */
#endif /* HAS_PROTOCOL_HTTP2 */
