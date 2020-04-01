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


#include "protocols/rtmp/rtmpmessagefactory.h"
#include "protocols/rtmp/messagefactories/genericmessagefactory.h"
#include "protocols/rtmp/header.h"
#include "protocols/rtmp/rtmpprotocolserializer.h"

RTMPMessageFactory::__Messages *RTMPMessageFactory::_pMsgs = NULL;

void RTMPMessageFactory::GetUsrCtrlStream(Variant &result, uint16_t operation,
		uint32_t streamId, double timeStamp, bool isAbsolute) {
	VH(result, HT_FULL, 2, timeStamp, 0, RM_HEADER_MESSAGETYPE_USRCTRL, 0,
			isAbsolute);

	M_USRCTRL_TYPE(result) = (uint16_t) operation;
	M_USRCTRL_TYPE_STRING(result) =
			RTMPProtocolSerializer::GetUserCtrlTypeString(operation);
	M_USRCTRL_STREAMID(result) = streamId;
}

void RTMPMessageFactory::GetInvoke(Variant &result, uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute, double requestId,
		const string &functionName) {
	VH(result, HT_FULL, channelId, timeStamp, 0, RM_HEADER_MESSAGETYPE_INVOKE,
			streamId, isAbsolute);

	M_INVOKE_ID(result) = (double) requestId;
	M_INVOKE_FUNCTION(result) = functionName;
}

void RTMPMessageFactory::GetInvokeOnStatus(Variant &result, uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &level,
		const string &code) {
	GetInvoke(result, 3, streamId, timeStamp, isAbsolute, 0,
			RM_INVOKE_FUNCTION_ONSTATUS);

	M_INVOKE_PARAMS(result)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(result)[(uint32_t) 1]["level"] = level;
	M_INVOKE_PARAMS(result)[(uint32_t) 1]["code"] = code;
}

void RTMPMessageFactory::GetNotify(Variant &result, uint32_t channelId,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &functionName) {
	VH(result, HT_FULL, channelId, timeStamp, 0, RM_HEADER_MESSAGETYPE_NOTIFY,
			streamId, isAbsolute);

	M_NOTIFY_PARAM(result, 0) = functionName;
}

void RTMPMessageFactory::GetNotifyOnStatus(Variant &result, uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &code) {
	GetNotify(result, 3, streamId, timeStamp, isAbsolute, "onStatus");

	M_NOTIFY_PARAM(result, 1)["code"] = code;
}

void RTMPMessageFactory::GetNotifyOnPlayStatus(Variant &result,
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &level, const string &code) {
	GetNotify(result, 3, streamId, timeStamp, isAbsolute, "onPlayStatus");

	M_NOTIFY_PARAM(result, 1)["level"] = level;
	M_NOTIFY_PARAM(result, 1)["code"] = code;
}

void RTMPMessageFactory::Init() {
	Cleanup();
	_pMsgs = new RTMPMessageFactory::__Messages;
}

void RTMPMessageFactory::Cleanup() {
	if (_pMsgs != NULL)
		delete _pMsgs;
	_pMsgs = NULL;
}

Variant & RTMPMessageFactory::GetUsrCtrlStreamBegin(uint32_t streamId,
		double timeStamp, bool isAbsolute) {
	GetUsrCtrlStream(_pMsgs->usrCtrl.stream.begin, RM_USRCTRL_TYPE_STREAM_BEGIN, streamId,
			timeStamp, isAbsolute);

	return _pMsgs->usrCtrl.stream.begin;
}

Variant & RTMPMessageFactory::GetUsrCtrlStreamEof(uint32_t streamId,
		double timeStamp, bool isAbsolute) {
	GetUsrCtrlStream(_pMsgs->usrCtrl.stream.eof, RM_USRCTRL_TYPE_STREAM_EOF, streamId,
			timeStamp, isAbsolute);

	return _pMsgs->usrCtrl.stream.eof;
}

Variant & RTMPMessageFactory::GetUsrCtrlStreamDry(uint32_t streamId,
		double timeStamp, bool isAbsolute) {
	GetUsrCtrlStream(_pMsgs->usrCtrl.stream.dry, RM_USRCTRL_TYPE_STREAM_DRY, streamId,
			timeStamp, isAbsolute);

	return _pMsgs->usrCtrl.stream.dry;
}

Variant & RTMPMessageFactory::GetUsrCtrlStreamIsRecorded(uint32_t streamId,
		double timeStamp, bool isAbsolute) {
	GetUsrCtrlStream(_pMsgs->usrCtrl.stream.isRecorded,
			RM_USRCTRL_TYPE_STREAM_IS_RECORDED, streamId, timeStamp, isAbsolute);

	return _pMsgs->usrCtrl.stream.isRecorded;
}

Variant & RTMPMessageFactory::GetInvokeCreateStream() {
	GetInvoke(_pMsgs->invoke.createStream, 3, 0, 0, false, 0, "createStream");
	M_INVOKE_PARAMS(_pMsgs->invoke.createStream)[(uint32_t) 0] = Variant();
	return _pMsgs->invoke.createStream;
}

Variant & RTMPMessageFactory::GetInvokeReleaseStream(const string &streamName) {
	GetInvoke(_pMsgs->invoke.releaseStream, 3, 0, 0, false, 0, "releaseStream");
	M_INVOKE_PARAMS(_pMsgs->invoke.releaseStream)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(_pMsgs->invoke.releaseStream)[(uint32_t) 1] = streamName;
	return _pMsgs->invoke.releaseStream;
}

Variant & RTMPMessageFactory::GetInvokePublish(uint32_t streamId,
		const string &streamName, const string &mode) {
	GetInvoke(_pMsgs->invoke.publish, 3, streamId, 0, false, 0, "publish");
	M_INVOKE_PARAMS(_pMsgs->invoke.publish)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(_pMsgs->invoke.publish)[(uint32_t) 1] = streamName;
	M_INVOKE_PARAMS(_pMsgs->invoke.publish)[(uint32_t) 2] = mode;
	return _pMsgs->invoke.publish;
}

Variant & RTMPMessageFactory::GetInvokePlay(uint32_t streamId, const string &streamName,
		double start, double length, bool reset) {
	GetInvoke(_pMsgs->invoke.play, 3, streamId, 0, false, 0, "play");
	M_INVOKE_PARAMS(_pMsgs->invoke.play)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(_pMsgs->invoke.play)[(uint32_t) 1] = streamName;
	M_INVOKE_PARAMS(_pMsgs->invoke.play)[(uint32_t) 2] = start;
	M_INVOKE_PARAMS(_pMsgs->invoke.play)[(uint32_t) 3] = length;
	M_INVOKE_PARAMS(_pMsgs->invoke.play)[(uint32_t) 4] = (bool)reset;
	return _pMsgs->invoke.play;
}

Variant & RTMPMessageFactory::GetInvokeFCSubscribe(const string &streamName) {
	GetInvoke(_pMsgs->invoke.fcSubscribe, 3, 0, 0, false, 0, "FCSubscribe");
	M_INVOKE_PARAMS(_pMsgs->invoke.fcSubscribe)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(_pMsgs->invoke.fcSubscribe)[(uint32_t) 1] = streamName;
	return _pMsgs->invoke.fcSubscribe;
}

Variant & RTMPMessageFactory::GetInvokeFCPublish(const string &streamName) {
	GetInvoke(_pMsgs->invoke.fcPublish, 3, 0, 0, false, 0, "FCPublish");
	M_INVOKE_PARAMS(_pMsgs->invoke.fcPublish)[(uint32_t) 0] = Variant();
	M_INVOKE_PARAMS(_pMsgs->invoke.fcPublish)[(uint32_t) 1] = streamName;
	return _pMsgs->invoke.fcPublish;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStreamNotFound(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &clientId, const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.streamNotFound,
			streamId, timeStamp, isAbsolute, "error",
			"NetStream.Play.StreamNotFound");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.streamNotFound, 1)["clientid"] =
			clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.streamNotFound, 1)["details"] =
			streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.streamNotFound, 1)["description"] =
			format("Failed to play %s; stream not found.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.play.streamNotFound;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayReset(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &clientId, const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.reset, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Play.Reset");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.reset, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.reset, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.reset, 1)["description"] =
			format("Playing and resetting %s.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.play.reset;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStart(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &clientId, const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.start, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Play.Start");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.start, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.start, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.start, 1)["description"] =
			format("Started playing %s.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.play.start;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &clientId, const string &streamName,
		const string &reason) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.stop, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Play.Stop");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.stop, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.stop, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.stop, 1)["description"] =
			format("Stopped playing %s.", STR(streamName));
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.stop, 1)["reason"] = reason;

	return _pMsgs->invoke.onStatus.netStream.play.stop;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayPublishNotify(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		const string &clientId, const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.publishNotify, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Play.PublishNotify");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.publishNotify, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.publishNotify, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.publishNotify, 1)["description"] =
			format("%s is now published.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.play.publishNotify;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayTransition(
		uint32_t streamId, double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName, bool forced) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.play.transition, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Play.Transition");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.transition, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.transition, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.transition, 1)["description"] =
			format("Transitioned to %s.", STR(streamName));
	if (forced)
		M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.transition, 1)["reason"] = "NetStream.Transition.Forced";
	else
		M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.play.transition, 1)["reason"] = "NetStream.Transition.Success";

	return _pMsgs->invoke.onStatus.netStream.play.transition;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamSeekFailed(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		double seekPoint) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.seek.failed, streamId, timeStamp,
			isAbsolute, "error", "NetStream.Seek.Failed");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.seek.failed, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.seek.failed, 1)["description"] =
			format("Failed to seek %.0f (stream ID: %"PRIu32").", seekPoint, streamId);

	return _pMsgs->invoke.onStatus.netStream.seek.failed;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamSeekNotify(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName, double seekPoint) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.seek.notify, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Seek.Notify");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.seek.notify, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.seek.notify, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.seek.notify, 1)["description"] =
			format("Seeking %.0f (stream ID: %"PRIu32").", seekPoint, streamId);

	return _pMsgs->invoke.onStatus.netStream.seek.notify;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPublishStart(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.publishStart, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Publish.Start");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.publishStart, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.publishStart, 1)["description"] =
			format("%s is now published.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.publishStart;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamUnpublishSuccess(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.unpublishSuccess, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Unpublish.Success");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.unpublishSuccess, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.unpublishSuccess, 1)["description"] =
			format("%s is now unpublished.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.unpublishSuccess;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamPauseNotify(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.pauseNotify, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Pause.Notify");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.pauseNotify, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.pauseNotify, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.pauseNotify, 1)["description"] =
			format("Pausing %s.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.pauseNotify;
}

Variant & RTMPMessageFactory::GetInvokeOnStatusNetStreamUnpauseNotify(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName) {
	GetInvokeOnStatus(_pMsgs->invoke.onStatus.netStream.unpauseNotify, streamId, timeStamp,
			isAbsolute, "status", "NetStream.Unpause.Notify");

	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.unpauseNotify, 1)["clientid"] = clientId;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.unpauseNotify, 1)["details"] = streamName;
	M_INVOKE_PARAM(_pMsgs->invoke.onStatus.netStream.unpauseNotify, 1)["description"] =
			format("Unpausing %s.", STR(streamName));

	return _pMsgs->invoke.onStatus.netStream.unpauseNotify;
}

Variant & RTMPMessageFactory::GetInvokeOnFCUnpublishNetStreamUnpublishSuccess(uint32_t streamId,
		double timeStamp, bool isAbsolute, const string &clientId,
		const string &streamName) {
	GetInvoke(_pMsgs->invoke.onFCUnpublishNetStreamUnpublishSuccess, 3, streamId, timeStamp,
			isAbsolute, 0, "onFCUnpublish");

	M_INVOKE_PARAM(_pMsgs->invoke.onFCUnpublishNetStreamUnpublishSuccess, 0) = Variant();
	M_INVOKE_PARAM(_pMsgs->invoke.onFCUnpublishNetStreamUnpublishSuccess, 1)["code"] = "NetStream.Unpublish.Success";
	M_INVOKE_PARAM(_pMsgs->invoke.onFCUnpublishNetStreamUnpublishSuccess, 1)["description"] = streamName;

	return _pMsgs->invoke.onFCUnpublishNetStreamUnpublishSuccess;
}

Variant & RTMPMessageFactory::GetNotifyRtmpSampleAccess(uint32_t streamId,
		double timeStamp, bool isAbsolute, bool audio,
		bool video) {
	GetNotify(_pMsgs->notify.rtmpSampleAccess, 3, streamId, timeStamp, isAbsolute,
			"|RtmpSampleAccess");

	M_NOTIFY_PARAM(_pMsgs->notify.rtmpSampleAccess, 1) = (bool)audio;
	M_NOTIFY_PARAM(_pMsgs->notify.rtmpSampleAccess, 2) = (bool)video;

	return _pMsgs->notify.rtmpSampleAccess;
}

Variant & RTMPMessageFactory::GetNotifyOnMetaData(uint32_t streamId,
		double timeStamp, bool isAbsolute, Variant &metadata) {
	GetNotify(_pMsgs->notify.onMetaData, 3, streamId, timeStamp, isAbsolute, "onMetaData");

	M_NOTIFY_PARAM(_pMsgs->notify.onMetaData, 1) = metadata;

	return _pMsgs->notify.onMetaData;
}

Variant & RTMPMessageFactory::GetNotifyOnStatusNetStreamDataStart(
		uint32_t streamId, double timeStamp, bool isAbsolute) {
	GetNotifyOnStatus(_pMsgs->notify.onStatusNetStreamDataStart, streamId, timeStamp,
			isAbsolute, "NetStream.Data.Start");

	return _pMsgs->notify.onStatusNetStreamDataStart;
}

Variant & RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlaySwitch(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		uint32_t bytes, double duration) {
	GetNotifyOnPlayStatus(_pMsgs->notify.onPlayStatusNetStreamPlaySwitch, streamId,
			timeStamp, isAbsolute, "status", "NetStream.Play.Switch");

	M_NOTIFY_PARAM(_pMsgs->notify.onPlayStatusNetStreamPlaySwitch, 1)["bytes"] = bytes;
	M_NOTIFY_PARAM(_pMsgs->notify.onPlayStatusNetStreamPlaySwitch, 1)["duration"] =
			duration;

	return _pMsgs->notify.onPlayStatusNetStreamPlaySwitch;
}

Variant & RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayComplete(
		uint32_t streamId, double timeStamp, bool isAbsolute,
		uint32_t bytes, double duration) {
	GetNotifyOnPlayStatus(_pMsgs->notify.onPlayStatusNetStreamPlayComplete, streamId,
			timeStamp, isAbsolute, "status", "NetStream.Play.Complete");

	M_NOTIFY_PARAM(_pMsgs->notify.onPlayStatusNetStreamPlayComplete, 1)["bytes"] = bytes;
	M_NOTIFY_PARAM(_pMsgs->notify.onPlayStatusNetStreamPlayComplete, 1)["duration"] =
			duration;

	return _pMsgs->notify.onPlayStatusNetStreamPlayComplete;
}

Variant & RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayTransitionComplete(
		uint32_t streamId, double timeStamp, bool isAbsolute) {
	GetNotifyOnPlayStatus(_pMsgs->notify.onPlayStatusNetStreamPlayTransitionComplete, streamId,
			timeStamp, isAbsolute, "status", "NetStream.Play.TransitionComplete");


	return _pMsgs->notify.onPlayStatusNetStreamPlayTransitionComplete;
}
