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


#include "appselectorapplication.h"
#include "protocols/protocoltypes.h"
#include "application/clientapplicationmanager.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/outboundrtmpprotocol.h"
#include "rtmpappprotocolhandler.h"
#include "httpappprotocolhandler.h"
using namespace app_appselector;

AppSelectorApplication::AppSelectorApplication(Variant &configuration)
: BaseClientApplication(configuration) {
#ifdef HAS_PROTOCOL_RTMP
	_pRTMPHandler = NULL;
#endif /* HAS_PROTOCOL_RTMP */
#ifdef HAS_PROTOCOL_HTTP
	_pHTTPHandler = NULL;
#endif /* HAS_PROTOCOL_HTTP */
}

AppSelectorApplication::~AppSelectorApplication() {
#ifdef HAS_PROTOCOL_RTMP
	UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
	UnRegisterAppProtocolHandler(PT_OUTBOUND_RTMP);
	if (_pRTMPHandler != NULL) {
		delete _pRTMPHandler;
		_pRTMPHandler = NULL;
	}
#endif /* HAS_PROTOCOL_RTMP */
#ifdef HAS_PROTOCOL_HTTP
	UnRegisterAppProtocolHandler(PT_INBOUND_HTTP);
	UnRegisterAppProtocolHandler(PT_OUTBOUND_HTTP);
	if (_pHTTPHandler != NULL) {
		delete _pHTTPHandler;
		_pHTTPHandler = NULL;
	}
#endif /* HAS_PROTOCOL_HTTP */
}

bool AppSelectorApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}
#ifdef HAS_PROTOCOL_RTMP
	_pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
	RegisterAppProtocolHandler(PT_INBOUND_RTMPS_DISC, _pRTMPHandler);
	RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);
#endif /* HAS_PROTOCOL_RTMP */
#ifdef HAS_PROTOCOL_HTTP
	_pHTTPHandler = new HTTPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_INBOUND_HTTP_FOR_RTMP, _pHTTPHandler);
	RegisterAppProtocolHandler(PT_OUTBOUND_HTTP_FOR_RTMP, _pHTTPHandler);
#endif /* HAS_PROTOCOL_HTTP */
	return true;
}
