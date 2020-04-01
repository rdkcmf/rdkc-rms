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


#ifdef HAS_LUA
#ifndef _BASEVMLUA_H
#define	_BASEVMLUA_H

#include "utils/misc/basevm.h"
#include "utils/lua/luautils.h"

class DLLEXP BaseVMLua
: public BaseVM {
private:
	lua_State *_pGlobalState;
	void *_pOpaque;
	Variant _dummy;
public:
	BaseVMLua();
	virtual ~BaseVMLua();

	void SetOpaque(void *pOpaque);
	virtual bool Supports64bit();
	virtual bool SupportsUndefined();
	virtual bool Initialize();
	virtual bool Shutdown();
	virtual bool LoadScriptFile(string scriptFileName, string scriptName);
	virtual bool LoadScriptString(string scriptContent, string scriptName);
	virtual bool HasFunction(string functionName);
	virtual bool CallWithParams(string functionName, Variant &results, size_t argCount, ...);
	virtual bool CallWithParams(string functionName, Variant &parameters, Variant &results);
	virtual bool CallWithoutParams(string functionName, Variant &results);
	virtual bool CallWithParams(int functionRef, Variant &parameters, Variant &results);
	virtual bool CallWithoutParams(int functionRef, Variant &results);
	bool AddPackagePath(string path);
	bool RegisterAPI(string name, luaL_Reg *pAPI);
	int GetFunctionReference(string path);
private:
	bool Call(bool hasParams, Variant &parameters, Variant &results);
};


#endif	/* _BASEVMLUA_H */
#endif /* HAS_LUA */

