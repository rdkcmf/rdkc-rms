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



#include "utils/logging/consoleloglocation.h"

ConsoleLogLocation::ConsoleLogLocation(Variant &configuration)
: BaseLogLocation(configuration) {
	_allowColors = false;
	ADD_VECTOR_END(_colors, FATAL_COLOR);
	ADD_VECTOR_END(_colors, ERROR_COLOR);
	ADD_VECTOR_END(_colors, WARNING_COLOR);
	ADD_VECTOR_END(_colors, INFO_COLOR);
	ADD_VECTOR_END(_colors, DEBUG_COLOR);
	ADD_VECTOR_END(_colors, FINE_COLOR);
	ADD_VECTOR_END(_colors, FINEST_COLOR);
}

ConsoleLogLocation::~ConsoleLogLocation() {
	SET_CONSOLE_TEXT_COLOR(NORMAL_COLOR);
}

bool ConsoleLogLocation::Init() {
	if (!BaseLogLocation::Init()) {
		return false;
	}
	if (_configuration.HasKeyChain(V_BOOL, false, 1, CONF_LOG_APPENDER_COLORED))
		_allowColors = (bool)_configuration.GetValue(
			CONF_LOG_APPENDER_COLORED, false);
	return true;
}

void ConsoleLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	if (_singleLine) {
		replace(message, "\r", "\\r");
		replace(message, "\n", "\\n");
	}
#ifdef ANDROID
	if (_allowColors) {
		printf("%s%s:%u %s%s\n",
				STR(_colors[level]),
				pFileName,
				lineNumber,
				STR(message),
				STR(_colors[6]));
	} else {
		printf("%s:%u %s\n",
				pFileName,
				lineNumber,
				STR(message));
	}
#else
	if (_allowColors) {
		SET_CONSOLE_TEXT_COLOR(_colors[level]);
		fprintf(stdout, "%s:%"PRIu32" %s", pFileName, lineNumber, STR(message));
		//fprintf(stdout, "%d %s:%"PRIu32" %s", (int) getpid(), pFileName, lineNumber, STR(message));
		SET_CONSOLE_TEXT_COLOR(_colors[6]);
		fprintf(stdout, "\n");
	} else {
		fprintf(stdout, "%s:%"PRIu32" %s\n", pFileName, lineNumber, STR(message));
	}
#endif /* ANDROID */
	fflush(stdout);
}

void ConsoleLogLocation::SignalFork() {

}
