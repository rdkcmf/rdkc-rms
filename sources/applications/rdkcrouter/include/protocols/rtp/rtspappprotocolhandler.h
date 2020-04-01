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



#ifdef HAS_PROTOCOL_RTP
#ifndef _RTSPAPPPROTOCOLHANDLER_H
#define	_RTSPAPPPROTOCOLHANDLER_H

#include "protocols/rtp/basertspappprotocolhandler.h"

namespace app_rdkcrouter {

	class RTSPAppProtocolHandler
	: public BaseRTSPAppProtocolHandler {
	public:
		RTSPAppProtocolHandler(Variant &configuration);
		virtual ~RTSPAppProtocolHandler();
	protected:
		virtual bool NeedAuthentication(RTSPProtocol *pFrom,
				Variant &requestHeaders, string &requestContent);
	};
}

#endif	/* _RTSPAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_RTP */

