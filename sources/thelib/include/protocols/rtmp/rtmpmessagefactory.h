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


#ifndef _RTMPMESSAGEFACTORY_H
#define	_RTMPMESSAGEFACTORY_H

#include "protocols/rtmp/messagefactories/genericmessagefactory.h"

class RTMPMessageFactory {
private:

	struct __Messages {

		struct {

			struct {
				Variant begin;
				Variant eof;
				Variant dry;
				Variant isRecorded;
			} stream;
		} usrCtrl;

		struct {
			Variant createStream;
			Variant releaseStream;
			Variant publish;
			Variant play;
			Variant fcSubscribe;
			Variant fcPublish;

			struct {

				struct {

					struct {
						Variant streamNotFound;
						Variant reset;
						Variant start;
						Variant stop;
						Variant publishNotify;
						Variant transition;
					} play;

					struct {
						Variant failed;
						Variant notify;
					} seek;
					Variant publishStart;
					Variant unpublishSuccess;
					Variant pauseNotify;
					Variant unpauseNotify;
				} netStream;
			} onStatus;
			Variant onFCUnpublishNetStreamUnpublishSuccess;
		} invoke;

		struct {
			Variant rtmpSampleAccess;
			Variant onMetaData;
			Variant onStatusNetStreamDataStart;
			Variant onPlayStatusNetStreamPlaySwitch;
			Variant onPlayStatusNetStreamPlayComplete;
			Variant onPlayStatusNetStreamPlayTransitionComplete;
		} notify;
	};
	static __Messages *_pMsgs;
private:
	static void GetUsrCtrlStream(Variant &result, uint16_t operation,
			uint32_t streamId, double timestamp, bool isAbsolute);
	static void GetInvoke(Variant &result, uint32_t channelId, uint32_t streamId,
			double timeStamp, bool isAbsolute, double requestId,
			const string &functionName);
	static void GetInvokeOnStatus(Variant &result, uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &level,
			const string &code);
	static void GetNotify(Variant &result, uint32_t channelId, uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &functionName);
	static void GetNotifyOnStatus(Variant &result, uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &code);
	static void GetNotifyOnPlayStatus(Variant &result, uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &level,
			const string &code);
public:
	static void Init();
	static void Cleanup();

	//usrCtrl messages
	static Variant & GetUsrCtrlStreamBegin(uint32_t streamId, double timeStamp,
			bool isAbsolute);
	static Variant & GetUsrCtrlStreamEof(uint32_t streamId, double timeStamp,
			bool isAbsolute);
	static Variant & GetUsrCtrlStreamDry(uint32_t streamId, double timeStamp,
			bool isAbsolute);
	static Variant & GetUsrCtrlStreamIsRecorded(uint32_t streamId,
			double timeStamp, bool isAbsolute);

	//invokes
	static Variant & GetInvokeCreateStream();
	static Variant & GetInvokeReleaseStream(const string &streamName);
	static Variant & GetInvokePublish(uint32_t streamId,
			const string &streamName, const string &mode);
	static Variant & GetInvokePlay(uint32_t streamId, const string &streamName,
			double start, double length, bool reset);
	static Variant & GetInvokeFCSubscribe(const string &streamName);
	static Variant & GetInvokeFCPublish(const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPlayStreamNotFound(
			uint32_t streamId, double timeStamp, bool isAbsolute,
			const string &clientId, const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPlayReset(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPlayStart(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPlayStop(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName, const string &reason);
	static Variant & GetInvokeOnStatusNetStreamPlayPublishNotify(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPlayTransition(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName, bool forced);
	static Variant & GetInvokeOnStatusNetStreamSeekFailed(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			double seekPoint);
	static Variant & GetInvokeOnStatusNetStreamSeekNotify(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName, double seekPoint);
	static Variant & GetInvokeOnStatusNetStreamPublishStart(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamUnpublishSuccess(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamPauseNotify(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnStatusNetStreamUnpauseNotify(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);
	static Variant & GetInvokeOnFCUnpublishNetStreamUnpublishSuccess(uint32_t streamId,
			double timeStamp, bool isAbsolute, const string &clientId,
			const string &streamName);

	//notifies
	static Variant & GetNotifyRtmpSampleAccess(uint32_t streamId,
			double timeStamp, bool isAbsolute, bool audio, bool video);
	static Variant & GetNotifyOnMetaData(uint32_t streamId, double timeStamp,
			bool isAbsolute, Variant &metadata);
	static Variant & GetNotifyOnStatusNetStreamDataStart(uint32_t streamId,
			double timeStamp, bool isAbsolute);
	static Variant & GetNotifyOnPlayStatusNetStreamPlaySwitch(uint32_t streamId,
			double timeStamp, bool isAbsolute, uint32_t bytes, double duration);
	static Variant & GetNotifyOnPlayStatusNetStreamPlayComplete(
			uint32_t streamId, double timeStamp, bool isAbsolute, uint32_t bytes,
			double duration);
	static Variant & GetNotifyOnPlayStatusNetStreamPlayTransitionComplete(
			uint32_t streamId, double timeStamp, bool isAbsolute);
};


#endif	/* _RTMPMESSAGEFACTORY_H */

