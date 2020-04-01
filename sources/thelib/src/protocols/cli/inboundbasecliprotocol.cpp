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


#ifdef HAS_PROTOCOL_CLI
#include "protocols/cli/inboundbasecliprotocol.h"
#include "application/clientapplicationmanager.h"
#include "protocols/cli/basecliappprotocolhandler.h"

InboundBaseCLIProtocol::InboundBaseCLIProtocol(uint64_t type)
: BaseProtocol(type) {
	_pProtocolHandler = NULL;
}

InboundBaseCLIProtocol::~InboundBaseCLIProtocol() {
}

bool InboundBaseCLIProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

void InboundBaseCLIProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pProtocolHandler = (BaseCLIAppProtocolHandler *)
				pApplication->GetProtocolHandler(this);
	} else {
		_pProtocolHandler = NULL;
	}
}

bool InboundBaseCLIProtocol::AllowFarProtocol(uint64_t type) {
#ifdef HAS_UDS
	return (type == PT_TCP) || (type == PT_HTTP_4_CLI) || (type == PT_UDS);
#else
	return (type == PT_TCP) || (type == PT_HTTP_4_CLI);
#endif
}

bool InboundBaseCLIProtocol::AllowNearProtocol(uint64_t type) {
	ASSERT("Operation not supported");
	return false;
}

IOBuffer * InboundBaseCLIProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

bool InboundBaseCLIProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool InboundBaseCLIProtocol::ProcessMessage(Variant &message) {
	if (_pProtocolHandler == NULL) {
		FATAL("No handler available yet");
		return false;
	}
	return _pProtocolHandler->ProcessMessage(this, message);
}
bool InboundBaseCLIProtocol::SendRaw(string str) {
	_outputBuffer.ReadFromString(str);
	return EnqueueForOutbound();
}

#endif /* HAS_PROTOCOL_CLI */
