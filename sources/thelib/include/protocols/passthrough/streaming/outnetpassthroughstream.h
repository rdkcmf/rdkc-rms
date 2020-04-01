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


#ifndef _OUTNETPASSTHROUGHSTREAM_H
#define	_OUTNETPASSTHROUGHSTREAM_H

#include "streaming/baseoutnetstream.h"

class UDPSenderProtocol;

class DLLEXP OutNetPassThroughStream
: public BaseOutNetStream {
private:
	UDPSenderProtocol *_pSender;
	string _pushUri;
	bool _enabled;
public:
	OutNetPassThroughStream(BaseProtocol *pProtocol, string name);
	virtual ~OutNetPassThroughStream();

	void Enable(bool value);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual bool IsCompatibleWithType(uint64_t type);
	bool InitUdpSink(Variant &config);
	uint16_t GetUdpBindPort();
protected:
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
};
#endif	/* _OUTNETPASSTHROUGHSTREAM_H */
