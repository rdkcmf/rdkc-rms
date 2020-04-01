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


#include "application/lmcontroller.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/variant/lmconsts.h"
#include "protocols/variant/lminterfacevariantappprotocolhandler.h"
#include "eventlogger/eventlogger.h"
using namespace app_lminterface;

LMController::LMController(BaseClientApplication *pApp) {
	_pVariantHandler = (LMInterfaceVariantAppProtocolHandler *)
			pApp->GetProtocolHandler(PT_JSON_VAR);

	Variant &configuration = pApp->GetConfiguration();

	if (configuration.HasKeyChain(V_STRING, false, 1, "licenseID")) {
		_licenseID = (string) configuration.GetValue("licenseID", true);
	}
	if (configuration.HasKeyChain(V_STRING, false, 1, "machineID")) {
		_machineID = (string) configuration.GetValue("machineID", true);
	}
	if (configuration.HasKeyChain(V_STRING, false, 1, "details")) {
		_details = (string) configuration.GetValue("details", true);
	}
	
	_heartBeatMessage["msgType"] = LM_STARTUP_MSG;
	_heartBeatMessage["licID"] = _licenseID;
	_heartBeatMessage["machineID"] = _machineID;
	if (_details != "")
		_heartBeatMessage["details"] = _details;
	_responseReceived = true;
	_lastSentMessageType = 0;

	_gracePeriod = HEARTBEAT_INTERVAL * 12;
	_startupTime = getutctime();
	_maxRunTime = 0;
	_lastHeartbeatTimestamp = getutctime();
	_lmConnected = false;
	_isTolerant = false;
}

LMController::~LMController() {
}

void LMController::GracefullyShutdown() {
	if ((_heartBeatMessage.HasKeyChain(V_STRING, true, 1, "transactionID"))
			&&(_heartBeatMessage["transactionID"] != "")) {
		_heartBeatMessage["msgType"] = LM_SHUTDOWN_MSG;
		SendHeartBeat();
	} else {
		WARN("Gracefully shutdown");
		EventLogger::SignalShutdown();
	}
}

void LMController::SetRuntime(uint64_t maxRunTime) {
	_maxRunTime = maxRunTime;
}

void LMController::SetTolerance(bool isTolerant) {
	_isTolerant = isTolerant;
}

void LMController::EvaluateShutdown() {
	uint64_t now = getutctime();
	if (!_lmConnected && !_isTolerant && ((now - _lastHeartbeatTimestamp) > _gracePeriod)) {
		FATAL("License manager communications lost");
		EventLogger::SignalShutdown();
	}

	if ((_maxRunTime != 0)&&((now - _startupTime) > _maxRunTime)) {
		FATAL("Runtime exceeded: current: %"PRIu64"; max: %"PRIu64,
				(now - _startupTime), _maxRunTime);
		GracefullyShutdown();
	}
}

void LMController::SendHeartBeat() {
	if (_responseReceived) {
		_responseReceived = false;
		_pVariantHandler->Send(_heartBeatMessage);
		_lastSentMessageType = (uint8_t) _heartBeatMessage["msgType"];
	}
}

void LMController::SignalResponse(Variant &request, Variant &response) {
	_responseReceived = true;
	int8_t msgType = response["msgType"];
	switch (msgType) {
		case LM_STARTUP_ACK:
		{
			if (response["status"] != "VALID") {
				FATAL("The license terms expired or invalid");
				EventLogger::SignalShutdown();
				return;
			}
			_lmConnected = true;
			_lastHeartbeatTimestamp = getutctime();
			if (response.HasKeyChain(V_MAP, false, 1, "limits")) {
				Variant limitSettings;
				limitSettings.Reset();
				limitSettings["header"] = "limits";
				limitSettings["payload"] = response.GetValue("limits", false);
				if (!ClientApplicationManager::SendMessageToApplication(
						"rdkcms", limitSettings)) {
					WARN("Failed to notify default application");
				}
			}
			uint64_t gracePeriod = 0;
			if (response.HasKeyChain(_V_NUMERIC, false, 1, "gracePeriod"))
				gracePeriod = (uint64_t) response.GetValue("gracePeriod", false) * 60;
			_gracePeriod = _gracePeriod > gracePeriod ? _gracePeriod : gracePeriod;

			if (response.HasKeyChain(_V_NUMERIC, false, 1, "runtime"))
				_maxRunTime = (uint64_t) response.GetValue("runtime", false);

			_heartBeatMessage.Reset();
			_heartBeatMessage["msgType"] = LM_HEARTBEAT_MSG;
			_heartBeatMessage["transactionID"] = response["transactionID"];
			break;
		}
		case LM_HEARTBEAT_ACK:
		{
			if (response["status"] != "VALID") {
				FATAL("The license terms expired or invalid");
				EventLogger::SignalShutdown();
				return;
			}
			_lastHeartbeatTimestamp = getutctime();
			break;
		}
		case LM_SHUTDOWN_ACK:
		{
			WARN("Gracefully shutdown");
			EventLogger::SignalShutdown();
			return;
		}
		default:
		{
			break;
		}
	}

	if (_lastSentMessageType == LM_SHUTDOWN_MSG) {
		WARN("Gracefully shutdown");
		EventLogger::SignalShutdown();
		return;
	}
}

void LMController::SignalFail(bool tryResend) {
	if (_lastSentMessageType == LM_SHUTDOWN_MSG) {
		WARN("Gracefully shutdown");
		EventLogger::SignalShutdown();
		return;
	}
	_responseReceived = true;
	if (tryResend)
		SendHeartBeat();
}
