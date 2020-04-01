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


#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
#include "protocols/ts/basetsappprotocolhandler.h"
#include "protocols/baseprotocol.h"
#include "protocols/ts/innettsstream.h"

BaseTSAppProtocolHandler::BaseTSAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

BaseTSAppProtocolHandler::~BaseTSAppProtocolHandler() {
}

void BaseTSAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
}

void BaseTSAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}
#endif	/* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */

