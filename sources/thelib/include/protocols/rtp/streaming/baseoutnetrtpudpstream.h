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
#ifndef _BASEOUTNETRTPUDPSTREAM_H
#define	_BASEOUTNETRTPUDPSTREAM_H

#include "streaming/baseoutnetstream.h"
#include "protocols/rtp/packetbuffer.h"
class OutboundConnectivity;

class DLLEXP BaseOutNetRTPUDPStream
: public BaseOutNetStream {
protected:
	uint32_t _audioSsrc;
	uint32_t _videoSsrc;
	OutboundConnectivity *_pConnectivity;
	uint16_t _videoCounter;
	uint16_t _audioCounter;
	bool _hasAudio;
	bool _hasVideo;
	bool _enabled;
	bool _zeroTimeBase;
	bool _isSrtp;
	PacketBuffer _videoPacketBuffer;

public:
	BaseOutNetRTPUDPStream(BaseProtocol *pProtocol, string name, bool zeroTimeBase, bool isSrtp = false);
	virtual ~BaseOutNetRTPUDPStream();

	void Enable(bool value);

	OutboundConnectivity *GetConnectivity();
	void SetConnectivity(OutboundConnectivity *pConnectivity);
	void HasAudioVideo(bool hasAudio, bool hasVideo);

	uint32_t AudioSSRC();
	uint32_t VideoSSRC();
	uint16_t VideoCounter();
	uint16_t AudioCounter();

	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();

	virtual bool IsCompatibleWithType(uint64_t type);
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();

	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void SetRTPBlockSize(uint32_t blockSize);
	
	void EnablePacketCache(bool enabled);
	inline bool HasCache(uint32_t ssrc) { return ssrc == _videoSsrc; } // video only
	uint32_t RetransmitFrames(uint32_t ssrc, vector<uint16_t> pids);
	bool PacketIsCached(uint32_t ssrc, uint16_t pid);
	bool PacketIsNewerThanCache(uint32_t ssrc, uint16_t pid);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);

};

#endif	/* _BASEOUTNETRTPUDPSTREAM_H */
#endif /* HAS_PROTOCOL_RTP */

