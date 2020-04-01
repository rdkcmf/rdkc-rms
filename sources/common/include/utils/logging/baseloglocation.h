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



#ifndef BASELOGLOCATION_H_
#define BASELOGLOCATION_H_

#include "platform/platform.h"
#include "utils/misc/variant.h"

/*!
	@class BaseLogLocation
	@brief Base class that all logging must derive from.
 */
class DLLEXP BaseLogLocation {
protected:
	int32_t _level;
	string _name;
	int32_t _specificLevel;
	bool _singleLine;
	Variant _configuration;
public:

	BaseLogLocation(Variant &configuration);
	virtual ~BaseLogLocation();
	int32_t GetLevel();
	void SetLevel(int32_t level);
	string GetName();
	void SetName(string name);
	virtual bool EvalLogLevel(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName);
	virtual bool Init();
	virtual void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, string &message) = 0;
	virtual void SignalFork() = 0;
private:
	bool EvalLogLevel(int32_t level);
};


#endif /*BASELOGLOCATION_H_*/

