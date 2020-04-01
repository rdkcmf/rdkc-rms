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


#include "recording/baseoutrecording.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"

BaseOutRecording::BaseOutRecording(BaseProtocol *pProtocol, uint64_t type,
		string name, Variant &settings)
: BaseOutFileStream(pProtocol, type, name) {
	_settings = settings;
	_hasAudio = true;
	if (settings.HasKeyChain(V_BOOL, false, 1, "hasAudio")) {
		if (((bool)settings.GetValue("hasAudio", false)) == false)
			_hasAudio = false;
	}
}

BaseOutRecording::~BaseOutRecording() {
	GetEventLogger()->LogStreamClosed(this);
}

void BaseOutRecording::GetStats(Variant &info, uint32_t namespaceId) {
	BaseOutStream::GetStats(info, namespaceId);
}

bool BaseOutRecording::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_FILE)
            || TAG_KIND_OF(type, ST_IN_NET_EXT);
}

bool BaseOutRecording::SignalPlay(double &dts, double &length) {
	return true;
}

bool BaseOutRecording::SignalPause() {
	return true;
}

bool BaseOutRecording::SignalResume() {
	return true;
}

bool BaseOutRecording::SignalSeek(double &dts) {
	return true;
}

bool BaseOutRecording::SignalStop() {
	return true;
}

void BaseOutRecording::SignalAttachedToInStream() {
}

void BaseOutRecording::SignalDetachedFromInStream() {
	_pProtocol->EnqueueForDelete();
}

void BaseOutRecording::SignalStreamCompleted() {
	_pProtocol->EnqueueForDelete();
}

bool BaseOutRecording::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
	uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
	packetsCount++;
	bytesCount += dataLength;
	return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
}

void BaseOutRecording::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if ((pOld == NULL)&&(pNew != NULL))
		return;
	WARN("Codecs changed and the recordings does not support it. Closing recording");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
}

void BaseOutRecording::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	if ((pOld == NULL)&&(pNew != NULL))
		return;

	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}

	WARN("Codecs changed and the recordings does not support it. Closing recording");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
}
