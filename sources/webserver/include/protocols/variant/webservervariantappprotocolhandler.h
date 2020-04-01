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


#ifndef _EDGEVARIANTAPPPROTOCOLHANDLER_H
#define	_EDGEVARIANTAPPPROTOCOLHANDLER_H

#include "protocols/variant/basevariantappprotocolhandler.h"
#include "application/mutex.h"

namespace webserver {

	class DLLEXP WebServerVariantAppProtocolHandler
	: public BaseVariantAppProtocolHandler {
	private:
		Variant _info;
		uint32_t _protocolId;
		Mutex *_eventLogMutex;
	public:
		WebServerVariantAppProtocolHandler(Variant &configuration);
		virtual ~WebServerVariantAppProtocolHandler();

		virtual void RegisterProtocol(BaseProtocol *pProtocol);

		virtual bool ProcessMessage(BaseVariantProtocol *pProtocol,
				Variant &lastSent, Variant &lastReceived);
		void SetConnectorInfo(Variant &info);
		bool SendHello();
		bool SendEventLog(Variant &eventDetails);
		
		bool SendResultToOrigin(string const &type, Variant result);

		void RegisterInfoProtocol(uint32_t protocolId);
		void UnRegisterInfoProtocol(uint32_t protocolId);
	private:
		bool Send(Variant message);
		bool ProcessMessageGetStats(Variant &message);
	};
}


#endif	/* _EDGEVARIANTAPPPROTOCOLHANDLER_H */

