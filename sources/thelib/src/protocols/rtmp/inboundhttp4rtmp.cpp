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



#ifdef HAS_PROTOCOL_HTTP
#ifdef HAS_PROTOCOL_RTMP
#include "protocols/rtmp/inboundhttp4rtmp.h"
#include "protocols/http/inboundhttpprotocol.h"
#include "netio/netio.h"
#include "protocols/rtmp/inboundrtmpprotocol.h"
#include "protocols/protocolmanager.h"

#define RTMPT_STATE_HEADER 1
#define RTMPT_STATE_FCS_IDENT 2
#define RTMPT_STATE_OPEN 3
#define RTMPT_STATE_IDLE 4
#define RTMPT_STATE_SEND 5
#define RTMPT_STATE_CLOSE 6

#define RETURN_FALSE do{FINEST("HERE");return false;}while(0)

#define MAGIC_BYTE 1

#define RTMPT_HEADERS_KEEP_ALIVE		HTTP_VERSION_1_1" 200 OK\r\n"HTTP_HEADERS_SERVER": "HTTP_HEADERS_SERVER_US"\r\n"HTTP_HEADERS_CONNECTION": "HTTP_HEADERS_CONNECTION_KEEP_ALIVE"\r\n"HTTP_HEADERS_CONTENT_LENGTH": "
#define RTMPT_HEADERS_KEEP_ALIVE_LEN	HTTP_VERSION_1_1_LEN+9+HTTP_HEADERS_SERVER_LEN+2+HTTP_HEADERS_SERVER_US_LEN+2+HTTP_HEADERS_CONNECTION_LEN+2+HTTP_HEADERS_CONNECTION_KEEP_ALIVE_LEN+2+HTTP_HEADERS_CONTENT_LENGTH_LEN+2
#define RTMPT_HEADERS_CLOSE				HTTP_VERSION_1_1" 200 OK\r\n"HTTP_HEADERS_SERVER": "HTTP_HEADERS_SERVER_US"\r\n"HTTP_HEADERS_CONNECTION": "HTTP_HEADERS_CONNECTION_CLOSE"\r\n"HTTP_HEADERS_CONTENT_LENGTH": "
#define RTMPT_HEADERS_CLOSE_LEN			HTTP_VERSION_1_1_LEN+9+HTTP_HEADERS_SERVER_LEN+2+HTTP_HEADERS_SERVER_US_LEN+2+HTTP_HEADERS_CONNECTION_LEN+2+HTTP_HEADERS_CONNECTION_CLOSE_LEN+2+HTTP_HEADERS_CONTENT_LENGTH_LEN+2

map<uint64_t, time_t> InboundHTTP4RTMP::_generatedSids;
map<uint64_t, uint32_t> InboundHTTP4RTMP::_protocolsBySid;
uint32_t InboundHTTP4RTMP::_activityTimer = 0;
uint16_t InboundHTTP4RTMP::_crc1 = 0;
uint64_t InboundHTTP4RTMP::_crc2 = 0;

InboundHTTP4RTMP::CleanupTimerProtocol::CleanupTimerProtocol() {

}

InboundHTTP4RTMP::CleanupTimerProtocol::~CleanupTimerProtocol() {

}

bool InboundHTTP4RTMP::CleanupTimerProtocol::TimePeriodElapsed() {
	time_t current = time(NULL);

	map<uint64_t, time_t>::iterator i = InboundHTTP4RTMP::_generatedSids.begin();
	while (i != InboundHTTP4RTMP::_generatedSids.end()) {
		//		FINEST("Sid: %016"PRIx64"; current: %"PRIz"u; sid_time: %"PRIz"u; diff: %"PRIu32,
		//				MAP_KEY(i), current, MAP_VAL(i), (uint32_t) (current - MAP_VAL(i)));
		if ((current - MAP_VAL(i)) > 10) {
			BaseProtocol *pProtocol = ProtocolManager::GetProtocol(InboundHTTP4RTMP::_protocolsBySid[MAP_KEY(i)]);
			if ((pProtocol == NULL) || (pProtocol->GetFarProtocol() == NULL)) {
				//				FINEST("SID: %016"PRIx64" removed", MAP_KEY(i));
				if (pProtocol != NULL)
					pProtocol->EnqueueForDelete();
				InboundHTTP4RTMP::_protocolsBySid.erase(MAP_KEY(i));
				InboundHTTP4RTMP::_generatedSids.erase(i++);
			} else {
				//FINEST("Skip SID: %016"PRIx64, MAP_KEY(i));
				i++;
			}
		} else {
			i++;
		}
	}

	return true;
}

InboundHTTP4RTMP::InboundHTTP4RTMP()
: BaseProtocol(PT_INBOUND_HTTP_FOR_RTMP) {
	_httpState = RTMPT_STATE_HEADER;
	_contentLength = -1;
	_keepAlive = false;
	if (_crc1 == 0)
		_crc1 = (uint16_t) rand();
	if (_crc2 == 0)
		_crc2 = ((uint64_t) rand() << 32) | rand();
	if (_activityTimer == 0) {
		CleanupTimerProtocol *pTimer = new CleanupTimerProtocol();
		pTimer->EnqueueForTimeEvent(10);
		_activityTimer = pTimer->GetId();
	}
	_sid = 0;
	DeleteNearProtocol(false);
}

InboundHTTP4RTMP::~InboundHTTP4RTMP() {
}

bool InboundHTTP4RTMP::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool InboundHTTP4RTMP::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP) || (type == PT_INBOUND_SSL);
}

bool InboundHTTP4RTMP::AllowNearProtocol(uint64_t type) {
	return (type == PT_INBOUND_RTMP) || (type == PT_RTMPE);
}

IOBuffer * InboundHTTP4RTMP::GetOutputBuffer() {
	//return GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0 ? &_outputBuffer : ((_pNearProtocol != NULL) ? _pNearProtocol->GetOutputBuffer() : NULL);
	return GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0 ? &_outputBuffer : NULL;
}

bool InboundHTTP4RTMP::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool InboundHTTP4RTMP::SignalInputData(IOBuffer &buffer) {
	char *pBuffer = (char *) GETIBPOINTER(buffer);
	char *pCursor = pBuffer;
	char *pStringBegin = NULL;
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
	uint8_t newHttpState;
	if (length == 0)
		return true;

	switch (_httpState) {
		case RTMPT_STATE_HEADER:
		{
			_contentLength = -1;
			if (length < 47)
				return true;
			if (memmem(pCursor, 6, "POST /", 6) != pBuffer)
				return false;
			pCursor += 6;
			_sid = 0;
			switch (pCursor[0]) {
				case 'f':
				{
					if (memmem(pCursor, 11, "fcs/ident2 ", 11) == pCursor) {
						pCursor += 11;
					} else if (memmem(pCursor, 10, "fcs/ident ", 10) == pCursor) {
						pCursor += 10;
					} else {
						return false;
					}
					pCursor += 10; //skip "HTTP/1.x\r\n"
					newHttpState = RTMPT_STATE_FCS_IDENT;
					break;
				}
				case 'o':
				{
					pCursor = (char *) memmem(pCursor, 17, "open/1 HTTP/1.1\r\n", 17);
					if (pCursor == NULL)
						return false;
					pCursor += 17;
					newHttpState = RTMPT_STATE_OPEN;
					break;
				}
				case 'i':
				{
					pCursor = (char *) memmem(pCursor, 5, "idle/", 5);
					if (pCursor == NULL)
						return false;
					pCursor += 5;
					_sid = strtoull(pCursor, NULL, 16);
					if (((((_sid^_crc2)&0x00000000ffffffff) >> 16)^((_sid^_crc2)&0x000000000000ffff)) != _crc1)
						return false;
					pCursor += 16 + 11; //skip the hex number and " HTTP/1.x\r\n"
					newHttpState = RTMPT_STATE_IDLE;
					break;
				}
				case 's':
				{
					pCursor = (char *) memmem(pCursor, 5, "send/", 5);
					if (pCursor == NULL)
						return false;
					pCursor += 5;
					_sid = strtoull(pCursor, NULL, 16);
					if (((((_sid^_crc2)&0x00000000ffffffff) >> 16)^((_sid^_crc2)&0x000000000000ffff)) != _crc1)
						return false;
					pCursor += 16 + 11; //skip the hex number and " HTTP/1.x\r\n"
					newHttpState = RTMPT_STATE_SEND;
					break;
				}
				case 'c':
				{
					pCursor = (char *) memmem(pCursor, 6, "close/", 6);
					if (pCursor == NULL)
						return false;
					pCursor += 6;
					_sid = strtoull(pCursor, NULL, 16);
					if (((((_sid^_crc2)&0x00000000ffffffff) >> 16)^((_sid^_crc2)&0x000000000000ffff)) != _crc1)
						return false;
					pCursor += 16 + 11; //skip the hex number and " HTTP/1.x\r\n"
					newHttpState = RTMPT_STATE_CLOSE;
					break;
				}
				default:
				{
					return false;
				}
			}

			_keepAlive = (memmem(pCursor, length - (pCursor - pBuffer), "Connection: keep-alive\r\n", 24) != NULL)
					|| (memmem(pCursor, length - (pCursor - pBuffer), "connection: keep-alive\r\n", 24) != NULL);

			pStringBegin = (char *) memmem(pCursor, length - (pCursor - pBuffer), "Content-Length: ", 16);
			if (pStringBegin == NULL)
				pStringBegin = (char *) memmem(pCursor, length - (pCursor - pBuffer), "content-length: ", 16);
			if (pStringBegin == NULL) {
				return length < 1024; //we don't allow more than 1k worth of HTTP headers
			}
			pStringBegin += 16;
			_contentLength = atoi(pStringBegin);
			if (_contentLength <= 0)
				return false;
			pCursor = (char *) memmem(pStringBegin, length - (pStringBegin - pBuffer), "\r\n\r\n", 4);
			if (pCursor == NULL) {
				return length < 1024; //we don't allow more than 1k worth of HTTP headers
			}
			pCursor += 4;
			buffer.Ignore((uint32_t)(pCursor - pBuffer));
			_httpState = newHttpState;
			length = GETAVAILABLEBYTESCOUNT(buffer);
			return ((_contentLength > 0)&&(length >= (uint32_t) _contentLength)) ? SignalInputData(buffer) : true;
		}
		case RTMPT_STATE_FCS_IDENT:
		{
			if ((_contentLength < 0) || (length < (uint32_t) _contentLength))
				return true;
			buffer.Ignore(_contentLength);
			_contentLength = -1;
			_httpState = RTMPT_STATE_HEADER;

			Variant &params = GetCustomParameters();
			string ident2 = "";
			if (params.HasKeyChain(V_STRING, "false", 1, "ident2"))
				ident2 = (string) params.GetValue("ident2", false);
			trim(ident2);
			if (ident2 == "") {
				IOHandler *pHandler = NULL;
				if (((pHandler = GetIOHandler()) != NULL)
						&&(pHandler->GetType() == IOHT_TCP_CARRIER))
					ident2 = ((TCPCarrier *) pHandler)->GetNearEndpointAddressIp();
			}
			if (ident2 == "")
				return false;
			if (!ComputeHTTPHeaders((uint32_t)ident2.size()))
				return false;
			_outputBuffer.ReadFromString(ident2);
			if (!BaseProtocol::EnqueueForOutbound())
				return false;
			if (!_keepAlive)
				GracefullyEnqueueForDelete();
			return true;
		}
		case RTMPT_STATE_OPEN:
		{
			if ((_contentLength < 0) || (length < (uint32_t) _contentLength))
				return true;
			buffer.Ignore(_contentLength);
			_contentLength = -1;
			_httpState = RTMPT_STATE_HEADER;
			if (!ComputeHTTPHeaders(17))
				return false;
			//2. create a SID, save it, and send it after that
			uint32_t rnd = rand()&0x0000ffff;
			rnd = (rnd << 16) | (rnd^_crc1);
			_sid = ((((uint64_t) GetId()) << 32) | rnd)^_crc2;
			string sidString = format("%016"PRIx64"\n", _sid);
			_generatedSids[_sid] = time(NULL);
			if (Bind(true) == NULL)
				return false;
			_outputBuffer.ReadFromString(sidString);
			if (!BaseProtocol::EnqueueForOutbound())
				return false;
			if (!_keepAlive)
				GracefullyEnqueueForDelete();
			return true;
		}
		case RTMPT_STATE_IDLE:
		{
			if ((_contentLength < 0) || (length < (uint32_t) _contentLength))
				return true;
			buffer.Ignore(_contentLength);
			_contentLength = -1;
			_httpState = RTMPT_STATE_HEADER;

			BaseProtocol *pProtocol = Bind(true);
			if (pProtocol == NULL)
				return false;

			IOBuffer *pNearBuffer = pProtocol->GetOutputBuffer();
			uint32_t size = ((pNearBuffer != NULL) ? GETAVAILABLEBYTESCOUNT(*pNearBuffer) : 0) + 1;
			if (!ComputeHTTPHeaders(size))
				return false;

			_outputBuffer.ReadFromByte(MAGIC_BYTE);
			if (pNearBuffer != NULL) {
				_outputBuffer.ReadFromBuffer(GETIBPOINTER(*pNearBuffer), GETAVAILABLEBYTESCOUNT(*pNearBuffer));
				pNearBuffer->IgnoreAll();
			}
			if (!BaseProtocol::EnqueueForOutbound())
				return false;

			if (!_keepAlive)
				GracefullyEnqueueForDelete();

			return true;
		}
		case RTMPT_STATE_SEND:
		{
			if ((_contentLength < 0) || (length < (uint32_t) _contentLength))
				return true;
			_contentLength = -1;
			_httpState = RTMPT_STATE_HEADER;

			BaseProtocol *pProtocol = Bind(false);
			if (pProtocol == NULL)
				return false;

			if (!pProtocol->SignalInputData(buffer))
				return false;
			if (!ComputeHTTPHeaders(1))
				return false;

			_outputBuffer.ReadFromByte(MAGIC_BYTE);
			if (!BaseProtocol::EnqueueForOutbound())
				return false;
			if (!_keepAlive)
				GracefullyEnqueueForDelete();
			return true;
		}
		case RTMPT_STATE_CLOSE:
		{
			if ((_contentLength < 0) || (length < (uint32_t) _contentLength))
				return true;
			buffer.Ignore(_contentLength);
			_contentLength = -1;
			_httpState = RTMPT_STATE_HEADER;

			_generatedSids.erase(_sid);
			BaseProtocol *pProtocol = ProtocolManager::GetProtocol(_protocolsBySid[_sid]);
			if (pProtocol != NULL) {
				pProtocol->ResetFarProtocol();
				ResetNearProtocol();
				pProtocol->EnqueueForDelete();
			}
			_protocolsBySid.erase(_sid);

			if (!ComputeHTTPHeaders(1))
				return false;
			_outputBuffer.ReadFromByte(1);
			if (!BaseProtocol::EnqueueForOutbound())
				return false;
			GracefullyEnqueueForDelete();
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool InboundHTTP4RTMP::EnqueueForOutbound() {
	return true;
}

void InboundHTTP4RTMP::ReadyForSend() {
	if (_pNearProtocol != NULL) {
		_pNearProtocol->ReadyForSend();
	}
}

bool InboundHTTP4RTMP::ComputeHTTPHeaders(uint32_t length) {
	char lengthString[16];
	_outputBuffer.ReadFromBuffer((const uint8_t *) (_keepAlive ? RTMPT_HEADERS_KEEP_ALIVE : RTMPT_HEADERS_CLOSE),
			_keepAlive ? RTMPT_HEADERS_KEEP_ALIVE_LEN : RTMPT_HEADERS_CLOSE_LEN);
	uint32_t size = snprintf(lengthString, 16, "%"PRIu32"\r\n\r\n", length);
	_outputBuffer.ReadFromBuffer((const uint8_t *) lengthString, size);
	return true;
}

BaseProtocol *InboundHTTP4RTMP::Bind(bool bind) {
	BaseProtocol *pResult = NULL;
	if (_pNearProtocol == NULL) {
		//14. This might be a new connection. Do we have that sid generated?
		if (!MAP_HAS1(_generatedSids, _sid))
			return NULL;

		//15. See if we have to generate a new connection or we just pick up
		//a disconnected one
		if (MAP_HAS1(_protocolsBySid, _sid)) {
			pResult = ProtocolManager::GetProtocol(_protocolsBySid[_sid]);
			_generatedSids[_sid] = time(NULL);
		} else {
			if (!bind)
				return NULL;
			pResult = new InboundRTMPProtocol();
			pResult->Initialize(GetCustomParameters());
			pResult->SetApplication(GetApplication());
			_protocolsBySid[_sid] = pResult->GetId();
			_generatedSids[_sid] = time(NULL);
			SetNearProtocol(pResult);
			pResult->SetFarProtocol(this);
		}
	} else {
		pResult = _pNearProtocol;
	}
	return pResult;
}
#endif /* HAS_PROTOCOL_RTMP */
#endif /* HAS_PROTOCOL_HTTP */


