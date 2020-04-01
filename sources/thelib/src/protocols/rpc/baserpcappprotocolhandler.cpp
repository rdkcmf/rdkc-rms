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


#ifdef HAS_PROTOCOL_RPC
#include "protocols/protocolfactorymanager.h"
#include "protocols/rpc/baserpcappprotocolhandler.h"
#include "protocols/rpc/outboundrpcprotocol.h"
#include "application/baseappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"

BaseRPCAppProtocolHandler::BaseRPCAppProtocolHandler(Variant& configuration)
: BaseAppProtocolHandler(configuration) {
	_outboundHttpRpc = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTP_RPC);
	_outboundHttpsRpc = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTPS_RPC);
	if (_outboundHttpRpc.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTP_RPC);
	}
	if (_outboundHttpsRpc.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTPS_RPC);
	}
	_urlCache["dummy"] = Variant();
}

BaseRPCAppProtocolHandler::~BaseRPCAppProtocolHandler() {
}

void BaseRPCAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void BaseRPCAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool BaseRPCAppProtocolHandler::Send(string ip, uint16_t port, Variant &variant,
		string serializer) {
	//1. Build the parameters
	Variant parameters;
	parameters["ip"] = ip;
	parameters["port"] = (uint16_t) port;
	parameters["applicationName"] = GetApplication()->GetName();
	parameters["payload"] = variant;
	parameters["serializerType"] = serializer;

	//2. Start the HTTP request
	if (!TCPConnector<BaseRPCAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetProtocolChain(parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool BaseRPCAppProtocolHandler::Send(string url, Variant &variant,
		string serializer) {
	//1. Build the parameters
	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		FATAL("Unable to get parameters scaffold");
		return false;
	}
	parameters["payload"] = variant;
	parameters["serializerType"] = serializer;

	//2. Start the HTTP request
	if (!TCPConnector<BaseRPCAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetProtocolChain(parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

Variant &BaseRPCAppProtocolHandler::GetScaffold(string &uriString) {
	//1. Search in the cache first
	if (_urlCache.HasKey(uriString)) {
		return _urlCache[uriString];
	}

	//2. Build it
	Variant result;

	//3. Split the URL into components
	URI uri;
	if (!URI::FromString(uriString, true, uri)) {
		FATAL("Invalid url: %s", STR(uriString));
		return _urlCache["dummy"];
	}

	//6. build the end result
	result["username"] = uri.userName();
	result["password"] = uri.password();
	result["host"] = uri.host();
	result["ip"] = uri.ip();
	result["port"] = uri.port();
	result["document"] = uri.fullDocumentPathWithParameters();
	result["isSsl"] = (bool)(uri.scheme() == "https");
	result["applicationName"] = GetApplication()->GetName();

	//7. Save it in the cache
	_urlCache[uriString] = result;

	//8. Done
	return _urlCache[uriString];
}

bool BaseRPCAppProtocolHandler::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant &parameters) {
	//1. Get the application
	BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
			parameters["applicationName"]);
	if (pApplication == NULL) {
		FATAL("Unable to find application %s",
				STR(parameters["applicationName"]));
		return false;
	}

	//2. get the protocol handler
	BaseAppProtocolHandler *pHandler = pApplication->GetProtocolHandler(PT_OUTBOUND_RPC);
	if (pHandler == NULL) {
		WARN("Unable to get protocol handler for variant protocol");
	}


	//3. Is the connection up
	if (pProtocol == NULL) {
		if (pHandler != NULL) {
			((BaseRPCAppProtocolHandler *) pHandler)->ConnectionFailed(parameters);
		} else {
			WARN("Connection failed:\n%s", STR(parameters.ToString()));
		}
		return false;
	}

	//1. Validate the protocol
	if (pProtocol->GetType() != PT_OUTBOUND_RPC) {
		FATAL("Invalid protocol type. Wanted: %s; Got: %s",
				STR(tagToString(PT_OUTBOUND_RPC)),
				STR(tagToString(pProtocol->GetType())));
		return false;
	}

	//3. Register the protocol to it
	pProtocol->SetApplication(pApplication);

	if (pProtocol->GetFarProtocol() == NULL) {
		FATAL("Invalid far protocol");
		return false;
	}

	//4. Do the actual request
	if (pProtocol->GetFarProtocol()->GetType() == PT_TCP) {
		return ((OutboundRPCProtocol *) pProtocol)->Send(parameters["payload"]);
	} else {
		return ((OutboundRPCProtocol *) pProtocol)->Send(parameters);
	}
}
void BaseRPCAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	WARN("Connection failed:<>"); //%s", STR(parameters.ToString()));
}

vector<uint64_t> &BaseRPCAppProtocolHandler::GetProtocolChain(
		bool isSSL) {
	if (isSSL)
		return _outboundHttpsRpc;
	else
		return _outboundHttpRpc;
}
#endif /* HAS_PROTOCOL_RPC */
