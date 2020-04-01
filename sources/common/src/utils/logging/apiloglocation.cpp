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

#ifdef HAS_PROTOCOL_API
#include "utils/logging/apiloglocation.h"

#define LOG_TAG "LOG.RDK.RMS"

ApiLogLocation::ApiLogLocation(Variant &configuration) : BaseLogLocation(configuration) {
	
}

ApiLogLocation::~ApiLogLocation() {
	
}

bool ApiLogLocation::Init() {
	string debugIniPath = "/etc/debug.ini"; // set the default path for init file
	string devdebugIniPath = "/opt/debug.ini"; // set the default path for dev init file

	// Check if there's an entry on the config.lua for the actual path
	if (_configuration.HasKeyChain(V_STRING, false, 1, "debugIniPath")) {
		debugIniPath = (string) _configuration.GetValue("debugIniPath", false);
	}

	// Check if there's an entry on the config.lua for the dev override
	if (_configuration.HasKeyChain(V_STRING, false, 1, "debugIniPath")) {
		devdebugIniPath = (string) _configuration.GetValue("devdebugIniPath", false);

		if (fileExists(devdebugIniPath)) {
			debugIniPath = devdebugIniPath;
                }
	}

#ifndef SDK_DISABLED	
	// Assume error if return value is non-zero
	int32_t ret = rdk_logger_init(STR(debugIniPath));
	if (ret) {
		FATAL("API logger initialization failed with error: %" PRIi32, ret);
		return false;
	}
#endif /* SDK_DISABLED */
	if (!BaseLogLocation::Init()) {
		return false;
	}

	FINEST("API logger initialized with %s", STR(debugIniPath));
	return true;
}

void ApiLogLocation::Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, string &message) {
#ifndef SDK_DISABLED
	rdk_LogLevel logLevel = RDK_LOG_NOTICE;
	switch(level) {
		case _FATAL_:
			logLevel = RDK_LOG_FATAL;
			break;
		case _ERROR_:
			logLevel = RDK_LOG_ERROR;
			break;
		case _WARNING_:
			logLevel = RDK_LOG_WARN;
			break;
		case _INFO_:
			logLevel = RDK_LOG_INFO;
			break;
		case _DEBUG_:
			logLevel = RDK_LOG_DEBUG;
			break;
		case _FINE_:
			logLevel = RDK_LOG_TRACE1;
			break;
		case _FINEST_:
			logLevel = RDK_LOG_TRACE2;
			break;
	}
	// TODO: no equivalent for RDK_LOG_NOTICE
	RDK_LOG(logLevel, LOG_TAG,  "%s:%" PRIu32" %s\n", pFileName, lineNumber, STR(message));
#endif /* SDK_DISABLED */
}

void ApiLogLocation::SignalFork() {
	
}
#endif	/* HAS_PROTOCOL_API */
