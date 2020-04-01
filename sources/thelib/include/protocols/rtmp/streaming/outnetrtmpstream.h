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
#ifndef _OUTNETRTMPSTREAM_H
#define	_OUTNETRTMPSTREAM_H

#include "streaming/baseoutnetstream.h"
#include "protocols/rtmp/header.h"
#include "protocols/rtmp/channel.h"
#include "protocols/timer/basetimerprotocol.h"
#include "mediaformats/readers/streammetadataresolver.h"

class BaseRTMPProtocol;
class RTMPPlaylistItem;

class DLLEXP OutNetRTMPStream
: public BaseOutNetStream {
private:

	class OutNetRTMPStreamTimer
	: public BaseTimerProtocol {
	private:
		OutNetRTMPStream *_pOutNetRTMPStream;
	public:
		OutNetRTMPStreamTimer(OutNetRTMPStream *pOutNetRTMPStream);
		virtual ~OutNetRTMPStreamTimer();
		void ResetStream();
		virtual bool TimePeriodElapsed();
	};
	friend class OutNetRTMPStreamTimer;

	struct TrackState {
		Channel *pChannel;
		uint32_t isFirstFrame;
		IOBuffer bucket;
		bool currentFrameDropped;
		double zeroBase;
		Header header;
		void Init(BaseRTMPProtocol *pProtocol, uint8_t type, uint32_t rtmpStreamId);
	};

	BaseRTMPProtocol *_pRTMPProtocol;
	RTMPPlaylistItem *_pPlaylistItem;
	uint32_t _rtmpStreamId;
	uint32_t _chunkSize;
	uint32_t _feederChunkSize;
	bool _canDropFrames;
	bool _directFeed;
	string _clientId;
	double _absoluteTimelineHead;
	uint32_t _maxBufferSize;
	double _playbackLength;
	bool _singleGop;
	bool _paused;
	uint32_t _timerId;

	TrackState _audio;
	TrackState _video;

	bool _independentZeroBase;
	double _zeroBase;

	bool _enableAudio;
	bool _enableVideo;
protected:
	OutNetRTMPStream(BaseProtocol *pProtocol, RTMPPlaylistItem *pPlaylistItem,
			string name, uint32_t rtmpStreamId, uint32_t chunkSize);
public:
	static OutNetRTMPStream *GetInstance(BaseProtocol *pProtocol,
			RTMPPlaylistItem *pPlaylistItem, StreamsManager *pStreamsManager,
			string name, uint32_t rtmpStreamId, uint32_t chunkSize);
	virtual ~OutNetRTMPStream();

	bool ResendMetadata();

	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool IsCodecSupported(uint64_t codec);
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	uint32_t GetRTMPStreamId();

	void SetAbsoluteTimelineHead(double absoluteTimelineHead);
	void SetPlaybackLength(double length, bool singleGop);
	void SetChunkSize(uint32_t chunkSize);
	uint32_t GetChunkSize();
	void SetFeederChunkSize(uint32_t feederChunkSize);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);

	virtual bool SendStreamMessage(Variant &message);
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();

	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool SignalStreamPublished(string streamName);

	virtual void SignalStreamCompleted();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
	double GetLastSentTimestamp();
	void EnableAudio(bool enable);
	void EnableVideo(bool enable);

	virtual bool SendMetadata(string const& metadataStr, int64_t pts);
protected:
	virtual bool InternalFeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double dts, bool isAudio);
	bool FeedAudioCodecBytes(StreamCapabilities *pCapabilities, double dts, bool isAbsolute);
	bool FeedVideoCodecBytes(StreamCapabilities *pCapabilities, double dts, bool isAbsolute);
private:
	bool ChunkAndSend(uint8_t *pData, uint32_t length, TrackState &ts);
	bool AllowExecution(uint32_t totalProcessed, uint32_t dataLength,
			TrackState &ts, BaseStream::BaseStreamStats::BaseStreamTrackStats &stats);
	void ArmTimer(double period);
	void DisarmTimer();
};


#endif	/* _OUTNETRTMPSTREAM_H */

#endif /* HAS_PROTOCOL_RTMP */

