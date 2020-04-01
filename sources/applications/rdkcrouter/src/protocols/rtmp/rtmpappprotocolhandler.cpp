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


#include "protocols/rtmp/rtmpappprotocolhandler.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"
#include "application/baseclientapplication.h"
#include "streaming/streamsmanager.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "streaming/baseinstream.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/basertmpprotocol.h"
using namespace app_rdkcrouter;

RTMPAppProtocolHandler::RTMPAppProtocolHandler(Variant &configuration)
: BaseRTMPAppProtocolHandler(configuration) {
	_authenticationEnabled = true;
}

RTMPAppProtocolHandler::~RTMPAppProtocolHandler() {
}

void RTMPAppProtocolHandler::SetAuthentication(bool enabled) {
	_authenticationEnabled = enabled;
	Variant temp = (bool)_authenticationEnabled;
	if (!temp.SerializeToXmlFile(_configuration["authPersistenceFile"])) {
		WARN("Unable to save auth persistence file");
	}
}

bool RTMPAppProtocolHandler::AuthenticateInbound(BaseRTMPProtocol *pFrom, Variant &request,
		Variant &authState) {

	if (GetApplication()->IsOrigin()) {
		TCPCarrier *pCarrier = (TCPCarrier *) pFrom->GetIOHandler();
		if ((pCarrier->GetFarEndpointAddressIp() == "127.0.0.1")
				&& (pCarrier->GetNearEndpointPort() == 1936)) {
			authState["stage"] = "authenticated";
			authState["canPublish"] = (bool)true;
			authState["canOverrideStreamName"] = (bool)true;
			return true;
		}
	}

	if (_authenticationEnabled) {
		return BaseRTMPAppProtocolHandler::AuthenticateInbound(pFrom, request,
				authState);
	} else {
		authState["stage"] = "authenticated";
		authState["canPublish"] = (bool)true;
		authState["canOverrideStreamName"] = (bool)false;
		return true;
	}
}

bool RTMPAppProtocolHandler::ProcessInvokeConnect(BaseRTMPProtocol *pFrom, Variant &request) {
	//1. Get the app name
	if ((VariantType) M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_CONNECT_APP] != V_STRING) {
		FATAL("No app specified");
		return false;
	}
	string appName = M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_CONNECT_APP];
	if (appName == "") {
		FATAL("No app specified");
		return false;
	}

	//2. Get the clean appName without the parameters
	vector<string> parts;
	split(appName, "?", parts);
	appName = parts[0];
	if (appName[appName.length() - 1] == '/')
		appName = appName.substr(0, appName.length() - 1);

	//2. Check and see if this is our app
	if (appName == GetApplication()->GetName())
		return BaseRTMPAppProtocolHandler::ProcessInvokeConnect(pFrom, request);

	//3. Ok, it wasn't. But maybe is an alias!?
	vector<string> aliases = GetApplication()->GetAliases();

	FOR_VECTOR(aliases, i) {
		if (aliases[i] == appName) {
			return BaseRTMPAppProtocolHandler::ProcessInvokeConnect(pFrom, request);
		}
	}

	//4. Nope. No alias. Abort
	FATAL("Invalid app name: %s", STR(appName));
	return false;
}

bool RTMPAppProtocolHandler::ProcessInvokePublish(BaseRTMPProtocol *pFrom,
		Variant &request) {
	//1. If this is origin, we simply do standard publish
	if (GetApplication()->IsOrigin())
		return BaseRTMPAppProtocolHandler::ProcessInvokePublish(pFrom, request);

	//2. Get the request parameters
	Variant &varStreamName = M_INVOKE_PARAM(request, 1);

	//3. Define the normalized parameters
	string requestedStreamName;

	//4. read requestedStreamName
	if (varStreamName == V_STRING) {
		requestedStreamName = (string) varStreamName;
		trim(requestedStreamName);
		string::size_type pos = requestedStreamName.find("?");
		if (pos != string::npos) {
			requestedStreamName = requestedStreamName.substr(0, pos);
		}
		trim(requestedStreamName);
		M_INVOKE_PARAM(request, 1) = requestedStreamName;
	} else {
		M_INVOKE_PARAM(request, 1) = (bool)false;
	}

	//5. Get the public stream name
	string publicStreamName = GetApplication()->GetIngestPointPublicName(requestedStreamName);
	if (publicStreamName == "") {
		WARN("No ingest point found for %s on %s", STR(requestedStreamName), STR(*pFrom));
		Variant response = StreamMessageFactory::GetInvokeOnStatusStreamPublishBadName(request, requestedStreamName);
		return pFrom->SendMessage(response);
	}

	//6. See if we have a stream with that public stream name and close it if
	//it was a pull. First, get the input streams
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(ST_IN_NET_RTMP,
			publicStreamName, false, false);
	if (streams.size() == 0)
		return BaseRTMPAppProtocolHandler::ProcessInvokePublish(pFrom, request);

	//7. get the custom parameters from the corresponding connection
	BaseProtocol *pProtocol = MAP_VAL(streams.begin())->GetProtocol();
	if ((pProtocol == NULL) || (pProtocol->GetType() != PT_OUTBOUND_RTMP))
		return BaseRTMPAppProtocolHandler::ProcessInvokePublish(pFrom, request);
	Variant &parameters = pProtocol->GetCustomParameters();

	//8. Is this a pull with localStreamName==publicStreamName? If so, close it.
	//Before that, make sure we manually unlink it, so it will be tagged as waiting subscriber
	if ((parameters.HasKeyChain(V_STRING, true, 3, "customParameters", "externalStreamConfig", "localStreamName"))
			&&(parameters["customParameters"]["externalStreamConfig"]["localStreamName"] == publicStreamName)) {
		((BaseInStream *)MAP_VAL(streams.begin()))->UnLinkAll();
		pProtocol->EnqueueForDelete();
	}


	//9. Done, just do the standard publish routine
	return BaseRTMPAppProtocolHandler::ProcessInvokePublish(pFrom, request);
}

uint64_t RTMPAppProtocolHandler::GetTotalBufferSize() {
	uint64_t retVal = 0;
	FOR_MAP(_connections, uint32_t, BaseRTMPProtocol *, i) {
		retVal += (uint64_t) GETAVAILABLEBYTESCOUNT(*MAP_VAL(i)->GetOutputBuffer());
	}
	return retVal;
}

void RTMPAppProtocolHandler::GetRtmpOutboundStats(Variant &report) {
//	uint64_t totalMaxBuffer = (uint64_t) (_connections.size() * MAX_RTMP_OUTPUT_BUFFER);
//	uint32_t rtmpMaxBuffer = GetApplication()->GetConfiguration().GetValue("maxRtmpOutBuffer", false);
//	uint64_t totalMaxBuffer = (uint64_t) _connections.size() * (uint64_t) rtmpMaxBuffer;
//	uint64_t totalBuffer = GetTotalBufferSize();

	uint64_t bufferCount = 0;
	uint32_t rtmpMaxBuffer = GetApplication()->GetConfiguration().GetValue("maxRtmpOutBuffer", false);
	uint64_t totalBufferSize = 0;
//	uint64_t totalBufferSize = 0x20000;

	FOR_MAP(_connections, uint32_t, BaseRTMPProtocol *, i) {
		BaseRTMPProtocol *pProto = MAP_VAL(i);
		if (pProto != NULL) {
			if (pProto->HasStreamsOfType(ST_OUT_NET)) {
				bufferCount++;
				if (pProto->GetOutputBuffer() != NULL)
					totalBufferSize += (uint64_t)GETAVAILABLEBYTESCOUNT(*pProto->GetOutputBuffer());
			}
		}
	}

//	INFO("[DEBUG] bufferCount=%"PRIu64, bufferCount);
//	INFO("[DEBUG] rtmpMaxBuffer=%"PRIu32, rtmpMaxBuffer);
//	INFO("[DEBUG] totalBufferSize=%"PRIu64, totalBufferSize);

	uint64_t totalMaxBuffer = bufferCount * (uint64_t)rtmpMaxBuffer;
	double averageBufferSize = totalMaxBuffer == 0 ? 0 : (double)totalBufferSize / (double)totalMaxBuffer;
	
//	INFO("[DEBUG] averageBufferSize=%.6f", averageBufferSize);

	report["rtmpOutboundBuffer"]["average"] = averageBufferSize;
	report["rtmpOutboundBuffer"]["count"] = bufferCount;
//	report["rtmpOutboundBuffer"]["count"] = (uint32_t)_connections.size();
	report["rtmpOutboundBuffer"]["maxBuffer"] = rtmpMaxBuffer;



}
