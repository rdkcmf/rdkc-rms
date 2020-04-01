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

#ifdef HAS_PROTOCOL_API
#ifndef _APIPROTOCOLHANDLER_H
#define	_APIPROTOCOLHANDLER_H
#include "application/baseappprotocolhandler.h"
#include "api3rdparty/apiprotocol.h"

class DLLEXP ApiProtocolHandler : public BaseAppProtocolHandler {
public:
	ApiProtocolHandler(Variant &configuration);
	virtual ~ApiProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
	virtual bool PullExternalStream(URI uri, Variant streamConfig);

	bool Initialize(Variant &config);
	bool Terminate(Variant &config);

#ifdef HAS_THREAD
	bool InitAsync(Variant &params);
    bool DoInitAsyncTask(Variant &params);
    void CompleteInitAsyncTask(Variant &params);
#endif  /* HAS_THREAD */

	void CompleteInit(Variant &params);
};

#endif	/* _APIPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_API */
