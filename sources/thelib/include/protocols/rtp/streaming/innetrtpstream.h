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
#ifndef _INNETRTPSTREAM_H
#define	_INNETRTPSTREAM_H

#include "streaming/baseinnetstream.h"
#include "protocols/rtp/rtpheader.h"

#define RTCP_PRESENCE_UNKNOWN 0
#define RTCP_PRESENCE_AVAILABLE 1
#define RTCP_PRESENCE_ABSENT 2

class DLLEXP InNetRTPStream
: public BaseInNetStream {
private:
	StreamCapabilities _capabilities;

	bool _hasAudio;
	bool _isLatm;
#ifdef HAS_G711
	bool _audioByteStream;
#endif	/* HAS_G711 */
	uint16_t _audioSequence;
	double _audioNTP;
	double _audioRTP;
	double _audioLastDts;
	uint32_t _audioLastRTP;
	uint32_t _audioRTPRollCount;
	double _audioFirstTimestamp;
	uint32_t _lastAudioRTCPRTP;
	uint32_t _audioRTCPRTPRollCount;
	double _audioSampleRate;

	bool _hasVideo;
	IOBuffer _currentNalu;
	uint16_t _videoSequence;
	double _videoNTP;
	double _videoRTP;
	double _videoLastPts;
	double _videoLastDts;
	uint32_t _videoLastRTP;
	uint32_t _videoRTPRollCount;
	double _videoFirstTimestamp;
	uint32_t _lastVideoRTCPRTP;
	uint32_t _videoRTCPRTPRollCount;
	double _videoSampleRate;

	uint8_t _rtcpPresence;
	uint8_t _rtcpDetectionInterval;
	time_t _rtcpDetectionStart;

	size_t _dtsCacheSize;
	map<double, double> _dtsCache;

	IOBuffer _sps;
	IOBuffer _pps;
	int16_t _a;
	int16_t _b;
	//double _maxDts;
public:
	InNetRTPStream(BaseProtocol *pProtocol, string name, Variant &videoTrack,
			Variant &audioTrack, uint32_t bandwidthHint,
			uint8_t rtcpDetectionInterval, int16_t a, int16_t b);
	virtual ~InNetRTPStream();

	virtual StreamCapabilities * GetCapabilities();
	virtual void ReadyForSend();
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	bool FeedVideoData(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader);
	bool FeedAudioData(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader);
	void ReportSR(uint64_t ntpMicroseconds, uint32_t rtpTimestamp, bool isAudio);
private:
	bool FeedAudioDataAU(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader);
	bool FeedAudioDataLATM(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader);
#ifdef HAS_G711
	bool FeedAudioDataByteStream(uint8_t *pData, uint32_t dataLength,
			RTPHeader &rtpHeader);
#endif	/* HAS_G711 */
	virtual bool InternalFeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double dts, bool isAudio);
	uint64_t ComputeRTP(uint32_t &currentRtp, uint32_t &lastRtp,
			uint32_t &rtpRollCount);
};

#endif	/* _INNETRTPSTREAM_H */
#endif /* HAS_PROTOCOL_RTP */
