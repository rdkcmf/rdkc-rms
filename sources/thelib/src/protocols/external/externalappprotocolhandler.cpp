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


#ifdef HAS_PROTOCOL_EXTERNAL

#include "protocols/external/externalappprotocolhandler.h"
#include "protocols/external/externalprotocol.h"
#include "protocols/external/externalmoduleapi.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/netio.h"

ExternalAppProtocolHandler::ExternalAppProtocolHandler(Variant &config,
		protocol_factory_t *pFactory)
: BaseAppProtocolHandler(config) {
	_pFactory = pFactory;
}

ExternalAppProtocolHandler::~ExternalAppProtocolHandler() {
}

void ExternalAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	Variant &params = pProtocol->GetCustomParameters();
	void *pUserData = NULL;
	if (params.HasKeyChain(V_UINT64, false, 1, "pUserData"))
		pUserData = (void *) ((uint64_t) params.GetValue("pUserData", false));
	ExternalProtocol *pExternalProtocol = (ExternalProtocol *) pProtocol;
	connection_t *pInterface = pExternalProtocol->GetInterface();
	pInterface->context.pUserData = pUserData;
}

void ExternalAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool ExternalAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	//1. Get the protocol chain name from the scheme
	string chainName = "";
	for (uint32_t i = 0; i < _pFactory->supportedPorotocolChainSize; i++) {
		if (_pFactory->pSupportedPorotocolChains[i].pSchema == uri.scheme()) {
			chainName = _pFactory->pSupportedPorotocolChains[i].pName;
			break;
		}
	}

	//2. Resolve the protocol chain
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(chainName);
	if (chain.size() == 0) {
		_pFactory->events.connection.connectFailed(_pFactory, STR(uri.fullUriWithAuth()), STR(uri.ip()), uri.port(), NULL);
		return false;
	}

	//3. Prepare the parameters
	Variant params;
	params["customParameters"]["externalStreamConfig"] = streamConfig;
	params["pFactory"] = (uint64_t) _pFactory;
	params["ip"] = uri.ip();
	params["port"] = uri.port();
	params["pUserData"] = (uint64_t) 0;
	if (!TCPConnector<ExternalModuleAPI>::Connect(uri.ip(), uri.port(), chain, params)) {
		_pFactory->events.connection.connectFailed(_pFactory, STR(uri.fullUriWithAuth()), STR(uri.ip()), uri.port(), NULL);
		return false;
	}
	return true;
}

bool ExternalAppProtocolHandler::PushLocalStream(Variant streamConfig) {
	//1. Get the protocol chain name from the scheme
	string chainName = "";
	for (uint32_t i = 0; i < _pFactory->supportedPorotocolChainSize; i++) {
		if (streamConfig["targetUri"]["scheme"] == _pFactory->pSupportedPorotocolChains[i].pSchema) {
			chainName = _pFactory->pSupportedPorotocolChains[i].pName;
			break;
		}
	}

	//2. Resolve the protocol chain
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(chainName);
	if (chain.size() == 0) {
		_pFactory->events.connection.connectFailed(_pFactory,
				STR(streamConfig["targetUri"]["fullUriWithAuth"]),
				STR(streamConfig["targetUri"]["ip"]),
				(uint16_t) streamConfig["targetUri"]["port"],
				NULL);
		return false;
	}

	//3. Prepare the parameters
	Variant params;
	params["customParameters"]["localStreamConfig"] = streamConfig;
	params["pFactory"] = (uint64_t) _pFactory;
	params["ip"] = streamConfig["targetUri"]["ip"];
	params["port"] = (uint16_t) streamConfig["targetUri"]["port"];
	params["pUserData"] = (uint64_t) 0;
	if (!TCPConnector<ExternalModuleAPI>::Connect(
			streamConfig["targetUri"]["ip"],
			(uint16_t) streamConfig["targetUri"]["port"],
			chain, params)) {
		_pFactory->events.connection.connectFailed(_pFactory,
				STR(streamConfig["targetUri"]["fullUriWithAuth"]),
				STR(streamConfig["targetUri"]["ip"]),
				(uint16_t) streamConfig["targetUri"]["port"],
				NULL);
		return false;
	}
	return true;
}

#endif /* HAS_PROTOCOL_EXTERNAL */
