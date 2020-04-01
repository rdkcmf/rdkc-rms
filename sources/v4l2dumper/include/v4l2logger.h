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
#include "common.h"

#define SCREEN 1
#define LOGFILE 2
#define ALL SCREEN | LOGFILE
class V4L2Logger {
private:
	static FILE *_pFile;
	static bool _initialized;
	static bool Initialize();
	static void StampTime();
	static string GetTime();
public:
	static void Log(int flag, const char *pFormatString, ...);
};

