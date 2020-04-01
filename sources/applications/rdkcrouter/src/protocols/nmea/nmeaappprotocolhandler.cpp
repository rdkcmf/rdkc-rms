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


#ifdef HAS_PROTOCOL_NMEA

#include "application/baseclientapplication.h"
#include "protocols/nmea/nmeaappprotocolhandler.h"
#include "protocols/nmea/inboundnmeaprotocol.h"

NMEAAppProtocolHandler::NMEAAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

NMEAAppProtocolHandler::~NMEAAppProtocolHandler() {
}

void NMEAAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
    WARN("Calling NMEAAppProtocolHandler::RegisterProtocol");
        //1. Is this an NMEA protocol?
	if (pProtocol->GetType() != PT_INBOUND_NMEA)
		return;
    WARN("I have no idea if this processing is necessary");
	//2. Get the pull/push stream config
 /*** fails 
        Variant &parameters = pProtocol->GetCustomParameters();
	if ((!parameters.HasKeyChain(V_MAP, true, 2, "customParameters", "externalStreamConfig"))
			&& (!parameters.HasKeyChain(V_MAP, true, 2, "customParameters", "localStreamConfig"))) {
		WARN("Bogus connection. Terminate it");
		pProtocol->EnqueueForDelete();
		return;
	}
  ***/
}

void NMEAAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
    WARN("No Op for now");
}

bool NMEAAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
        //1. Verify this is the proper handler
	if (lowerCase(uri.scheme()) != "nmea") {
		FATAL("Not a valid URI");
		return false;
	}

        //2. Prepare the custom parameters
	Variant parameters;
	parameters["customParameters"]["externalStreamConfig"] = streamConfig;
	parameters[CONF_APPLICATION_NAME] = GetApplication()->GetName();
	parameters[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_NMEA;
        
        return InboundNMEAProtocol::Connect(uri.ip(), uri.port(), parameters);
}

#endif //HAS_PROTOCOL_NMEA
