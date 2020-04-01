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


#ifndef _PROTOCOLFACTORY_H
#define	_PROTOCOLFACTORY_H

#include "protocols/baseprotocolfactory.h"

class BaseClientApplication;
class BaseAppProtocolHandler;
#ifdef HAS_PROTOCOL_EXTERNAL
class ExternalProtocolModule;
class ExternalModuleAPI;
#define __HPP ExternalProtocolModule *
#else /* HAS_PROTOCOL_EXTERNAL */
#define __HPP void *
#endif /* HAS_PROTOCOL_EXTERNAL */

namespace app_rdkcrouter {

	class ProtocolFactory
	: public BaseProtocolFactory {
	private:
		static ProtocolFactory *_pThis;
#ifdef HAS_PROTOCOL_EXTERNAL
		vector<ExternalProtocolModule *> _modules;
#endif /* HAS_PROTOCOL_EXTERNAL */
		map<uint64_t, __HPP> _handledProtocols;
		map<string, vector<uint64_t> > _resolvedChains;
	public:
		ProtocolFactory();
		virtual ~ProtocolFactory();

		virtual vector<uint64_t> HandledProtocols();
		virtual vector<string> HandledProtocolChains();
		virtual vector<uint64_t> ResolveProtocolChain(string name);
		virtual BaseProtocol *SpawnProtocol(uint64_t type, Variant &parameters);
#ifdef HAS_PROTOCOL_EXTERNAL
		bool LoadModule(Variant &config);
		static bool FinishInitModules(ExternalModuleAPI *pAPI,
				BaseClientApplication *pApp);
		static void UnRegisterExternalModuleProtocolHandlers(BaseClientApplication *pApp);
		static BaseAppProtocolHandler *GetProtocolHandler(string &scheme);
#endif /* HAS_PROTOCOL_EXTERNAL */
	};

}

#endif	/* _PROTOCOLFACTORY_H */

