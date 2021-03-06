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
extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}
#include "common.h"

BaseVMLua::BaseVMLua()
: BaseVM() {
	_pGlobalState = NULL;
	_pOpaque = NULL;
}

BaseVMLua::~BaseVMLua() {
	Shutdown();
}

void BaseVMLua::SetOpaque(void *pOpaque) {
	_pOpaque = pOpaque;
}

bool BaseVMLua::Supports64bit() {
	return false;
}

bool BaseVMLua::SupportsUndefined() {
	return false;
}

bool BaseVMLua::Initialize() {
	_pGlobalState = CreateLuaState(_pOpaque);
	if (_pGlobalState == NULL) {
		FATAL("Unable to initialize lua virtual machine");
		return false;
	}
	return true;
}

bool BaseVMLua::Shutdown() {
	if (_pGlobalState != NULL) {
		DestroyLuaState(_pGlobalState);
		_pGlobalState = NULL;
	}
	return true;
}

bool BaseVMLua::LoadScriptFile(string scriptFileName, string scriptName) {
	if (scriptName == "" && scriptFileName == "" ) {
		FINE("Empty script name, not loading");
		return false;
	}
	if (scriptName == "")
		scriptName = scriptFileName;
	if (!LoadLuaScriptFromFile(scriptFileName, _pGlobalState, true)) {
		WARN("Unable to load script: %s", STR(scriptFileName));
		return false;
	}
	return true;
}

bool BaseVMLua::LoadScriptString(string scriptContent, string scriptName) {
	NYIR;
}

bool BaseVMLua::HasFunction(string functionName) {
	FINEST("Searching function %s", STR(functionName));
	lua_getglobal(_pGlobalState, STR(functionName));
	bool result = lua_isfunction(_pGlobalState, -1);
	lua_pop(_pGlobalState, 1);
	return result;
}

bool BaseVMLua::CallWithParams(string functionName, Variant &results, size_t argCount, ...) {
	//1. Find out if the function is available or not
	lua_getglobal(_pGlobalState, STR(functionName));
	if (!lua_isfunction(_pGlobalState, -1)) {
		lua_pop(_pGlobalState, 1);
		FATAL("Function not available: %s", STR(functionName));
		return false;
	}

	va_list arguments;
	va_start(arguments, argCount);
	for (size_t i = 0; i < argCount; i++) {
		Variant *pArg = va_arg(arguments, Variant *);
		if (!PushVariant(_pGlobalState, *pArg, true)) {
			FATAL("Unable to push parameters");
			va_end(arguments);
			return false;
		}
	}
	va_end(arguments);

	if (lua_pcall(_pGlobalState, (int) argCount, LUA_MULTRET, 0) != 0) {
		Variant error;
		PopVariant(_pGlobalState, error);
		FATAL("Unable to call function\n%s", STR(error.ToString()));
		return false;
	}

	results.Reset();

	int32_t returnCount = lua_gettop(_pGlobalState);
	if (returnCount == 1) {
		if (!PopVariant(_pGlobalState, results)) {
			FATAL("Unable to pop variant");
			return false;
		}
	} else {
		results.IsArray(true);
		for (int32_t i = 1; i <= returnCount; i++) {
			if (!PopVariant(_pGlobalState, results[(uint32_t) (i - 1)])) {
				FATAL("Unable to pop variant");
				return false;
			}
		}
	}

	return true;
}

bool BaseVMLua::CallWithParams(string functionName, Variant &parameters, Variant &results) {
	//1. Find out if the function is available or not
	lua_getglobal(_pGlobalState, STR(functionName));
	if (!lua_isfunction(_pGlobalState, -1)) {
		lua_pop(_pGlobalState, 1);
		FATAL("Function not available: %s", STR(functionName));
		return false;
	}

	//2. Call it
	return Call(true, parameters, results);
}

bool BaseVMLua::CallWithoutParams(string functionName, Variant &results) {
	//1. Find out if the function is available or not
	lua_getglobal(_pGlobalState, STR(functionName));
	if (!lua_isfunction(_pGlobalState, -1)) {
		lua_pop(_pGlobalState, 1);
		FATAL("Function not available: %s", STR(functionName));
		return false;
	}

	//2. Call it
	return Call(false, _dummy, results);
}

bool BaseVMLua::CallWithParams(int functionRef, Variant &parameters, Variant &results) {
	//1. Find out if the function is available or not
	lua_rawgeti(_pGlobalState, LUA_REGISTRYINDEX, functionRef);
	if (lua_type(_pGlobalState, -1) != LUA_TFUNCTION) {
		FATAL("This is not a function");
		lua_pop(_pGlobalState, -1);
		return false;
	}

	//2. Call it
	return Call(true, parameters, results);
}

bool BaseVMLua::CallWithoutParams(int functionRef, Variant &results) {
	//1. Find out if the function is available or not
	lua_rawgeti(_pGlobalState, LUA_REGISTRYINDEX, functionRef);
	if (lua_type(_pGlobalState, -1) != LUA_TFUNCTION) {
		FATAL("This is not a function");
		lua_pop(_pGlobalState, -1);
		return false;
	}

	//2. Call it
	return Call(false, _dummy, results);
}

bool BaseVMLua::AddPackagePath(string path) {
	//1. get the package table and store it in the stack
	lua_getglobal(_pGlobalState, "package");
	if (lua_type(_pGlobalState, -1) != LUA_TTABLE) {
		//stack: (null)
		FATAL("package is not a table");
		lua_pop(_pGlobalState, 1);
		//stack:
		return false;
	}
	//stack: package

	//2. get the path value
	lua_getfield(_pGlobalState, -1, "path");
	//stack: package,value

	//3. Inspect the last element which is the value
	if (lua_type(_pGlobalState, -1) != LUA_TSTRING) {
		FATAL("package.path is not a string: %d", lua_type(_pGlobalState, -1));
		lua_pop(_pGlobalState, 2);
		//stack:
		return false;
	}

	//4. get the value
	string temp = lua_tostring(_pGlobalState, -1);

	//5. remove the value from stack
	lua_pop(_pGlobalState, 1);
	//stack: package

	//6. compute the new value
	temp += ";" + path;

	//7. push it on stack
	lua_pushstring(_pGlobalState, STR(temp));
	//stack: package,new_value

	//9. save it back on the table
	lua_setfield(_pGlobalState, -2, "path");
	//stack: package

	//10. Remove the table from stack
	lua_pop(_pGlobalState, 1);
	//stack:

	return true;
}

bool BaseVMLua::RegisterAPI(string name, luaL_Reg *pAPI) {
	luaL_register(_pGlobalState, STR(name), pAPI);
	lua_pop(_pGlobalState, 1);
	return true;
}

int BaseVMLua::GetFunctionReference(string path) {
	if (luaL_dostring(_pGlobalState, STR("return " + path)) != 0) {
		Variant v;
		PopStack(_pGlobalState, v);
		FATAL("Unable to get path %s\n%s", STR(path), STR(v.ToString()));
		return 0;
	}
	if (lua_type(_pGlobalState, -1) != LUA_TFUNCTION) {
		FATAL("Path %s is not a lua function", STR(path));
		lua_pop(_pGlobalState, 1);
		return 0;
	}
	int result = luaL_ref(_pGlobalState, LUA_REGISTRYINDEX);
	if (result < 0) {
		Variant v;
		PopStack(_pGlobalState, v);
		FATAL("Unable to get reference\n%s", STR(v.ToString()));
		return 0;
	}

	return result;
}

bool BaseVMLua::Call(bool hasParams, Variant &parameters, Variant &results) {
	uint32_t paramsCount = 0;
	if (hasParams) {
		if (parameters.IsArray()) {
			paramsCount = parameters.MapSize();

			FOR_MAP(parameters, string, Variant, i) {
				if (!PushVariant(_pGlobalState, MAP_VAL(i), true)) {
					FATAL("Unable to push parameters");
					return false;
				}
			}
		} else {
			paramsCount = 1;
			if (!PushVariant(_pGlobalState, parameters, true)) {
				FATAL("Unable to push parameters");
				return false;
			}
		}
	}

	if (lua_pcall(_pGlobalState, paramsCount, LUA_MULTRET, 0) != 0) {
		Variant error;
		PopVariant(_pGlobalState, error);
		FATAL("Unable to call function\n%s", STR(error.ToString()));
		return false;
	}

	results.Reset();

	int32_t returnCount = lua_gettop(_pGlobalState);
	if (returnCount == 1) {
		if (!PopVariant(_pGlobalState, results)) {
			FATAL("Unable to pop variant");
			return false;
		}
	} else {
		results.IsArray(true);
		for (int32_t i = 1; i <= returnCount; i++) {
			if (!PopVariant(_pGlobalState, results[(uint32_t) (i - 1)])) {
				FATAL("Unable to pop variant");
				return false;
			}
		}
	}

	return true;
}

#endif /* HAS_LUA */


