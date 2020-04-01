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


#include "dissectorapplication.h"
#include "rtmpappprotocolhandler.h"

DissectorApplication::DissectorApplication(Variant &configuration)
: BaseClientApplication(configuration) {
	_pRTMPHandler = NULL;
}

DissectorApplication::~DissectorApplication() {
	UnRegisterAppProtocolHandler(PT_RTMP_DISSECTOR);
	if (_pRTMPHandler != NULL) {
		delete _pRTMPHandler;
		_pRTMPHandler = NULL;
	}
}

bool DissectorApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		return false;
	}
	_pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_RTMP_DISSECTOR, _pRTMPHandler);

	return true;
}

bool DissectorApplication::SetOutputFile(string outputFile) {
	return _pRTMPHandler->SetOutputFile(outputFile);
}
