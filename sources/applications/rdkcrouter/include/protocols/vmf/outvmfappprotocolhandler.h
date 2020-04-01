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
/**
 * outvmfappprotocolhandler.h - own the Video Metafile psuedo-stream stack
 * 
 * ----------------------------------------------------
 **/
#ifndef __OUTVMFAPPPROTOCOLHANDLER_H__
#define __OUTVMFAPPPROTOCOLHANDLER_H__

#include "application/baseappprotocolhandler.h"
#include "streaming/baseoutstream.h"

#define SCHEME_OUTBOUND_VMF "outvmf"

class DLLEXP OutVmfAppProtocolHandler
: public BaseAppProtocolHandler {
public:
	OutVmfAppProtocolHandler(Variant &configuration);

	bool ConnectToVmf(string streamName, Variant &settings);

	virtual void RegisterProtocol(BaseProtocol *pProtocol) {;}

	virtual void UnRegisterProtocol(BaseProtocol *pProtocol) {;}

};

#endif //__OUTVMFAPPPROTOCOLHANDLER_H__
