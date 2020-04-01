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


#ifndef _AXISLICENSEINTERFACEAPPLICATION_H
#define _AXISLICENSEINTERFACEAPPLICATION_H

#include "application/baseclientapplication.h"

namespace app_axislicenseinterface {
	class AxisLicenseInterfaceAppProtocolHandler;
	class AxisLicenseTimerAppProtocolHandler;
	class ShutdownTimerProtocol;

	class DLLEXP AxisLicenseInterfaceApplication :
	public BaseClientApplication {
	private:
		AxisLicenseInterfaceAppProtocolHandler * _pLMIFHandler;
		AxisLicenseTimerAppProtocolHandler * _pShutdown;
		string _username;
		string _password;
	public:
		AxisLicenseInterfaceApplication(Variant &configuration);
		AxisLicenseInterfaceApplication();
		virtual ~AxisLicenseInterfaceApplication();
		virtual bool Initialize();
		virtual void SignalApplicationMessage(Variant &message);
		virtual BaseAppProtocolHandler *GetProtocolHandler(uint64_t protocolType);
		void ShutdownServer();
	};
}
#endif /* _AXISLICENSEINTERFACEAPPLICATION_H */

