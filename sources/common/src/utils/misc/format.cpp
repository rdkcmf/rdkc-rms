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

string format(const char *pFormat, ...) {
	char *pBuffer = NULL;
	va_list arguments;
	va_start(arguments, pFormat);
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif /* __clang__ */
	if (vasprintf(&pBuffer, pFormat, arguments) == -1) {
		va_end(arguments);
		o_assert(false);
		return "";
	}
#ifdef __clang__
#pragma clang diagnostic pop
#endif /* __clang__ */
	va_end(arguments);
	string result;
	if (pBuffer != NULL) {
		result = pBuffer;
		free(pBuffer);
	}
	return result;
}

string vFormat(const char *pFormat, va_list args) {
	char *pBuffer = NULL;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif /* __clang__ */
	if (vasprintf(&pBuffer, pFormat, args) == -1) {
		o_assert(false);
		return "";
	}
#ifdef __clang__
#pragma clang diagnostic pop
#endif /* __clang__ */
	string result;
	if (pBuffer != NULL) {
		result = pBuffer;
		free(pBuffer);
	}
	return result;
}
