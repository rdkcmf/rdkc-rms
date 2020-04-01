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
#include "protocols/variant/lminterfacevariantappprotocolhandler.h"
#include "protocols/variant/lmconsts.h"
#include "protocols/variant/basevariantprotocol.h"
#include "application/lminterfaceapplication.h"
#include "application/clientapplicationmanager.h"
#include "application/lmcontroller.h"
using namespace app_lminterface;

LMInterfaceVariantAppProtocolHandler::LMInterfaceVariantAppProtocolHandler(
		Variant &configuration)
: BaseVariantAppProtocolHandler(configuration) {
	if (configuration.HasKeyChain(V_STRING, false, 1, "key")) {
		_clientCertificateKey = unb64((string) configuration.GetValue("key", true));
	}
	if (configuration.HasKeyChain(V_STRING, false, 1, "cert")) {
		_clientCertificate = unb64((string) configuration.GetValue("cert", true));
	}
	if (configuration.HasKeyChain(V_STRING, false, 1, "lmCert")) {
		_serverCertificate = unb64((string) configuration.GetValue("lmCert", true));
	}

	for (uint32_t i = 0; i < 16; i++)
		ADD_VECTOR_END(_licenseServers, format("https://rms-%02"PRIu32".lmdomain.com/lm.php", i));
}

LMInterfaceVariantAppProtocolHandler::~LMInterfaceVariantAppProtocolHandler(void) {
}

void LMInterfaceVariantAppProtocolHandler::SetController(LMController *pController) {
	_pController = pController;
}

bool LMInterfaceVariantAppProtocolHandler::Send(Variant &message) {
	string url = GetLicenseServer();
	if (url == "") {
		FATAL("Unable to get a valid URL");
		_pController->SignalFail(_licenseServers.size() != 0);
		return false;
	}
	//FINEST("OUT url: %s; message:\n%s", STR(url), STR(message.ToString()));
#ifdef HAS_THREAD
    return BaseVariantAppProtocolHandler::SendAsync(url, message, 
            VariantSerializer_JSON,	_serverCertificate, 
            _clientCertificate, _clientCertificateKey);
#else
	return BaseVariantAppProtocolHandler::Send(url, message, VariantSerializer_JSON,
			_serverCertificate, _clientCertificate, _clientCertificateKey);
#endif  /* HAS_THREAD */
}

void LMInterfaceVariantAppProtocolHandler::UnRegisterProtocol(
		BaseProtocol *pProtocol) {
	Variant &customParam = pProtocol->GetCustomParameters();
	if (!customParam.HasKeyChain(V_BOOL, true, 1, "responseReceived")) {
		MoveActiveToDeadServers();
		_pController->SignalFail(_licenseServers.size() != 0);
	}
}

void LMInterfaceVariantAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	MoveActiveToDeadServers();
	_pController->SignalFail(_licenseServers.size() != 0);
}

string LMInterfaceVariantAppProtocolHandler::GetLicenseServer() {
	if (_licenseServers.size() == 0) {
		_licenseServers = _deadLicenseServers;
		_deadLicenseServers.clear();
		return "";
	}
	if ((_activeLicenseServer == "")&&(_licenseServers.size() != 0)) {
		_activeLicenseServer = _licenseServers[rand() % _licenseServers.size()];
	}
	return _activeLicenseServer;
}

bool LMInterfaceVariantAppProtocolHandler::ProcessMessage(
		BaseVariantProtocol *pProtocol, Variant &lastSent, Variant &lastReceived) {
	pProtocol->GetCustomParameters()["responseReceived"] = (bool)true;
	pProtocol->EnqueueForDelete();
	_pController->SignalResponse(lastSent, lastReceived);
	return true;
}

void LMInterfaceVariantAppProtocolHandler::MoveActiveToDeadServers() {

	FOR_VECTOR_ITERATOR(string, _licenseServers, it) {
		if (VECTOR_VAL(it) == _activeLicenseServer) {
			_licenseServers.erase(it);
			break;
		}
	}
	ADD_VECTOR_END(_deadLicenseServers, _activeLicenseServer);
	_activeLicenseServer = "";
}

