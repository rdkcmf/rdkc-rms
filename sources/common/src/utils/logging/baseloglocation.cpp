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



#include "utils/logging/baseloglocation.h"

BaseLogLocation::BaseLogLocation(Variant &configuration) {
	_level = -1;
	_name = "";
	_configuration = configuration;
	_specificLevel = 0;
	_singleLine = false;
}

BaseLogLocation::~BaseLogLocation() {
}

int32_t BaseLogLocation::GetLevel() {
	return _level;
}

void BaseLogLocation::SetLevel(int32_t level) {
	_level = level;
}

string BaseLogLocation::GetName() {
	return _name;
}

void BaseLogLocation::SetName(string name) {
	_name = name;
}

bool BaseLogLocation::EvalLogLevel(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName) {
	return EvalLogLevel(level);
}

bool BaseLogLocation::Init() {
	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1,
			CONF_LOG_APPENDER_SPECIFIC_LEVEL))
		_specificLevel = (int32_t) _configuration.GetValue(
			CONF_LOG_APPENDER_SPECIFIC_LEVEL, false);
	if (_configuration.HasKeyChain(V_BOOL, false, 1, CONF_LOG_APPENDER_SINGLE_LINE))
		_singleLine = (bool)_configuration.GetValue(
			CONF_LOG_APPENDER_SINGLE_LINE, false);
	return true;
}

bool BaseLogLocation::EvalLogLevel(int32_t level) {
	if (_specificLevel != 0) {
		if (_specificLevel != level)
			return false;
		return true;
	} else {
		if (_level < 0 || level > _level)
			return false;
		return true;
	}
}
