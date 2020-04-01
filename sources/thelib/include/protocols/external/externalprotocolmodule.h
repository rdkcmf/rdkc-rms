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


#ifdef HAS_PROTOCOL_EXTERNAL
#ifndef _EXTERNALPROTOCOLMODULE_H
#define	_EXTERNALPROTOCOLMODULE_H

#include "common.h"
#include "protocols/external/rmsprotocol.h"

class BaseProtocol;
class BaseClientApplication;
class BaseAppProtocolHandler;

class ExternalModuleAPI;
class ExternalAppProtocolHandler;

class DLLEXP ExternalProtocolModule {
private:
	Variant _config;
	string _file;
	LIB_HANDLER _libHandler;
	libInit_f _init;
	libGetProtocolFactory_f _getProtocolFactory;
	libReleaseProtocolFactory_f _releaseProtocolFactory;
	libRelease_f _release;
	protocol_factory_t *_pFactory;
	bool _initCalled;
	ExternalAppProtocolHandler *_pHandler;
public:
	ExternalProtocolModule(Variant &config);
	virtual ~ExternalProtocolModule();
	void UnRegisterExternalModuleProtocolHandlers(BaseClientApplication *pApp);
	bool Init();
	bool FinishInit(ExternalModuleAPI *pAPI, BaseClientApplication *pApp);
	string GetFile();
	vector<uint64_t> GetHandledProtocols();
	map<string, vector<uint64_t> > GetHandledProtocolChains();
	BaseProtocol *SpawnProtocol(uint64_t type);
	BaseAppProtocolHandler *GetProtocolHandler(string &scheme);
private:
	bool LoadFunction(string &fileName, string &functionName, void **ppDest);
};
#endif	/* _EXTERNALPROTOCOLMODULE_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
