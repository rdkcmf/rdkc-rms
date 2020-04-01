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



#ifdef HAS_PROTOCOL_EXTERNAL
#ifndef _OUTNETEXTERNALSTREAM_H
#define	_OUTNETEXTERNALSTREAM_H

#include "streaming/baseoutnetstream.h"
#include "protocols/external/rmsprotocol.h"

class OutNetExternalStream
: public BaseOutNetStream {
private:
	out_stream_t _streamInterface;
	uint64_t _audioPacketsCount;
	uint64_t _audioBytesCount;
	uint64_t _videoPacketsCount;
	uint64_t _videoBytesCount;
public:
	OutNetExternalStream(BaseProtocol *pProtocol, string name, void *pUserData,
			connection_t *pConnection);
	virtual ~OutNetExternalStream();

	out_stream_t *GetInterface();

	virtual bool IsCompatibleWithType(uint64_t type);
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
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
private:
	void AddStreamToConnectionInterface();
	void RemoveStreamFromConnectionInterface();
};


#endif	/* _OUTNETEXTERNALSTREAM_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
