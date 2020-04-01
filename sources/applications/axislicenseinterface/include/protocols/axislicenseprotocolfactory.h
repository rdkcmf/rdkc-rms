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


#ifndef _AXISPROTOCOLFACTORY_H
#define	_AXISPROTOCOLFACTORY_H
#define CONF_PROTOCOL_HTTP_AXIS_LICENSE_INTERFACE "lmInterface"
#define CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE "SslLMInterface"
#include "protocols/baseprotocolfactory.h"

class BaseClientApplication;
class BaseAppProtocolHandler;

namespace app_axislicenseinterface {

	class AxisLicenseProtocolFactory
	: public BaseProtocolFactory {
	private:

		map<string, vector<uint64_t> > _resolvedChains;
	public:
		AxisLicenseProtocolFactory();
		virtual ~AxisLicenseProtocolFactory();

		virtual vector<uint64_t> HandledProtocols();
		virtual vector<string> HandledProtocolChains();
		virtual vector<uint64_t> ResolveProtocolChain(string name);
		virtual BaseProtocol *SpawnProtocol(uint64_t type, Variant &parameters);
	};

}

#endif	/* _AXISPROTOCOLFACTORY_H */

