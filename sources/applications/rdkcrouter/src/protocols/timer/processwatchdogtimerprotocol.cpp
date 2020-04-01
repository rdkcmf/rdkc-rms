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


#include "protocols/timer/processwatchdogtimerprotocol.h"
#include "eventlogger/eventlogger.h"
#include "application/baseclientapplication.h"

using namespace app_rdkcrouter;

ProcessWatchdogTimerProtocol::ProcessWatchdogTimerProtocol()
: BaseTimerProtocol() {
}

ProcessWatchdogTimerProtocol::~ProcessWatchdogTimerProtocol() {
	Reset();
}

bool ProcessWatchdogTimerProtocol::TimePeriodElapsed() {
	vector<pid_t> pids;
	bool noMorePids;
	Process::GetFinished(pids, noMorePids);
	for (uint32_t i = 0; i < pids.size(); i++) {
		RestartPid(pids[i]);
	}
	return true;
}

void ProcessWatchdogTimerProtocol::RestartPid(pid_t pid) {
	if (!MAP_HAS1(_pidToConfig, pid)) {
		WARN("Unknown pid %"PRIu64" terminated", (uint64_t) pid);
		return;
	}
	uint32_t configId = _pidToConfig[pid];
	_pidToConfig.erase(pid);
	_configToPid.erase(configId);

	if (!MAP_HAS1(_watchByConfigId, configId))
		return;

	Variant settings = _watchByConfigId[configId];
	GetEventLogger()->LogProcessStopped(settings);
	_watchByConfigId.erase(configId);
	
	BaseClientApplication * pApp = GetApplication();
	
	if (pApp != NULL && !((bool)settings["keepAlive"])) {
		Variant message;
		message["header"] = "removeConfig";
		message["payload"]["configId"] = configId;
		message["payload"]["deleteHlsFiles"] = false;
		pApp->SignalApplicationMessage(message);
	}
	if ((bool)settings["keepAlive"]) {
		if (!Watch(settings)) {
			WARN("Unable to restart process:\n%s", STR(settings.ToString()));
		}
	}
}

bool ProcessWatchdogTimerProtocol::Watch(Variant &settings) {
	if (!settings.HasKeyChain(V_UINT32, true, 1, "configId")) {
		FATAL("No configId found");
		return false;
	}

	uint32_t configId = (uint32_t) settings["configId"];
	if (MAP_HAS1(_watchByConfigId, configId)) {
		WARN("process already watched. configId: %"PRIu32, configId);
		return true;
	}

	pid_t pid = 0;
	if (!Process::Launch(settings, pid)) {
		FATAL("Unable to launch process:\n%s", STR(settings.ToString()));
		return false;
	}

	_watchByConfigId[configId] = settings;
	_pidToConfig[pid] = configId;
	_configToPid[configId] = pid;

	GetEventLogger()->LogProcessStarted(settings);

	return true;
}

void ProcessWatchdogTimerProtocol::UnWatch(Variant &settings) {
	if (!settings.HasKeyChain(V_UINT32, true, 1, "configId")) {
		WARN("No configId found");
		return;
	}

	uint32_t configId = (uint32_t) settings["configId"];
	if (!MAP_HAS1(_watchByConfigId, configId))
		return;

	_watchByConfigId[configId]["keepAlive"] = (bool)false;

	if (!MAP_HAS1(_configToPid, configId)) {
		WARN("process not running");
		return;
	}

	pid_t pid = _configToPid[configId];
	killProcess(pid);

	TimePeriodElapsed();
}

void ProcessWatchdogTimerProtocol::Reset() {
	if (_pidToConfig.size() == 0)
		return;

	FOR_MAP(_pidToConfig, pid_t, uint32_t, i) {
		killProcess(MAP_KEY(i));
	}
	bool noMorePids = false;
	vector<pid_t> pids;
	for (uint32_t i = 0; (i < 10)&&(!noMorePids); i++) {
		Process::GetFinished(pids, noMorePids);
		sleep(1);
	}
	_watchByConfigId.clear();
	_configToPid.clear();
	_pidToConfig.clear();
}
