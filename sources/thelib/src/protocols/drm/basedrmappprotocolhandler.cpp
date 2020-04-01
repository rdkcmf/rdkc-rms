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


#ifdef HAS_PROTOCOL_DRM

#include "protocols/drm/basedrmappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolfactorymanager.h"
#include "application/baseclientapplication.h"
#include "netio/netio.h"
#include "application/clientapplicationmanager.h"
#include "protocols/drm/basedrmprotocol.h"

BaseDRMAppProtocolHandler::BaseDRMAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {
	_urlCache["dummy"] = Variant();
#ifdef HAS_PROTOCOL_HTTP
	_outboundHttpDrm = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_DRM_HTTP);
	if (_outboundHttpDrm.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s", CONF_PROTOCOL_DRM_HTTP);
	}
	_outboundHttpsDrm = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_DRM_HTTPS);
	if (_outboundHttpsDrm.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s", CONF_PROTOCOL_DRM_HTTPS);
	}
#else
	FATAL("HTTP protocol not supported");
#endif /* HAS_PROTOCOL_HTTP */
	_lastSentId = 0;
	uint16_t reserveKeys = 10;
	uint16_t reserveIds = 10;
	if (configuration.HasKeyChain(V_STRING, false, 2, "drm", "reserveKeys")) {
		reserveKeys = (uint16_t) (configuration.GetValue("drm", false).GetValue("reserveKeys", false));
	}
	if (configuration.HasKeyChain(V_STRING, false, 2, "drm", "reserveIds")) {
		reserveIds = (uint16_t) (configuration.GetValue("drm", false).GetValue("reserveIds", false));
	}
	//INFO("--- reserveKeys=%d, reserveIds=%d", reserveKeys, reserveIds);
	_pDRMQueue = new DRMKeys(reserveKeys, reserveIds);
}

BaseDRMAppProtocolHandler::~BaseDRMAppProtocolHandler() {
	if (_pDRMQueue != NULL) {
		delete _pDRMQueue;
	}
}

void BaseDRMAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
}

void BaseDRMAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}

bool BaseDRMAppProtocolHandler::Send(string ip, uint16_t port, Variant &variant,
		DRMSerializer serializer) {

	if (serializer != DRMSerializer_Verimatrix) {
		FATAL("Incorrect DRM");
		return false;
	}

	//1. Build the parameters
	Variant parameters;
	parameters["ip"] = ip;
	parameters["port"] = (uint16_t) port;
	parameters["applicationName"] = GetApplication()->GetName();
	parameters["payload"] = variant;

	//2. Start the HTTP request
	if (!TCPConnector<BaseDRMAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(false),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool BaseDRMAppProtocolHandler::Send(string url, Variant &variant,
		DRMSerializer serializer, string serverCertificate,
		string clientCertificate, string clientCertificateKey) {

	if (serializer != DRMSerializer_Verimatrix) {
		FATAL("Incorrect DRM");
		return false;
	}

	//1. Build the parameters
	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		Variant temp;
		temp["payload"] = variant;
		temp["serverCert"] = serverCertificate;
		temp[CONF_SSL_KEY] = clientCertificateKey;
		temp[CONF_SSL_CERT] = clientCertificate;
		ConnectionFailed(temp);
		FATAL("Unable to get parameters scaffold");
		return false;
	}

	parameters["payload"] = "";
	parameters["serverCert"] = serverCertificate;
	parameters[CONF_SSL_KEY] = clientCertificateKey;
	parameters[CONF_SSL_CERT] = clientCertificate;

	_lastSentId = (uint32_t) variant["id"];

	//2. Start the HTTP request
	if (!TCPConnector<BaseDRMAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool BaseDRMAppProtocolHandler::SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters) {
	//1. Get the application
	BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
			parameters["applicationName"]);
	if (pApplication == NULL) {
		FATAL("Unable to find application %s",
				STR(parameters["applicationName"]));
		return false;
	}

	//2. get the protocol handler
	BaseAppProtocolHandler *pHandler = NULL;
	if (pApplication->HasProtocolHandler(PT_DRM)) {
		pHandler = pApplication->GetProtocolHandler(PT_DRM);
	}
	if (pHandler == NULL) {
		WARN("Unable to get protocol handler for DRM protocol");
	}

	//3. Is the connection up
	if (pProtocol == NULL) {
		if (pHandler != NULL) {
			((BaseDRMAppProtocolHandler *) pHandler)->ConnectionFailed(parameters);
		} else {
			WARN("Connection failed:\n%s", STR(parameters.ToString()));
		}
		return false;
	}

	//4. Validate the protocol
	if (pProtocol->GetType() != PT_DRM) {
		FATAL("Invalid protocol type. Wanted: %s; Got: %s",
				STR(tagToString(PT_DRM)),
				STR(tagToString(pProtocol->GetType())));
		return false;
	}

	//5. Register the protocol to it
	pProtocol->SetApplication(pApplication);

	if (pProtocol->GetFarProtocol() == NULL) {
		FATAL("Invalid far protocol");
		return false;
	}

	//6. Do the actual request
	if (pProtocol->GetFarProtocol()->GetType() == PT_TCP) {
		return ((BaseDRMProtocol *) pProtocol)->Send(parameters["payload"]);
	} else {
		return ((BaseDRMProtocol *) pProtocol)->Send(parameters);
	}
}

void BaseDRMAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	WARN("Connection failed:\n%s", STR(parameters.ToString()));
}

bool BaseDRMAppProtocolHandler::ProcessMessage(BaseDRMProtocol *pProtocol,
		Variant &lastSent, Variant &lastReceived) {
	//FINEST("lastSent:\n%s", STR(lastSent.ToString()));
	//FINEST("lastReceived:\n%s", STR(lastReceived.ToString()));

	string key("");
	string location("");
	if (!lastReceived.HasKey("key") ||
			!lastReceived.HasKey("location")) {
		WARN("Unable to receive the DRM key or location");
		pProtocol->EnqueueForDelete();
		return false;
	}
	key = (string) lastReceived["key"];
	location = (string) lastReceived["location"];

	DRMKeys *pDRMQueue = GetDRMQueue();
	if (pDRMQueue == NULL) {
		WARN("Unable to get the DRM queue");
		pProtocol->EnqueueForDelete();
		return false;
	}
	if (!pDRMQueue->HasDRMEntry(_lastSentId) ||
			pDRMQueue->IsFullOfKeys(_lastSentId)) {
		INFO("The DRM ID is invalid or the key queue is full, discard response");
		return true;	//disregard error
	}

	uint32_t position = pDRMQueue->GetPos(_lastSentId);
	if (!pDRMQueue->PutKey(_lastSentId, DRMKeys::_DRMEntry(key, location, position)) ||
			!pDRMQueue->ClearSentId()) {
		WARN("Unable to put the DRM key or to clear the DRM ID");
		pProtocol->EnqueueForDelete();
		return false;
	}
	//FINEST("Put id=%d, key=%s, loc=%s, pos=%d", _lastSentId, STR(key), STR(location), position);

	pProtocol->EnqueueForDelete();
	return true;
}

Variant &BaseDRMAppProtocolHandler::GetScaffold(string &uriString) {
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

vector<uint64_t> &BaseDRMAppProtocolHandler::GetTransport(bool isSsl) {
	if (isSsl) {
		return _outboundHttpsDrm;
	} else {
		return _outboundHttpDrm;
	}
}

DRMKeys *BaseDRMAppProtocolHandler::GetDRMQueue() {
	return _pDRMQueue;
}

#endif	/* HAS_PROTOCOL_DRM */
