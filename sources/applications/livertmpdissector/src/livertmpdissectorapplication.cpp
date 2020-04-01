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



#include "livertmpdissectorapplication.h"
#include "protocols/baseprotocol.h"
#include "localdefines.h"
#include "rawtcpappprotocolhandler.h"
#include "dissector/rtmpappprotocolhandler.h"
using namespace app_livertmpdissector;

LiveRTMPDissectorApplication::LiveRTMPDissectorApplication(Variant &configuration)
: BaseClientApplication(configuration) {
	_pRawTcpHandler = NULL;
	_pRTMPHandler = NULL;
}

LiveRTMPDissectorApplication::~LiveRTMPDissectorApplication() {
	UnRegisterAppProtocolHandler(PT_RAW_TCP);
	if (_pRawTcpHandler != NULL) {
		delete _pRawTcpHandler;
		_pRawTcpHandler = NULL;
	}

	UnRegisterAppProtocolHandler(PT_RTMP_DISSECTOR);
	if (_pRTMPHandler != NULL) {
		delete _pRTMPHandler;
		_pRTMPHandler = NULL;
	}
}

bool LiveRTMPDissectorApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}

	if ((!_configuration.HasKeyChain(V_STRING, false, 1, "targetIp"))
			|| (!_configuration.HasKeyChain(_V_NUMERIC, false, 1, "targetPort"))) {
		FATAL("Invalid targetIp and targetPort configuration");
		return false;
	}

	string tmpStrUri = format("rtmp://%s:%"PRIu16,
			STR(_configuration.GetValue("targetIp", false)),
			(uint16_t) _configuration.GetValue("targetPort", false));
	URI tmpUri;
	if (!URI::FromString(tmpStrUri, true, tmpUri)) {
		FATAL("Invalid targetIp and targetPort configuration");
		return false;
	}
	_targetIp = tmpUri.ip();
	_targetPort = tmpUri.port();
	INFO("Target is %s:%"PRIu16, STR(_targetIp), _targetPort);

	_pRawTcpHandler = new RawTcpAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_RAW_TCP, _pRawTcpHandler);

	_pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_RTMP_DISSECTOR, _pRTMPHandler);

	return true;
}

string LiveRTMPDissectorApplication::GetTargetIp() {
	return _targetIp;
}

uint16_t LiveRTMPDissectorApplication::GetTargetPort() {
	return _targetPort;
}
