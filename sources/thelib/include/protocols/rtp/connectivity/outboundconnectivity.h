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
#ifndef _OUTBOUNDCONNECTIVITY_H
#define	_OUTBOUNDCONNECTIVITY_H

#include "protocols/rtp/connectivity/baseconnectivity.h"
#include "utils/misc/socketaddress.h"

class BaseOutNetRTPUDPStream;
class RTSPProtocol;
class BaseProtocol;

struct RTPClient {
	uint32_t protocolId;
	bool isUdp;

	bool hasAudio;
	SocketAddress audioDataAddress;
	SocketAddress audioRtcpAddress;
	uint32_t audioPacketsCount;
	uint32_t audioBytesCount;
	uint8_t audioDataChannel;
	uint8_t audioRtcpChannel;

	bool hasVideo;
	SocketAddress videoDataAddress;
	SocketAddress videoRtcpAddress;
	uint32_t videoPacketsCount;
	uint32_t videoBytesCount;
	uint8_t videoDataChannel;
	uint8_t videoRtcpChannel;

	bool isSRTP;

	RTPClient() {
		protocolId = 0;
		isUdp = false;

		hasAudio = false;
		// memset(&audioDataAddress, 0, sizeof (audioDataAddress));
		// memset(&audioRtcpAddress, 0, sizeof (audioRtcpAddress));
		audioPacketsCount = 0;
		audioBytesCount = 0;
		audioDataChannel = 0xff;
		audioRtcpChannel = 0xff;

		hasVideo = false;
		// memset(&videoDataAddress, 0, sizeof (videoDataAddress));
		// memset(&videoRtcpAddress, 0, sizeof (videoRtcpAddress));
		videoPacketsCount = 0;
		videoBytesCount = 0;
		videoDataChannel = 0xff;
		videoRtcpChannel = 0xff;

		isSRTP = false;
	}
};

class DLLEXP OutboundConnectivity
: public BaseConnectivity {
private:
	bool _forceTcp;
	BaseProtocol *_pProtocol;
	BaseOutNetRTPUDPStream *_pOutStream;
	MSGHDR _dataMessage;
	MSGHDR _rtcpMessage;
	uint8_t *_pRTCPNTP;
	uint8_t *_pRTCPRTP;
	uint8_t *_pRTCPSPC;
	uint8_t *_pRTCPSOC;
	uint64_t _startupTime;
	RTPClient _rtpClient;

	bool _hasVideo;
	SOCKET _videoDataFd;
	uint16_t _videoDataPort;
	SOCKET _videoRTCPFd;
	uint16_t _videoRTCPPort;
	uint32_t _videoNATDataId;
	uint32_t _videoNATRTCPId;
	double _videoSampleRate;

	bool _hasAudio;
	SOCKET _audioDataFd;
	uint16_t _audioDataPort;
	SOCKET _audioRTCPFd;
	uint16_t _audioRTCPPort;
	uint32_t _audioNATDataId;
	uint32_t _audioNATRTCPId;
	double _audioSampleRate;

	string _multicastIP;
	int32_t _amountSent;
	uint32_t _dummyValue;

	bool _firstFrameSent;
//	bool _pureRTP;

public:
	OutboundConnectivity(bool forceTcp, BaseProtocol *pProtocol);
	virtual ~OutboundConnectivity();
	bool Initialize();
	void Enable(bool value);
	void SetOutStream(BaseOutNetRTPUDPStream *pOutStream);
	string GetVideoPorts();
	string GetAudioPorts();
	string GetVideoChannels();
	string GetAudioChannels();
	uint32_t GetAudioSSRC();
	uint32_t GetVideoSSRC();
	uint16_t GetLastVideoSequence();
	uint16_t GetLastAudioSequence();
	void SetMulticastIP(string ip);
	void HasAudio(bool value);
	void HasVideo(bool value);
	void SetRTPBlockSize(uint32_t blockSize);
	bool RegisterUDPVideoClient(uint32_t rtspProtocolId, SocketAddress& data,
			SocketAddress& rtcp);
	bool RegisterUDPVideoClient(uint32_t protocolId, SocketAddress &data);
	bool RegisterUDPAudioClient(uint32_t rtspProtocolId, SocketAddress& data,
			SocketAddress& rtcp);
	bool RegisterUDPAudioClient(uint32_t protocolId, SocketAddress &data);
	bool RegisterTCPVideoClient(uint32_t protocolId, uint8_t data, uint8_t rtcp);
	bool RegisterTCPAudioClient(uint32_t protocolId, uint8_t data, uint8_t rtcp);
	void SignalDetachedFromInStream();
	bool FeedVideoData(MSGHDR &message, double pts, double dts);
	bool FeedAudioData(MSGHDR &message, double pts, double dts);
	void ReadyForSend();
	bool PushMetaData(string const& vmfMetadata, int64_t pts);
	void SendDirect(IOBuffer &data);
private:
	bool InitializePorts(SOCKET &dataFd, uint16_t &dataPort,
			uint32_t &natDataId, SOCKET &RTCPFd, uint16_t &RTCPPort,
			uint32_t &natRTCPId);
	bool InitializeRTPPort(SOCKET &rtpFd, uint16_t &rtpPort, uint32_t &natRtpId);
	bool FeedData(MSGHDR &message, double pts, double dts, bool isAudio);
	void WriteToBuffer(IOBuffer &dest, MSGHDR src);
};


#endif	/* _OUTBOUNDCONNECTIVITY_H */
#endif /* HAS_PROTOCOL_RTP */
