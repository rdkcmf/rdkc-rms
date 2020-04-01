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




#include "streaming/baseoutnetstream.h"
#include "streaming/streamstypes.h"
#include "protocols/baseprotocol.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "streaming/streamsmanager.h"

BaseOutNetStream::BaseOutNetStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseOutStream(pProtocol, type, name) {
	if (!TAG_KIND_OF(type, ST_OUT_NET)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_OUT_NET)), STR(tagToString(type)));
	}
	if (pProtocol->GetCustomParameters().HasKeyChain(V_BOOL, false, 3, "customParameters","localStreamConfig","useSourcePts"))
		_useSourcePts = (bool) pProtocol->GetCustomParameters()["customParameters"]["localStreamConfig"]["useSourcePts"];
	_useSourcePts = false;
	_keyframeCacheConsumed = false;
}

BaseOutNetStream::~BaseOutNetStream() {
}

bool BaseOutNetStream::UseSourcePts() {
	return _useSourcePts;
}

bool BaseOutNetStream::GenericProcessData(
	uint8_t * pData, uint32_t dataLength,
	uint32_t processedLength, uint32_t totalLength,
	double pts, double dts,
	bool isAudio)
{
	bool result = false;
	uint32_t origInStreamId = 0;
	BaseInStream* pOrigInStream = NULL;
	Variant& streamConfig = GetProtocol()->GetCustomParameters();
	if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "_origId")) {
		origInStreamId = (uint32_t)(GetProtocol()->GetCustomParameters()["_origId"]);
		pOrigInStream = (BaseInStream*) (GetStreamsManager()->FindByUniqueId(origInStreamId));
	}
	// We've got a cached keyframe?
	// pData is not a keyframe?
	// Cache hasn't been used yet?
	//
	// IF yes to all, then let's send out the cached keyframe first.
	if (pOrigInStream && pOrigInStream->_hasCachedKeyFrame() &&
		(NALU_TYPE(pData[0]) != NALU_TYPE_IDR) &&
		!_keyframeCacheConsumed &&
		!isAudio) {
		//!_pInStream->_cacheConsumed() ) {
		IOBuffer cachedKeyFrame;
		if (pOrigInStream->_consumeCachedKeyFrame(cachedKeyFrame)) {
			INFO("Keyframe Cache HIT! SIZE=%d PTS=%lf",
				GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
				pts);
			result = BaseOutStream::GenericProcessData(
				GETIBPOINTER(cachedKeyFrame),
				GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
				pOrigInStream->_getCachedProcLen(),
				pOrigInStream->_getCachedTotLen(),
				pts, dts, isAudio);
			if (!result) {
				return false;
			}
			_keyframeCacheConsumed = true;
		}
	} else if (NALU_TYPE(pData[0]) == NALU_TYPE_IDR && !_keyframeCacheConsumed) {
		INFO("Outbound frame was a keyframe, sending cached keyframe is not needed anymore.");
		_keyframeCacheConsumed = true;
		result = BaseOutStream::GenericProcessData(
			pData, dataLength,
			processedLength, totalLength,
			pts, dts,
			isAudio);
	} else {
		result = BaseOutStream::GenericProcessData(
			pData, dataLength,
			processedLength, totalLength,
			pts, dts,
			isAudio);
	}
	return result;
}

void BaseOutNetStream::SetKeyFrameConsumed(bool keyFrameStatus) {
	_keyframeCacheConsumed = keyFrameStatus;
}