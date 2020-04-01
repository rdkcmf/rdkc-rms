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


#ifndef _LMCONTROLLER_H
#define	_LMCONTROLLER_H

#include "common.h"

class BaseClientApplication;

namespace app_lminterface {
	class LMInterfaceVariantAppProtocolHandler;

	class LMController {
	private:
		LMInterfaceVariantAppProtocolHandler *_pVariantHandler;
		string _licenseID;
		string _machineID;
		string _details;
		Variant _heartBeatMessage;
		bool _responseReceived;
		uint8_t _lastSentMessageType;

		uint64_t _startupTime;
		uint64_t _maxRunTime;
		uint64_t _gracePeriod;
		uint64_t _lastHeartbeatTimestamp;
		bool _lmConnected;
		bool _isTolerant;
	public:
		LMController(BaseClientApplication *pApp);
		virtual ~LMController();

		void SignalResponse(Variant &request, Variant &response);
		void SignalFail(bool tryResend);
		void SetTolerance(bool isTolerant);
		void SendHeartBeat();
		void GracefullyShutdown();
		void SetRuntime(uint64_t maxRunTime);
		void EvaluateShutdown();
	};
}

#endif	/* _LMCONTROLLER_H */
