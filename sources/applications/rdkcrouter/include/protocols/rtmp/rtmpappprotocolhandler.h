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


#ifdef HAS_PROTOCOL_RTMP
#ifndef _RTMPAPPPROTOCOLHANDLER_H
#define	_RTMPAPPPROTOCOLHANDLER_H

#include "protocols/rtmp/basertmpappprotocolhandler.h"
namespace app_rdkcrouter {

	class RTMPAppProtocolHandler
	: public BaseRTMPAppProtocolHandler {
	private:
		bool _authenticationEnabled;
	public:
		RTMPAppProtocolHandler(Variant &configuration);
		virtual ~RTMPAppProtocolHandler();

		void SetAuthentication(bool enabled);
		virtual bool AuthenticateInbound(BaseRTMPProtocol *pFrom,
				Variant &request, Variant &authState);
		void GetRtmpOutboundStats(Variant &report);
	protected:
		virtual bool ProcessInvokeConnect(BaseRTMPProtocol *pFrom,
				Variant &request);
		virtual bool ProcessInvokePublish(BaseRTMPProtocol *pFrom,
				Variant &request);
	private:
		uint64_t GetTotalBufferSize();
	};
}
#endif	/* _RTMPAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_RTMP */

