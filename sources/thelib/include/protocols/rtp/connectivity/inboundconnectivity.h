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
#ifndef _INBOUNDCONNECTIVITY_H
#define	_INBOUNDCONNECTIVITY_H

#include "protocols/rtp/connectivity/baseconnectivity.h"

class InboundRTPProtocol;
class RTCPProtocol;
class InNetRTPStream;
class BaseRTSPProtocol;
class BaseProtocol;
class SocketAddress;

struct EndpointInfo {
	//Rtp
	string RtpAddress;
	uint16_t RtpPort;
	//Rtcp
	string RtcpAddress;
	uint16_t RtcpPort;
};

class DLLEXP InboundConnectivity
: public BaseConnectivity {
private:
	BaseRTSPProtocol *_pRTSP;

	uint32_t _rtpVideoId;
	uint32_t _rtcpVideoId;
	uint8_t _videoRR[60];
	Variant _videoTrack;

	uint32_t _rtpAudioId;
	uint32_t _rtcpAudioId;
	uint8_t _audioRR[60];
	Variant _audioTrack;

	InNetRTPStream *_pInStream;

	BaseProtocol *_pProtocols[256];
	IOBuffer _inputBuffer;
	SocketAddress _dummyAddress;

	bool _forceTcp;
	string _privateStreamName;
	string _publicStreamName;
	uint32_t _bandwidthHint;

	uint8_t _rtcpDetectionInterval;

	int16_t _a;
	int16_t _b;
public:
	InboundConnectivity(BaseRTSPProtocol *pRTSP, string streamName,
			uint32_t bandwidthHint, uint8_t rtcpDetectionInterval,
			int16_t a, int16_t b);
	virtual ~InboundConnectivity();
	void EnqueueForDelete();

	bool AdjustChannels(uint16_t data, uint16_t rtcp, bool isAudio);
	bool AddTrack(Variant &track, bool isAudio);
	bool Initialize(Variant &parameters);

	string GetTransportHeaderLine(bool isAudio, bool isClient);

	bool FeedData(uint32_t channelId, uint8_t *pBuffer, uint32_t bufferLength);

	string GetClientPorts(bool isAudio);
	string GetAudioClientPorts();
	string GetVideoClientPorts();
	bool SendRR(bool isAudio);
	void ReportSR(uint64_t ntpMicroseconds, uint32_t rtpTimestamp, bool isAudio);
	bool OpenUDPPorts(EndpointInfo destEndpointInfo);
	InNetRTPStream *GetInStream();
private:
	void Cleanup();
	bool CreateCarriers(InboundRTPProtocol *pRTP, RTCPProtocol *pRTCP);
};


#endif	/* _INBOUNDCONNECTIVITY_H */
#endif /* HAS_PROTOCOL_RTP */
