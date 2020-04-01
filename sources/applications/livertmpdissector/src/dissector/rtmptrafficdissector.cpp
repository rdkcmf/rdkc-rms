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


#include "dissector/rtmptrafficdissector.h"
#include "application/clientapplicationmanager.h"
#include "dissector/rtmpdissectorprotocol.h"
#include "protocols/protocolmanager.h"

RTMPTrafficDissector::RTMPTrafficDissector() {
	_pClientServer = NULL;
	_pServerClient = NULL;
}

RTMPTrafficDissector::~RTMPTrafficDissector() {
	if (_pClientServer != NULL) {
		_pClientServer->EnqueueForDelete();
		_pClientServer = NULL;
	}
	if (_pServerClient != NULL) {
		_pServerClient->EnqueueForDelete();
		_pServerClient = NULL;
	}
}

bool RTMPTrafficDissector::Init(Variant &parameters, BaseClientApplication *pApp) {
	//3. Create the flows
	Variant dummy;
	dummy.IsArray(false);
	_pClientServer = new RTMPDissectorProtocol(true);
	if (!_pClientServer->Initialize(dummy)) {
		FATAL("Unable to initialize protocol");
		return false;
	}
	_pClientServer->SetApplication(pApp);

	_pServerClient = new RTMPDissectorProtocol(false);
	if (!_pServerClient->Initialize(dummy)) {
		FATAL("Unable to initialize protocol");
		return false;
	}
	_pServerClient->SetApplication(pApp);

	return true;
}

bool RTMPTrafficDissector::FeedData(IOBuffer &buffer, bool clientToServer) {
	return (clientToServer ? _pClientServer : _pServerClient)->Ingest(buffer);
}
