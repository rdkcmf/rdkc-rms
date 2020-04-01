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


#include "application/filters.h"
#include "protocols/protocolmanager.h"
#include "protocols/baseprotocol.h"
using namespace app_rdkcrouter;

bool app_rdkcrouter::protocolManagerProtocolTypeFilter(BaseProtocol *pProtocol) {
	uint64_t type = pProtocol->GetType();
	return ((pProtocol->GetNearProtocol() == NULL)
			&& ((type == PT_INBOUND_RTMP)
			|| (type == PT_OUTBOUND_RTMP)
			|| (type == PT_INBOUND_TS)
			|| (type == PT_PASSTHROUGH)
			|| (type == PT_RTSP)
			|| (type == PT_INBOUND_LIVE_FLV)));
}
