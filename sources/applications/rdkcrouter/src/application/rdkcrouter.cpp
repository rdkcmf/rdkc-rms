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


#include "application/baseclientapplication.h"
#include "application/rdkcrouter.h"
#include "application/originapplication.h"
#include "protocols/protocolfactory.h"
#include "application/edgeapplication.h"

using namespace app_rdkcrouter;

extern "C" BaseClientApplication *GetApplication_rdkcrouter(Variant configuration) {
	bool isOrigin = true;
	if (configuration.HasKeyChain(V_BOOL, false, 1, "isOrigin"))
		isOrigin = (bool)configuration.GetValue("isOrigin", false);
	if (isOrigin)
		return new OriginApplication(configuration);
	else
		return new EdgeApplication(configuration);
}

extern "C" DLLEXP BaseProtocolFactory *GetFactory_rdkcrouter(Variant configuration) {
	ProtocolFactory *pResult = new ProtocolFactory();
#ifdef HAS_PROTOCOL_EXTERNAL
	if (!configuration.HasKeyChain(V_MAP, false, 1, "protocolModules"))
		return pResult;

	Variant protocolModules = configuration.GetValue("protocolModules", false);

	FOR_MAP(protocolModules, string, Variant, i) {
		if (!pResult->LoadModule(MAP_VAL(i))) {
			ASSERT("Unable to load module. Definition was: %s", STR(MAP_VAL(i).ToString()));
			return NULL;
		}
	}
#endif /* HAS_PROTOCOL_EXTERNAL */
	return pResult;
}
