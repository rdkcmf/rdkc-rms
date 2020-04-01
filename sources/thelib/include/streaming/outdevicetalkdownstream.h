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



#ifndef _OUTDEVICETALKDOWNSTREAM_H
#define	_OUTDEVICETALKDOWNSTREAM_H

#include "streaming/baseoutdevicestream.h"

#ifdef PLAYBACK_SUPPORT
#include "xaudioapi.h"
#define TALKDOWN_SAMPLE_RATE	8000
#define TALKDOWN_CHANNEL_COUNT	1	// 1 for mono and 2 for stereo
#endif

class OutboundConnectivity;

class DLLEXP OutDeviceTalkDownStream
: public BaseOutDeviceStream {
protected:
	OutboundConnectivity *_pConnectivity;
	bool _hasAudio;
	bool _hasVideo;
	bool _enabled;
	double _audioSampleRate;
        AudioCodecInfo *_pAudioInfo;

	bool _firstVideoFrame;
	double _videoSampleRate;
        VideoCodecInfo *_pVideoInfo;

#ifdef PLAYBACK_SUPPORT
	rdkc_xaudio* pxa;
	rdkc_xaConfig config;
	IOBuffer _audioBuffer;
#endif


public:
	OutDeviceTalkDownStream(BaseProtocol *pProtocol, string name);
	virtual ~OutDeviceTalkDownStream();

	void Enable(bool value);

	OutboundConnectivity *GetConnectivity();
	void SetConnectivity(OutboundConnectivity *pConnectivity);
	void HasAudioVideo(bool hasAudio, bool hasVideo);

	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();

private:
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual void SignalAttachedToInStream();
	virtual void SignalAudioStreamCapabilitiesChanged(
                        StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
                        AudioCodecInfo *pNew);
        virtual void SignalVideoStreamCapabilitiesChanged(
                        StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
                        VideoCodecInfo *pNew);

protected:
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
                        bool isKeyFrame);
        virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
        virtual bool IsCodecSupported(uint64_t codec);

public:
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);

};

#endif	/* _OUTDEVICETALKDOWNSTREAM_H */

