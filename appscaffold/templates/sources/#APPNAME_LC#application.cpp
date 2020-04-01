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



#include "#APPNAME_LC#application.h"
#include "rtmpappprotocolhandler.h"
#include "protocols/baseprotocol.h"
using namespace app_#APPNAME_LC#;

#APPNAME#Application::#APPNAME#Application(Variant &configuration)
: BaseClientApplication(configuration) {
#ifdef HAS_PROTOCOL_RTMP
    _pRTMPHandler=NULL;
#endif /* HAS_PROTOCOL_RTMP */
}

#APPNAME#Application::~#APPNAME#Application() {
#ifdef HAS_PROTOCOL_RTMP
    UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
    UnRegisterAppProtocolHandler(PT_OUTBOUND_RTMP);
    if(_pRTMPHandler!=NULL) {
	delete _pRTMPHandler;
	_pRTMPHandler=NULL;
    }
#endif /* HAS_PROTOCOL_RTMP */
}

bool #APPNAME#Application::Initialize() {
    //TODO: Add your app init code here
    //Things like parsing custom sections inside _configuration for example,
    //initialize the protocol handler(s)
    
    //1. Initialize the protocol handler(s)
#ifdef HAS_PROTOCOL_RTMP    
    _pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
    RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);
#endif /* HAS_PROTOCOL_RTMP */

    //2. Use your custom values inside your app config node
    //I'll just print the config for now... Watch the logs
    FINEST("%s app config node:\n%s",
            STR(GetName()), STR(_configuration.ToString()));
    return true;
}

