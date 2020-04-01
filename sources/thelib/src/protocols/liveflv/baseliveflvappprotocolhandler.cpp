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



#ifdef HAS_PROTOCOL_LIVEFLV

#include "protocols/liveflv/baseliveflvappprotocolhandler.h"
#include "protocols/baseprotocol.h"
#include "application/baseclientapplication.h"

BaseLiveFLVAppProtocolHandler::BaseLiveFLVAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

BaseLiveFLVAppProtocolHandler::~BaseLiveFLVAppProtocolHandler() {
}

void BaseLiveFLVAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	if (MAP_HAS1(_protocols, pProtocol->GetId())) {
		ASSERT("Protocol ID %u already registered", pProtocol->GetId());
		return;
	}
	if (pProtocol->GetType() != PT_INBOUND_LIVE_FLV) {
		ASSERT("This protocol can't be registered here");
		return;
	}
	_protocols[pProtocol->GetId()] = (InboundLiveFLVProtocol *) pProtocol;
	FINEST("protocol %s registered to app %s", STR(*pProtocol), STR(GetApplication()->GetName()));
}

void BaseLiveFLVAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
	if (!MAP_HAS1(_protocols, pProtocol->GetId())) {
		ASSERT("Protocol ID %u not registered", pProtocol->GetId());
		return;
	}
	if (pProtocol->GetType() != PT_INBOUND_LIVE_FLV) {
		ASSERT("This protocol can't be unregistered here");
		return;
	}
	_protocols.erase(pProtocol->GetId());
	FINEST("protocol %s unregistered from app %s", STR(*pProtocol), STR(GetApplication()->GetName()));
}
#endif /* HAS_PROTOCOL_LIVEFLV */
