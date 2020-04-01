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



#ifndef _BASEVM_H
#define	_BASEVM_H

#include "platform/platform.h"
#include "utils/misc/variant.h"

class DLLEXP BaseVM {
private:
	Variant _emptyParams;
public:
	BaseVM();
	virtual ~BaseVM();

	virtual bool Supports64bit() = 0;
	virtual bool SupportsUndefined() = 0;
	virtual bool Initialize() = 0;
	virtual bool Shutdown() = 0;
	virtual bool LoadScriptFile(string scriptFileName, string scriptName) = 0;
	virtual bool LoadScriptString(string scriptContent, string scriptName) = 0;
	virtual bool HasFunction(string functionName) = 0;
	virtual bool CallWithParams(string functionName, Variant &results, size_t argCount, ...) = 0;
	virtual bool CallWithParams(string functionName, Variant &parameters, Variant &results) = 0;
	virtual bool CallWithoutParams(string functionName, Variant &results) = 0;
};

#endif	/* _BASEVM_H */


