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
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "streaming/baseinstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/streaming/innetrtmpstream.h"
#include "protocols/rtmp/streaming/infilertmpstream.h"
#include "protocols/rtmp/messagefactories/streammessagefactory.h"
#include "protocols/rtmp/messagefactories/genericmessagefactory.h"
#include "streaming/streamstypes.h"
#include "protocols/liveflv/innetliveflvstream.h"
#include "application/baseclientapplication.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "protocols/rtmp/streaming/rtmpplaylistitem.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/rtmpmessagefactory.h"
#include "streaming/nalutypes.h"

//#define TRACK_HEADER(header,channel) do{if(channel->lastOutProcBytes==0) FINEST("%s; %.2f",STR(header),channel->lastOutAbsTs);}while(0)
#define TRACK_HEADER(header,channel)

//#define TRACK_PAYLOAD(processedLength,dataLength,pData) do {if (processedLength == 0) {uint32_t ___temp = dataLength > 32 ? 32 : dataLength; FINEST("%s", STR(hex(pData, ___temp)));}} while (0)
#define TRACK_PAYLOAD(processedLength,dataLength,pData)

//if needed, we can simulate dropping frames
//the number represents the percent of frames that we will drop (0-100)
//#define SIMULATE_DROPPING_FRAMES 10

OutNetRTMPStream::OutNetRTMPStreamTimer::OutNetRTMPStreamTimer(
		OutNetRTMPStream *pOutNetRTMPStream) {
	_pOutNetRTMPStream = pOutNetRTMPStream;
}

OutNetRTMPStream::OutNetRTMPStreamTimer::~OutNetRTMPStreamTimer() {

}

void OutNetRTMPStream::OutNetRTMPStreamTimer::ResetStream() {
	_pOutNetRTMPStream = NULL;
}

bool OutNetRTMPStream::OutNetRTMPStreamTimer::TimePeriodElapsed() {
	if (_pOutNetRTMPStream != NULL)
		_pOutNetRTMPStream->SignalStreamCompleted();
	return true;
}

void OutNetRTMPStream::TrackState::Init(BaseRTMPProtocol *pProtocol,
		uint8_t type, uint32_t rtmpStreamId) {
	pChannel = pProtocol->ReserveChannel();
	isFirstFrame = true;
	bucket.IgnoreAll();
	currentFrameDropped = false;
	zeroBase = -1;
	H_CI(header) = pChannel->id;
	H_MT(header) = type;
	H_SI(header) = rtmpStreamId;
	header.readCompleted = 0;
}

OutNetRTMPStream::OutNetRTMPStream(BaseProtocol *pProtocol,
		RTMPPlaylistItem *pPlaylistItem, string name, uint32_t rtmpStreamId,
		uint32_t chunkSize)
: BaseOutNetStream(pProtocol, ST_OUT_NET_RTMP, name) {
	_pRTMPProtocol = (BaseRTMPProtocol *) pProtocol;
	_pPlaylistItem = pPlaylistItem;
	_rtmpStreamId = rtmpStreamId;
	_chunkSize = chunkSize;
	_feederChunkSize = 0xffffffff;
	_canDropFrames = true;
	_directFeed = true;
	_clientId = _pRTMPProtocol->GetClientId();
	_absoluteTimelineHead = 0;
	if ((pProtocol != NULL)
			&& (pProtocol->GetApplication() != NULL)
			&& (pProtocol->GetApplication()->GetConfiguration().HasKeyChain(_V_NUMERIC, false, 1, "maxRtmpOutBuffer")))
		_maxBufferSize = (uint32_t) pProtocol->GetApplication()->GetConfiguration().GetValue("maxRtmpOutBuffer", false);
	else
		_maxBufferSize = 65536 * 2;

	_playbackLength = -1;
	_singleGop = false;
	_paused = false;
	_timerId = 0;
	_audio.Init(_pRTMPProtocol, RM_HEADER_MESSAGETYPE_AUDIODATA, _rtmpStreamId);
	_video.Init(_pRTMPProtocol, RM_HEADER_MESSAGETYPE_VIDEODATA, _rtmpStreamId);
	_zeroBase = -1;
	_independentZeroBase = true;
	_enableAudio = true;
	_enableVideo = true;
}

OutNetRTMPStream *OutNetRTMPStream::GetInstance(BaseProtocol *pProtocol,
		RTMPPlaylistItem *pPlaylistItem, StreamsManager *pStreamsManager,
		string name, uint32_t rtmpStreamId, uint32_t chunkSize) {
	OutNetRTMPStream *pResult = new OutNetRTMPStream(pProtocol,
			pPlaylistItem, name, rtmpStreamId, chunkSize);
	if ((pResult->_audio.pChannel == NULL)
			|| (pResult->_video.pChannel == NULL)
			|| (!pResult->SetStreamsManager(pStreamsManager))) {
		delete pResult;
		return NULL;
	}

	return pResult;
}

OutNetRTMPStream::~OutNetRTMPStream() {
	GetEventLogger()->LogStreamClosed(this);
	DisarmTimer();
	_pRTMPProtocol->ReleaseChannel(_audio.pChannel);
	_pRTMPProtocol->ReleaseChannel(_video.pChannel);
}

bool OutNetRTMPStream::ResendMetadata() {
	if (_pPlaylistItem == NULL)
		return true;
	return _pPlaylistItem->ResendMetadata();
}

bool OutNetRTMPStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_FILE_RTMP)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_EXT);
}

bool OutNetRTMPStream::IsCodecSupported(uint64_t codec) {
	return true;
}

bool OutNetRTMPStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_SIZE, //naluMarkerType,
			false, //insertPDNALU,
			true, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
#ifdef HAS_G711
	StreamCapabilities *pCaps = GetCapabilities();
	if (pCaps != NULL && 
			((pCaps->GetAudioCodecType() & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)) {
		pGenericProcessDataSetup->audio.g711._aLaw = 
				(pCaps->GetAudioCodecType() == CODEC_AUDIO_G711A ? true : false);
	} else {
#endif	/* HAS_G711 */
		pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
		pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = true;
#ifdef HAS_G711
	}
#endif	/* HAS_G711 */

	//misc setup
	pGenericProcessDataSetup->_timeBase = 0;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

	return true;
}

bool OutNetRTMPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
//	WARN("[Debug] Data being fed to %s [%s]", STR(GetName()), _directFeed ? "InternalFeedData" : "GenericProcessData");
	return _directFeed ?
			InternalFeedData(pData, dataLength, processedLength, totalLength, dts, isAudio)
			: GenericProcessData(pData, dataLength, processedLength, totalLength, pts, dts, isAudio);
}

bool OutNetRTMPStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	return InternalFeedData(
			GETIBPOINTER(buffer), //pData
			GETAVAILABLEBYTESCOUNT(buffer), //dataLength
			0, //processedLength
			GETAVAILABLEBYTESCOUNT(buffer), //totalLength
			dts, //dts
			false //isAudio
			);
}

bool OutNetRTMPStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	return InternalFeedData(
			GETIBPOINTER(buffer), //pData
			GETAVAILABLEBYTESCOUNT(buffer), //dataLength
			0, //processedLength
			GETAVAILABLEBYTESCOUNT(buffer), //totalLength
			dts, //dts
			true //isAudio
			);
}

void OutNetRTMPStream::SetAbsoluteTimelineHead(double absoluteTimelineHead) {
	_absoluteTimelineHead = absoluteTimelineHead;
	_zeroBase = -1;
	_audio.zeroBase = -1;
	H_IA(_audio.header) = true;
	H_TS(_audio.header) = (uint32_t) absoluteTimelineHead;
	H_ML(_audio.header) = 0;
	ChunkAndSend(NULL, 0, _audio); //_pRTMPProtocol->SendRawData(_audio.header, *_audio.pChannel, NULL, 0);

	_video.zeroBase = -1;
	H_IA(_video.header) = true;
	H_TS(_video.header) = (uint32_t) absoluteTimelineHead;
	StreamCapabilities *pCapabilities = NULL;
	if ((GetInStream() != NULL)
			&& TAG_KIND_OF(GetInStream()->GetType(), ST_IN_FILE)
			&& ((pCapabilities = GetCapabilities()) != NULL)
			&& (pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)
			) {
		uint8_t tmp[2] = {0x57, 0x00};
		H_ML(_video.header) = sizeof (tmp);
		ChunkAndSend(tmp, 2, _video); //_pRTMPProtocol->SendRawData(_video.header, *_video.pChannel, NULL, 0);
	} else {
		H_ML(_video.header) = 0;
		ChunkAndSend(NULL, 0, _video); //_pRTMPProtocol->SendRawData(_video.header, *_video.pChannel, NULL, 0);
	}
}

void OutNetRTMPStream::SetPlaybackLength(double length, bool singleGop) {
	_playbackLength = length;
	_singleGop = singleGop;
	if (_singleGop)
		return;
	if (_playbackLength >= 0)
		ArmTimer(_playbackLength);
}

void OutNetRTMPStream::SetChunkSize(uint32_t chunkSize) {
	_chunkSize = chunkSize;
}

uint32_t OutNetRTMPStream::GetChunkSize() {
	return _chunkSize;
}

void OutNetRTMPStream::SetFeederChunkSize(uint32_t feederChunkSize) {
	_feederChunkSize = feederChunkSize;
}

void OutNetRTMPStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseOutNetStream::GetStats(info, namespaceId);
	info["canDropFrames"] = (bool)_canDropFrames;
	Variant &protoConfig = _pProtocol->GetCustomParameters();
	if (protoConfig.HasKeyChain(V_STRING, false, 1, "tcUrl"))
		info["tcUrl"] = protoConfig["tcUrl"];
	if (protoConfig.HasKeyChain(V_STRING, false, 1, "swfUrl"))
		info["swfUrl"] = protoConfig["swfUrl"];
	if (protoConfig.HasKeyChain(V_STRING, false, 1, "pageUrl"))
		info["pageUrl"] = protoConfig["pageUrl"];
}

bool OutNetRTMPStream::SendStreamMessage(Variant &message) {
	//1. Set the channel id
	VH_CI(message) = _video.pChannel->id;

	//2. Reset the timer
	VH_TS(message) = (uint32_t) 0;

	//3. Set as relative ts
	VH_IA(message) = false;

	//4. Set the stream id
	VH_SI(message) = _rtmpStreamId;

	//5. Send it
	return _pRTMPProtocol->SendMessage(message);
}

void OutNetRTMPStream::SignalAttachedToInStream() {
	//1. Store the attached stream type to know how we should proceed on detach
	_directFeed = ((_pInStream->GetType() == ST_IN_NET_RTMP)
			|| (_pInStream->GetType() == ST_IN_FILE_RTMP));

	//2. Mirror the feeder chunk size
	if (TAG_KIND_OF(_pInStream->GetType(), ST_IN_NET_RTMP)) {
		_feederChunkSize = ((InNetRTMPStream *) _pInStream)->GetChunkSize();
	} else if (TAG_KIND_OF(_pInStream->GetType(), ST_IN_FILE)) {
		_feederChunkSize = ((InFileRTMPStream *) _pInStream)->GetChunkSize();
	} else {
		_feederChunkSize = 0xffffffff;
	}

	_canDropFrames = !(TAG_KIND_OF(_pInStream->GetType(), ST_IN_FILE));
	_independentZeroBase = (_pInStream->GetType() == ST_IN_FILE_RTMP);
}

void OutNetRTMPStream::SignalDetachedFromInStream() {
	if (_playbackLength > 0) {
		double totalSent = GetLastSentTimestamp() - _absoluteTimelineHead;
		_playbackLength = _playbackLength - totalSent;
		ArmTimer(_playbackLength);
	}
	SetAbsoluteTimelineHead(GetLastSentTimestamp());
	_audio.isFirstFrame = true;
	_video.isFirstFrame = true;
}

bool OutNetRTMPStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutNetRTMPStream::SignalPause() {
	DisarmTimer();
	_paused = true;
	return true;
}

bool OutNetRTMPStream::SignalResume() {
	if (_playbackLength > 0) {
		double totalSent = GetLastSentTimestamp() - _absoluteTimelineHead;
		_playbackLength = _playbackLength - totalSent;
		if (_playbackLength <= 0) {
			SignalStreamCompleted();
			return true;
		}
		ArmTimer(_playbackLength);
	}
	SetAbsoluteTimelineHead(GetLastSentTimestamp());
	_audio.isFirstFrame = true;
	_video.isFirstFrame = true;
	_paused = false;
	return true;
}

bool OutNetRTMPStream::SignalSeek(double &dts) {
	NYIA;
	return false;
}

bool OutNetRTMPStream::SignalStop() {
	NYIA;
	return false;
}

bool OutNetRTMPStream::SignalStreamPublished(string streamName) {
	return _pRTMPProtocol->SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayPublishNotify(
			_rtmpStreamId, 0, false, _clientId, streamName));
}

void OutNetRTMPStream::SignalStreamCompleted() {
	DisarmTimer();
	if (_pPlaylistItem != NULL) {
		_pPlaylistItem->SignalStreamCompleted(GetLastSentTimestamp());
	}
}

void OutNetRTMPStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if (pCapabilities == NULL)
		return;
	if (!FeedAudioCodecBytes(pCapabilities, 0, false)) {
		FATAL("Unable to feed audio codec bytes");
		_pRTMPProtocol->EnqueueForDelete();
	}
}

void OutNetRTMPStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	if (pCapabilities == NULL)
		return;
	if (!FeedVideoCodecBytes(pCapabilities, 0, false)) {
		FATAL("Unable to feed video codec bytes");
		_pRTMPProtocol->EnqueueForDelete();
	}
}

double OutNetRTMPStream::GetLastSentTimestamp() {
	return _audio.pChannel->lastOutAbsTs > _video.pChannel->lastOutAbsTs ?
			_audio.pChannel->lastOutAbsTs
			: _video.pChannel->lastOutAbsTs;
}

bool OutNetRTMPStream::InternalFeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double dts, bool isAudio) {
	//	if(dts>20000)
	//		return false;
	if (isAudio && !_enableAudio)
		return true;
	if (!isAudio && !_enableVideo)
		return true;
	if (_paused || (_singleGop && isAudio))
		return true;

	TrackState &ts = isAudio ? _audio : _video;
	BaseStream::BaseStreamStats::BaseStreamTrackStats &stats = isAudio ? _stats.audio : _stats.video;
	if (!UseSourcePts()) {
		double &zeroBase = _independentZeroBase ? ts.zeroBase : _zeroBase;
		if (zeroBase < 0)
			zeroBase = dts;
		dts -= zeroBase;
	}
	dts = (dts < 0) ? _absoluteTimelineHead : (dts + _absoluteTimelineHead);

	//1. Update the stats
	if (processedLength == 0)
		stats.packetsCount++;
	stats.bytesCount += dataLength;

	if (ts.isFirstFrame) {
		ts.currentFrameDropped = false;
		StreamCapabilities *pCapabilities = NULL;
		if ((dataLength == 0)
				|| (processedLength != 0)
				|| ((pCapabilities = GetCapabilities()) == NULL)
				) {
			_pRTMPProtocol->EnqueueForOutbound();
			return true;
		}

		if ((_singleGop)&&(((pData[0]) >> 4) != 0x01)) {
			_pRTMPProtocol->EnqueueForOutbound();
			return true;
		}

		if (isAudio ? (!FeedAudioCodecBytes(pCapabilities, dts, true))
				: (!FeedVideoCodecBytes(pCapabilities, dts, true))) {
			FATAL("Unable to feed codec bytes");
			return false;
		}

		if ((!isAudio)
				&&(GetInStream() != NULL)
				&& TAG_KIND_OF(GetInStream()->GetType(), ST_IN_FILE)
				&& (pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)
				) {
			uint8_t tmp[2] = {0x57, 0x01};
			H_IA(ts.header) = true;
			H_TS(ts.header) = (uint32_t) dts;
			H_ML(ts.header) = sizeof (tmp);
			if (!ChunkAndSend(tmp, 2, ts))
				return false;
		}

		ts.isFirstFrame = false;

		H_IA(ts.header) = true;
		H_TS(ts.header) = (uint32_t) dts;
		H_ML(ts.header) = totalLength;
	} else {
		if (!AllowExecution(processedLength, dataLength, ts, stats))
			return true;
		if (processedLength == 0) {
			H_IA(ts.header) = false;
			H_TS(ts.header) = (uint32_t) (dts - ts.pChannel->lastOutAbsTs);
			H_ML(ts.header) = totalLength;
		}
	}

	TRACK_PAYLOAD(processedLength, dataLength, pData);
	uint32_t origInStreamId = 0;
	BaseInStream* pOrigInStream = NULL;
	Variant& streamConfig = GetProtocol()->GetCustomParameters();
	if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "_origId")) {
		origInStreamId = (uint32_t)(GetProtocol()->GetCustomParameters()["_origId"]);
		pOrigInStream = (BaseInStream*)(GetStreamsManager()->FindByUniqueId(origInStreamId));
	}
	if (pOrigInStream && pOrigInStream->_hasCachedKeyFrame() &&
		(NALU_TYPE(pData[0]) != NALU_TYPE_IDR) &&
		!_keyframeCacheConsumed) {
		//!_pInStream->_cacheConsumed() ) {
		IOBuffer cachedKeyFrame;
		if (pOrigInStream->_consumeCachedKeyFrame(cachedKeyFrame)) {
			INFO("Keyframe Cache HIT! SIZE=%d PTS=%lf",
				GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
				pOrigInStream->_getCachedPTS());
			bool result = BaseOutStream::GenericProcessData(
				GETIBPOINTER(cachedKeyFrame),
				GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
				pOrigInStream->_getCachedProcLen(),
				pOrigInStream->_getCachedTotLen(),
				dts, dts,
				false);
			if (!result)
				return false;
			_keyframeCacheConsumed = true;
		}
	} else if (pData && NALU_TYPE(pData[0]) == NALU_TYPE_IDR && !_keyframeCacheConsumed) {
		INFO("Outbound frame was a keyframe, sending cached keyframe is not needed anymore.");
		_keyframeCacheConsumed = true;
	}
	if (!ChunkAndSend(pData, dataLength, ts)) {
		FATAL("Unable to chunk and send data");
		return false;
	}

	if (_singleGop) {
		if (totalLength == (dataLength + processedLength)) {
			_singleGop = false;
			_paused = true;
			_audio.isFirstFrame = true;
			_video.isFirstFrame = true;
			SignalStreamCompleted();
			return true;
		}
	}

	return true;
}

bool OutNetRTMPStream::FeedAudioCodecBytes(StreamCapabilities *pCapabilities,
		double dts, bool isAbsolute) {
	if (dts < 0)
		dts = 0;
	if (pCapabilities == NULL)
		return true;
	switch (pCapabilities->GetAudioCodecType()) {
		case CODEC_AUDIO_AAC:
		{
			AudioCodecInfoAAC *pInfo = pCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
			if (pInfo == NULL)
				return true;
			IOBuffer &buffer = pInfo->GetRTMPRepresentation();
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
				return true;
			H_IA(_audio.header) = isAbsolute;
			H_TS(_audio.header) = (uint32_t) dts;
			H_ML(_audio.header) = GETAVAILABLEBYTESCOUNT(buffer);
			return ChunkAndSend(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer),
					_audio);
		}
		case CODEC_AUDIO_PASS_THROUGH:
		{
			AudioCodecInfoPassThrough *pInfo = pCapabilities->GetAudioCodec<AudioCodecInfoPassThrough > ();
			if (pInfo == NULL)
				return true;
			IOBuffer &buffer = pInfo->GetRTMPRepresentation();
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
				return true;
			H_IA(_audio.header) = isAbsolute;
			H_TS(_audio.header) = (uint32_t) dts;
			H_ML(_audio.header) = GETAVAILABLEBYTESCOUNT(buffer);
			return ChunkAndSend(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer),
					_audio);
		}
#ifdef HAS_G711
		case CODEC_AUDIO_G711A:
		case CODEC_AUDIO_G711U:
		{
			AudioCodecInfoG711 *pInfo = pCapabilities->GetAudioCodec<AudioCodecInfoG711 > ();
			if (pInfo == NULL)
				return true;
			IOBuffer &buffer = pInfo->GetRTMPRepresentation();
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
				return true;
			H_IA(_audio.header) = isAbsolute;
			H_TS(_audio.header) = (uint32_t) dts;
			H_ML(_audio.header) = GETAVAILABLEBYTESCOUNT(buffer);
			return ChunkAndSend(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer),
					_audio);
		}
#endif	/* HAS_G711 */
		default:
		{
			return true;
		}
	}
}

bool OutNetRTMPStream::FeedVideoCodecBytes(StreamCapabilities *pCapabilities,
		double dts, bool isAbsolute) {
	if (dts < 0)
		dts = 0;
	if (pCapabilities == NULL)
		return true;
	switch (pCapabilities->GetVideoCodecType()) {
		case CODEC_VIDEO_H264:
		{
			VideoCodecInfoH264 *pInfo = pCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
			if (pInfo == NULL)
				return true;
			IOBuffer &buffer = pInfo->GetRTMPRepresentation();
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
				return true;
			H_IA(_video.header) = isAbsolute;
			H_TS(_video.header) = (uint32_t) dts;
			H_ML(_video.header) = GETAVAILABLEBYTESCOUNT(buffer);
			return ChunkAndSend(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer),
					_video);
		}
		case CODEC_VIDEO_PASS_THROUGH:
		{
			VideoCodecInfoPassThrough *pInfo = pCapabilities->GetVideoCodec<VideoCodecInfoPassThrough > ();
			if (pInfo == NULL)
				return true;
			IOBuffer &buffer = pInfo->GetRTMPRepresentation();
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0)
				return true;
			H_IA(_video.header) = isAbsolute;
			H_TS(_video.header) = (uint32_t) dts;
			H_ML(_video.header) = GETAVAILABLEBYTESCOUNT(buffer);
			return ChunkAndSend(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer),
					_video);
		}
		default:
		{
			return true;
		}
	}
}

bool OutNetRTMPStream::ChunkAndSend(uint8_t *pData, uint32_t length,
		TrackState &ts) {
	if (H_ML(ts.header) == 0) {
		TRACK_HEADER(ts.header, ts.pChannel);
		return _pRTMPProtocol->SendRawData(ts.header, *ts.pChannel, NULL, 0);
	}

	if ((_feederChunkSize == _chunkSize) &&
			(GETAVAILABLEBYTESCOUNT(ts.bucket) == 0)) {
		TRACK_HEADER(ts.header, ts.pChannel);
		if (!_pRTMPProtocol->SendRawData(ts.header, *ts.pChannel, pData, length)) {
			FATAL("Unable to feed data");
			return false;
		}
		ts.pChannel->lastOutProcBytes += length;
		ts.pChannel->lastOutProcBytes %= H_ML(ts.header);
		return true;
	}

	uint32_t availableDataInBuffer = GETAVAILABLEBYTESCOUNT(ts.bucket);
	uint32_t totalAvailableBytes = availableDataInBuffer + length;
	uint32_t leftBytesToSend = H_ML(ts.header) - ts.pChannel->lastOutProcBytes;

	if (totalAvailableBytes < _chunkSize &&
			totalAvailableBytes != leftBytesToSend) {
		ts.bucket.ReadFromBuffer(pData, length);
		return true;
	}

	if (availableDataInBuffer != 0) {
		//Send data
		TRACK_HEADER(ts.header, ts.pChannel);
		if (!_pRTMPProtocol->SendRawData(ts.header, *ts.pChannel,
				GETIBPOINTER(ts.bucket), availableDataInBuffer)) {
			FATAL("Unable to send data");
			return false;
		}
		//cleanup buffer
		ts.bucket.IgnoreAll();

		//update counters
		totalAvailableBytes -= availableDataInBuffer;
		leftBytesToSend -= availableDataInBuffer;
		ts.pChannel->lastOutProcBytes += availableDataInBuffer;
		uint32_t leftOvers = _chunkSize - availableDataInBuffer;
		availableDataInBuffer = 0;

		//bite from the pData
		leftOvers = leftOvers <= length ? leftOvers : length;
		if (!_pRTMPProtocol->SendRawData(pData, leftOvers)) {
			FATAL("Unable to send data");
			return false;
		}

		//update the counters
		pData += leftOvers;
		length -= leftOvers;
		totalAvailableBytes -= leftOvers;
		leftBytesToSend -= leftOvers;
		ts.pChannel->lastOutProcBytes += leftOvers;
	}

	while (totalAvailableBytes >= _chunkSize) {
		TRACK_HEADER(ts.header, ts.pChannel);
		if (!_pRTMPProtocol->SendRawData(ts.header, *ts.pChannel, pData, _chunkSize)) {
			FATAL("Unable to send data");
			return false;
		}
		totalAvailableBytes -= _chunkSize;
		leftBytesToSend -= _chunkSize;
		ts.pChannel->lastOutProcBytes += _chunkSize;
		length -= _chunkSize;
		pData += _chunkSize;
	}

	if (totalAvailableBytes > 0 && totalAvailableBytes == leftBytesToSend) {
		TRACK_HEADER(ts.header, ts.pChannel);
		if (!_pRTMPProtocol->SendRawData(ts.header, *ts.pChannel, pData, leftBytesToSend)) {
			FATAL("Unable to send data");
			return false;
		}
		totalAvailableBytes -= leftBytesToSend;
		ts.pChannel->lastOutProcBytes += leftBytesToSend;
		length -= leftBytesToSend;
		pData += leftBytesToSend;
		leftBytesToSend -= leftBytesToSend;
	}

	if (length > 0) {
		ts.bucket.ReadFromBuffer(pData, length);
	}

	if (leftBytesToSend == 0) {
		o_assert(ts.pChannel->lastOutProcBytes == H_ML(ts.header));
		ts.pChannel->lastOutProcBytes = 0;
	}

	return true;
}

bool OutNetRTMPStream::AllowExecution(uint32_t totalProcessed,
		uint32_t dataLength, TrackState &ts,
		BaseStream::BaseStreamStats::BaseStreamTrackStats &stats) {
#ifndef SIMULATE_DROPPING_FRAMES
	if (!_canDropFrames) {
		// we are not allowed to drop frames
		return true;
	}
#endif /* SIMULATE_DROPPING_FRAMES */

	//we are allowed to drop frames
	if (ts.currentFrameDropped) {
		//current frame was dropped. Test to see if we are in the middle
		//of it or this is a new one
		if (totalProcessed != 0) {
			//we are in the middle of it. Don't allow execution
			stats.droppedBytesCount += dataLength;
			return false;
		} else {
			//this is a new frame. We will detect later if it can be sent
			ts.currentFrameDropped = false;
		}
	}

	if (totalProcessed != 0) {
		//we are in the middle of a non-dropped frame. Send it anyway
		return true;
	}

#ifdef SIMULATE_DROPPING_FRAMES
	if ((rand() % 100) < SIMULATE_DROPPING_FRAMES) {
		//we have too many data left unsent. Drop the frame
		stats.droppedPacketsCount++;
		stats.droppedBytesCount += dataLength;
		ts.currentFrameDropped = true;
		FINEST("Dropped frame");
		return false;
	} else {
		//we can still pump data
		return true;
	}
#else /* SIMULATE_DROPPING_FRAMES */
	//do we have any data?
	if (_pRTMPProtocol->GetOutputBuffer() == NULL) {
		//no data in the output buffer. Allow to send it
		return true;
	}

	//we have some data in the output buffer
	uint32_t outBufferSize = GETAVAILABLEBYTESCOUNT(*_pRTMPProtocol->GetOutputBuffer());
	if (outBufferSize > _maxBufferSize) {
		//we have too many data left unsent. Drop the frame
		stats.droppedPacketsCount++;
		stats.droppedBytesCount += dataLength;
		ts.currentFrameDropped = true;
		_pRTMPProtocol->SignalOutBufferFull(outBufferSize, _maxBufferSize);
		return false;
	} else {
		//we can still pump data
		return true;
	}
#endif /* SIMULATE_DROPPING_FRAMES */
}

void OutNetRTMPStream::ArmTimer(double period) {
	if (period <= 0)
		period = 1;
	DisarmTimer();
	OutNetRTMPStreamTimer *pTemp = new OutNetRTMPStreamTimer(this);
	pTemp->EnqueueForHighGranularityTimeEvent((uint32_t) period);
	_timerId = pTemp->GetId();
}

void OutNetRTMPStream::DisarmTimer() {
	OutNetRTMPStreamTimer *pTemp = (OutNetRTMPStreamTimer *) ProtocolManager::GetProtocol(
			_timerId);
	_timerId = 0;
	if (pTemp == NULL)
		return;
	pTemp->ResetStream();
	pTemp->EnqueueForDelete();
}


void OutNetRTMPStream::EnableAudio(bool enable) {
	_enableAudio = enable;
}

void OutNetRTMPStream::EnableVideo(bool enable) {
	_enableVideo = enable;
}

bool OutNetRTMPStream::SendMetadata(string const& metadataStr, int64_t pts) {
	// form a variant suitable to send as an Notify
    Variant parm; // need to make this an array
    //for Notify, functionName becomes parm 0
    parm.PushToArray(metadataStr); // string is parm 1
    string func = RM_NOTIFY_FUNCTION_RMS_METADATA;
    Variant message = GenericMessageFactory::GetNotify(
    		3, _rtmpStreamId, 0, false, func, parm);
    bool ok = false;	// pessimistic
	// .. then send it to this guy's protocol
	BaseRTMPProtocol * pProt = (BaseRTMPProtocol *) GetProtocol();
	if (pProt) {
		// use: SendMessage(variant message) !??
		ok = pProt->SendMessage(message);
		// ToDo: Handle error - how?
		if (!ok) {
			//WARN("$b2$: failed to SendMessage(metadata) out an RTMP Stream");
		}else{
			//FATAL("$b2$: DID SendMessage(metadata) out an RTMP Stream");
		}
	}else{
		FATAL("No protocol for RMTP stream!");
	}
	return ok;
}

uint32_t OutNetRTMPStream::GetRTMPStreamId() {
	return _rtmpStreamId;
}
#endif /* HAS_PROTOCOL_RTMP */
