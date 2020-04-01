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


#include "protocols/timer/clusteringtimerprotocol.h"
#include "application/baseclientapplication.h"
#include "protocols/variant/originvariantappprotocolhandler.h"
using namespace app_rdkcrouter;

ClusteringTimerProtocol::ClusteringTimerProtocol() {
}

ClusteringTimerProtocol::~ClusteringTimerProtocol() {
}

bool ClusteringTimerProtocol::TimePeriodElapsed() {
	BaseClientApplication *pApp = GetApplication();
	if (pApp == NULL) {
		WARN("No app");
		return true;
	}
	OriginVariantAppProtocolHandler *pHandler
			= (OriginVariantAppProtocolHandler *) pApp->GetProtocolHandler(PT_BIN_VAR);
	if (pHandler == NULL) {
		WARN("No handler");
		return true;
	}

	//1. refresh the connections stats
	pHandler->RefreshConnectionsCount();

	//2. unlock blocked async requests
	pHandler->FinishCallbacks();

	return true;
}
