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


#ifndef _PROCESSWATCHDOGTIMERPROTOCOL_H
#define	_PROCESSWATCHDOGTIMERPROTOCOL_H

#include "protocols/timer/basetimerprotocol.h"
namespace app_rdkcrouter {

	class ProcessWatchdogTimerProtocol
	: public BaseTimerProtocol {
	private:
		map<uint32_t, Variant> _watchByConfigId;
		map<uint32_t, pid_t> _configToPid;
		map<pid_t, uint32_t> _pidToConfig;
	public:
		ProcessWatchdogTimerProtocol();
		virtual ~ProcessWatchdogTimerProtocol();

		virtual bool TimePeriodElapsed();

		bool Watch(Variant &settings);
		void UnWatch(Variant &settings);
		void Reset();
	private:
		void RestartPid(pid_t pid);
	};
}
#endif	/* _PROCESSWATCHDOGTIMERPROTOCOL_H */
