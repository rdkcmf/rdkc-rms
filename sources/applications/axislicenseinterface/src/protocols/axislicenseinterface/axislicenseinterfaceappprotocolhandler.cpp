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


#include "common.h"
#include "protocols/axislicenseinterface/axislicenseinterfaceappprotocolhandler.h"
#include "protocols/axislicenseinterface/axislicenseinterfaceprotocol.h"
#include "protocols/axislicenseinterface/certificates.h"
#include "protocols/axislicenseprotocolfactory.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"

using namespace app_axislicenseinterface;

AxisLicenseInterfaceAppProtocolHandler::AxisLicenseInterfaceAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {
	_urlCache.IsArray(false);
	_certs.IsArray(true);
	_certs.PushToArray(unb64((string) CERT1));
	_certs.PushToArray(unb64((string) CERT2));
	_certs.PushToArray(unb64((string) CERT3));
	_certs.PushToArray(unb64((string) CERT4));
	_certs.PushToArray(unb64((string) CERT5));
	_certs.PushToArray(unb64((string) CERT6));
	_certs.PushToArray(unb64((string) CERT7));
	_certs.PushToArray(unb64((string) CERT8));
	_currCertIndex = 0;
	_httpLm = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_HTTP_AXIS_LICENSE_INTERFACE);
	_httpsLm = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE);

}

AxisLicenseInterfaceAppProtocolHandler::~AxisLicenseInterfaceAppProtocolHandler() {
}

void AxisLicenseInterfaceAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void AxisLicenseInterfaceAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool AxisLicenseInterfaceAppProtocolHandler::Send(Variant &variant) {
	NYIR;
}

vector<uint64_t> &AxisLicenseInterfaceAppProtocolHandler::GetProtocolChain(bool isSSL) {
	return (isSSL ? _httpsLm : _httpLm);
}

bool AxisLicenseInterfaceAppProtocolHandler::Send(string username, string password) {
	_lastLoginData["providername"] = username;
	_lastLoginData["providerpass"] = password;

	string url = (string) LICENSE_URL + "?providername=" + MakeURLSafe(username)
			+ "&providerpass=" + MakeURLSafe(password) + "&" + DEFAULT_PARAMS;

	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		FATAL("Unable to get parameters scaffold");
		return false;
	}
	//	parameters["serverCert"] = _certs[_currCertIndex];
	parameters["serverCert"] = _certs;

	if (!TCPConnector<AxisLicenseInterfaceAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"], GetProtocolChain(parameters["isSsl"]), parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool AxisLicenseInterfaceAppProtocolHandler::Resend(bool useDiffCert) {
	string username = "";
	string password = "";

	if (useDiffCert)
		//		_currCertIndex = (_currCertIndex + 1) % _certs.size();
		_currCertIndex = (_currCertIndex + 1) % _certs.MapSize();
	if (_lastLoginData.HasKeyChain(V_STRING, false, 1, "providername")) {
		username = (string) _lastLoginData["providername"];
	}
	if (_lastLoginData.HasKeyChain(V_STRING, false, 1, "providerpass")) {
		password = (string) _lastLoginData["providerpass"];
	}
	return Send(username, password);
}

Variant &AxisLicenseInterfaceAppProtocolHandler::GetScaffold(string &uriString) {
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

void AxisLicenseInterfaceAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	//	_currCertIndex = (_currCertIndex + 1) % _certs.size();
	FATAL("Cannot connect to Dispatcher.");
}

bool AxisLicenseInterfaceAppProtocolHandler::SignalProtocolCreated(
		BaseProtocol *pProtocol, Variant &parameters) {
	//1. Get the application
	BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
			parameters["applicationName"]);
	if (pApplication == NULL) {
		FATAL("Unable to find application %s",
				STR(parameters["applicationName"]));
		return false;
	}

	//2. get the protocol handler
	BaseAppProtocolHandler *pHandler = pApplication->GetProtocolHandler(PT_AXIS_LICENSE_INTERFACE);
	if (pHandler == NULL) {
		WARN("Unable to get protocol handler for variant protocol");
	}


	//3. Is the connection up
	if (pProtocol == NULL) {
		if (pHandler != NULL) {
			((AxisLicenseInterfaceAppProtocolHandler *) pHandler)->ConnectionFailed(parameters);
		} else {
			WARN("Connection failed:\n%s", STR(parameters.ToString()));
		}
		return false;
	}

	//4. Validate the protocol
	if (pProtocol->GetType() != PT_AXIS_LICENSE_INTERFACE) {
		FATAL("Invalid protocol type. Wanted: %s; Got: %s",
				STR(tagToString(PT_AXIS_LICENSE_INTERFACE)),
				STR(tagToString(pProtocol->GetType())));
		return false;
	}

	//5. Register the protocol to it
	pProtocol->SetApplication(pApplication);

	if (pProtocol->GetFarProtocol() == NULL) {
		FATAL("Invalid far protocol");
		return false;
	}

	return ((AxisLicenseInterfaceProtocol *) pProtocol)->Send(parameters);
}

string AxisLicenseInterfaceAppProtocolHandler::MakeURLSafe(string str) {
	string ret = str;
	replace(ret, "$", "%24");
	replace(ret, "&", "%26");
	replace(ret, "+", "%2B");
	replace(ret, ",", "%2C");
	replace(ret, "/", "%2F");
	replace(ret, ":", "%3A");
	replace(ret, ";", "%3B");
	replace(ret, "=", "%3D");
	replace(ret, "?", "%3F");
	replace(ret, "@", "%40");
	replace(ret, " ", "%20");
	replace(ret, "\"", "%22");
	replace(ret, "<", "%3C");
	replace(ret, ">", "%3E");
	replace(ret, "#", "%23");
	replace(ret, "%", "%25");
	replace(ret, "{", "%7B");
	replace(ret, "}", "%7D");
	replace(ret, "|", "%7C");
	replace(ret, "\\", "%5C");
	replace(ret, "^", "%5E");
	replace(ret, "~", "%7E");
	replace(ret, "[", "%5B");
	replace(ret, "]", "%5D");
	replace(ret, "`", "%60");
	return ret;
}
