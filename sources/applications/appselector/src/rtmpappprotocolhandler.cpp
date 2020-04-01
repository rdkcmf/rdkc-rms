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



#ifdef HAS_PROTOCOL_RTMP
#include "rtmpappprotocolhandler.h"
#include "application/clientapplicationmanager.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/outboundrtmpprotocol.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"
using namespace app_appselector;

RTMPAppProtocolHandler::RTMPAppProtocolHandler(Variant &configuration)
: BaseRTMPAppProtocolHandler(configuration) {

}

RTMPAppProtocolHandler::~RTMPAppProtocolHandler() {
}

bool RTMPAppProtocolHandler::ProcessInvokeConnect(BaseRTMPProtocol *pFrom,
		Variant &request) {
	//1. Test the wanted name
	if ((VariantType) M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_CONNECT_APP] == V_NULL) {
		FATAL("No app specified");
		return false;
	}
	string appName = M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_CONNECT_APP];
	if (appName == "") {
		FATAL("No app specified");
		return false;
	}

	//2. Get the clean appName without the parameters
	vector<string> parts;
	split(appName, "?", parts);
	appName = parts[0];
	if (appName[appName.length() - 1] == '/')
		appName = appName.substr(0, appName.length() - 1);

	//3. Find the wanted application
	BaseClientApplication *pApp = ClientApplicationManager::FindAppByName(appName);
	if (pApp == NULL) {
		FATAL("Application %s not found", STR(appName));
		return false;
	}

	//4. Are we going to land on the same app (we are in appselector now)?
	if (pApp->GetId() == GetApplication()->GetId()) {
		FATAL("appselector can be a final destination");
		return false;
	}

	//5. Get the RTMP handler from that app
	BaseRTMPAppProtocolHandler *pHandler =
			(BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(pFrom);
	if (pHandler == NULL) {
		FATAL("Unable to get a valid handler");
		return false;
	}

	//6. Some fancy logging
	string realAppName = pApp->GetName();
	if (realAppName == appName)
		FINEST("Selected application: %s", STR(appName));
	else
		FINEST("Selected application: %s (%s)", STR(realAppName), STR(appName));

	//7. Set the new application inside the connection
	pFrom->SetApplication(pApp);

	//8. Remove any authentication state so it will be
	//constructed by the target RTMP handler
	Variant &parameters = pFrom->GetCustomParameters();
	if ((parameters == V_MAP) && (parameters.HasKey("authState")))
		parameters.RemoveKey("authState");

	//9. Pass the control over
	return pHandler->InboundMessageAvailable(pFrom, request);
}

bool RTMPAppProtocolHandler::OutboundConnectionEstablished(
		OutboundRTMPProtocol *pFrom) {
	//1. Get the parameters
	Variant &parameters = pFrom->GetCustomParameters();

	//2. Test them a little bit
	if ((VariantType) parameters[CONF_APPLICATION_NAME] != V_STRING) {
		FATAL("No app specified in the outbound connection parameters");
		return false;
	}

	//3. Get the target app name
	string appName = parameters[CONF_APPLICATION_NAME];
	if (appName == "") {
		FATAL("No app specified in the outbound connection parameters");
		return false;
	}
	if (appName[appName.length() - 1] == '/')
		appName = appName.substr(0, appName.length() - 1);

	//4. Find wanted application
	BaseClientApplication *pApp = ClientApplicationManager::FindAppByName(appName);
	if (pApp == NULL) {
		FATAL("Application %s not found", STR(appName));
		return false;
	}

	//5. Are we going to land on the same app (we are in appselector now)?
	if (pApp->GetId() == GetApplication()->GetId()) {
		FATAL("appselector can be a final destination");
		return false;
	}

	//6. Get the RTMP handler
	BaseRTMPAppProtocolHandler *pHandler =
			(BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(pFrom);
	if (pHandler == NULL) {
		FATAL("Unable to get a valid handler");
		return false;
	}

	//7. Set the new application inside the connection
	pFrom->SetApplication(pApp);

	//8. Pass control over
	return pHandler->OutboundConnectionEstablished(pFrom);
}
#endif /* HAS_PROTOCOL_RTMP */
