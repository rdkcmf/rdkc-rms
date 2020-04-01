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
/**
* HISTORY:
 * , 1/29/2015 
 * - ripped from nmeaappprotocolhandler.cpp
* 
**/

//#ifdef HAS_PROTOCOL_JSONMETADATA

#include "application/baseclientapplication.h"
#include "protocols/metadata/jsonmetadataappprotocolhandler.h"
#include "protocols/metadata/jsonmetadataprotocol.h"

JsonMetadataAppProtocolHandler::JsonMetadataAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {
	//FATAL("$b2$ JsonMetadataAppProtocolHandler() config=%s", STR(configuration.ToString()));
	_pApp = NULL;
}

JsonMetadataAppProtocolHandler::~JsonMetadataAppProtocolHandler() {
	_pApp = NULL;
}

void JsonMetadataAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
    //WARN("Calling JsonMetadataAppProtocolHandler::RegisterProtocol");
        //1. Is this an JsonMetadata protocol?
	if (pProtocol->GetType() != PT_JSONMETADATA)
		return;
}

void JsonMetadataAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
    //WARN("No Op for now");
}

void JsonMetadataAppProtocolHandler::SetApplication(BaseClientApplication *pApplication) {
	_pApp = (app_rdkcrouter::OriginApplication *) pApplication;
}

bool JsonMetadataAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	//WARN("$b2:  THIS IS NOT CALLED IN THE NORMAL SCHEME OF THINGS!");
	//1. Verify this is the proper handler
	if (lowerCase(uri.scheme()) != "jsonmeta") {
		FATAL("Not a valid URI");
		return false;
		
	}
	//2. Prepare the custom parameters
	Variant parameters;
	parameters["customParameters"]["externalStreamConfig"] = streamConfig;
	parameters[CONF_APPLICATION_NAME] = GetApplication()->GetName();
	parameters[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_JSONMETADATA;
        
	return JsonMetadataProtocol::Connect(uri.ip(), uri.port(), parameters);
}

//#endif //HAS_PROTOCOL_JSONMETADATA
