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



#ifdef HAS_PROTOCOL_RTP

#include "protocols/rtp/rtspappprotocolhandler.h"
#include "protocols/rtp/rtspprotocol.h"
#include "protocols/rtp/sdp.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/netio.h"
#include "protocols/protocolmanager.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinnetstream.h"
#include "application/baseclientapplication.h"
#include "streaming/hls/outnethlsstream.h"
#include "protocols/passthrough/streaming/outnetpassthroughstream.h"
using namespace app_rdkcrouter;

RTSPAppProtocolHandler::RTSPAppProtocolHandler(Variant& configuration)
: BaseRTSPAppProtocolHandler(configuration) {

}

RTSPAppProtocolHandler::~RTSPAppProtocolHandler() {
}

bool RTSPAppProtocolHandler::NeedAuthentication(RTSPProtocol *pFrom,
		Variant &requestHeaders, string &requestContent) {
	string method = requestHeaders[RTSP_FIRST_LINE][RTSP_METHOD];
	if (method == RTSP_METHOD_OPTIONS)
		return false;

	Variant &params = pFrom->GetCustomParameters();
	if (method == RTSP_METHOD_DESCRIBE) {
		params["needAuthentication"] = _authenticatePlay;
		return _authenticatePlay;
	}

	if (method == RTSP_METHOD_ANNOUNCE) {
		params["needAuthentication"] = (bool)true;
		return true;
	}

	if (!params.HasKeyChain(V_BOOL, true, 1, "needAuthentication")) {
		params["needAuthentication"] = (bool)true;
	}

	return (bool)params["needAuthentication"];
}
#endif /* HAS_PROTOCOL_RTP */

