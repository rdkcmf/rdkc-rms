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
#include "protocols/rtmp/messagefactories/connectionmessagefactory.h"
#include "protocols/rtmp/rtmpprotocolserializer.h"

Variant ConnectionMessageFactory::GetPong(uint32_t pingValue) {
	Variant result;

	VH(result, HT_FULL, 2, 0, 0, RM_HEADER_MESSAGETYPE_USRCTRL,
			0, true);

	M_USRCTRL_TYPE(result) = (uint16_t) RM_USRCTRL_TYPE_PING_RESPONSE;
	M_USRCTRL_TYPE_STRING(result) =
			RTMPProtocolSerializer::GetUserCtrlTypeString(RM_USRCTRL_TYPE_PING_RESPONSE);
	uint64_t ts = pingValue;
	if (ts == 0)
		ts = (uint64_t) time(NULL)*1000;
	M_USRCTRL_PONG(result) = (uint32_t) (ts & 0x00000000ffffffffLL);

	return result;
}

Variant ConnectionMessageFactory::GetInvokeConnect(string appName, string tcUrl,
		double audioCodecs, double capabilities, string flashVer, bool fPad,
		string pageUrl, string swfUrl, double videoCodecs, double videoFunction,
		double objectEncoding) {

	Variant connectRequest;

	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_APP] = appName;
	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_AUDIOCODECS] = audioCodecs;
	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_FLASHVER] = flashVer;
	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_FPAD] = (bool)fPad;

	if (pageUrl != "")
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_PAGEURL] = pageUrl;
	else
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_PAGEURL] = Variant();

	if (swfUrl != "")
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_SWFURL] = swfUrl;
	else
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_SWFURL] = Variant();

	if (tcUrl != "")
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_TCURL] = tcUrl;
	else
		connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_TCURL] = Variant();

	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_VIDEOCODECS] = videoCodecs;
	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_VIDEOFUNCTION] = videoFunction;
	connectRequest[(uint32_t) 0][RM_INVOKE_PARAMS_CONNECT_CAPABILITIES] = capabilities;
	connectRequest[(uint32_t) 0]["objectEncoding"] = objectEncoding;

	return GenericMessageFactory::GetInvoke(3, 0, 0, false, 1, RM_INVOKE_FUNCTION_CONNECT, connectRequest);
}

// Function to build the bandwidth information message sent when a pushStream occurs
Variant ConnectionMessageFactory::GetPushNotify(uint32_t streamId, Variant &metadata, bool forceRtmpDatarate) {
	Variant notifyRequest;
	metadata["framerate"] = metadata["videoframerate"]; //twitch and nginx

	if (forceRtmpDatarate) {
		if (!(metadata.HasKeyChain(V_DOUBLE, false, 1, "videodatarate")))
			metadata["videodatarate"] = 3500; //azure; beam requires lower values or no videodatarate param at all
		if (!(metadata.HasKeyChain(V_DOUBLE, false, 1, "audiodatarate")))
			metadata["audiodatarate"] = 320; //azure; beam requires lower values or no audiodatarate param at all
	}

	notifyRequest[(uint32_t) 0] = metadata;

	vector<string> handlers;
	handlers.push_back("@setDataFrame");
	handlers.push_back("onMetaData");
	return GenericMessageFactory::GetNotify(3, streamId, 0, false, handlers, notifyRequest);
//	return GenericMessageFactory::GetNotify(3, streamId, 0, false, "onMetaData", notifyRequest);
}

Variant ConnectionMessageFactory::GetInvokeConnect(
		Variant &extraParams, ConnectExtraParameters connectExtraParameters,
		string appName, string tcUrl, double audioCodecs, double capabilities,
		string flashVer, bool fPad, string pageUrl, string swfUrl, double videoCodecs,
		double videoFunction, double objectEncoding) {
	Variant connectRequest = GetInvokeConnect(appName, tcUrl, audioCodecs,
			capabilities, flashVer, fPad, pageUrl, swfUrl, videoCodecs,
			videoFunction, objectEncoding);
	StoreConnectExtraParameters(connectRequest, extraParams, connectExtraParameters);
	return connectRequest;
}

Variant ConnectionMessageFactory::GetInvokeConnect(Variant &firstParam,
		Variant &extraParams, ConnectExtraParameters connectExtraParameters) {
	Variant connectParams;
	connectParams.PushToArray(firstParam);
	Variant connectRequest = GenericMessageFactory::GetInvoke(3, 0, 0, false, 1, RM_INVOKE_FUNCTION_CONNECT, connectParams);
	StoreConnectExtraParameters(connectRequest, extraParams, connectExtraParameters);
	return connectRequest;
}

Variant ConnectionMessageFactory::GetInvokeClose() {
	Variant close;
	close[(uint32_t) 0] = Variant();
	return GenericMessageFactory::GetInvoke(3, 0, 0, false, 0, RM_INVOKE_FUNCTION_CLOSE,
			close);
}

Variant ConnectionMessageFactory::GetInvokeConnectResult(uint32_t channelId,
		uint32_t streamId, uint32_t requestId, string level, string code,
		string decription, double objectEncoding) {
	Variant firstParams;

	firstParams["fmsVer"] = "FMS/3,0,1,123";
	firstParams["capabilities"] = 31.0;

	Variant secondParams;
	secondParams[RM_INVOKE_PARAMS_RESULT_LEVEL] = level;
	secondParams[RM_INVOKE_PARAMS_RESULT_CODE] = code;
	secondParams[RM_INVOKE_PARAMS_RESULT_DESCRIPTION] = decription;
	secondParams[RM_INVOKE_PARAMS_RESULT_OBJECTENCODING] = objectEncoding;


	return GenericMessageFactory::GetInvokeResult(channelId, streamId, requestId,
			firstParams, secondParams);
}

Variant ConnectionMessageFactory::GetInvokeConnectResult(Variant request, string level,
		string code, string decription) {

	double objectEncoding = 0;
	if (M_INVOKE_PARAM(request, 0).HasKey(RM_INVOKE_PARAMS_RESULT_OBJECTENCODING))
		objectEncoding = M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_RESULT_OBJECTENCODING];
	return GetInvokeConnectResult(
			VH_CI(request),
			VH_SI(request),
			M_INVOKE_ID(request),
			level,
			code,
			decription,
			objectEncoding);
}

Variant ConnectionMessageFactory::GetInvokeConnectError(uint32_t channelId, uint32_t streamId,
		uint32_t requestId, string level, string code, string decription) {

	Variant secondParams;
	secondParams[RM_INVOKE_PARAMS_RESULT_LEVEL] = level;
	secondParams[RM_INVOKE_PARAMS_RESULT_CODE] = code;
	secondParams[RM_INVOKE_PARAMS_RESULT_DESCRIPTION] = decription;


	return GenericMessageFactory::GetInvokeError(channelId, streamId, requestId,
			Variant(), secondParams);
}

Variant ConnectionMessageFactory::GetInvokeConnectError(Variant request,
		string decription, string level, string code) {
	/*double objectEncoding = 0;
	if (M_INVOKE_PARAM(request, 0).HasKey(RM_INVOKE_PARAMS_RESULT_OBJECTENCODING))
		objectEncoding = M_INVOKE_PARAM(request, 0)[RM_INVOKE_PARAMS_RESULT_OBJECTENCODING];
	 */
	return GetInvokeConnectError(
			VH_CI(request),
			VH_SI(request),
			M_INVOKE_ID(request),
			level,
			code,
			decription);
}

void ConnectionMessageFactory::StoreConnectExtraParameters(Variant &connectRequest,
		Variant &extraParams, ConnectExtraParameters connectExtraParameters) {
	if (connectExtraParameters == CEP_AUTO) {
		if (extraParams.IsArray())
			connectExtraParameters = CEP_INLINE;
		else
			connectExtraParameters = CEP_OBJECT;
	}
	if (connectExtraParameters == CEP_INLINE) {

		FOR_MAP(extraParams, string, Variant, i) {
			M_INVOKE_PARAMS(connectRequest).PushToArray(MAP_VAL(i));
		}
	} else {
		M_INVOKE_PARAMS(connectRequest).PushToArray(extraParams);
	}
}
#endif /* HAS_PROTOCOL_RTMP */

