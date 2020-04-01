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
#ifndef _OUTNETRTPUDPH264STREAM_H
#define	_OUTNETRTPUDPH264STREAM_H

#include "protocols/rtp/streaming/baseoutnetrtpudpstream.h"

class DLLEXP OutNetRTPUDPH264Stream
: public BaseOutNetRTPUDPStream {
private:
	bool _forceTcp;
	bool _waitForIdr;
	MSGHDR _videoData;
	double _videoSampleRate;
	VideoCodecInfo *_pVideoInfo;
	bool _firstVideoFrame;
	double _lastVideoPts;

	MSGHDR _audioData;
	double _audioSampleRate;
	AudioCodecInfo *_pAudioInfo;
	IOBuffer _auBuffer;
	double _auPts;
	uint32_t _auCount;
	bool _isPlayerPaused;
	bool _inboundVod;
	bool _dtsNeedsUpdate;
	double _pauseDts;
	double _pausePts;
	double _resumeDts;
	double _resumePts;
	double _dtsOffset;
	double _ptsOffset;
	queue<double> _tsDeltasVideo; //max of 200 dts samples for average, first in, first out
	queue<double> _tsDeltasAudio; //max of 200 dts samples for average, first in, first out
	double _aveTsDeltaVideo;
	double _aveTsDeltaAudio;
	double _lastTsVideo;
	double _lastTsAudio;
	double _tsOffset;
	uint32_t _videoResumeCount;
#ifdef HAS_G711
	bool _firstAudioSample;
#endif	/* HAS_G711 */

	uint32_t _maxRTPPacketSize;

protected:
	uint8_t _audioPT;
	uint8_t _videoPT;
public:
	OutNetRTPUDPH264Stream(BaseProtocol *pProtocol, string name, bool forceTcp,
			bool zeroTimeBase, bool isSrtp = false);
	void SetRTPBlockSize(uint32_t blockSize);
	void SetSSRC(uint32_t value, bool isAudio);
	void UpdatePauseStatus(bool newPauseStatus);
	void StorePauseTs(double dts, double pts);
	void StoreResumeTs(double dts, double pts);
	void UpdateTsOffsetsAndResetTs();
	virtual ~OutNetRTPUDPH264Stream();
	virtual uint8_t GetPayloadType(bool isAudio);
	virtual void SetPayloadType(uint8_t pt, bool isAudio);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool PushMetaData(string const& vmfMetadata, int64_t pts);
	virtual void SetInboundVod(bool inboundVod);
	virtual bool GenericProcessData(uint8_t * pData, uint32_t dataLength, uint32_t processedLength, uint32_t totalLength, double pts, double dts, bool isAudio);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
private:
	virtual void SignalAttachedToInStream();
	bool SendMetadata(Variant sdpMetadata, int64_t pts);
	bool SendMetadata(int64_t pts);

	/**
	 * Added field to indicate if payload is an SPS/PPS. Used to address issue
	 * with Edge.
	 * @param buffer
	 * @param pts
	 * @param dts
	 * @param isKeyFrame
	 * @param isParamSet
	 * @return
	 */
	bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame, bool isParamSet);
};


#endif	/* _OUTNETRTPUDPH264STREAM_H */
#endif /* HAS_PROTOCOL_RTP */

