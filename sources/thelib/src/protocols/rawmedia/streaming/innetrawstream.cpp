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


#ifdef HAS_PROTOCOL_RAWMEDIA
#include "protocols/rawmedia/streaming/innetrawstream.h"
#include "streaming/streamstypes.h"
#include "streaming/nalutypes.h"
#include "streaming/baseoutstream.h"
#include "streaming/codectypes.h"

InNetRawStream::InNetRawStream(BaseProtocol *pProtocol, string name)
		: BaseInNetStream(pProtocol, ST_IN_NET_RAW, name) {
	_videoCapsChanged = false;
	//ENABLE_LATENCY_LOGGING;
}

InNetRawStream::~InNetRawStream() {

}

bool InNetRawStream::SignalPlay(double &dts, double &length){
	return true;
}

bool InNetRawStream::SignalPause(){
	return true;
}

bool InNetRawStream::SignalResume(){
	return true;
}

bool InNetRawStream::SignalSeek(double &dts){
	return true;
}

bool InNetRawStream::SignalStop(){
	return true;
}

void InNetRawStream::ReadyForSend(){

}

bool InNetRawStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio){
	return true;
}

void InNetRawStream::SignalOutStreamAttached(BaseOutStream *pOutStream){

}

void InNetRawStream::SignalOutStreamDetached(BaseOutStream *pOutStream){

}

bool InNetRawStream::IsCompatibleWithType(uint64_t type){
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
			|| TAG_KIND_OF(type, ST_OUT_NET_RTP)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HLS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HDS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MSS)
           		|| TAG_KIND_OF(type, ST_OUT_FILE_DASH)
			|| TAG_KIND_OF(type, ST_OUT_FILE_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MP4)
			|| TAG_KIND_OF(type, ST_OUT_FILE_RTMP_FLV)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_VMF)
			|| TAG_KIND_OF(type, ST_OUT_NET_FMP4)
			|| TAG_KIND_OF(type, ST_OUT_NET_MP4)
			|| TAG_KIND_OF(type, ST_OUT)
			;
}

bool InNetRawStream::FeedVideoData(uint8_t *pData, uint32_t dataLength, double ts) {
	// 1. Validate and get nalu prefix
	// DEBUG("[DEBUG] Video data received. Length=%"PRIu32"\tTS=%.3f", dataLength, ts);
	if (dataLength < 4) {
		FATAL("Incorrect NAL-U size. Got %"PRIu32, dataLength);
		return false;
	}
	if (_nalPrefix.size() == 0) {
		if (pData[0] != 0 || pData[1] != 0) {
			FATAL("Invalid NAL-u prefix");
			return false;
		}
		if (pData[2] == 1) {
			_nalPrefix.push_back(0);
			_nalPrefix.push_back(0);
			_nalPrefix.push_back(1);
		} else if (pData[2] == 0 && pData[3] == 1) {
			_nalPrefix.push_back(0);
			_nalPrefix.push_back(0);
			_nalPrefix.push_back(0);
			_nalPrefix.push_back(1);
		} else {
			FATAL("Invalid NAL-u prefix");
			return false;
		}
	}
	// 2. cycle through NAL-u's
	uint8_t *begin = FindNalu(pData, 0, dataLength);
	uint32_t nalPrefixLength = _nalPrefix.size();
	bool hasMoreNalu = (begin != NULL);
	while (hasMoreNalu) {
		hasMoreNalu = false;
		begin += nalPrefixLength;

		uint8_t type = NALU_TYPE(*begin);

		uint8_t *end = FindNalu(begin, 0, dataLength - (begin - pData));
		uint32_t naluLength = 0;
		if (end == NULL) {
			naluLength = dataLength - (begin - pData);
		} else {
			hasMoreNalu = true;
			naluLength = end - begin;
		}
#ifdef UDS_DEBUG
		FATAL("[Debug] Got NAL-u type %"PRIu8" (%"PRIu32" bytes)", type, naluLength);
#endif
		switch (type) {
			case NALU_TYPE_SPS:
				if (GETAVAILABLEBYTESCOUNT(_sps) == 0) {
					_sps.IgnoreAll();
					_sps.ReadFromBuffer(begin, naluLength);
					_videoCapsChanged = true & (GETAVAILABLEBYTESCOUNT(_pps) > 0);
				}
				break;
			case NALU_TYPE_PPS:
				if (GETAVAILABLEBYTESCOUNT(_pps) == 0) {
					_pps.IgnoreAll();
					uint8_t *pos = begin + naluLength - 1;
					while (*pos == 0) {
						pos--;
					}
					naluLength = pos - begin + 1;
					_pps.ReadFromBuffer(begin, naluLength);
					_videoCapsChanged = true & (GETAVAILABLEBYTESCOUNT(_sps) > 0);
				}
				break;
			case NALU_TYPE_IDR:
				// cache the I-frame
				_cachedKeyFrame.IgnoreAll();
				_cachedKeyFrame.ReadFromBuffer(begin, naluLength);
				_cachedDTS = ts;
				_cachedPTS = ts;
				_cachedProcLen = 0;
				_cachedTotLen = naluLength;
			case NALU_TYPE_SLICE:
#ifndef _HAS_XSTREAM_
			case NALU_TYPE_SEI:
#endif
				FeedVideoFrame(begin, naluLength, ts);
				break;
			default:
#ifndef _HAS_XSTREAM_
				WARN("Unsupported NAL unit: %"PRIu8" (ignored)", type);
#endif
				break;
		}
		
		
		if (hasMoreNalu)
			begin = end;
	}
	if (_videoCapsChanged) {
		_streamCaps.AddTrackVideoH264(
				GETIBPOINTER(_sps), GETAVAILABLEBYTESCOUNT(_sps),
				GETIBPOINTER(_pps), GETAVAILABLEBYTESCOUNT(_pps),
				0, -1, -1, false, this);
//#ifdef UDS_DEBUG
//		FATAL("[Debug] Stream %s capabilities", GetCapabilities() != NULL ? "has" : "does not have");
//		FATAL("[Debug] SPS: %s", STR(_sps.ToString()));
//		FATAL("[Debug] PPS: %s", STR(_pps.ToString()));
//		IOBuffer temp;
//		temp.ReadFromBuffer(pData, dataLength);
//		FATAL("[Debug] Buffer: %s", STR(temp.ToString()));
//#endif
		_videoCapsChanged = false;
	}
	return true;
}

bool InNetRawStream::FeedAudioData(uint8_t *pData, uint32_t dataLength, double ts) {
	vector<BaseOutStream *> outStreams = GetOutStreams();

	LOG_LATENCY("Feed to Outstream|Audio packet|ts: %0.3f", ts);
	//FINE("InNetRawStream:: Feeding Audio Data ----->\n" );
	FOR_VECTOR(outStreams, i) {
		//FINE("OutStream found!!! Feeding Audio Data ----->\n" );
		outStreams[i]->FeedData(pData, dataLength, 0, dataLength, ts, ts, true);
	}

	return true;
}

void InNetRawStream::FeedVideoFrame(uint8_t *pData, uint32_t dataLength, double ts) {
//	IOBuffer temp;
//	temp.ReadFromBuffer(pData, dataLength);
//	INFO("[Debug] Feeding to outbound streams: %s", STR(temp.ToString()));
	//FINE("InNetRawStream:: Feeding Video Frame ----->\n" );
	vector<BaseOutStream *> outStreams = GetOutStreams();
	LOG_LATENCY("Feed to Outstream|Video packet|ts: %0.3f", ts);
	FOR_VECTOR(outStreams, i) {
		//FINE("OutStream found!!! Feeding Video Frame ----->\n" );
		outStreams[i]->FeedData(pData, dataLength, 0, dataLength, ts, ts, false);
	}
}

uint8_t *InNetRawStream::FindNalu(uint8_t *pData, uint32_t offset, uint32_t dataLength) {
	uint8_t *ptr = pData + offset;
	uint32_t jump = 0;
	uint32_t nalPrefixLength = _nalPrefix.size();

	while (ptr <= pData + dataLength - nalPrefixLength) {
		if (nalPrefixLength == 4) {
			if (ptr[3] != 1) {
				jump = ptr[3] == 0 ? 1 : nalPrefixLength;
			} else if (ptr[2] != 0 || ptr[1] != 0 || ptr[0] != 0) {
				jump = nalPrefixLength;
			} else {
				return ptr;
			}
		} else if (nalPrefixLength == 3) {
			if (ptr[2] != 1) {
				jump = ptr[2] == 0 ? 1 : nalPrefixLength;
			} else if (ptr[1] != 0 || ptr[0] != 0) {
				jump = nalPrefixLength;
			} else {
				return ptr;
			}
		}
		ptr += jump;
	}
	return NULL;
}
bool InNetRawStream::FeedData(uint8_t *pData, uint32_t dataLength, double ts, bool isAudio) {
	LOG_LATENCY_INC("START|Raw %s packet|ts: %0.3f", isAudio ? "audio" : "video", ts);
	if (isAudio) {
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += dataLength;
		return FeedAudioData(pData, dataLength, ts);
	} else {
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
		return FeedVideoData(pData, dataLength, ts);
	}
}

StreamCapabilities *InNetRawStream::GetCapabilities() {
	return &_streamCaps;
}

void InNetRawStream::SetAudioConfig(uint64_t type, uint8_t *pConfigData, uint32_t dataLength) {
	switch (type) {
		case CODEC_AUDIO_AAC:
			_streamCaps.AddTrackAudioAAC(pConfigData, dataLength, true, this);
			break;
#ifdef HAS_G711
		case CODEC_AUDIO_G711:
			_streamCaps.AddTrackAudioG711((bool)pConfigData[0], this);
			break;
#endif
		default:
			FATAL("Invalid audio codec type!");
			break;
	}
}

void InNetRawStream::SetVideoConfig(uint64_t type, uint8_t *pConfigData, uint32_t dataLength) {
	NYI;
}
#endif	/* HAS_PROTOCOL_RAWMEDIA */

