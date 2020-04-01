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



#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
#include "protocols/ts/innettsstream.h"
#include "streaming/streamstypes.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "streaming/baseoutstream.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"

InNetTSStream::InNetTSStream(BaseProtocol *pProtocol, string name,
        uint32_t bandwidthHint)
: BaseInNetStream(pProtocol, ST_IN_NET_TS, name) {
	//audio section
	_hasAudio = false;

	//video section
	_hasVideo = false;

	_streamCapabilities.SetTransferRate(bandwidthHint);
	_enabled = false;
}

InNetTSStream::~InNetTSStream() {
	GetEventLogger()->LogStreamClosed(this);
}

StreamCapabilities * InNetTSStream::GetCapabilities() {
    return &_streamCapabilities;
}

bool InNetTSStream::HasAudio() {
	return _hasAudio;
}

void InNetTSStream::HasAudio(bool value) {
	_hasAudio = value;
}

bool InNetTSStream::HasVideo() {
	return _hasVideo;
}

void InNetTSStream::HasVideo(bool value) {
	_hasVideo = value;
}

void InNetTSStream::Enable(bool value) {
	_enabled = value;
}

bool InNetTSStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	SavePts(pts); // stash timestamp for MetadataManager
	if (((_hasAudio)&&(_streamCapabilities.GetAudioCodecType() != CODEC_AUDIO_AAC))
			|| ((_hasVideo)&&(_streamCapabilities.GetVideoCodecType() != CODEC_VIDEO_H264))
			|| (!_enabled)) {
		if (isAudio) {
			_stats.audio.droppedBytesCount += dataLength;
			_stats.audio.droppedPacketsCount++;
		} else {
			_stats.video.droppedBytesCount += dataLength;
			_stats.video.droppedPacketsCount++;
		}
		return true;
	}
	if (isAudio) {
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += dataLength;
	} else {
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
	}

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->FeedData(pData, dataLength, processedLength, totalLength,
				pts, dts, isAudio)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}
	return true;
}

void InNetTSStream::ReadyForSend() {
    NYI;
}

bool InNetTSStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
			|| (type == ST_OUT_NET_RTP)
			|| (type == ST_OUT_FILE_HLS)
			|| (type == ST_OUT_FILE_HDS)
			|| (type == ST_OUT_FILE_MSS)
            || (type == ST_OUT_FILE_DASH)
			|| (type == ST_OUT_FILE_MP4)
			|| (type == ST_OUT_NET_EXT)
			|| (type == ST_OUT_FILE_TS)
			|| (type == ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_VMF)
			|| (type == ST_OUT_NET_FMP4)
			|| (type == ST_OUT_NET_MP4)
			;
}

void InNetTSStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
    //NYI;
}

void InNetTSStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
    //NYI;
}

bool InNetTSStream::SignalPlay(double &dts, double &length) {
    return true;
}

bool InNetTSStream::SignalPause() {
    return true;
}

bool InNetTSStream::SignalResume() {
    return true;
}

bool InNetTSStream::SignalSeek(double &dts) {
    return true;
}

bool InNetTSStream::SignalStop() {
	return true;
}

#endif	/* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */

