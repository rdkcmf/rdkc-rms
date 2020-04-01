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

#include "application/lminterfaceapplication.h"
#include "protocols/timer/lmtimerappprotocolhandler.h"
#include "protocols/timer/heartbeattimerprotocol.h"
#include "protocols/timer/shutdowntimerprotocol.h"
#include "protocols/variant/lminterfacevariantappprotocolhandler.h"
#include "protocols/variant/lmconsts.h"
#include "protocols/protocolmanager.h"
#include "application/lmcontroller.h"
#include "application/clientapplicationmanager.h"
#include "eventlogger/eventlogger.h"
using namespace app_lminterface;

LMInterfaceApplication::LMInterfaceApplication(Variant &configuration)
: BaseClientApplication(configuration) {
	_pTimerHandler = NULL;
	_pLMIFHandler = NULL;
	_pController = NULL;
	_heartbeatTimerId = 0;
}

LMInterfaceApplication::~LMInterfaceApplication() {
	if (_pTimerHandler != NULL) {
		delete _pTimerHandler;
		_pTimerHandler = NULL;
	}
	if (_pLMIFHandler != NULL) {
		delete _pLMIFHandler;
		_pLMIFHandler = NULL;
	}
	if (_pController != NULL) {
		delete _pController;
		_pController = NULL;
	}
}

bool LMInterfaceApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}

	if (IsEdge())
		return true;

	_pLMIFHandler = new LMInterfaceVariantAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_JSON_VAR, _pLMIFHandler);

	_pTimerHandler = new LMTimerAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_TIMER, _pTimerHandler);

	_pController = new LMController(this);

	_pLMIFHandler->SetController(_pController);

	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, "runtime")) {
		_pController->SetRuntime((uint64_t) _configuration.GetValue("runtime", true));
	}

	if (_configuration.HasKeyChain(V_BOOL, false, 1, "tolerant")) {
		_pController->SetTolerance((bool) _configuration.GetValue("tolerant", false));
	}

	BaseTimerProtocol *pTimer = new ShutdownTimerProtocol(_pController);
	pTimer->SetApplication(this);
	if (!pTimer->EnqueueForTimeEvent(SHUTDOWN_TIMEOUT)) {
		FATAL("Unable to initialize shutdown timer");
		return false;
	}

	pTimer = new HeartbeatTimerProtocol(_pController);
	pTimer->SetApplication(this);
	if (!pTimer->EnqueueForTimeEvent(HEARTBEAT_INTERVAL)) {
		FATAL("Unable to initialize heartbeat timer");
		return false;
	}
	_heartbeatTimerId = pTimer->GetId();

	_pController->SendHeartBeat();

	return true;
}

void LMInterfaceApplication::SignalApplicationMessage(Variant &message) {

	if ((!message.HasKeyChain(V_STRING, true, 1, "header"))
			|| (message["header"] != "shutdown"))
		return;
	if (_pController != NULL) {
		Variant message;
		message["header"] = "prepareForShutdown";
		message["payload"] = Variant();
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
		_pController->GracefullyShutdown();
	} else {
		WARN("Gracefully shutdown");
		EventLogger::SignalShutdown();
	}
}
