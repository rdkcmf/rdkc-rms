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
#include "stresstestapplication.h"
#include "rtmpappprotocolhandler.h"
#include "protocols/baseprotocol.h"
using namespace app_stresstest;

StressTestApplication::StressTestApplication(Variant &configuration)
: BaseClientApplication(configuration) {
#ifdef HAS_PROTOCOL_RTMP
	_pRTMPHandler = NULL;
#endif /* HAS_PROTOCOL_RTMP */
}

StressTestApplication::~StressTestApplication() {
#ifdef HAS_PROTOCOL_RTMP
	UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
	UnRegisterAppProtocolHandler(PT_OUTBOUND_RTMP);
	if (_pRTMPHandler != NULL) {
		delete _pRTMPHandler;
		_pRTMPHandler = NULL;
	}
#endif /* HAS_PROTOCOL_RTMP */
}

bool StressTestApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}
	//1. Validate the configuration
	if (!NormalizeConfiguration()) {
		FATAL("Unable to normalize configuration");
		return false;
	}

	//2. Initialize the protocol handler(s)
#ifdef HAS_PROTOCOL_RTMP
	_pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
	RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);
#endif /* HAS_PROTOCOL_RTMP */

	//3. Spawn the connections
#ifdef HAS_PROTOCOL_RTMP
	if ((bool)_configuration["active"])
		_pRTMPHandler->SpawnConnections();
#endif /* HAS_PROTOCOL_RTMP */

	//3. Done
	return true;
}

bool StressTestApplication::NormalizeConfiguration() {
	//1. targetServer
	if (!_configuration.HasKey("targetServer")) {
		FATAL("Invalid configuration: targetServer node was not found");
		return false;
	}
	if (_configuration["targetServer"] != V_STRING) {
		FATAL("Invalid configuration: targetServer node is not a string value");
		return false;
	}

	//2. targetApp
	if (!_configuration.HasKey("targetApp")) {
		FATAL("Invalid configuration: targetApp node was not found");
		return false;
	}
	if (_configuration["targetApp"] != V_STRING) {
		FATAL("Invalid configuration: targetApp node is not a string value");
		return false;
	}
	if (_configuration["targetApp"] == "") {
		FATAL("targetApp `%s` is invalid", STR(_configuration["targetApp"]));
		return false;
	}

	//3. streams
	if (!_configuration.HasKey("streams")) {
		FATAL("Invalid configuration: streams node was not found");
		return false;
	}
	if (_configuration["streams"] != V_MAP) {
		FATAL("Invalid configuration: streams node is not an array");
		return false;
	}
	if (_configuration["streams"].MapSize() == 0) {
		FATAL("streams array is empty");
		return false;
	}

	Variant temp;
	for (uint32_t i = 0; i < _configuration["streams"].MapSize(); i++) {
		if (_configuration["streams"][i] != V_STRING) {
			continue;
		}
		temp.PushToArray(_configuration["streams"][i]);
	}
	_configuration["streams"] = temp;

	//4. numberOfConnections
	if (!_configuration.HasKey("numberOfConnections")) {
		FATAL("Invalid configuration: numberOfConnections node was not found");
		return false;
	}
	if (_configuration["numberOfConnections"] != _V_NUMERIC) {
		FATAL("Invalid configuration: numberOfConnections node is not a number");
		return false;
	}
	if (((double) _configuration["numberOfConnections"] < 1)
			|| ((double) _configuration["numberOfConnections"] > 1000)) {
		FATAL("numberOfConnections must be between 1 and 1000");
		return false;
	}

	//5. randomAccessStreams
	if (!_configuration.HasKey("randomAccessStreams")) {
		FATAL("Invalid configuration: randomAccessStreams node was not found");
		return false;
	}
	if (_configuration["randomAccessStreams"] != V_BOOL) {
		FATAL("Invalid configuration: randomAccessStreams node is not a bool value");
		return false;
	}

	//6. active
	if (!_configuration.HasKey("active")) {
		FATAL("Invalid configuration: active node was not found");
		return false;
	}
	if (_configuration["active"] != V_BOOL) {
		FATAL("Invalid configuration: active node is not a bool value");
		return false;
	}

	return true;
}

