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

#include "protocols/dev/basedeviceprotocol.h"
#include "streaming/basestream.h"

BaseDeviceProtocol::BaseDeviceProtocol(uint64_t type) : BaseProtocol(type) {
	_pStream = NULL;
}

BaseDeviceProtocol::~BaseDeviceProtocol() {
    INFO("Protocol Destroyed");
	if (_pStream != NULL) {
            delete _pStream;
            _pStream = NULL;
        }
}

bool BaseDeviceProtocol::Initialize(Variant &parameters) {
	return true;
}

bool BaseDeviceProtocol::AllowFarProtocol(uint64_t type) {
	return false;
}

bool BaseDeviceProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool BaseDeviceProtocol::SignalInputData(int32_t recvAmount) {
	return true;
}

bool BaseDeviceProtocol::SignalInputData(IOBuffer &buffer) {
	return true;
}
void BaseDeviceProtocol::SetStream(BaseStream *pStream) {
	_pStream = pStream;
}

