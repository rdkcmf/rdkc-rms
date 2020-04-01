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


#ifndef _AXISLICENSEINTERFACEAPPPROTOCOLHANDLER_H
#define _AXISLICENSEINTERFACEAPPPROTOCOLHANDLER_H

#if 0
#define LICENSE_URL "http://dispatchse1.avhs.axis.com/dispatcher/dispatcher.php"
#else
#define LICENSE_URL "https://dispatchse1.avhs.axis.com/dispatcher/dispatcher.php"
#endif
#ifdef AXIS_TEST_PARAM
#define DEFAULT_PARAMS "cameraid=00408C000000&camerasubscriptionhash=f5478b4a3785f7c3610e40ae71e5abe359ac6155&cmd=keyverify"
#else
#define DEFAULT_PARAMS "cameraid=00408C000000&camerasubscriptionhash=292b369285803ad72534b4ae2cfd63893d737828&cmd=keyverify"
#endif /* AXIS_TEST_PARAM */

#include "application/baseappprotocolhandler.h"

namespace app_axislicenseinterface {

	class AxisLicenseInterfaceAppProtocolHandler
	: public BaseAppProtocolHandler {
	private:
		vector<uint64_t> _httpLm;
		vector<uint64_t> _httpsLm;
		Variant _urlCache;
		//		vector<string> _certs;
		Variant _certs;
		uint8_t _currCertIndex;
		Variant _lastLoginData;
	public:
		AxisLicenseInterfaceAppProtocolHandler(Variant &configuration);
		virtual ~AxisLicenseInterfaceAppProtocolHandler();
		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

		bool Send(Variant &variant);
		bool Send(string username, string password);
		bool Resend(bool useDiffCert = true);

		static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters);
		virtual void ConnectionFailed(Variant &parameters);

	private:
		Variant &GetScaffold(string &uriString);
		vector<uint64_t> &GetProtocolChain(bool isSSL = false);
		string MakeURLSafe(string str);
	};
}

#endif /* _AXISLICENSEINTERFACEAPPPROTOCOLHANDLER_H */
