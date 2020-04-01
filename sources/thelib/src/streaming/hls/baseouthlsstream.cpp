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



#ifdef HAS_PROTOCOL_HLS
#include "streaming/streamstypes.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "streaming/hls/baseouthlsstream.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "eventlogger/eventlogger.h"
#include "streaming/codectypes.h"

BaseOutHLSStream::BaseOutHLSStream(BaseProtocol *pProtocol, uint64_t type,
		string name, Variant &settings, double timeBase)
: BaseOutStream(pProtocol, type, name) {
	_settings = settings;

	//time base
	_timeBase = timeBase;
}

BaseOutHLSStream::~BaseOutHLSStream() {
	GetEventLogger()->LogStreamClosed(this);
}

bool BaseOutHLSStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_FILE)
            || TAG_KIND_OF(type, ST_IN_NET_EXT);
}

bool BaseOutHLSStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool BaseOutHLSStream::SignalPause() {
	return true;
}

bool BaseOutHLSStream::SignalResume() {
	return true;
}

bool BaseOutHLSStream::SignalSeek(double &dts) {
	return true;
}

bool BaseOutHLSStream::SignalStop() {
	return true;
}

void BaseOutHLSStream::SignalAttachedToInStream() {
}

void BaseOutHLSStream::SignalDetachedFromInStream() {
	_pProtocol->EnqueueForDelete();
}

void BaseOutHLSStream::SignalStreamCompleted() {
	_pProtocol->EnqueueForDelete();
}

bool BaseOutHLSStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
	uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
	packetsCount++;
	bytesCount += dataLength;
	return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
}

bool BaseOutHLSStream::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264)
			|| (codec == CODEC_AUDIO_AAC) 
#ifdef HAS_G711
			|| ((codec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
#endif	/* HAS_G711 */
			;
}

bool BaseOutHLSStream::SendMetadata(string const& metadataStr, int64_t pts) {
	return BaseOutStream::ProcessMetadata(metadataStr, pts);
}

bool BaseOutHLSStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_0001, //naluMarkerType,
			true, //insertPDNALU,
			false, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
#ifdef HAS_G711
	if (pGenericProcessDataSetup->_audioCodec == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
	pGenericProcessDataSetup->audio.aac._insertADTSHeader = true;
	pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;
#ifdef HAS_G711
	} else if ((pGenericProcessDataSetup->_audioCodec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
		pGenericProcessDataSetup->audio.g711._aLaw = (pGenericProcessDataSetup->_audioCodec == CODEC_AUDIO_G711A);
	}
#endif	/* HAS_G711 */
	//misc setup
	pGenericProcessDataSetup->_timeBase = _timeBase;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

	return true;
}

#endif /* HAS_PROTOCOL_HLS */

