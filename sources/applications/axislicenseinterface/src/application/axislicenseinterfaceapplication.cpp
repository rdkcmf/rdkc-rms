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

#include "application/axislicenseinterfaceapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolmanager.h"
#include "protocols/axislicenseinterface/axislicenseinterfaceprotocol.h"
#include "protocols/axislicenseinterface/axislicenseinterfaceappprotocolhandler.h"
#include "protocols/timer/axislicensetimerappprotocolhandler.h"
#include "protocols/timer/shutdowntimerprotocol.h"
#include "eventlogger/eventlogger.h"
using namespace app_axislicenseinterface;

AxisLicenseInterfaceApplication::AxisLicenseInterfaceApplication(Variant &configuration)
: BaseClientApplication(configuration) {
	_pLMIFHandler = NULL;
	_pShutdown = NULL;
}

AxisLicenseInterfaceApplication::~AxisLicenseInterfaceApplication() {
	if (_pLMIFHandler != NULL) {
		delete _pLMIFHandler;
		_pLMIFHandler = NULL;
	}
	if (_pShutdown != NULL) {
		delete _pShutdown;
		_pShutdown = NULL;
	}

}

bool AxisLicenseInterfaceApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}

	if (IsEdge())
		return true;

	_pLMIFHandler = new AxisLicenseInterfaceAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_AXIS_LICENSE_INTERFACE, _pLMIFHandler);

	_pShutdown = new AxisLicenseTimerAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_TIMER, _pLMIFHandler);

	if (_configuration.HasKeyChain(V_STRING, false, 1, "licenseUser"))
		_username = (string) _configuration["licenseUser"];

	if (_configuration.HasKeyChain(V_STRING, false, 1, "licensePassword"))
		_password = (string) _configuration["licensePassword"];

	if (_username == "") {
		FATAL("Invalid user name provided");
		return false;
	}

	if (_username == "demo") {
		BaseTimerProtocol *pTimer = new ShutdownTimerProtocol();
		pTimer->SetApplication(this);
		WARN("Running on demo");
		if (!pTimer->EnqueueForTimeEvent(4 * 60 * 60)) {
			FATAL("Unable to initialize shutdown timer");
			return false;
		}
	} else if (!_pLMIFHandler->Send(_username, _password)) {
		FATAL("Unable to connect to license manager");
		return false;
	}
	return true;
}

void AxisLicenseInterfaceApplication::SignalApplicationMessage(Variant &message) {
	if ((!message.HasKeyChain(V_STRING, true, 1, "header"))
			|| (message["header"] != "shutdown"))
		return;
	WARN("Gracefully shutdown");
	EventLogger::SignalShutdown();
}

BaseAppProtocolHandler *AxisLicenseInterfaceApplication::GetProtocolHandler(
		uint64_t protocolType) {
	return protocolType == PT_AXIS_LICENSE_INTERFACE ?
			_pLMIFHandler : BaseClientApplication::GetProtocolHandler(protocolType);
}

void AxisLicenseInterfaceApplication::ShutdownServer() {
	EventLogger::SignalShutdown();
}
