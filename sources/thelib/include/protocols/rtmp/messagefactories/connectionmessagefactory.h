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
#ifndef _CONNECTIONMESSAGEFACTORY_H
#define	_CONNECTIONMESSAGEFACTORY_H

#include "protocols/rtmp/messagefactories/genericmessagefactory.h"

enum ConnectExtraParameters {
	CEP_INLINE,
	CEP_OBJECT,
	CEP_AUTO
};

class DLLEXP ConnectionMessageFactory {
public:
	static Variant GetPong(uint32_t pingValue);
	static Variant GetInvokeConnect(string appName,
			string tcUrl = "",
			double audioCodecs = 615,
			double capabilities = 15,
			string flashVer = "LNX 9,0,48,00",
			bool fPad = false,
			string pageUrl = "file:///mac.html",
			string swfUrl = "file:///mac.flv",
			double videoCodecs = 124,
			double videoFunction = 1,
			double objectEncoding = 0);
	static Variant GetInvokeConnect(
			Variant &extraParams,
			ConnectExtraParameters connectExtraParameters,
			string appName,
			string tcUrl = "",
			double audioCodecs = 615,
			double capabilities = 15,
			string flashVer = "LNX 9,0,48,00",
			bool fPad = false,
			string pageUrl = "file:///mac.html",
			string swfUrl = "file:///mac.flv",
			double videoCodecs = 124,
			double videoFunction = 1,
			double objectEncoding = 0);
	static Variant GetInvokeConnect(Variant &firstParam, Variant &extraParams,
			ConnectExtraParameters connectExtraParameters);
	static Variant GetPushNotify(uint32_t streamId, Variant &metadata, bool forceRtmpDatarate);
	static Variant GetInvokeClose();

	static Variant GetInvokeConnectResult(uint32_t channelId, uint32_t streamId,
			uint32_t requestId, string level, string code, string decription,
			double objectEncoding);
	static Variant GetInvokeConnectResult(Variant request,
			string level = RM_INVOKE_PARAMS_RESULT_LEVEL_STATUS,
			string code = RM_INVOKE_PARAMS_RESULT_CODE_NETCONNECTIONCONNECTSUCCESS,
			string decription = RM_INVOKE_PARAMS_RESULT_DESCRIPTION_CONNECTIONSUCCEEDED);

	static Variant GetInvokeConnectError(uint32_t channelId, uint32_t streamId,
			uint32_t requestId, string level, string code, string decription);
	static Variant GetInvokeConnectError(Variant request,
			string decription,
			string level = RM_INVOKE_PARAMS_RESULT_LEVEL_ERROR,
			string code = RM_INVOKE_PARAMS_RESULT_CODE_NETCONNECTIONCONNECTREJECTED);
private:
	static void StoreConnectExtraParameters(Variant &connectRequest,
			Variant &extraParams, ConnectExtraParameters connectExtraParameters);
};


#endif	/* _CONNECTIONMESSAGEFACTORY_H */

#endif /* HAS_PROTOCOL_RTMP */

