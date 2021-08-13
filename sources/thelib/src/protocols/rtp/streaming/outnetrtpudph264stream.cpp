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



#ifdef HAS_PROTOCOL_RTP
#include "streaming/streamsmanager.h"
#include "protocols/rtp/streaming/outnetrtpudph264stream.h"
#include "streaming/nalutypes.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "streaming/codectypes.h"
#include "protocols/baseprotocol.h"
#include "protocols/rtp/rtspprotocol.h"

#define MAX_RTP_PACKET_SIZE 1350
#define MAX_AUS_COUNT 8
#define MAX_TSDELTAS_SIZE 200
#define MIN_TSDELTAS_SIZE 10

OutNetRTPUDPH264Stream::OutNetRTPUDPH264Stream(BaseProtocol *pProtocol,
	string name, bool forceTcp, bool zeroTimeBase, bool isSrtp)
: BaseOutNetRTPUDPStream(pProtocol, name, zeroTimeBase, true) {
	_forceTcp = forceTcp;
	_waitForIdr = false;
	Variant params = pProtocol->GetCustomParameters();
	if (params.HasKeyChain(V_BOOL, false, 1, "waitForIdr"))
		_waitForIdr = params.GetValue("waitForIdr", false);

	if (_forceTcp)
		_maxRTPPacketSize = 1500;
	else
		_maxRTPPacketSize = MAX_RTP_PACKET_SIZE;

	memset(&_videoData, 0, sizeof (_videoData));
	_videoData.MSGHDR_MSG_IOV = new IOVEC[2];
	_videoData.MSGHDR_MSG_IOVLEN = 2;
	_videoData.MSGHDR_MSG_NAMELEN = sizeof (sockaddr_in);
	_videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE = new IOVEC_IOV_BASE_TYPE[14];
	((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[0] = 0x80;
	EHTONLP(((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 8, _videoSsrc);
	_videoSampleRate = 0;

	memset(&_audioData, 0, sizeof (_audioData));
	_audioData.MSGHDR_MSG_IOV = new IOVEC[3];
	_audioData.MSGHDR_MSG_IOVLEN = 3;
	_audioData.MSGHDR_MSG_NAMELEN = sizeof (sockaddr_in);
	_audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN = 14;
	_audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE = new IOVEC_IOV_BASE_TYPE[14];
	((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[0] = 0x80;
//	((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = 0xe0;
	EHTONLP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 8, _audioSsrc);
	_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = 0;
	_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE = new IOVEC_IOV_BASE_TYPE[MAX_AUS_COUNT * 2];
	_audioSampleRate = 0;

	_auPts = -1;
	_auCount = 0;

	_pVideoInfo = NULL;
	_pAudioInfo = NULL;
	_firstVideoFrame = true;
	_lastVideoPts = -1;
	_audioPT = 96;
	_videoPT = 97;
	_isPlayerPaused = false;
	_inboundVod = false;
	_dtsOffset = 0;
	_ptsOffset = 0;
	_pauseDts = -1;
	_resumeDts = -1;
	_pausePts = -1;
	_resumePts = -1;
	_dtsNeedsUpdate = false;
	_aveTsDeltaVideo = -1;
	_aveTsDeltaAudio = -1;
	_lastTsVideo = 0;
	_lastTsAudio = 0;
	_tsOffset = 0;
	_videoResumeCount = 0;
#ifdef HAS_G711
	_firstAudioSample = true;
#endif	/* HAS_G711 */

	//ENABLE_LATENCY_LOGGING;
}

OutNetRTPUDPH264Stream::~OutNetRTPUDPH264Stream() {
	/* This is a good thought, but it causes a seg fault if the instream goes down first
	In reality, there should only be one out for any _inboundVod==true streams so resuming
	on destruction is not really necessary.  keeping the code here in case in some future
	situation this becomes necessary. Maybe check _inboundVod with the initial if(_pOrigInStream)
	would solve the crash?

	-- I will readd this part, now with safe checking if stream is still existing.
	It is now being checked thru unique ids, not using instream pointers -  */
	BaseStream* pOrigInStream = GetOrigInstream();
	BaseProtocol* pProtocol = NULL;
	if (pOrigInStream) {
		pProtocol = pOrigInStream->GetProtocol();
		//check if pInstream's protocol is RTSP, stream is a vodstream, and is paused,
		//send resume message so that instream still continues its inbound process
		if (pProtocol && pProtocol->GetType() == PT_RTSP
			&& _inboundVod	&& pOrigInStream->IsPaused()) {
			pOrigInStream->SignalResume();
		}
	}

	delete[] (uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE;
	delete[] _videoData.MSGHDR_MSG_IOV;
	memset(&_videoData, 0, sizeof (_videoData));

	delete[] (uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE;
	delete[] (uint8_t *) _audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE;
	delete[] _audioData.MSGHDR_MSG_IOV;
	memset(&_audioData, 0, sizeof (_audioData));
}

bool OutNetRTPUDPH264Stream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutNetRTPUDPStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}
	if (pGenericProcessDataSetup->_hasVideo) {
		_pVideoInfo = pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfo > ();
		_videoSampleRate = _pVideoInfo->_samplingRate;
	} else {
		_videoSampleRate = 1;
	}

	if (pGenericProcessDataSetup->_hasAudio) {
		_pAudioInfo = pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfo > ();
		_audioSampleRate = _pAudioInfo->_samplingRate;
	} else {
		_audioSampleRate = 1;
	}
	return true;
}
void OutNetRTPUDPH264Stream::SetSSRC(uint32_t value, bool isAudio) {
	uint32_t &ssrc = isAudio ? _audioSsrc : _videoSsrc;
	ssrc = value;
	MSGHDR &data = isAudio ? _audioData : _videoData;
	EHTONLP(((uint8_t *)data.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 8, value);
}
void OutNetRTPUDPH264Stream::SetRTPBlockSize(uint32_t blockSize) {
	_maxRTPPacketSize = min(_maxRTPPacketSize, blockSize);
}
bool OutNetRTPUDPH264Stream::SendMetadata(Variant sdpMetadata, int64_t pts) {
	string data = "";
	sdpMetadata.SerializeToJSON(data, true);

	return PushMetaData(data, pts);
}
bool OutNetRTPUDPH264Stream::PushMetaData(string const& vmfMetadata, int64_t pts) {
	return _pConnectivity->PushMetaData(vmfMetadata, pts);
}

bool OutNetRTPUDPH264Stream::SendMetadata(int64_t pts) {
	Variant &streamConfig = GetProtocol()->GetCustomParameters();
	if (!streamConfig.HasKeyChain(V_MAP, false, 1, "_metadata")) {
		return false;
	}
	if (!SendMetadata(streamConfig["_metadata"], pts))
		return false;
	if (!streamConfig.HasKeyChain(V_MAP, false, 1, "_metadata2")) {
		return false;
	}
	if (!SendMetadata(streamConfig["_metadata2"], pts))
		return false;

	BaseInStream* pOrigInStream = (BaseInStream*)GetOrigInstream();
	if (pOrigInStream) {
		BaseProtocol* pProtocol = pOrigInStream->GetProtocol();
		if (pProtocol && pProtocol->GetType() == PT_RTSP) {
			Variant &playNotifyReq = ((RTSPProtocol*)pProtocol)->GetPlayNotifyReq();
			if (playNotifyReq != V_NULL) {
				SendMetadata(playNotifyReq, (int64_t)pts);
				// removes the content once sent, should refresh once a new play notify req is received
				playNotifyReq.Reset();
			}
		}
	}

	streamConfig.RemoveKey("_metadata");
	streamConfig.RemoveKey("_metadata2");
	return true;
}

bool OutNetRTPUDPH264Stream::PushVideoData(IOBuffer &buffer, double pts, double dts,
 		bool isKeyFrame) {
	return PushVideoData(buffer, pts, dts, isKeyFrame, false);
}

bool OutNetRTPUDPH264Stream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame, bool isParamSet) {
	//dump data if player is paused and it is a livestream

	SendMetadata((int64_t)pts);
	if (_pVideoInfo == NULL || (_isPlayerPaused && !_inboundVod)) {
		if (_isPlayerPaused && !_inboundVod && _dtsNeedsUpdate) {
			StorePauseTs(dts, pts);
		}
		_stats.video.droppedPacketsCount++;
		_stats.video.droppedBytesCount += GETAVAILABLEBYTESCOUNT(buffer);
		LOG_LATENCY("RTP Dropped Video Packet|%0.3f", pts);
		return true;
	}
	if (_waitForIdr && !isKeyFrame && _firstVideoFrame) {
		LOG_LATENCY("RTP Dropped Video Packet|%0.3f", pts);
		return true;
	}
	if ((isKeyFrame || _firstVideoFrame)
			&&(_pVideoInfo->_type == CODEC_VIDEO_H264)
			&&(_lastVideoPts != pts)) {
		_firstVideoFrame = false;
		//fix for mode=0 kind of transfer where we get sliced IDRs
		//only send the SPS/PPS on the first IDR slice from the keyframe
		_lastVideoPts = pts;
		VideoCodecInfoH264 *pTemp = (VideoCodecInfoH264 *) _pVideoInfo;
		if (!PushVideoData(pTemp->GetSPSBuffer(), dts, dts, false, true)) {
			FATAL("Unable to feed SPS");
			return false;
		}
		if (!PushVideoData(pTemp->GetPPSBuffer(), dts, dts, false, true)) {
			FATAL("Unable to feed PPS");
			return false;
		}
	}
	if (_dtsNeedsUpdate && !_isPlayerPaused) {
		//INFO("Toggle Resume video");
		StoreResumeTs(dts, pts);
		UpdateTsOffsetsAndResetTs();
	}
	dts = dts - _dtsOffset;
	pts = pts - _ptsOffset;

	//Handling of bloated dts's during resume in inbound vod streams
	if (_inboundVod) {
		double tsDelta = dts - _lastTsVideo;
		if (tsDelta != 0 ) {
			//don't store ts if it is too large or too small (outlier)
			//store tts whatever its value if the number of ts delta samples is still too small
			if (_aveTsDeltaVideo == -1
				|| _tsDeltasVideo.size() < MIN_TSDELTAS_SIZE ||
				(tsDelta > (0.5 * _aveTsDeltaVideo) && tsDelta < (2 * _aveTsDeltaVideo))) {
				_tsDeltasVideo.push(tsDelta);
				if (_tsDeltasVideo.size() > MAX_TSDELTAS_SIZE)
					_tsDeltasVideo.pop();

				//compute for the new average dts delta
				queue<double> tsDeltaTemp = _tsDeltasVideo;
				double sum = 0;
				while (!tsDeltaTemp.empty()) {
					sum += tsDeltaTemp.front();
				tsDeltaTemp.pop();
				}
				_aveTsDeltaVideo = (double)(sum / (double)_tsDeltasVideo.size());
			} else if (tsDelta >= 2 * _aveTsDeltaVideo) {
				//if the delta dts is greater than 2x of ave delta dts
				_videoResumeCount++;
				_tsOffset += tsDelta + (_videoResumeCount * _aveTsDeltaVideo);				
			}
			_lastTsVideo = dts;
		}
		dts = dts - _tsOffset;
		pts = pts - _tsOffset;
	}

	// https://tools.ietf.org/html/rfc3550#section-5.1

	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(buffer);
	uint8_t *pData = GETIBPOINTER(buffer);

	uint32_t sentAmount = 0;
	uint32_t chunkSize = 0;
	//uint16_t firstPid = _videoCounter;
	//uint16_t lastPid = _videoCounter;
	//DEBUG("firstPid = %d\n",firstPid);
	//DEBUG("lastPid  = %d\n",lastPid);
	if (isKeyFrame) {
		_videoPacketBuffer.Clear(); // retain only one keyframe
//		WARN("Packet buffer cleared");
	}
	while (sentAmount != dataLength) {
		uint16_t pid = _videoCounter;
		chunkSize = dataLength - sentAmount;
		chunkSize = chunkSize < _maxRTPPacketSize ? chunkSize : _maxRTPPacketSize;

		//1. Flags
		/*
		if (sentAmount + chunkSize == dataLength) {
			((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = 0xe1;
		} else {
			((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = 0x61;
		}
		*/
		uint8_t tmp = _videoPT;
		if ((sentAmount + chunkSize == dataLength) && !isParamSet) {
			// Only set the marker bit if this is NOT an SPS/PPS
			tmp |= 0x80;
		}

		((uint8_t *)_videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = tmp;
		
			//2. counter
		EHTONSP(((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 2, _videoCounter);
		_videoCounter++;

		//3. Timestamp
		EHTONLP(((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 4,
				BaseConnectivity::ToRTPTS(pts, (uint32_t) _videoSampleRate));
		string chunkPart = "";
		//uint8_t naluType = NALU_TYPE(pData[0]);
		if (chunkSize == dataLength) {
			//4. No chunking
			_videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN = 12;
			_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) pData;
			_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = chunkSize;
			chunkPart = "WHOLE";
		} else {
			//5. Chunking
			_videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN = 14;

			if (sentAmount == 0) {
				//6. First chunk
				((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[12] = (pData[0]&0xe0) | NALU_TYPE_FUA;
				((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[13] = (pData[0]&0x1f) | 0x80;
				_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) (pData + 1);
				_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = chunkSize - 1;
				chunkPart = "FIRST";
			} else {
				if (sentAmount + chunkSize == dataLength) {
					//7. Last chunk
					((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[13] &= 0x1f;
					((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[13] |= 0x40;
					chunkPart = "END";
				}
				else {
					//8. Middle chunk
					((uint8_t *) _videoData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[13] &= 0x1f;
					chunkPart = "MID";
				}
				_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) pData;
				_videoData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = chunkSize;
			}
		}
		LOG_LATENCY("RTP Chunk|Video|SSRC:%"PRIu32"|dts: %0.3f|%s|%s", _videoSsrc, pts, STR(chunkPart), STR(NALUToString(naluType)));
		if (isKeyFrame) {
			//lastPid = pid;
			_videoPacketBuffer.PushPacket(pid, _videoData);
		}
		_pConnectivity->FeedVideoData(_videoData, pts, dts);
		sentAmount += chunkSize;
		pData += chunkSize;
	}
	if (isKeyFrame) {
//		DEBUG("Packet buffer has %"PRIu32" frames. [%"PRIu16" - %"PRIu16"]", _videoPacketBuffer.GetBufferSize(), firstPid, lastPid);
	}
	_stats.video.packetsCount++;
	_stats.video.bytesCount += GETAVAILABLEBYTESCOUNT(buffer);
	return true;
}

//#define MULTIPLE_AUS

bool OutNetRTPUDPH264Stream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	SendMetadata((int64_t)pts);

	if (_pAudioInfo == NULL || (_isPlayerPaused && !_inboundVod)) {
		/*if (_isPlayerPaused && !_inboundVod && _dtsNeedsUpdate) {
			StorePauseTs(dts, pts);
		}*/
		_stats.audio.droppedPacketsCount++;
		_stats.audio.droppedBytesCount += GETAVAILABLEBYTESCOUNT(buffer);
		LOG_LATENCY("RTP Dropped Audio Packet|%0.3f", pts);
		return true;
	}
	if (_waitForIdr && _firstVideoFrame) {
		LOG_LATENCY("RTP Dropped Audio Packet|%0.3f", pts);
		return true;
	}
	//commented out for the moment, might edit once audio in srtp is fixed.
	/*if (_dtsNeedsUpdate && !_isPlayerPaused) {
		//INFO("Toggle resume audio");
		StoreResumeTs(dts, pts);
		UpdateTsOffsetsAndResetTs();
	}*/
	dts = dts - _dtsOffset;
	pts = pts - _ptsOffset;
	/*if (_inboundVod) {
		double tsDelta = dts - _lastTsAudio;
		//don't store ts if it is too large or too small (outlier)
		//store tts whatever its value if the number of ts delta samples is still too small
		if (tsDelta != 0) {
			if (_aveTsDeltaAudio == -1
				|| _tsDeltasAudio.size() < MIN_TSDELTAS_SIZE ||
				(tsDelta > (0.5 * _aveTsDeltaAudio) && tsDelta < (2 * _aveTsDeltaAudio))) {
				_tsDeltasAudio.push(tsDelta);
				if (_tsDeltasAudio.size() > MAX_TSDELTAS_SIZE)
					_tsDeltasAudio.pop();

				//compute for the new average dts delta
				queue<double> tsDeltaTemp = _tsDeltasAudio;
				double sum = 0;
				while (!tsDeltaTemp.empty()) {
					sum += tsDeltaTemp.front();
					tsDeltaTemp.pop();
				}
				_aveTsDeltaAudio = (double)(sum / (double)_tsDeltasAudio.size());
			} else if (tsDelta >= 2 * _aveTsDeltaAudio) {
				//if the delta dts is greater than 2x of ave delta dts
				dts = _lastTsAudio + _aveTsDeltaAudio;
				pts = _lastTsAudio + _aveTsDeltaAudio;
			}
			_lastTsAudio = pts;
		}
	}*/	

	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(buffer);
	uint8_t *pData = GETIBPOINTER(buffer);

	((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = 0x80 | _audioPT;

#ifdef MULTIPLE_AUS
	//	FINEST("_auCount: %"PRIu32"; max: %"PRIu32"; have: %"PRIu32"; total: %"PRIu32,
	//			_auCount,
	//			MAX_AUS_COUNT,
	//			12 //RTP header
	//			+ 2 //AU-headers-length
	//			+ _auCount * 2 //n instances of AU-header
	//			+ GETAVAILABLEBYTESCOUNT(_auBuffer), //existing data
	//
	//			12 //RTP header
	//			+ 2 //AU-headers-length
	//			+ _auCount * 2 //n instances of AU-header
	//			+ GETAVAILABLEBYTESCOUNT(_auBuffer) //existing data
	//			+ dataLength
	//			);
	if ((_auCount >= MAX_AUS_COUNT)
			|| ((
			12 //RTP header
			+ 2 //AU-headers-length
			+ _auCount * 2 //n instances of AU-header
			+ GETAVAILABLEBYTESCOUNT(_auBuffer) //existing data
			+ dataLength) //new data about to be appended
			> MAX_RTP_PACKET_SIZE)) {

		//5. counter
		EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 2, _audioCounter);
		_audioCounter++;

		//6. Timestamp
		EHTONLP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 4,
				(uint32_t) (_auPts * _audioSampleRate / 1000.000));

		//7. AU-headers-length
		EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 12, _auCount * 16);

		//7. put the actual buffer
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_LEN = GETAVAILABLEBYTESCOUNT(_auBuffer);
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) (GETIBPOINTER(_auBuffer));

		//8. Send the data
		//FINEST("-----SEND------");
		if (!_pConnectivity->FeedAudioData(_audioData, pts, dts)) {
			FATAL("Unable to feed data");
			return false;
		}

		_auCount = 0;
	}

	//9. reset the pts and au buffer if this is the first AU
	if (_auCount == 0) {
		_auBuffer.IgnoreAll();
		_auPts = pts;
		_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = 0;
	}

	//10. Store the data
	_auBuffer.ReadFromBuffer(pData, dataLength);

	//11. Store the AU-header
	uint16_t auHeader = (uint16_t) ((dataLength) << 3);
	EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE + 2 * _auCount), auHeader);
	_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN += 2;

	//12. increment the number of AUs
	_auCount++;

	_stats.audio.packetsCount++;
	_stats.audio.bytesCount += GETAVAILABLEBYTESCOUNT(buffer);

	//13. Done
	return true;

#else /* MULTIPLE_AUS */
/* Table from RFC3550 */
	/*
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
   |AU-headers-length|AU-header|AU-header|      |AU-header|padding|
   |                 |   (1)   |   (2)   |      |   (n)   | bits  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
	 */

	//5. counter
	EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 2, _audioCounter);
	_audioCounter++;

	//6. Timestamp
	EHTONLP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 4,
			BaseConnectivity::ToRTPTS(pts, (uint32_t) _audioSampleRate));
#ifdef HAS_G711
	uint64_t codecType = GetCapabilities()->GetAudioCodecType();
	if (codecType == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
		//7. AU-headers-length
		EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 12, 16);
	
		//8. AU-header
		uint16_t auHeader = (uint16_t) ((dataLength) << 3);
		EHTONSP(((uint8_t *) _audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_BASE), auHeader);
		_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = 2;
		//7. put the actual buffer
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_LEN = dataLength;
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) (pData);
#ifdef HAS_G711
	} else {
		uint8_t pt = (codecType == CODEC_AUDIO_G711U ? 0 : 8);
		((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = pt;
		if (_firstAudioSample) {
			((uint8_t *) _audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] |= 0x80;
			_firstAudioSample = false;
		}
		
		_audioData.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN = 12;
		_audioData.MSGHDR_MSG_IOV[1].IOVEC_IOV_LEN = 0;
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_LEN = dataLength;
		_audioData.MSGHDR_MSG_IOV[2].IOVEC_IOV_BASE = (IOVEC_IOV_BASE_TYPE *) (pData);
	}
#endif	/* HAS_G711 */
	LOG_LATENCY("RTP Chunk|Audio|SSRC:%"PRIu32"|dts: %0.3f", _audioSsrc, pts);
	if (!_pConnectivity->FeedAudioData(_audioData, pts, dts)) {
		FATAL("Unable to feed data");
		return false;
	}

	_stats.audio.packetsCount++;
	_stats.audio.bytesCount += GETAVAILABLEBYTESCOUNT(buffer);

	return true;
#endif /* MULTIPLE_AUS */
}

bool OutNetRTPUDPH264Stream::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264)
			|| (codec == CODEC_AUDIO_AAC)
#ifdef HAS_G711
			|| ((codec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
#endif	/* HAS_G711 */
			;
}

void OutNetRTPUDPH264Stream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
	if ((pNew == NULL) || (!IsCodecSupported(pNew->_type))) {
		_pAudioInfo = NULL;
		_audioSampleRate = 1;
	}
	_pAudioInfo = pNew;
	_audioSampleRate = (_pAudioInfo != NULL) ? _pAudioInfo->_samplingRate : 0;
}

void OutNetRTPUDPH264Stream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
	if ((pNew == NULL) || (!IsCodecSupported(pNew->_type))) {
		_pVideoInfo = NULL;
		_videoSampleRate = 1;
	}
	_pVideoInfo = pNew;
	_firstVideoFrame = true;
	_videoSampleRate = (_pVideoInfo != NULL) ? _pVideoInfo->_samplingRate : 0;
}

void OutNetRTPUDPH264Stream::SignalAttachedToInStream() {
}

uint8_t OutNetRTPUDPH264Stream::GetPayloadType(bool isAudio) {
	return isAudio ? _audioPT : _videoPT;
}
void OutNetRTPUDPH264Stream::SetPayloadType(uint8_t pt, bool isAudio) {
	uint8_t &newPt = isAudio ? _audioPT : _videoPT;
	newPt = pt;
}

void OutNetRTPUDPH264Stream::UpdatePauseStatus(bool newPauseStatus) {
	_isPlayerPaused = newPauseStatus;
	if (!_inboundVod)
		_dtsNeedsUpdate = true;
	BaseStream* pOrigInStream = GetOrigInstream();
	if (newPauseStatus == false) {
		if (pOrigInStream && _inboundVod && pOrigInStream->IsPaused()) {
			pOrigInStream->SignalResume();
		}
	} else {
		//this flag will be turned to false again so that outbound stream will look for keyframe first when stream resumes
		_keyframeCacheConsumed = false;
		if (pOrigInStream && _inboundVod) {
			pOrigInStream->SignalPause();
		}
	}
}

bool OutNetRTPUDPH264Stream::SignalPause() {
	UpdatePauseStatus(true);
	_pConnectivity->SignalPause();
	return true;
}

bool OutNetRTPUDPH264Stream::SignalResume() {
	UpdatePauseStatus(false);
	_pConnectivity->SignalResume();
	return true;
}

void OutNetRTPUDPH264Stream::StorePauseTs(double dts, double pts) {
	_pauseDts = dts;
	_pausePts = pts;
	_dtsNeedsUpdate = false;
}

void OutNetRTPUDPH264Stream::StoreResumeTs(double dts, double pts) {
	_resumeDts = dts;
	_resumePts = pts;
	_dtsNeedsUpdate = false;
}

void OutNetRTPUDPH264Stream::UpdateTsOffsetsAndResetTs() {
	_dtsOffset += _resumeDts - _pauseDts;
	_ptsOffset += _resumePts - _pausePts;
	_resumeDts = -1;
	_pauseDts = -1;
	_resumePts = -1;
	_pausePts = -1;
}

void OutNetRTPUDPH264Stream::SetInboundVod(bool inboundVod) {
	_inboundVod = inboundVod;
}

bool OutNetRTPUDPH264Stream::GenericProcessData(
	uint8_t * pData, uint32_t dataLength,
	uint32_t processedLength, uint32_t totalLength,
	double pts, double dts,
	bool isAudio)
{
	bool result = false;
	BaseInStream* pOrigInStream = (BaseInStream*) GetOrigInstream();
	// We've got a cached keyframe?
	// pData is not a keyframe?
	// Cache hasn't been used yet?
	//
	// IF yes to all, then let's send out the cached keyframe first.
	if (pOrigInStream && pOrigInStream->_hasCachedKeyFrame() &&
		(NALU_TYPE(pData[0]) != NALU_TYPE_IDR) &&
		!_keyframeCacheConsumed &&
		!_isPlayerPaused &&
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
	} else if (NALU_TYPE(pData[0]) == NALU_TYPE_IDR && !_keyframeCacheConsumed && !_isPlayerPaused) {
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
#endif /* HAS_PROTOCOL_RTP */

