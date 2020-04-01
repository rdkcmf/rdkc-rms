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
*/

#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/streaming/innetrtpstream.h"
#include "streaming/streamstypes.h"
#include "streaming/nalutypes.h"
#include "streaming/baseoutstream.h"
#include "protocols/baseprotocol.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "protocols/rtp/sdp.h"
#include "eventlogger/eventlogger.h"
#include "protocols/rtp/rtspprotocol.h"
#include "streaming/codectypes.h"

#ifdef PLAYBACK_SUPPORT
#define DUMP_G711 0

#if DUMP_G711
static FILE *g711Fd;
#endif

#endif

InNetRTPStream::InNetRTPStream(BaseProtocol *pProtocol, string name, Variant &videoTrack,
		Variant &audioTrack, uint32_t bandwidthHint, uint8_t rtcpDetectionInterval,
		int16_t a, int16_t b)
: BaseInNetStream(pProtocol, ST_IN_NET_RTP, name) {
	_hasAudio = false;
	_isLatm = false;
#ifdef HAS_G711
	_audioByteStream = false;
#endif	/* HAS_G711 */
	_audioSampleRate = 1;
	if (audioTrack != V_NULL) {
		uint32_t sdpSampleRate = (uint32_t) SDP_TRACK_CLOCKRATE(audioTrack);
		AudioCodecInfo *pInfo = NULL;
#ifdef HAS_G711
		uint64_t codecType = SDP_AUDIO_CODEC(audioTrack);
		if (codecType == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
		string raw = unhex(SDP_AUDIO_CODEC_SETUP(audioTrack));
		_isLatm = (SDP_AUDIO_TRANSPORT(audioTrack) == "mp4a-latm");
			pInfo = _capabilities.AddTrackAudioAAC(
				(uint8_t *) raw.data(),
				(uint8_t) raw.length(),
				!_isLatm,
				this);
#ifdef HAS_G711
		} else if (codecType == CODEC_AUDIO_G711A || CODEC_AUDIO_G711U) {
			pInfo = _capabilities.AddTrackAudioG711((codecType == CODEC_AUDIO_G711A), this);
			_audioByteStream = true;
		}
#endif	/* HAS_G711 */
		_hasAudio = (bool)(pInfo != NULL);
		if (pInfo != NULL) {
			if (sdpSampleRate != pInfo->_samplingRate) {
				WARN("Audio sample rate advertised inside SDP is different from the actual value compued from the codec setup bytes. SDP: %"PRIu32"; codec setup bytes: %"PRIu32". Using the value from SDP",
						sdpSampleRate, pInfo->_samplingRate);
				_audioSampleRate = sdpSampleRate;
			} else {
				_audioSampleRate = pInfo->_samplingRate;
			}
		}
	}

	_hasVideo = false;
	_videoSampleRate = 1;
	if (videoTrack != V_NULL) {
		_a = a;
		_b = b;
		string rawSps = unb64(SDP_VIDEO_CODEC_H264_SPS(videoTrack));
		string rawPps = unb64(SDP_VIDEO_CODEC_H264_PPS(videoTrack));
		VideoCodecInfo *pInfo = _capabilities.AddTrackVideoH264(
				(uint8_t *) rawSps.data(),
				(uint32_t) rawSps.length(),
				(uint8_t *) rawPps.data(),
				(uint32_t) rawPps.length(),
				(uint32_t) SDP_TRACK_CLOCKRATE(videoTrack),
				a,
				b,
				true,
				this
				);
		_hasVideo = (bool)(pInfo != NULL);
		if (_hasVideo) {
			_videoSampleRate = pInfo->_samplingRate;
			// Insert FPS attribute
			pInfo->_fps = SDP_VIDEO_FRAME_RATE(videoTrack);
		}
	}
	FATAL("Sampling rate: %f", _videoSampleRate);
	
	if (bandwidthHint > 0)
		_capabilities.SetTransferRate(bandwidthHint);

	_audioSequence = 0;
	_audioNTP = 0;
	_audioRTP = 0;
	_audioLastDts = -1;
	_audioRTPRollCount = 0;
	_audioLastRTP = 0;
	_audioFirstTimestamp = -1;
	_lastAudioRTCPRTP = 0;
	_audioRTCPRTPRollCount = 0;

	_videoSequence = 0;
	_videoNTP = 0;
	_videoRTP = 0;
	_videoLastPts = -1;
	_videoLastDts = -1;
	_videoRTPRollCount = 0;
	_videoLastRTP = 0;
	_videoFirstTimestamp = -1;
	_lastVideoRTCPRTP = 0;
	_videoRTCPRTPRollCount = 0;

	_rtcpPresence = RTCP_PRESENCE_UNKNOWN;
	//_rtcpPresence = (((_hasAudio)&&(!_hasVideo)) || ((!_hasAudio)&&(_hasVideo)))
	//			? RTCP_PRESENCE_ABSENT : RTCP_PRESENCE_UNKNOWN;
	_rtcpDetectionInterval = rtcpDetectionInterval;
	_rtcpDetectionStart = 0;

	_dtsCacheSize = 1;
	//_maxDts = -1;

#ifdef PLAYBACK_SUPPORT

#if DUMP_G711
	//INFO("Opening g711inet.ulw file");
        g711Fd = fopen("/opt/g711inet.ulw", "a");
#endif

#endif

}

InNetRTPStream::~InNetRTPStream() {

#ifdef PLAYBACK_SUPPORT

#if DUMP_G711
	INFO("Closing /opt/g711inet.ulw file");
        fclose(g711Fd);
#endif

#endif
    GetEventLogger()->LogStreamClosed(this);
}

StreamCapabilities * InNetRTPStream::GetCapabilities() {
    return &_capabilities;
}

bool InNetRTPStream::IsCompatibleWithType(uint64_t type) {
	return (type == ST_OUT_NET_RTMP)
			|| (type == ST_OUT_NET_TS)
			|| (type == ST_OUT_FILE_HLS)
			|| (type == ST_OUT_FILE_TS)
			|| (type == ST_OUT_FILE_HDS)
			|| (type == ST_OUT_FILE_MSS)
            || (type == ST_OUT_FILE_DASH)
			|| (type == ST_OUT_FILE_MP4)
			|| (type == ST_OUT_NET_RTP)
			|| (type == ST_OUT_NET_EXT)
			|| (type == ST_OUT_FILE_RTMP_FLV)
			|| (type == ST_OUT_NET_FMP4)
			|| (type == ST_OUT_NET_RAW)
			|| (type == ST_OUT_NET_MP4)
			;
}

void InNetRTPStream::ReadyForSend() {

}

void InNetRTPStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
#ifdef HAS_PROTOCOL_RTMP
	if (TAG_KIND_OF(pOutStream->GetType(), ST_OUT_NET_RTMP)) {
		Variant customConfig = pOutStream->GetProtocol()->GetCustomParameters();
		bool sendRequest = true;
		if (customConfig.HasKeyChain(V_BOOL, false, 3, "customParameters", "localStreamConfig", "sendChunkSizeRequest")) {
			sendRequest = (bool)customConfig.GetValue("customParameters", false).GetValue("localStreamConfig", false).GetValue("sendChunkSizeRequest", false);
		}
		if (sendRequest) {
			((BaseRTMPProtocol *) pOutStream->GetProtocol())->TrySetOutboundChunkSize(4 * 1024);
			((OutNetRTMPStream *) pOutStream)->SetFeederChunkSize(4 * 1024 * 1024);
		}
		((OutNetRTMPStream *) pOutStream)->ResendMetadata();
	}
#endif /* HAS_PROTOCOL_RTMP */
	BaseProtocol *pProtocol = GetProtocol();
	if ((pProtocol != NULL)&&(pProtocol->GetType() == PT_RTSP))
		((RTSPProtocol *) pProtocol)->SignalOutStreamAttached();
}

void InNetRTPStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
}

bool InNetRTPStream::SignalPlay(double &dts, double &length) {
    return true;
}

bool InNetRTPStream::SignalPause() {
	((RTSPProtocol*) _pProtocol)->SendPauseReq();
    return true;
}

bool InNetRTPStream::SignalResume() {
	((RTSPProtocol*) _pProtocol)->SendPlayResume();
    return true;
}

bool InNetRTPStream::SignalSeek(double &dts) {
    return true;
}

bool InNetRTPStream::SignalStop() {
    return true;
}

bool InNetRTPStream::FeedData(uint8_t *pData, uint32_t dataLength,
        uint32_t processedLength, uint32_t totalLength,
        double pts, double dts, bool isAudio) {
    ASSERT("Operation not supported");
    return false;
}

bool InNetRTPStream::FeedVideoData(uint8_t *pData, uint32_t dataLength,
		RTPHeader &rtpHeader) {
	if (!_hasVideo)
		return false;

	//1. Check the counter first
	if (_videoSequence == 0) {
		_videoSequence = GET_RTP_SEQ(rtpHeader);
	} else {
		if ((uint16_t) (_videoSequence + 1) != (uint16_t) GET_RTP_SEQ(rtpHeader)) {
			WARN("Missing video packet. Wanted: %"PRIu16"; got: %"PRIu16" on stream: %s",
					(uint16_t) (_videoSequence + 1),
					(uint16_t) GET_RTP_SEQ(rtpHeader),
					STR(GetName()));
			_currentNalu.IgnoreAll();
			_stats.video.droppedPacketsCount++;
			_stats.video.droppedBytesCount += dataLength;
			_videoSequence = 0;
			return true;
		} else {
			_videoSequence++;
		}
	}

	//2. get the nalu
	uint64_t rtpTs = ComputeRTP(rtpHeader._timestamp, _videoLastRTP, _videoRTPRollCount);
	double ts = (double)((double) rtpTs / _videoSampleRate * 1000.0);
	//FINEST("ts: %f", ts);
	uint8_t naluType = NALU_TYPE(pData[0]);
	if (naluType <= 23) {
		//3. Standard NALU
		//FINEST("V: %08"PRIx32, rtpHeader._timestamp);
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
		//_currentNalu.IgnoreAll();
		//FINE("HERE1");
		return InternalFeedData(pData, dataLength, 0, dataLength, ts, false);
	} else if (naluType == NALU_TYPE_FUA) {
		//FINE("HERE2");
		if (GETAVAILABLEBYTESCOUNT(_currentNalu) == 0) {
			if ((pData[1] >> 7) == 0) {
				WARN("Bogus nalu: %s", STR(bits(pData, 2)));
				_currentNalu.IgnoreAll();
				return true;
			}
			//FINE("HERE2A");
			pData[1] = (pData[0]&0xe0) | (pData[1]&0x1f);
			_currentNalu.ReadFromBuffer(pData + 1, dataLength - 1);
			return true;
		} else {
			//FINE("HERE2B");
			//middle NAL
			_currentNalu.ReadFromBuffer(pData + 2, dataLength - 2);
			if (((pData[1] >> 6)&0x01) == 1) {
				//FINEST("V: %08"PRIx32, rtpHeader._timestamp);
				_stats.video.packetsCount++;
				_stats.video.bytesCount += GETAVAILABLEBYTESCOUNT(_currentNalu);
				//FINE("HERE2C");
				if (!InternalFeedData(GETIBPOINTER(_currentNalu),
						GETAVAILABLEBYTESCOUNT(_currentNalu),
						0,
						GETAVAILABLEBYTESCOUNT(_currentNalu),
						ts,
						false)) {
					FATAL("Unable to feed NALU");
					return false;
				}
				_currentNalu.IgnoreAll();
			}
			return true;
		}
	} else if (naluType == NALU_TYPE_STAPA) {
		//FINE("HERE3");
		uint32_t index = 1;
		while (index + 3 < dataLength) {
			uint16_t length = ENTOHSP(pData + index);
			index += 2;
			if (index + length > dataLength) {
				WARN("Bogus STAP-A");
				_currentNalu.IgnoreAll();
				_videoSequence = 0;
				return true;
			}
			_stats.video.packetsCount++;
			_stats.video.bytesCount += length;
			if (!InternalFeedData(pData + index,
					length, 0,
					length,
					ts, false)) {
				FATAL("Unable to feed NALU");
				return false;
			}
			index += length;
		}
		return true;
	} else {
		WARN("invalid NAL: %s", STR(NALUToString(naluType)));
		_currentNalu.IgnoreAll();
		_videoSequence = 0;
		return true;
	}
}

bool InNetRTPStream::FeedAudioData(uint8_t *pData, uint32_t dataLength,
		RTPHeader &rtpHeader) {
	//INFO("Feeding the audio frame of length %d to input network RTP stream, _hasAudio %d", dataLength, _hasAudio);
	if (!_hasAudio)
		return false;
	if (_isLatm)
		return FeedAudioDataLATM(pData, dataLength, rtpHeader);
#ifdef HAS_G711
	else if (_audioByteStream)
		return FeedAudioDataByteStream(pData, dataLength, rtpHeader);
#endif	/* HAS_G711 */
	else
		return FeedAudioDataAU(pData, dataLength, rtpHeader);
}

void InNetRTPStream::ReportSR(uint64_t ntpMicroseconds, uint32_t rtpTimestamp,
        bool isAudio) {
    if (isAudio) {
        _audioRTP = (double) ComputeRTP(rtpTimestamp, _lastAudioRTCPRTP,
                _audioRTCPRTPRollCount) / _audioSampleRate * 1000.0;
        _audioNTP = (double) ntpMicroseconds / 1000.0;
    } else {
        _videoRTP = (double) ComputeRTP(rtpTimestamp, _lastVideoRTCPRTP,
                _videoRTCPRTPRollCount) / _videoSampleRate * 1000.0;
        _videoNTP = (double) ntpMicroseconds / 1000.0;
    }
}

bool InNetRTPStream::FeedAudioDataAU(uint8_t *pData, uint32_t dataLength,
		RTPHeader &rtpHeader) {
	if (_audioSequence == 0) {
		_audioSequence = GET_RTP_SEQ(rtpHeader);
	} else {
		if ((uint16_t) (_audioSequence + 1) != (uint16_t) GET_RTP_SEQ(rtpHeader)) {
			WARN("Missing audio packet. Wanted: %"PRIu16"; got: %"PRIu16" on stream: %s",
					(uint16_t) (_audioSequence + 1),
					(uint16_t) GET_RTP_SEQ(rtpHeader),
					STR(GetName()));
			_stats.audio.droppedPacketsCount++;
			_stats.audio.droppedBytesCount += dataLength;
			_audioSequence = 0;
			return true;
		} else {
			_audioSequence++;
		}
	}

	//1. Compute chunks count
	uint16_t chunksCount = ENTOHSP(pData);
	uint8_t adtsHeaderSize = 0;
	if ((chunksCount % 16) != 0) {
		FATAL("Invalid AU headers length: %"PRIx16, chunksCount);
		return false;
	}
	chunksCount = chunksCount / 16;

	//3. Feed the buffer chunk by chunk
	uint32_t cursor = 2 + 2 * chunksCount;
	uint16_t chunkSize = 0;
	double ts = 0;
	uint64_t rtpTs = ComputeRTP(rtpHeader._timestamp, _audioLastRTP, _audioRTPRollCount);
	for (uint32_t i = 0; i < chunksCount; i++) {
		if (i != (uint32_t) (chunksCount - 1)) {
			chunkSize = (ENTOHSP(pData + 2 + 2 * i)) >> 3;
		} else {
			chunkSize = (uint16_t) (dataLength - cursor);
		}
		ts = (double) (rtpTs + i * 1024) / _audioSampleRate * 1000.00;
		if ((cursor + chunkSize) > dataLength) {
			FATAL("Unable to feed data: cursor: %"PRIu32"; chunkSize: %"PRIu16"; dataLength: %"PRIu32"; chunksCount: %"PRIu16,
					cursor, chunkSize, dataLength, chunksCount);
			return false;
		}
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += chunkSize;
		if ((chunkSize > 7)
				&&(pData[cursor] == 0xff)
				&&((pData[cursor + 1] >> 4) == 0x0f))
			adtsHeaderSize = (pData[cursor + 1]&0x01) ? 7 : 9;
		else
			adtsHeaderSize = 0;
		if (!InternalFeedData(pData + cursor + adtsHeaderSize - 2,
				chunkSize - adtsHeaderSize + 2,
				0,
				chunkSize - adtsHeaderSize + 2,
				ts, true)) {
			FATAL("Unable to feed data");
			return false;
		}
		cursor += chunkSize;

	}

	return true;
}

#ifdef HAS_G711
bool InNetRTPStream::FeedAudioDataByteStream(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader) {
	uint64_t rtpTs = ComputeRTP(rtpHeader._timestamp, _audioLastRTP, _audioRTPRollCount);
	double ts = (double) (rtpTs) / _audioSampleRate * 1000.00;
		if (!InternalFeedData(pData, dataLength, 0, dataLength, (double) ts, true)) {
			FATAL("Unable to feed data");
			return false;
		}
	_stats.audio.bytesCount += dataLength;
	_stats.audio.packetsCount++;
	//INFO("Audio status: Packets Count %llu Bytes Count %llu", _stats.audio.packetsCount, _stats.audio.bytesCount);
	return true;
}
#endif	/* HAS_G711 */
bool InNetRTPStream::FeedAudioDataLATM(uint8_t *pData, uint32_t dataLength,
		RTPHeader &rtpHeader) {
	_stats.audio.packetsCount++;
	_stats.audio.bytesCount += dataLength;
	if (dataLength == 0)
		return true;
	uint32_t cursor = 0;
	uint32_t currentLength = 0;
	uint32_t index = 0;
	uint64_t rtpTs = ComputeRTP(rtpHeader._timestamp, _audioLastRTP, _audioRTPRollCount);
	double ts = (double) (rtpTs) / _audioSampleRate * 1000.00;
	double constant = 1024.0 / _audioSampleRate * 1000.00;

	while (cursor < dataLength) {
		currentLength = 0;
		while (cursor < dataLength) {
			uint8_t val = pData[cursor++];
			currentLength += val;
			if (val != 0xff)
				break;
		}
		if ((cursor + currentLength) > dataLength) {
			WARN("Invalid LATM packet size");
			return true;
		}
		if (!InternalFeedData(pData + cursor - 2,
				currentLength + 2,
				0,
				currentLength + 2,
				ts + (double) index * constant, true)) {
			FATAL("Unable to feed data");
			return false;
		}
		cursor += currentLength;
		//      FINEST("currentLength: %"PRIu32"; dataLength: %"PRIu32"; ts: %.2f",
		//              currentLength, dataLength, ts / 1000.0);
		index++;
	}

	return true;
}

//#define DEBUG_RTCP_PRESENCE(...) FINEST(__VA_ARGS__)
#define DEBUG_RTCP_PRESENCE(...)

//#define RTSP_DUMP_PTSDTS
#ifdef RTSP_DUMP_PTSDTS
double __lastPts = 0;
double __lastDts = 0;
#endif /* RTSP_DUMP_PTSDTS */

bool InNetRTPStream::InternalFeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, bool isAudio) {
	if ((!isAudio) && (dataLength > 0) && (_hasVideo)) {
		SavePts(pts); // stash timestamp for MetadataManager
		switch (NALU_TYPE(pData[0])) {
			case NALU_TYPE_SPS:
			{
				_pps.IgnoreAll();
				_sps.IgnoreAll();
				_sps.ReadFromBuffer(pData, dataLength);
				break;
			}
			case NALU_TYPE_PPS:
			{
				_pps.IgnoreAll();
				_pps.ReadFromBuffer(pData, dataLength);
				if ((GETAVAILABLEBYTESCOUNT(_sps) == 0) || (GETAVAILABLEBYTESCOUNT(_pps) == 0)) {
					_sps.IgnoreAll();
					_pps.IgnoreAll();
					break;
				}
				if (!_capabilities.AddTrackVideoH264(
						GETIBPOINTER(_sps),
						GETAVAILABLEBYTESCOUNT(_sps),
						GETIBPOINTER(_pps),
						GETAVAILABLEBYTESCOUNT(_pps),
						uint32_t(_videoSampleRate),
						_a,
						_b,
						true,
						this
						)) {
					_sps.IgnoreAll();
					_pps.IgnoreAll();
					WARN("Unable to initialize SPS/PPS for the video track");
				}
				_sps.IgnoreAll();
				_pps.IgnoreAll();
				break;
			}
			default:
			{
				break;
			}
		}
	}

	switch (_rtcpPresence) {
		case RTCP_PRESENCE_UNKNOWN:
		{
			DEBUG_RTCP_PRESENCE("RTCP_PRESENCE_UNKNOWN: %"PRIz"u", (time(NULL) - _rtcpDetectionStart));
			if (_rtcpDetectionInterval == 0) {
				WARN("RTCP disabled on stream %s. A/V drifting may occur over long periods of time",
						STR(*this));
				_rtcpPresence = RTCP_PRESENCE_ABSENT;
				return InternalFeedData(pData, dataLength, processedLength,
						totalLength, pts, isAudio);
			}
			if (_rtcpDetectionStart == 0) {
				_rtcpDetectionStart = time(NULL);
				break;
				//return true;
			}
			if ((time(NULL) - _rtcpDetectionStart) > _rtcpDetectionInterval) {
				WARN("Stream %s doesn't have RTCP. A/V drifting may occur over long periods of time",
						STR(*this));
				_rtcpPresence = RTCP_PRESENCE_ABSENT;
				return true;
			}
			bool audioRTCPPresent = false;
			bool videoRTCPPresent = false;
			if (_hasAudio) {
				//				if (_audioNTP != 0)
				//					DEBUG_RTCP_PRESENCE("Audio RTCP detected");
				audioRTCPPresent = (_audioNTP != 0);
			} else {
				audioRTCPPresent = true;
			}
			if (_hasVideo) {
				//				if (_videoNTP != 0)
				//					DEBUG_RTCP_PRESENCE("Video RTCP detected");
				videoRTCPPresent = (_videoNTP != 0);
			} else {
				videoRTCPPresent = true;
			}
			if (audioRTCPPresent && videoRTCPPresent) {
				DEBUG_RTCP_PRESENCE("RTCP available on stream %s", STR(*this));
				_rtcpPresence = RTCP_PRESENCE_AVAILABLE;
				return InternalFeedData(pData, dataLength, processedLength,
						totalLength, pts, isAudio);
			}
			//return true;
			break;
		}
		case RTCP_PRESENCE_AVAILABLE:
		{
			DEBUG_RTCP_PRESENCE("RTCP_PRESENCE_AVAILABLE");
			double &rtp = isAudio ? _audioRTP : _videoRTP;
			double &ntp = isAudio ? _audioNTP : _videoNTP;
			pts = ntp + pts - rtp;
			break;
		}
		case RTCP_PRESENCE_ABSENT:
		{
			DEBUG_RTCP_PRESENCE("RTCP_PRESENCE_ABSENT");
			double &firstTimestamp = isAudio ? _audioFirstTimestamp : _videoFirstTimestamp;
			if (firstTimestamp < 0)
				firstTimestamp = pts;
			pts -= firstTimestamp;
			break;
		}
		default:
		{
			ASSERT("Invalid _rtcpPresence: %"PRIu8, _rtcpPresence);
			return false;
		}
	}

	double dts = -1;
	if (isAudio) {
		dts = pts;
	} else {
		if (_videoLastPts == pts) {
			dts = _videoLastDts;
		} else {
			if (_dtsCacheSize == 1) {
				dts = pts;
			} else {
				_dtsCache[pts] = pts;
				if (_dtsCache.size() >= _dtsCacheSize) {
					map<double, double>::iterator i = _dtsCache.begin();
					dts = i->first;
					_dtsCache.erase(i);
				}
			}

#ifdef RTSP_DUMP_PTSDTS
            if (__lastPts != pts) {
                FINEST("pts: %8.2f\tdts: %8.2f\tcts: %8.2f\tptsd: %+6.2f\tdtsd: %+6.2f\t%s",
                        pts,
                        dts,
                        pts - dts,
                        pts - __lastPts,
                        dts - __lastDts,
                        pts == dts ? "" : "DTS Present");
                __lastPts = pts;
                __lastDts = dts;
            }
#endif /* RTSP_DUMP_PTSDTS */
		}
		
		if (_videoLastDts > dts) {
			FINE("_videoLastDts: %f, dts: %f", _videoLastDts, dts);
		}
		
		_videoLastPts = pts;
		_videoLastDts = dts;
	}

	if (dts < 0)
		return true;

	double &lastDts = isAudio ? _audioLastDts : _videoLastDts;
	//	if (!isAudio) {
	//		FINEST("last: %.2f; current: %.2f", lastDts, dts);
	//	}

	if (lastDts > dts) {
#ifdef DEBUG_TS
		WARN("Back time on %s. ATS: %.08f LTS: %.08f; D: %.8f; isAudio: %d; _dtsCacheSize: %"PRIz"u",
				STR(GetName()),
				dts,
				lastDts,
				dts - lastDts,
				isAudio, (size_t) (isAudio ? 0 : _dtsCacheSize));
#endif
		if (isAudio) {
			dts = lastDts;
		} else {
			_dtsCacheSize++;
			return true;
		}
	}
	lastDts = dts;

	if (isAudio) {
		if (_hasAudio && (_audioLastDts < 0))
			return true;
	} else {
		if (_hasVideo && (_videoLastDts < 0))
			return true;
	}

	// if NALU is a KEYFRAME
	// then cache it
	if ((NALU_TYPE(pData[0]) == NALU_TYPE_IDR) &&
		!isAudio) {
		_cachedKeyFrame.IgnoreAll();
		_cachedKeyFrame.ReadFromBuffer(pData, dataLength);
		_cachedDTS = dts;
		_cachedPTS = pts;
		_cachedProcLen = processedLength;
		_cachedTotLen = totalLength;
		//INFO("Keyframe Cache has been armed! SIZE=%d PTS=%lf", dataLength, pts);
	}

#ifdef PLAYBACK_SUPPORT

#if DUMP_G711
	fwrite((uint8_t *)pData, dataLength, 1, g711Fd);
	//INFO("Dumping G711 data to file");
#endif

#endif
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

uint64_t InNetRTPStream::ComputeRTP(uint32_t &currentRtp, uint32_t &lastRtp,
		uint32_t &rtpRollCount) {
	if (lastRtp > currentRtp) {
		if (((lastRtp >> 31) == 0x01) && ((currentRtp >> 31) == 0x00)) {
			FINEST("RTP roll over on for stream %s", STR(*this));
			rtpRollCount++;
		}
		FATAL("lastRtp: %"PRIu32", currentRtp: %"PRIu32, lastRtp, currentRtp);
	}
	lastRtp = currentRtp;
	return (((uint64_t) rtpRollCount) << 32) | currentRtp;
}

#endif /* HAS_PROTOCOL_RTP */

