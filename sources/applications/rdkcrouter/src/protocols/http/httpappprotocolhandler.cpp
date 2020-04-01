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
*/



#if defined(HAS_PROTOCOL_HTTP) || defined(HAS_PROTOCOL_HTTP2)
#include "protocols/http/httpappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "protocols/http/httpadaptorprotocol.h"
#include "protocols/http/httpmssliveingest.h"
using namespace app_rdkcrouter;

HTTPAppProtocolHandler::HTTPAppProtocolHandler(Variant &configuration)
: BaseHTTPAppProtocolHandler(configuration) {

}

HTTPAppProtocolHandler::~HTTPAppProtocolHandler() {
}

void HTTPAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	//do the standard stuff
	BaseHTTPAppProtocolHandler::RegisterProtocol(pProtocol);

	//done if this is origin
	if (GetApplication()->IsOrigin())
		return;

	//ok, this is an edge and it just accepted an http connection. That is bad
	if (pProtocol != NULL)
		pProtocol->EnqueueForDelete();
}

bool HTTPAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	//1. normalize the stream name
	string localStreamName = "";
	if (streamConfig["localStreamName"] == V_STRING)
		localStreamName = (string) streamConfig["localStreamName"];
	trim(localStreamName);
	if (localStreamName == "") {
		streamConfig["localStreamName"] = "stream_" + generateRandomString(8);
		WARN("No localstream name for external URI: %s. Defaulted to %s",
				STR(uri.fullUri()),
				STR(streamConfig["localStreamName"]));
	}

	//2. Prepare the custom parameters
	Variant parameters;
	parameters["customParameters"]["externalStreamConfig"] = streamConfig;
	parameters[CONF_APPLICATION_NAME] = GetApplication()->GetName();
	string scheme = uri.scheme();
	if (scheme == "http") {
		parameters[CONF_PROTOCOL] = CONF_PROTOCOL_OUTBOUND_HTTP2;
	} else {
		FATAL("scheme %s not supported by HTTP handler", STR(scheme));
		return false;
	}

	//3. start the connecting sequence
	return HTTPAdaptorProtocol::Connect(uri.ip(), uri.port(), parameters);
}

bool HTTPAppProtocolHandler::SendPOSTRequest(string const& protocol, Variant streamConfig) {
    bool result = false;
	//1. Prepare the custom parameters
	Variant parameters;
	parameters = streamConfig;
	parameters[CONF_APPLICATION_NAME] = GetApplication()->GetName();
	parameters[CONF_PROTOCOL] = protocol;
	
	//2. start the connecting sequence
    if (protocol == CONF_PROTOCOL_OUTBOUND_HTTP_MSS_INGEST)
	    result = HTTPMssLiveIngest::Connect((string)parameters["host"], (uint16_t)parameters["port"], parameters);

    return result;
}

#endif	/* HAS_PROTOCOL_HTTP */

