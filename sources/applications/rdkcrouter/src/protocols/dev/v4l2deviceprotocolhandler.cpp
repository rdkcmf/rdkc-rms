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

#ifdef HAS_V4L2
// dev://localhost/v4l2

#include "application/baseclientapplication.h"
#include "protocols/dev/v4l2deviceprotocolhandler.h"
#include "protocols/dev/v4l2deviceprotocol.h"
#include "protocols/dev/streaming/inv4l2devicestream.h"
#include "streaming/streamsmanager.h"

using namespace app_rdkcrouter;

V4L2DeviceProtocolHandler::V4L2DeviceProtocolHandler(Variant &configuration)
		: BaseDevProtocolHandler(configuration) {
}
V4L2DeviceProtocolHandler::~V4L2DeviceProtocolHandler() {
}

bool V4L2DeviceProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	if (lowerCase(uri.fullDocumentPath()) != "/v4l2") {
		FATAL("Not a valid URI");
		return false;
	}
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
	streamConfig["localStreamName"] = localStreamName;
    BaseClientApplication * pApp = GetApplication();
    InV4L2DeviceStream * pStream = NULL;
    if (pApp != NULL) {
//        WARN("Getting streams manager from %s", pApp->IsEdge() ? "edge" : "origin");
        StreamsManager *pSM = pApp->GetStreamsManager();
        if (pSM != NULL) {
            WARN("Creating Instance");
            pStream = InV4L2DeviceStream::GetInstance(pApp, streamConfig, pSM);
        }
    }
	if (pStream == NULL) {
		FATAL("Failed to create device stream");
		return false;
	}
	return true;
}
#endif /* HAS_V4L2 */
