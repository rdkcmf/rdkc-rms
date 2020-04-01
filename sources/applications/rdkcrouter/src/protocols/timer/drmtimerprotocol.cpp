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


#ifdef HAS_PROTOCOL_DRM

#include "protocols/timer/drmtimerprotocol.h"
#include "application/baseclientapplication.h"
#include "application/originapplication.h"
#include "application/rdkcrouter.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "protocols/drm/drmappprotocolhandler.h"
#include "mediaformats/readers/streammetadataresolver.h"
using namespace app_rdkcrouter;

DRMTimerProtocol::DRMTimerProtocol() {
}

DRMTimerProtocol::~DRMTimerProtocol() {
}

bool DRMTimerProtocol::TimePeriodElapsed() {
	if (!RequestKey()) {
		EnqueueForDelete();
	}
	return true;
}

bool DRMTimerProtocol::RequestKey() {
	BaseClientApplication *pApp = GetApplication();
	if (pApp == NULL) {
		WARN("Unable to get application");
		return false;
	}
	BaseAppProtocolHandler *pHandler = pApp->GetProtocolHandler(PT_DRM);
	if (pHandler == NULL) {
		WARN("Unable to get protocol handler");
		return false;
	}
	DRMKeys *pDRMQueue = ((DRMAppProtocolHandler *) pHandler)->GetDRMQueue();
	if (pDRMQueue == NULL) {
		WARN("Unable to get DRM queue");
		return false;
	}

	if (pDRMQueue->GetSentId() != 0) {
		return true; //no response yet after last send
	}

	Variant variant;
	uint32_t id = pDRMQueue->GetRandomId(pDRMQueue->GetDRMEntryCount());
	if (id == 0) {
		return true; //disregard error (empty queue)
	}
	if (!pDRMQueue->SetSentId(id)) {
		WARN("Unable to set DRM ID");
		return false;
	}
	if (!pDRMQueue->HasDRMEntry(id)) {
		INFO("The DRM entry for id=%d was deleted or not yet created, discard request", id);
		pDRMQueue->ClearSentId();
		return true; //disregard error (deleted entry or not yet created)
	}
	if (pDRMQueue->IsFullOfKeys(id)) {
		//INFO("The key queue for id=%d is full, discard request", id);
		pDRMQueue->ClearSentId();
		return true; //disregard error (full queue)
	}

	variant["id"] = id;
	variant["position"] = pDRMQueue->NextPos(id);
	variant["location"] = "";

	string urlPrefix = ((DRMAppProtocolHandler *) pHandler)->GetUrlPrefix();
	string url = format("%s?r=%"PRIu32"&t=VOD&p=%"PRIu32"", urlPrefix.c_str(), id, pDRMQueue->GetPos(id));
	((DRMAppProtocolHandler *) pHandler)->Send(url, variant);
	INFO("Sent URL: %s", url.c_str());

	return true;
}

#endif  /* HAS_PROTOCOL_DRM */
