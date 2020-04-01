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

#ifdef ANDROID
#include "utils/logging/androidapploglocation.h"
#include "utils/misc/format.h"

AndroidAppLogLocation::AndroidAppLogLocation(Variant &configuration)
	: BaseLogLocation(configuration) {

}

AndroidAppLogLocation::~AndroidAppLogLocation() {

}

bool AndroidAppLogLocation::Init() {
	if (!BaseLogLocation::Init()) {
		return false;
	}
	return true;
}

void AndroidAppLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	string logLine = format("[%"PRIu64"|", (uint64_t) time(NULL));
	getlocaltime();
	if (pFileName != NULL)
		logLine += format("%s|", pFileName);
	if (lineNumber != 0)
		logLine += format("%d|", lineNumber);
	if (pFunctionName != NULL)
		logLine += format("%s|", pFunctionName);
	logLine += "]";
	logLine += message;
	logLine += "\n";
	AndroidPlatform::Log(level, logLine);
}

void AndroidAppLogLocation::SignalFork() {
}

#endif /* ANDROID */

