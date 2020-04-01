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
#ifndef _EXTERNALAPPPROTOCOLHANDLER_H
#define	_EXTERNALAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"
#include "protocols/external/rmsprotocol.h"

class ExternalAppProtocolHandler
: public BaseAppProtocolHandler {
private:
	protocol_factory_t *_pFactory;
public:
	ExternalAppProtocolHandler(Variant &config,
			protocol_factory_t *pFactory);
	virtual ~ExternalAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	virtual bool PullExternalStream(URI uri, Variant streamConfig);
	virtual bool PushLocalStream(Variant streamConfig);
};
#endif	/* _EXTERNALAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
