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


#include "protocols/variant/webservervariantappprotocolhandler.h"
#include "protocols/variant/clusteringmessages.h"
#include "protocols/protocolmanager.h"
#include "netio/netio.h"
#include "streaming/streamsmanager.h"
#include "application/baseclientapplication.h"
#include "streaming/basestream.h"
#include "streaming/streamstypes.h"
#include "protocols/variant/basevariantprotocol.h"
#include "application/filters.h"
#include "application/edgeapplication.h"
#include "application/webserverapplication.h"
using namespace app_rdkcrouter;
using namespace webserver;

#define CONFIG_RMSPORT              "rmsPort"
#define DEFAULTRMSCONNECTIONPORT    1113

WebServerVariantAppProtocolHandler::WebServerVariantAppProtocolHandler(Variant &configuration)
: BaseVariantAppProtocolHandler(configuration), _eventLogMutex(new Mutex) {
	_protocolId = 0;
}

WebServerVariantAppProtocolHandler::~WebServerVariantAppProtocolHandler() {
	if (_eventLogMutex) {
		delete _eventLogMutex;
	}
}

void WebServerVariantAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	BaseVariantAppProtocolHandler::RegisterProtocol(pProtocol);
	_protocolId = pProtocol->GetId();
}

bool WebServerVariantAppProtocolHandler::ProcessMessage(BaseVariantProtocol * /*pProtocol*/,
		Variant & /*lastSent*/, Variant &lastReceived) {
	if (!lastReceived.HasKeyChain(V_STRING, true, 1, "type")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		return false;
	}
	if (!lastReceived.HasKeyChain(V_MAP, true, 1, "payload")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		return false;
	}
	string type = lastReceived["type"];
	if (type == "getStats") {
		return ProcessMessageGetStats(lastReceived["payload"]);
	} else if (type == "getHttpStreamingSessions" ||
			   type == "addGroupNameAlias" ||
			   type == "getGroupNameByAlias" ||
			   type == "removeGroupNameAlias" ||
			   type == "flushGroupNameAliases" ||
			   type == "listGroupNameAliases" ||
			   type == "getHttpClientsConnected") {
		return static_cast<WebServerApplication *>(GetApplication())->ProcessOriginMessage(lastReceived);
	} else {
		WARN("Invalid message type: %s", STR(type));
		return false;
	}
}

void WebServerVariantAppProtocolHandler::SetConnectorInfo(Variant &info) {
	_info = info;
}

bool WebServerVariantAppProtocolHandler::SendHello() {
	AutoMutex am(_eventLogMutex);
	Variant message = ClusteringMessages::Hello(_info["ip"], _info["port"]);
	message["payload"]["isWebServer"] = true;
	return Send(message);
}

bool WebServerVariantAppProtocolHandler::SendResultToOrigin(string const &type, Variant result) {
	AutoMutex am(_eventLogMutex);
	Variant message = ClusteringMessages::SendResultToOrigin(type, result);
	return Send(message);
}

bool WebServerVariantAppProtocolHandler::SendEventLog(Variant &eventDetails) {
	WARN("Sending log event to origin");
	AutoMutex am(_eventLogMutex);
	Variant &message = ClusteringMessages::SendEventLog(eventDetails);
	return Send(message);
}

void WebServerVariantAppProtocolHandler::RegisterInfoProtocol(
	uint32_t /*protocolId*/) {
}

void WebServerVariantAppProtocolHandler::UnRegisterInfoProtocol(
	uint32_t /*protocolId*/) {
}

bool WebServerVariantAppProtocolHandler::Send(Variant message) {
	BaseVariantProtocol *pProtocol =
			(BaseVariantProtocol *) ProtocolManager::GetProtocol(_protocolId);
	if (pProtocol == NULL) {
		uint16_t rmsPort = _configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_RMSPORT) ?
            (uint16_t) _configuration.GetValue(CONFIG_RMSPORT, false) : DEFAULTRMSCONNECTIONPORT;
		return BaseVariantAppProtocolHandler::Send("127.0.0.1", rmsPort, message,
				VariantSerializer_BIN);
	} else {
		return pProtocol->Send(message);
	}
}

bool WebServerVariantAppProtocolHandler::ProcessMessageGetStats(
	Variant & /*message*/) {
	return true;
}
