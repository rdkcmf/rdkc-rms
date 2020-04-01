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



#ifndef CONSOLELOGLOCATION_H_
#define CONSOLELOGLOCATION_H_

#include "utils/logging/baseloglocation.h"

class DLLEXP ConsoleLogLocation
: public BaseLogLocation {
private:
	bool _allowColors;
	vector<COLOR_TYPE> _colors;
public:
	ConsoleLogLocation(Variant &configuration);
	virtual ~ConsoleLogLocation();
	virtual bool Init();
	virtual void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, string &message);
	virtual void SignalFork();
};

#endif /*CONSOLELOGLOCATION_H_*/

