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
#include "protocols/ws/basewsprotocol.h"
#include "protocols/ws/wsinterface.h"

#define MAX_WS_INPUT_BUFFER_LENGTH (64*1024*1024)
#define MAX_WS_OUTPUT_BUFFER_LENGTH MAX_WS_INPUT_BUFFER_LENGTH

BaseWSProtocol::BaseWSProtocol(uint64_t type)
: BaseProtocol(type) {
	assert((type == PT_INBOUND_WS) || (type == PT_OUTBOUND_WS));
	_wsHandshakeComplete = false;
	_isText = false;
	_outIsBinary = true;
}

BaseWSProtocol::~BaseWSProtocol() {
}

bool BaseWSProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool BaseWSProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP) || ((_type == PT_INBOUND_WS) ?
			(type == PT_INBOUND_SSL) : (type == PT_OUTBOUND_SSL));
}

bool BaseWSProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

IOBuffer * BaseWSProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

bool BaseWSProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool BaseWSProtocol::SignalInputData(IOBuffer &buffer) {
	return _wsHandshakeComplete ? SignalWSInputData(buffer) : ProcessWSHandshake(buffer);
}

bool BaseWSProtocol::SendOutOfBandData(const IOBuffer &buffer, void *pUserData) {
	if (!_wsHandshakeComplete) {
		//DEBUG("$b2 WSProtocol::SendOutOfBandData finishing handshake");
		_tempBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
		return true;
	}

	// If pUserData is not null, then it means we have to send metadata instead of
	// stream data.
	if (pUserData) {
		string jsonData;
		((Variant*)pUserData)->SerializeToJSON(jsonData,true);
		size_t size = jsonData.length();
		_metadataBuffer.IgnoreAll();
		_metadataBuffer.ReadFromU32((uint32_t)size | (0xEE << 24), true);
		_metadataBuffer.ReadFromString(jsonData);
		return SendBuffer(_metadataBuffer, GetType() == PT_OUTBOUND_WS)
			&& BaseProtocol::EnqueueForOutbound();
	} else {
		return SendBuffer(_tempBuffer, GetType() == PT_OUTBOUND_WS)
			&& _tempBuffer.IgnoreAll()
			&& SendBuffer(buffer, GetType() == PT_OUTBOUND_WS)
			&& BaseProtocol::EnqueueForOutbound()
			;
	}
}

bool BaseWSProtocol::SignalWSInputData(IOBuffer &buffer) {
	WSInterface *pInterface = NULL;
	if ((_pNearProtocol == NULL) || ((pInterface = _pNearProtocol->GetWSInterface()) == NULL)) {
		FATAL("Protocol not compatible with WebSocket transport");
		return false;
	}

	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint64_t length = GETAVAILABLEBYTESCOUNT(buffer);
	if (length >= (MAX_WS_INPUT_BUFFER_LENGTH + 14)) {
		FATAL("WebSocket payload too big or malformed");
		return false;
	}
	if (length < 2)
		return true;

	bool hasMask = (pBuffer[1] >> 7) != 0;
	uint64_t messageLength = (pBuffer[1]&0x7f);
	uint8_t headersLength = hasMask ? 4 : 0;
	switch (messageLength) {
		case 126:
		{
			if (length < 4)
				return true;
			messageLength = ENTOHSP(pBuffer + 2);
			headersLength += 4;
			break;
		}
		case 127:
		{
			if (length < 10)
				return true;
			messageLength = ENTOHLLP(pBuffer + 2);
			headersLength += 8;
			break;
		}
		default:
		{
			headersLength += 2;
			break;
		}
	}

	if ((messageLength + headersLength) < length)
		return true;

	//demasking only the data frames, when mask flag is on and the upper
	//protocol wants demasking
	if ((hasMask) && pInterface->DemaskBuffer()) {
		for (size_t i = 0; i < messageLength; i++)
			pBuffer[i + headersLength] = pBuffer[i + headersLength]^pBuffer[headersLength - 4 + (i % 4)];
	}

	switch (pBuffer[0]&0x0f) {
		case 0x00://continuation
		{
			return (_isText ? pInterface->SignalTextData(pBuffer + headersLength, (size_t) messageLength)
					: pInterface->SignalBinaryData(pBuffer + headersLength, (size_t) messageLength))
					&&buffer.Ignore((uint32_t) (messageLength + headersLength));
		}
		case 0x01://text
		{
			_isText = true;
			return pInterface->SignalTextData(pBuffer + headersLength, (size_t) messageLength)
					&& buffer.Ignore((uint32_t) (messageLength + headersLength));
		}
		case 0x02://binary
		{
			_isText = false;
			return pInterface->SignalBinaryData(pBuffer + headersLength, (size_t) messageLength)
					&& buffer.Ignore((uint32_t) (messageLength + headersLength));
		}
		case 0x08://connection close
		{
			if ((messageLength != 0)&&(messageLength < 2)) {
				FATAL("Invalid WebSocket close message");
				return false;
			}
			if (messageLength != 0) {
				uint16_t code = ENTOHSP(pBuffer + headersLength);
				pInterface->SignalConnectionClose(code, pBuffer + headersLength + 2, (size_t) (messageLength - 2));
			} else {
				pInterface->SignalConnectionClose(0, NULL, 0);
			}
			_outputBuffer.IgnoreAll();
			GracefullyEnqueueForDelete();
			return true;
		}
		case 0x09://ping
		{
			pInterface->SignalPing(messageLength == 0 ? NULL : (pBuffer + headersLength), (size_t) messageLength);
			_outputBuffer.ReadFromRepeat(0x8a, 1);
			_outputBuffer.ReadFromRepeat((uint8_t) messageLength, 1);
			_outputBuffer.ReadFromBuffer(pBuffer + headersLength, (uint32_t) messageLength);
			buffer.Ignore((uint32_t) (messageLength + headersLength));
			EnqueueForOutbound();
			return true;
		}
		case 0x0a://pong
		{
			return buffer.Ignore((uint32_t) (messageLength + headersLength));
		}
		default:
		{
			FATAL("Invalid WebSocket operation: 0x%02"PRIx8, (uint8_t) (pBuffer[0]&0x0f));
			return false;
		}
	}
}

bool BaseWSProtocol::SendBuffer(const IOBuffer &buffer, bool mask) {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) >= MAX_WS_OUTPUT_BUFFER_LENGTH) {
		FATAL("Output buffer grew too big");
		return false;
	}

	size_t totalSize = GETAVAILABLEBYTESCOUNT(buffer);
	if (totalSize == 0)
		return true;

	_outputBuffer.ReadFromRepeat(_outIsBinary ? 0x82 : 0x81, 1);

	if (totalSize < 126) {
		_outputBuffer.ReadFromRepeat((uint8_t) totalSize | (mask ? 0x80 : 0x00), 1);
	} else if (totalSize < 65536) {
		_outputBuffer.ReadFromRepeat(126 | (mask ? 0x80 : 0x00), 1);
		_outputBuffer.ReadFromU16((uint16_t) totalSize, true);
	} else {
		_outputBuffer.ReadFromRepeat(127 | (mask ? 0x80 : 0x00), 1);
		// Fixed issue for data more than 64k
		_outputBuffer.ReadFromU64((uint64_t) totalSize, true);
	}

	if (mask) {
		_outputBuffer.ReadFromRepeat(0, 4);
	}
	_outputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
	return true;
}

#endif /* HAS_PROTOCOL_WS */
