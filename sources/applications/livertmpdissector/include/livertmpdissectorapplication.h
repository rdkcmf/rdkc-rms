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


#ifndef _LIVERTMPDISSECTORAPPLICATION_H
#define _LIVERTMPDISSECTORAPPLICATION_H

#include "application/baseclientapplication.h"

class RTMPAppProtocolHandler;

namespace app_livertmpdissector {

class RawTcpAppProtocolHandler;

class LiveRTMPDissectorApplication
: public BaseClientApplication {
private:
	RawTcpAppProtocolHandler *_pRawTcpHandler;
	RTMPAppProtocolHandler *_pRTMPHandler;
	string _targetIp;
	uint16_t _targetPort;
public:
	LiveRTMPDissectorApplication(Variant &configuration);
	virtual ~LiveRTMPDissectorApplication();
	virtual bool Initialize();

	string GetTargetIp();
	uint16_t GetTargetPort();
};
}

#endif	/* _LIVERTMPDISSECTORAPPLICATION_H */

