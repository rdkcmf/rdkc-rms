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



#ifdef HAS_PROTOCOL_RTMP
#include "protocols/rtmp/rtmpeprotocol.h"

RTMPEProtocol::RTMPEProtocol(RC4_KEY *pKeyIn, RC4_KEY *pKeyOut, uint32_t skipBytes)
: BaseProtocol(PT_RTMPE) {
	_pKeyIn = pKeyIn;
	_pKeyOut = pKeyOut;
	_skipBytes = skipBytes;
}

RTMPEProtocol::~RTMPEProtocol() {
}

bool RTMPEProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_INBOUND_HTTP_FOR_RTMP)
			|| (type == PT_TCP);
}

bool RTMPEProtocol::AllowNearProtocol(uint64_t type) {
	return (type == PT_INBOUND_RTMP)
			|| (type == PT_OUTBOUND_RTMP);
}

IOBuffer * RTMPEProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

IOBuffer * RTMPEProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;
	return NULL;
}

bool RTMPEProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool RTMPEProtocol::SignalInputData(IOBuffer &buffer) {
	RC4(_pKeyIn, GETAVAILABLEBYTESCOUNT(buffer),
			GETIBPOINTER(buffer),
			GETIBPOINTER(buffer));

	_inputBuffer.ReadFromBuffer(GETIBPOINTER(buffer),
			GETAVAILABLEBYTESCOUNT(buffer));
	buffer.IgnoreAll();

	if (_pNearProtocol != NULL)
		return _pNearProtocol->SignalInputData(_inputBuffer);
	return true;
}

bool RTMPEProtocol::EnqueueForOutbound() {

	IOBuffer *pOutputBuffer = _pNearProtocol->GetOutputBuffer();

	if (pOutputBuffer == NULL)
		return true;

	RC4(_pKeyOut, GETAVAILABLEBYTESCOUNT(*pOutputBuffer) - _skipBytes,
			GETIBPOINTER(*pOutputBuffer) + _skipBytes,
			GETIBPOINTER(*pOutputBuffer) + _skipBytes);
	_skipBytes = 0;

	_outputBuffer.ReadFromInputBuffer(pOutputBuffer, 0, GETAVAILABLEBYTESCOUNT(*pOutputBuffer));
	pOutputBuffer->Ignore(GETAVAILABLEBYTESCOUNT(*pOutputBuffer));

	if (_pFarProtocol != NULL)
		return _pFarProtocol->EnqueueForOutbound();

	return true;
}

#endif /* HAS_PROTOCOL_RTMP */

