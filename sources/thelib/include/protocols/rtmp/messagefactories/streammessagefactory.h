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
#ifndef _STREAMMESSAGEFACTORY_H
#define	_STREAMMESSAGEFACTORY_H

#include "protocols/rtmp/messagefactories/genericmessagefactory.h"

class DLLEXP StreamMessageFactory {
public:
	//management responses
	static Variant GetInvokeCreateStreamResult(Variant &request,
			double createdStreamId);
	static Variant GetInvokeCreateStreamResult(uint32_t channelId,
			uint32_t streamId, uint32_t requestId, double createdStreamId);
	static Variant GetInvokeReleaseStreamResult(uint32_t channelId,
			uint32_t streamId, uint32_t requestId, double releasedStreamId);
	static Variant GetInvokeReleaseStreamErrorNotFound(Variant &request);
	static Variant GetInvokeOnFCPublish(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string code, string description);
	static Variant GetInvokeOnFCSubscribe(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string code, string description);

	//event notifications
	static Variant GetInvokeOnStatusStreamPublishBadName(Variant &request,
			string streamName);
	static Variant GetInvokeOnStatusStreamPublishBadName(uint32_t channelId,
			uint32_t streamId, double requestId, string streamName);
	static Variant GetInvokeOnStatusStreamPublished(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string level, string code, string description,
			string details, string clientId);
	static Variant GetInvokeOnStatusStreamPlayFailed(Variant &request,
			string streamName);
	static Variant GetInvokeOnStatusStreamPlayFailed(uint32_t channelId,
			uint32_t streamId, double requestId, string streamName);
	static Variant GetInvokeOnStatusStreamPlayReset(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetInvokeOnStatusStreamPlayStart(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetInvokeOnStatusStreamPlayStop(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetInvokeOnStatusStreamPlayUnpublishNotify(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string clientId);
	static Variant GetInvokeOnStatusStreamSeekNotify(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetInvokeOnStatusStreamPauseNotify(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetInvokeOnStatusStreamUnpauseNotify(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double requestId, string description, string details, string clientId);
	static Variant GetNotifyRtmpSampleAccess(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			bool unknown1, bool unknown2);
	static Variant GetNotifyOnMetaData(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			Variant metadata, bool dataKeyFrame);
	static Variant GetNotifyOnPlayStatusPlayComplete(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute,
			double bytes, double duration);
	static Variant GetNotifyOnStatusDataStart(uint32_t channelId,
			uint32_t streamId, double timeStamp, bool isAbsolute);

	//generic stream send
	static Variant GetFlexStreamSend(uint32_t channelId, uint32_t streamId,
			double timeStamp, bool isAbsolute, string function,
			Variant &parameters);
};

#endif	/* _STREAMMESSAGEFACTORY_H */

#endif /* HAS_PROTOCOL_RTMP */

