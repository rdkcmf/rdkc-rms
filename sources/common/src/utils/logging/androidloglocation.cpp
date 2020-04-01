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
#include <android/log.h>
#include "utils/logging/androidloglocation.h"

#define LOG_TAG "rdkcms"

AndroidLogLocation::AndroidLogLocation(Variant &configuration)
: BaseLogLocation(configuration) {
}

AndroidLogLocation::~AndroidLogLocation() {

}

bool AndroidLogLocation::Init() {
	if (!BaseLogLocation::Init()) {
		return false;
	}
	return true;
}

void AndroidLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	uint32_t logLevel = 0;
	switch(level) {
		case _FATAL_:
			logLevel = ANDROID_LOG_FATAL;
			break;
		case _ERROR_:
			logLevel = ANDROID_LOG_ERROR;
			break;
		case _WARNING_:
			logLevel = ANDROID_LOG_WARN;
			break;
		case _INFO_:
			logLevel = ANDROID_LOG_INFO;
			break;
		case _DEBUG_:
			logLevel = ANDROID_LOG_DEBUG;
			break;
		case _FINE_:
			logLevel = ANDROID_LOG_VERBOSE;
			break;
		case _FINEST_:
			logLevel = ANDROID_LOG_DEFAULT;
			break;
	}
	__android_log_print(logLevel, LOG_TAG,  "%s:%"PRIu32" %s\n", pFileName, lineNumber, STR(message));
}

void AndroidLogLocation::SignalFork() {

}
#endif /* ANDROID */
