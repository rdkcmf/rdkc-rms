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


#ifdef HAS_PROTOCOL_RTMP
#include "rtmpappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "protocols/rtmp/outboundrtmpprotocol.h"
#include "protocols/rtmp/messagefactories/connectionmessagefactory.h"
#include "protocols/rtmp/messagefactories/streammessagefactory.h"
using namespace app_stresstest;

RTMPAppProtocolHandler::RTMPAppProtocolHandler(Variant &configuration)
: BaseRTMPAppProtocolHandler(configuration) {
	_activeConnections = 0;
}

RTMPAppProtocolHandler::~RTMPAppProtocolHandler() {
}

void RTMPAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	BaseRTMPAppProtocolHandler::RegisterProtocol(pProtocol);
	_activeConnections++;
	SpawnConnections();
}

void RTMPAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
	BaseRTMPAppProtocolHandler::UnRegisterProtocol(pProtocol);
	_activeConnections--;
	SpawnConnections();
}

void RTMPAppProtocolHandler::SpawnConnections() {
	string targetServer = _configuration["targetServer"];
	string targetApp = _configuration["targetApp"];
	uint32_t numberOfConnections = (uint32_t) _configuration["numberOfConnections"];
	if (_activeConnections >= numberOfConnections)
		return;
	bool randomAccessStreams = (bool)_configuration["randomAccessStreams"];
	string streamName = GetStreamName(_activeConnections, randomAccessStreams);

	string fullUri = format("rtmp://%s/%s/%s",
			STR(targetServer),
			STR(targetApp),
			STR(streamName));

	URI uri;
	if (!URI::FromString(fullUri, true, uri)) {
		FATAL("Unable to parse uri: %s", STR(fullUri));
		return;
	}

	Variant streamConfig;
	streamConfig["uri"] = uri;
	streamConfig["localStreamName"] = generateRandomString(8);

	if (!PullExternalStream(uri, streamConfig)) {
		FATAL("Unable to pull external stream %s", STR(fullUri));
		return;
	}
}

string RTMPAppProtocolHandler::GetStreamName(uint32_t index, bool randomAccessStreams) {
	if (randomAccessStreams) {
		return _configuration["streams"][rand() % _configuration["streams"].MapSize()];
	} else {
		return _configuration["streams"][index % _configuration["streams"].MapSize()];
	}
}
#endif /* HAS_PROTOCOL_RTMP */

