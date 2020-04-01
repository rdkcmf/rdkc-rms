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


#include "rawtcpprotocol.h"
#include "localdefines.h"
#include "session.h"
using namespace app_livertmpdissector;

RawTcpProtocol::RawTcpProtocol()
: BaseProtocol(PT_RAW_TCP) {
	_sessionId = 0;
	_isOutbound = false;
}

RawTcpProtocol::~RawTcpProtocol() {
}

bool RawTcpProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

void RawTcpProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	Variant &params = GetCustomParameters();
	_isOutbound = params.HasKeyChain(V_BOOL, true, 1, "isOutbound")&&((bool)params["isOutbound"]);
}

bool RawTcpProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_TCP;
}

bool RawTcpProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

IOBuffer * RawTcpProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) == 0)
		return NULL;
	return &_outputBuffer;
}

bool RawTcpProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}

bool RawTcpProtocol::SignalInputData(IOBuffer &buffer) {
	Session *pSession = Session::GetSession(GetCustomParameters()["sessionId"]);
	if (pSession == NULL) {
		FATAL("Unable to get session");
		return false;
	}

	return pSession->FeedData(buffer, this);
}

bool RawTcpProtocol::FeedData(IOBuffer &buffer) {
	_outputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
	return EnqueueForOutbound();
}
