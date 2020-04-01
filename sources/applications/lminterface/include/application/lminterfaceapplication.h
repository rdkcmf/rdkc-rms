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


#ifndef _LMINTERFACEAPPLICATION_H
#define _LMINTERFACEAPPLICATION_H

#include "application/baseclientapplication.h"

namespace app_lminterface {
	class LMInterfaceVariantAppProtocolHandler;
	class LMTimerAppProtocolHandler;
	class LMController;

	class DLLEXP LMInterfaceApplication :
	public BaseClientApplication {
	private:
		LMInterfaceVariantAppProtocolHandler * _pLMIFHandler;
		LMTimerAppProtocolHandler * _pTimerHandler;
		LMController *_pController;
		uint32_t _heartbeatTimerId;
	public:
		LMInterfaceApplication(Variant &configuration);
		LMInterfaceApplication();
		virtual ~LMInterfaceApplication();
		virtual bool Initialize();
		virtual void SignalApplicationMessage(Variant &message);
	};
}
#endif /* _LMINTERFACEAPPLICATION_H */

