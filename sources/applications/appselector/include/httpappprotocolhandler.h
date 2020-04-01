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


#ifdef HAS_PROTOCOL_HTTP
#ifndef _HTTPAPPPROTOCOLHANDLER_H
#define	_HTTPAPPPROTOCOLHANDLER_H

#include "protocols/http/basehttpprotocolhandler.h"

namespace app_appselector {

	class HTTPAppProtocolHandler
	: public BaseHTTPAppProtocolHandler {
	public:
		HTTPAppProtocolHandler(Variant &configuration);
		virtual ~HTTPAppProtocolHandler();
	};
}

#endif	/* _HTTPAPPPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_HTTP */


