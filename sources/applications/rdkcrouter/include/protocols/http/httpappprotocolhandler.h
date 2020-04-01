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


#if defined(HAS_PROTOCOL_HTTP) || defined(HAS_PROTOCOL_HTTP2)
#ifndef _HTTPAPPPROTOCOLHANDLER_H
#define	_HTTPAPPPROTOCOLHANDLER_H

#include "protocols/http/basehttpprotocolhandler.h"

namespace app_rdkcrouter {

	class HTTPAppProtocolHandler
	: public BaseHTTPAppProtocolHandler {
	public:
		HTTPAppProtocolHandler(Variant &configuration);
		virtual ~HTTPAppProtocolHandler();

		/*
		 * We will use this to reject the HTTP based RTMP connections from
		 * edges
		 */
		virtual void RegisterProtocol(BaseProtocol *pProtocol);

		/*
		 * This is called by the framework when a stream needs to be pulled in
		 * Basically, this will open an HTTP client
		 * */
		virtual bool PullExternalStream(URI uri, Variant streamConfig);
        bool SendPOSTRequest(string const& protocol, Variant streamConfig);
	};
}

#endif	/* _HTTPAPPPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_HTTP */


