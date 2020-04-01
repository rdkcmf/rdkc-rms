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
#include "protocols/rtmp/messagefactories/streammessagefactory.h"
#include "protocols/rtmp/header.h"
#include "protocols/rtmp/rtmpprotocolserializer.h"

Variant StreamMessageFactory::GetInvokeCreateStreamResult(Variant &request,
		double createdStreamId) {
	return GetInvokeCreateStreamResult(VH_CI(request), VH_SI(request),
			M_INVOKE_ID(request), createdStreamId);
}

Variant StreamMessageFactory::GetInvokeCreateStreamResult(uint32_t channelId,
		uint32_t streamId, uint32_t requestId, double createdStreamId) {
	Variant result = (double) createdStreamId;
	return GenericMessageFactory::GetInvokeResult(channelId, streamId, requestId,
			Variant(), result);
}

Variant StreamMessageFactory::GetInvokeReleaseStreamResult(uint32_t channelId,
		uint32_t streamId, uint32_t requestId, double releasedStreamId) {
	Variant result;
	if (streamId != 0)
		result = streamId;
	return GenericMessageFactory::GetInvokeResult(channelId, streamId, requestId,
			Variant(), result);
}

Variant StreamMessageFactory::GetInvokeReleaseStreamErrorNotFound(Variant &request) {
	Variant secondParams;
	secondParams[RM_INVOKE_PARAMS_RESULT_LEVEL] = RM_INVOKE_PARAMS_RESULT_LEVEL_ERROR;
	secondParams[RM_INVOKE_PARAMS_RESULT_CODE] = "NetConnection.Call.Failed";
	secondParams[RM_INVOKE_PARAMS_RESULT_DESCRIPTION] = "Specified stream not found in call to releaseStream";

	return GenericMessageFactory::GetInvokeError(
			VH_CI(request),
			VH_SI(request),
			M_INVOKE_ID(request),
			Variant(),
			secondParams);
}

Variant StreamMessageFactory::GetInvokeOnFCPublish(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string code, string description) {
	Variant params;

	params[(uint32_t) 0] = Variant();
	params[(uint32_t) 1][RM_INVOKE_PARAMS_ONSTATUS_CODE] = code;
	params[(uint32_t) 1][RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;

	return GenericMessageFactory::GetInvoke(channelId, streamId,
			timeStamp, isAbsolute, requestId, "onFCPublish", params);
}

Variant StreamMessageFactory::GetInvokeOnFCSubscribe(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string code, string description) {
	Variant params;

	params[(uint32_t) 0] = Variant();
	params[(uint32_t) 1][RM_INVOKE_PARAMS_ONSTATUS_CODE] = code;
	params[(uint32_t) 1][RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;

	return GenericMessageFactory::GetInvoke(channelId, streamId,
			timeStamp, isAbsolute, requestId, "onFCSubscribe", params);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPublishBadName(Variant &request,
		string streamName) {
	return GetInvokeOnStatusStreamPublishBadName(
			VH_CI(request),
			VH_SI(request),
			M_INVOKE_ID(request),
			streamName);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPublishBadName(uint32_t channelId,
		uint32_t streamId, double requestId, string streamName) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] = RM_INVOKE_PARAMS_RESULT_LEVEL_ERROR;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] = "NetStream.Publish.BadName";
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = format("%s is not available", STR(streamName));
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = streamName;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = "";

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			0, false, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPublished(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string level, string code, string description,
		string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] = level;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] = code;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayFailed(Variant &request,
		string streamName) {
	return GetInvokeOnStatusStreamPlayFailed(
			VH_CI(request),
			VH_SI(request),
			M_INVOKE_ID(request),
			streamName);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayFailed(uint32_t channelId,
		uint32_t streamId, double requestId, string streamName) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] = RM_INVOKE_PARAMS_RESULT_LEVEL_ERROR;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] = "NetStream.Play.Failed";
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = format("Fail to play %s", STR(streamName));
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = streamName;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = "";

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			0, false, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayReset(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			RM_INVOKE_PARAMS_ONSTATUS_CODE_NETSTREAMPLAYRESET;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayStart(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			RM_INVOKE_PARAMS_ONSTATUS_CODE_NETSTREAMPLAYSTART;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayStop(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			"NetStream.Play.Stop";
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;
	result["reason"] = "";

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPlayUnpublishNotify(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			"NetStream.Play.UnpublishNotify";
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamSeekNotify(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			RM_INVOKE_PARAMS_ONSTATUS_CODE_NETSTREAMSEEKNOTIFY;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamPauseNotify(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			RM_INVOKE_PARAMS_ONSTATUS_CODE_NETSTREAMPAUSENOTIFY;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetInvokeOnStatusStreamUnpauseNotify(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double requestId, string description, string details, string clientId) {
	Variant result;

	result[RM_INVOKE_PARAMS_ONSTATUS_LEVEL] =
			RM_INVOKE_PARAMS_ONSTATUS_LEVEL_STATUS;
	result[RM_INVOKE_PARAMS_ONSTATUS_CODE] =
			RM_INVOKE_PARAMS_ONSTATUS_CODE_NETSTREAMUNPAUSENOTIFY;
	result[RM_INVOKE_PARAMS_ONSTATUS_DESCRIPTION] = description;
	result[RM_INVOKE_PARAMS_ONSTATUS_DETAILS] = details;
	result[RM_INVOKE_PARAMS_ONSTATUS_CLIENTID] = clientId;

	return GenericMessageFactory::GetInvokeOnStatus(channelId, streamId,
			timeStamp, isAbsolute, requestId, result);
}

Variant StreamMessageFactory::GetNotifyRtmpSampleAccess(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		bool unknown1, bool unknown2) {
	Variant parameters;
	parameters[(uint32_t) 0] = (bool)unknown1;
	parameters[(uint32_t) 1] = (bool)unknown2;

	return GenericMessageFactory::GetNotify(channelId, streamId, timeStamp,
			isAbsolute, "|RtmpSampleAccess", parameters);
}

Variant StreamMessageFactory::GetNotifyOnMetaData(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		Variant metadata, bool dataKeyFrame) {
	Variant parameters;
	metadata[HTTP_HEADERS_SERVER] = BRANDING_BANNER;
	if (dataKeyFrame) {
		parameters[(uint32_t) 0] = "onMetaData";
		parameters[(uint32_t) 1] = metadata;
		return GenericMessageFactory::GetNotify(channelId, streamId, timeStamp,
				isAbsolute, "@setDataFrame", parameters);
	} else {
		parameters[(uint32_t) 0] = metadata;
		return GenericMessageFactory::GetNotify(channelId, streamId, timeStamp,
				isAbsolute, "onMetaData", parameters);
	}
}

Variant StreamMessageFactory::GetNotifyOnPlayStatusPlayComplete(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		double bytes, double duration) {
	Variant parameters;
	parameters[(uint32_t) 0]["bytes"] = bytes;
	parameters[(uint32_t) 0]["duration"] = duration;
	parameters[(uint32_t) 0]["level"] = "status";
	parameters[(uint32_t) 0]["code"] = "NetStream.Play.Complete";
	return GenericMessageFactory::GetNotify(channelId, streamId, timeStamp,
			isAbsolute, "onPlayStatus", parameters);
}

Variant StreamMessageFactory::GetNotifyOnStatusDataStart(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute) {
	Variant parameters;
	parameters[(uint32_t) 0]["code"] = "NetStream.Data.Start";
	return GenericMessageFactory::GetNotify(channelId, streamId, timeStamp,
			isAbsolute, "onStatus", parameters);
}

Variant StreamMessageFactory::GetFlexStreamSend(uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute, string function,
		Variant &parameters) {
	Variant result;

	VH(result, HT_FULL, channelId, timeStamp, 0,
			RM_HEADER_MESSAGETYPE_FLEXSTREAMSEND, streamId, isAbsolute);

	result[RM_FLEXSTREAMSEND][RM_FLEXSTREAMSEND_UNKNOWNBYTE] = (uint8_t) 0;
	result[RM_FLEXSTREAMSEND][RM_FLEXSTREAMSEND_PARAMS][(uint32_t) 0] = function;

	FOR_MAP(parameters, string, Variant, i) {
		result[RM_FLEXSTREAMSEND][RM_FLEXSTREAMSEND_PARAMS][
				result[RM_FLEXSTREAMSEND][RM_FLEXSTREAMSEND_PARAMS].MapSize()] = MAP_VAL(i);
	}

	return result;
}
#endif /* HAS_PROTOCOL_RTMP */

