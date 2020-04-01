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
#include "common.h"
#include <android/log.h>

int LogCatLogLocation::_levelsMap[] = {
	ANDROID_LOG_FATAL,
	ANDROID_LOG_ERROR,
	ANDROID_LOG_WARN,
	ANDROID_LOG_INFO,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_VERBOSE
};

LogCatLogLocation::LogCatLogLocation(Variant &configuration)
: BaseLogLocation(configuration) {

}

LogCatLogLocation::~LogCatLogLocation() {
}

void LogCatLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	if (_level < 0 || level > _level) {
		return;
	}

	__android_log_write(_levelsMap[level], "rms",
			STR(format("%s:%u %s", pFileName, lineNumber, STR(message))));
}

void LogCatLogLocation::SignalFork() {

}

#endif /* ANDROID */
