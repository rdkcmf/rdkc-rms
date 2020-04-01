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


#ifdef HAS_PROTOCOL_EXTERNAL
#ifndef _EXTERNALMODULEAPI_H
#define	_EXTERNALMODULEAPI_H

#include "common.h"
#include "protocols/external/rmsprotocol.h"

class BaseProtocol;
class BaseClientApplication;
class ExternalModuleAPI;
class ExternalProtocol;
class ExternalProtocolModule;
class ExternalAppProtocolHandler;
class InNetExternalStream;
class OutNetExternalStream;
class ExternalJobTimerProtocol;

struct BaseIC {
	ExternalModuleAPI *pAPI;
	ExternalProtocolModule *pModule;
	BaseClientApplication *pApp;
};

struct ProtocolIC
: public BaseIC {
	ExternalProtocol *pProtocol;
	ExternalAppProtocolHandler *pHandler;
};

struct InStreamIC
: public ProtocolIC {
	InNetExternalStream *pInStream;
};

struct OutStreamIC
: public ProtocolIC {
	OutNetExternalStream *pOutStream;
};

struct TimerIC
: public BaseIC {
	ExternalJobTimerProtocol *pTimer;
};

class DLLEXP ExternalModuleAPI {
private:
	static BaseClientApplication *_pApp;
public:
	ExternalModuleAPI(BaseClientApplication *pApp);
	virtual ~ExternalModuleAPI();
	void InitAPI(api_t *pAPI);
	static bool SignalProtocolCreated(BaseProtocol *pProtocol,
			Variant &parameters);
};
#endif	/* _EXTERNALMODULEAPI_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
