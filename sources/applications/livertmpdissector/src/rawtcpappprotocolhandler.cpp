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


#include "rawtcpappprotocolhandler.h"
#include "protocols/baseprotocol.h"
#include "session.h"
#include "livertmpdissectorapplication.h"
#include "protocolfactory.h"
#include "application/clientapplicationmanager.h"

using namespace app_livertmpdissector;

RawTcpAppProtocolHandler::RawTcpAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {
}

RawTcpAppProtocolHandler::~RawTcpAppProtocolHandler() {
}

void RawTcpAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	Variant &parameters = pProtocol->GetCustomParameters();
	if (parameters.HasKeyChain(V_BOOL, true, 1, "isOutbound")&&((bool)parameters["isOutbound"])) {
	} else {
		if (!CreateSession(pProtocol)) {
			FATAL("Unable to create session");
			pProtocol->EnqueueForDelete();
		}
	}
}

void RawTcpAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
	if (pProtocol == NULL)
		return;
	Session *pSession = Session::GetSession(
			pProtocol->GetCustomParameters()["sessionId"]);
	if (pSession != NULL)
		delete pSession;
}

bool RawTcpAppProtocolHandler::SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters) {
	if (pProtocol == NULL)
		return false;

	BaseClientApplication *pApp = ClientApplicationManager::FindAppById(parameters["appId"]);
	if (pApp == NULL) {
		pProtocol->EnqueueForDelete();
		FATAL("Application not found");
		return false;
	}

	Session *pSession = Session::GetSession(parameters["sessionId"]);
	if (pSession == NULL) {
		pProtocol->EnqueueForDelete();
		FATAL("Session not found");
		return false;
	}

	pProtocol->SetOutboundConnectParameters(parameters);

	pProtocol->SetApplication(pApp);

	pSession->OutboundProtocolId(pProtocol);

	return true;
}

bool RawTcpAppProtocolHandler::CreateSession(BaseProtocol *pInbound) {
	Session *pSession = new Session(_configuration, GetApplication());
	Variant &parameters = pInbound->GetCustomParameters();
	parameters["sessionId"] = pSession->GetId();

	pSession->InboundProtocolId(pInbound);

	LiveRTMPDissectorApplication *pApp = (LiveRTMPDissectorApplication *) GetApplication();

	Variant connectParams;
	connectParams["appId"] = pApp->GetId();
	connectParams["sessionId"] = pSession->GetId();
	connectParams["isOutbound"] = (bool)true;

	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain("rawTcp");
	if (chain.size() == 0) {
		FATAL("Unable to resolve protocol chain rawTcp");
		return false;
	}

	return TCPConnector<RawTcpAppProtocolHandler>::Connect(
			pApp->GetTargetIp(),
			pApp->GetTargetPort(),
			chain,
			connectParams
			);
}
