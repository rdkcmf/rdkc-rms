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


#ifdef HAS_PROTOCOL_RPC
#include "protocols/rpc/baserpcprotocol.h"
#include "protocols/rpc/rpcserializer.h"
#include "protocols/rtmp/header_le_ba.h"

BaseRPCProtocol::BaseRPCProtocol(uint64_t type)
: BaseProtocol(type) {
	_available = 0;
	_pInputSerializer = NULL;
	_pOutputSerializer = NULL;
}

BaseRPCProtocol::~BaseRPCProtocol() {
	if (_pInputSerializer != NULL) {
		delete _pInputSerializer;
		_pInputSerializer = NULL;
	}
	if (_pOutputSerializer != NULL) {
		delete _pOutputSerializer;
		_pOutputSerializer = NULL;
	}
}

bool BaseRPCProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool BaseRPCProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool BaseRPCProtocol::SignalInputData(IOBuffer &buffer) {
	if ((_pFarProtocol != NULL)
			&& (_pFarProtocol->GetType() != PT_INBOUND_HTTP2)
			&& (_pFarProtocol->GetType() != PT_OUTBOUND_HTTP2)) {
		_available = GETAVAILABLEBYTESCOUNT(buffer);
	}
	string a = string((const char *) GETIBPOINTER(buffer), _available);
	//FINEST("available: %"PRIu32"; \n%s", _available, STR(a));
	buffer.Ignore(_available);
	return true;
}

bool BaseRPCProtocol::SerializeOutput(Variant &source) {
	if (_pOutputSerializer != NULL) {
		return _pOutputSerializer->Serialize(source, _outputBuffer);
	}
	return false;
}

IOBuffer *BaseRPCProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer))
		return &_outputBuffer;
	return NULL;
}

#endif /* HAS_PROTOCOL_RPC */
