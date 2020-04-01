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


#include "dissector/rtmpdissectorprotocol.h"
#include "protocols/protocoltypes.h"

RTMPDissectorProtocol::RTMPDissectorProtocol(bool isClient)
: BaseRTMPProtocol(PT_RTMP_DISSECTOR) {
	_isClient = isClient;
}

RTMPDissectorProtocol::~RTMPDissectorProtocol() {
}

bool RTMPDissectorProtocol::EnqueueForOutbound() {
	ASSERT("OOOps!");
}

bool RTMPDissectorProtocol::PerformHandshake(IOBuffer &buffer) {
	if (GETAVAILABLEBYTESCOUNT(buffer) < 3073)
		return true;
	if (_isClient) {
		if (GETIBPOINTER(buffer)[0] != 0x03) {
			FATAL("RTMPE not supported");
			return false;
		}
	}
	buffer.Ignore(3073);
	_handshakeCompleted = true;
	return true;
}

bool RTMPDissectorProtocol::Ingest(DataBlock &block) {
	if (block.packetPayloadLength == 0)
		return true;
	_internalBuffer.ReadFromBuffer(block.pPacketPayload, block.packetPayloadLength);

	return SignalInputData(_internalBuffer);
}

bool RTMPDissectorProtocol::Ingest(IOBuffer &buffer) {
	if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
		return true;
	_internalBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));

	return SignalInputData(_internalBuffer);
}

bool RTMPDissectorProtocol::IsClient() {
	return _isClient;
}
