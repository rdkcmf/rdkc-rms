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
#include "protocols/rtp/connectivity/inboundconnectivity.h"
#include "protocols/rtp/basertspprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/rtp/inboundrtpprotocol.h"
#include "protocols/rtp/rtcpprotocol.h"
#include "protocols/rtp/rtspprotocol.h"
#include "netio/netio.h"
#include "protocols/rtp/sdp.h"
#include "application/baseclientapplication.h"
#include "protocols/rtp/streaming/innetrtpstream.h"
#include "protocols/udpprotocol.h"
#include "protocols/http/basehttpprotocol.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "protocols/protocolmanager.h"

InboundConnectivity::InboundConnectivity(BaseRTSPProtocol *pRTSP, string streamName,
		uint32_t bandwidthHint, uint8_t rtcpDetectionInterval,
		int16_t a, int16_t b)
: BaseConnectivity() {
	_pRTSP = pRTSP;
	_rtpVideoId = 0;
	_rtcpVideoId = 0;
	_rtpAudioId = 0;
	_rtcpAudioId = 0;
	_pInStream = NULL;
	_forceTcp = false;
	memset(_pProtocols, 0, sizeof (_pProtocols));
	memset(&_dummyAddress, 0, sizeof (_dummyAddress));

	memset(_audioRR, 0, sizeof (_audioRR));
	_audioRR[0] = '$'; //marker
	_audioRR[1] = 0; //channel
	_audioRR[2] = 0; //size
	_audioRR[3] = 56; //size
	_audioRR[4] = 0x81; //V,P,RC
	_audioRR[5] = 0xc9; //PT
	_audioRR[6] = 0x00; //length
	_audioRR[7] = 0x07; //length
	EHTONLP(_audioRR + 16, 0x00ffffff); //fraction lost/cumulative number of packets lost
	EHTONLP(_audioRR + 24, 0); //interarrival jitter
	EHTONLP(_audioRR + 32, 0); // delay since last SR (DLSR)
	_audioRR[36] = 0x81; //V,P,RC
	_audioRR[37] = 0xca; //PT
	_audioRR[38] = 0x00; //length
	_audioRR[39] = 0x05; //length
	_audioRR[44] = 0x01; //type
	_audioRR[45] = 0x0d; //length
	memcpy(_audioRR + 46, "machine.local", 0x0d); //name of the machine
	_audioRR[59] = 0; //padding

	memset(_videoRR, 0, sizeof (_videoRR));
	_videoRR[0] = '$'; //marker
	_videoRR[1] = 0; //channel
	_videoRR[2] = 0; //size
	_videoRR[3] = 56; //size
	_videoRR[4] = 0x81; //V,P,RC
	_videoRR[5] = 0xc9; //PT
	_videoRR[6] = 0x00; //length
	_videoRR[7] = 0x07; //length
	EHTONLP(_videoRR + 16, 0x00ffffff); //fraction lost/cumulative number of packets lost
	EHTONLP(_videoRR + 24, 0); //interarrival jitter
	EHTONLP(_videoRR + 32, 0); // delay since last SR (DLSR)
	_videoRR[36] = 0x81; //V,P,RC
	_videoRR[37] = 0xca; //PT
	_videoRR[38] = 0x00; //length
	_videoRR[39] = 0x05; //length
	_videoRR[44] = 0x01; //type
	_videoRR[45] = 0x0d; //length
	memcpy(_videoRR + 46, "machine.local", 0x0d); //name of the machine
	_videoRR[59] = 0; //padding

	_privateStreamName = streamName;
	_bandwidthHint = bandwidthHint;
	_rtcpDetectionInterval = rtcpDetectionInterval;

	_a = a;
	_b = b;
}

InboundConnectivity::~InboundConnectivity() {
	Cleanup();
}

void InboundConnectivity::EnqueueForDelete() {
	Cleanup();
	_pRTSP->EnqueueForDelete();
}

bool InboundConnectivity::AdjustChannels(uint16_t data, uint16_t rtcp, bool isAudio) {
	if ((!_forceTcp)
			|| (data > 255)
			|| (rtcp > 255)
			|| ((data + 1) != rtcp)
			)
		return false;
	uint32_t &rtpId = isAudio ? _rtpAudioId : _rtpVideoId;
	for (uint32_t i = 0; i < (255 - 1); i++) {
		if ((_pProtocols[i] != NULL) && (_pProtocols[i]->GetId() == rtpId)) {
			if (i == data)
				return true;
			if ((_pProtocols[data] != NULL) || (_pProtocols[rtcp] != NULL))
				return false;
			_pProtocols[data] = _pProtocols[i];
			_pProtocols[rtcp] = _pProtocols[i + 1];
			_pProtocols[i] = NULL;
			_pProtocols[i + 1] = NULL;
			return true;
		}
	}

	return false;
}

bool InboundConnectivity::AddTrack(Variant& track, bool isAudio) {
	Variant &_track = isAudio ? _audioTrack : _videoTrack;
	Variant &_oppositeTrack = isAudio ? _videoTrack : _audioTrack;
	uint32_t &rtpId = isAudio ? _rtpAudioId : _rtpVideoId;
	uint32_t &rtcpId = isAudio ? _rtcpAudioId : _rtcpVideoId;
	uint8_t *pRR = isAudio ? _audioRR : _videoRR;

	if (_track != V_NULL) {
		return false;
	}

	BaseClientApplication *pApplication = _pRTSP->GetApplication();
	if (pApplication == NULL) {
		FATAL("RTSP protocol not yet assigned to an application");
		return false;
	}

	_track = track;
	if (_oppositeTrack != V_NULL) {
		if (_oppositeTrack["isTcp"] != _track["isTcp"])
			return false;
	}
	_forceTcp = (bool)_track["isTcp"];

	Variant dummy;
	InboundRTPProtocol *pRTP = (InboundRTPProtocol *) ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_INBOUND_UDP_RTP, dummy);
	if (pRTP == NULL) {
		FATAL("Unable to create the protocol chain");
		Cleanup();
		return false;
	}
	rtpId = pRTP->GetId();

	RTCPProtocol *pRTCP = (RTCPProtocol *) ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_UDP_RTCP, dummy);
	if (pRTCP == NULL) {
		FATAL("Unable to create the protocol chain");
		Cleanup();
		return false;
	}
	rtcpId = pRTCP->GetId();
	pRTCP->SetInbboundConnectivity(this, isAudio);
	if ((bool)_track["isTcp"]) {
		uint16_t dataIdx = 0;
		uint16_t rtcpIdx = 0;

		//2. Add them in the fast-pickup array
		if ((_track.HasKeyChain(V_UINT16, true, 2, "portsOrChannels", "data"))
				&& (_track.HasKeyChain(V_UINT16, true, 2, "portsOrChannels", "rtcp"))) {
			dataIdx = (uint16_t) _track["portsOrChannels"]["data"];
			rtcpIdx = (uint16_t) _track["portsOrChannels"]["rtcp"];
		} else {
			uint8_t idx = (uint8_t) ((uint32_t) SDP_TRACK_GLOBAL_INDEX(_track)*2);
			dataIdx = idx;
			rtcpIdx = idx + 1;
		}

		if ((dataIdx >= 256) || (rtcpIdx >= 256)) {
			FATAL("Invalid channel numbers");
			Cleanup();
			return false;
		}
		if ((_pProtocols[dataIdx] != NULL) || (_pProtocols[rtcpIdx] != NULL)) {
			FATAL("Invalid channel numbers");
			Cleanup();
			return false;
		}
		_pProtocols[dataIdx] = pRTP;
		_pProtocols[rtcpIdx] = pRTCP;
		EHTONLP(pRR + 8, pRTCP->GetSSRC()); //SSRC of packet sender
		EHTONLP(pRR + 40, pRTCP->GetSSRC()); //SSRC of packet sender
		pRR[1] = (uint8_t) rtcpIdx;
	} else {
		if (!CreateCarriers(pRTP, pRTCP)) {
			FATAL("Unable to create carriers");
			Cleanup();
			return false;
		}
	}
	INFO("Created RTP[%d] and RTCP[%d] protocol for %s track of the inbound connection", rtpId, rtcpId, isAudio ? STR("audio") : STR("video"));

	pRTP->SetApplication(pApplication);
	pRTCP->SetApplication(pApplication);
	return true;
}

bool InboundConnectivity::Initialize(Variant &parameters) {
	//Get the application
	BaseClientApplication *pApplication = _pRTSP->GetApplication();
	if (pApplication == NULL) {
		FATAL("RTSP protocol not yet assigned to an application");
		return false;
	}

	//Compute the bandwidth
	uint32_t bandwidth = 0;
	if (_videoTrack != V_NULL) {
		bandwidth += (uint32_t) SDP_TRACK_BANDWIDTH(_videoTrack);
	}
	if (_audioTrack != V_NULL) {
		bandwidth += (uint32_t) SDP_TRACK_BANDWIDTH(_audioTrack);
	}
	if (bandwidth == 0) {
		bandwidth = _bandwidthHint;
	}

	if ((uint64_t)SDP_VIDEO_CODEC(_videoTrack) != CONTAINER_AUDIO_VIDEO_MP2T) {
		//Set the default stream name if required
		if (_privateStreamName == "")
			_privateStreamName = format("rtsp_%u", _pRTSP->GetId());

		//Ensure the stream name to be ingested is available
		_publicStreamName = pApplication->GetIngestPointPublicName(_privateStreamName);
		if (_publicStreamName == "") {
			FATAL("No ingest point found for %s on %s", STR(_privateStreamName), STR(*_pRTSP));
			return false;
		}
		if (_publicStreamName == _privateStreamName) {
			if (!pApplication->StreamNameAvailable(_publicStreamName, _pRTSP, true)) {
				FATAL("Stream name %s already taken", STR(_privateStreamName));
				return false;
			}
		}

		//create the input network RTSP stream
		_pInStream = new InNetRTPStream(_pRTSP, _publicStreamName, _videoTrack, _audioTrack,
			bandwidth, _rtcpDetectionInterval, _a, _b);
		bool registerStreamExpiry = (bool)parameters["registerStreamExpiry"];
		if (!_pInStream->SetStreamsManager(pApplication->GetStreamsManager(), registerStreamExpiry)) {
			FATAL("Unable to set the streams manager");
			delete _pInStream;
			_pInStream = NULL;
			return false;
		}
	}
	//	pCap->avc._widthOverride=session["width"];
	//	pCap->avc._widthOverride=session[""];

	//Make the stream known to inbound RTP protocols
	//and plug in the connectivity
	InboundRTPProtocol *pTempRTP = NULL;
	RTCPProtocol *pTempRTCP = NULL;

	//INFO("Initialized audio and video ids: _rtpVideoId %d _rtcpVideoId %d _rtpAudioId %d _rtcpAudioId %d", _rtpVideoId, _rtcpVideoId, _rtpAudioId, _rtcpAudioId);
	if ((pTempRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(_rtpVideoId)) != NULL) {
		pTempRTP->SetStream(_pInStream, false, false);
		pTempRTP->SetInbboundConnectivity(this);
		pTempRTP->SetVideoCodec((uint64_t)SDP_VIDEO_CODEC(_videoTrack));
		if ((uint64_t)SDP_VIDEO_CODEC(_videoTrack) == CONTAINER_AUDIO_VIDEO_MP2T)
			pTempRTP->Initialize(parameters);
	}
	if ((pTempRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(_rtcpVideoId)) != NULL) {
		pTempRTCP->SetInbboundConnectivity(this, false);
	}
	if ((pTempRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(_rtpAudioId)) != NULL) {
		pTempRTP->SetStream(_pInStream, true, false);
		pTempRTP->SetInbboundConnectivity(this);
	}
	if ((pTempRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(_rtcpAudioId)) != NULL) {
		pTempRTCP->SetInbboundConnectivity(this, true);
	}

	if ((uint64_t)SDP_VIDEO_CODEC(_videoTrack) != CONTAINER_AUDIO_VIDEO_MP2T) {
		//Pickup all outbound waiting streams
		map<uint32_t, BaseOutStream *> subscribedOutStreams =
			pApplication->GetStreamsManager()->GetWaitingSubscribers(
			_publicStreamName, _pInStream->GetType(), true);
		//FINEST("subscribedOutStreams count: %"PRIz"u", subscribedOutStreams.size());

		//Bind the waiting subscribers

		FOR_MAP(subscribedOutStreams, uint32_t, BaseOutStream *, i) {
			BaseOutStream *pBaseOutStream = MAP_VAL(i);
			pBaseOutStream->Link(_pInStream);
		}
	}

	//Done
	return true;
}

string InboundConnectivity::GetTransportHeaderLine(bool isAudio, bool isClient) {
	if (_forceTcp) {
		uint32_t &rtpId = isAudio ? _rtpAudioId : _rtpVideoId;
		for (uint32_t i = 0; i < 255; i++) {
			if ((_pProtocols[i] != NULL) && (_pProtocols[i]->GetId() == rtpId)) {
				string result = format("RTP/AVP/TCP;unicast;interleaved=%u-%u", i, i + 1);
				return result;
			}
		}
		return "";
	} else {
		Variant &track = isAudio ? _audioTrack : _videoTrack;
		InboundRTPProtocol *pRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(
				isAudio ? _rtpAudioId : _rtpVideoId);
		RTCPProtocol *pRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(
				isAudio ? _rtcpAudioId : _rtcpVideoId);
		if ((pRTP == NULL) || (pRTCP == NULL)) {
			return "";
		}
		if (isClient) {
			return format("RTP/AVP;unicast;client_port=%"PRIu16"-%"PRIu16,
					((UDPCarrier *) pRTP->GetIOHandler())->GetNearEndpointPort(),
					((UDPCarrier *) pRTCP->GetIOHandler())->GetNearEndpointPort());
		} else {
			return format("RTP/AVP;unicast;client_port=%s;server_port=%"PRIu16"-%"PRIu16,
					STR(track["portsOrChannels"]["all"]),
					((UDPCarrier *) pRTP->GetIOHandler())->GetNearEndpointPort(),
					((UDPCarrier *) pRTCP->GetIOHandler())->GetNearEndpointPort());
		}
	}
}

bool InboundConnectivity::FeedData(uint32_t channelId, uint8_t *pBuffer,
		uint32_t bufferLength) {
	//1. Is the chanel number a valid chanel?
	if (channelId > 255) {
		FATAL("Invalid chanel number: %u", channelId);
		return false;
	}

	//2. Get the protocol
	BaseProtocol *pProtocol = _pProtocols[channelId];
	if (pProtocol == NULL) {
		FATAL("Invalid chanel number: %u", channelId);
		return false;
	}

	//3. prepare the buffer
	_inputBuffer.IgnoreAll();
	_inputBuffer.ReadFromBuffer(pBuffer, bufferLength);

	//4. feed the data
	uint32_t dummy(0);
	return pProtocol->SignalInputData(_inputBuffer, dummy, &_dummyAddress);
}

string InboundConnectivity::GetClientPorts(bool isAudio) {
	InboundRTPProtocol *pRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(
			isAudio ? _rtpAudioId : _rtpVideoId);
	RTCPProtocol *pRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(
			isAudio ? _rtcpAudioId : _rtcpVideoId);
	if ((pRTP == NULL) || (pRTCP == NULL))
		return "";
	return format("%"PRIu16"-%"PRIu16,
			((UDPCarrier *) pRTP->GetIOHandler())->GetNearEndpointPort(),
			((UDPCarrier *) pRTCP->GetIOHandler())->GetNearEndpointPort());
}

string InboundConnectivity::GetAudioClientPorts() {
	return GetClientPorts(true);
}

string InboundConnectivity::GetVideoClientPorts() {
	return GetClientPorts(false);
}

bool InboundConnectivity::SendRR(bool isAudio) {
//	if (_forceTcp)
//		return true;
	/*
	//Table from RFC3550
			0                   1                   2                   3
			0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	header |V=2|P|    RC   |   PT=RR=201   |             length            |0
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                     SSRC of packet sender                     |4
		   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	report |                 SSRC_1 (SSRC of first source)                 |8
	block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  1    | fraction lost |       cumulative number of packets lost       |12
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |           extended highest sequence number received           |16
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                      interarrival jitter                      |20
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                         last SR (LSR)                         |24
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                   delay since last SR (DLSR)                  |28
		   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	header |V=2|P|    SC   |  PT=SDES=202  |             length            |
		   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	chunk  |                          SSRC/CSRC_1                          |
	  1    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                           SDES items                          |
		   |                              ...                              |
		   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	chunk  |                          SSRC/CSRC_2                          |
	  2    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		   |                           SDES items                          |
		   |                              ...                              |
		   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	 */

	InboundRTPProtocol *pRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(
			isAudio ? _rtpAudioId : _rtpVideoId);
	RTCPProtocol *pRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(
			isAudio ? _rtcpAudioId : _rtcpVideoId);
	if ((pRTP == NULL) || (pRTCP == NULL))
		return true;

	uint8_t *pBuffer = isAudio ? _audioRR : _videoRR;

	//1. prepare the buffer
	EHTONLP(pBuffer + 12, pRTP->GetSSRC()); //SSRC_1 (SSRC of first source)
	EHTONLP(pBuffer + 20, pRTP->GetExtendedSeq()); //extended highest sequence number received
	EHTONLP(pBuffer + 28, pRTCP->GetLastSenderReport()); //last SR (LSR)

	if (_forceTcp) {
		return _pRTSP->SendRawUplink(pBuffer, 60, true);
	} else {
		if (pRTCP->GetLastAddress() != NULL) {
			if (sendto(pRTCP->GetIOHandler()->GetOutboundFd(),
					(char *) (pBuffer + 4), 56, 0,
				//(sockaddr *)pRTCP->GetLastAddress(), sizeof(sockaddr)) != 56) {
				pRTCP->GetLastAddress()->getSockAddr(), 
				pRTCP->GetLastAddress()->getLength()) != 56) {
				int err = LASTSOCKETERROR;
				FATAL("Unable to send data: %d", err);
				return false;
			}
			ADD_OUT_BYTES_MANAGED(IOHT_UDP_CARRIER, 56);
		}
		return true;
	}
}

void InboundConnectivity::ReportSR(uint64_t ntpMicroseconds,
		uint32_t rtpTimestamp, bool isAudio) {
	if (_pInStream != NULL) {
		_pInStream->ReportSR(ntpMicroseconds, rtpTimestamp, isAudio);
	}
}

InNetRTPStream* InboundConnectivity::GetInStream() {
	return _pInStream;
}

void InboundConnectivity::Cleanup() {
	_audioTrack.Reset();
	_videoTrack.Reset();
	memset(_pProtocols, 0, sizeof (_pProtocols));
	if (_pInStream != NULL) {
		delete _pInStream;
		_pInStream = NULL;
	}

	InboundRTPProtocol *pTempRTP = NULL;
	RTCPProtocol *pTempRTCP = NULL;

	if ((pTempRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(_rtpVideoId)) != NULL) {
		pTempRTP->SetStream(NULL, false, false);
		pTempRTP->EnqueueForDelete();
		_rtpVideoId = 0;
	}
	if ((pTempRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(_rtcpVideoId)) != NULL) {
		pTempRTCP->EnqueueForDelete();
		_rtcpVideoId = 0;
	}
	if ((pTempRTP = (InboundRTPProtocol *) ProtocolManager::GetProtocol(_rtpAudioId)) != NULL) {
		pTempRTP->SetStream(NULL, true, false);
		pTempRTP->EnqueueForDelete();
		_rtpAudioId = 0;
	}
	if ((pTempRTCP = (RTCPProtocol *) ProtocolManager::GetProtocol(_rtcpAudioId)) != NULL) {
		pTempRTCP->EnqueueForDelete();
		_rtcpAudioId = 0;
	}
}

bool InboundConnectivity::CreateCarriers(InboundRTPProtocol *pRTP, RTCPProtocol * pRTCP) {
	UDPCarrier *pCarrier1 = NULL;
	UDPCarrier *pCarrier2 = NULL;
	for (uint32_t i = 0; i < 10; i++) {
		if (pCarrier1 != NULL) {
			delete pCarrier1;
			pCarrier1 = NULL;
		}
		if (pCarrier2 != NULL) {
			delete pCarrier2;
			pCarrier2 = NULL;
		}
		
		// Necessary evil -- need to match the RTSP IP version that RMS is bound to.
		Variant accept = _pRTSP->GetApplication()->GetConfiguration().GetValue("acceptors", false);
		//string rtspIP = "::";
		string rtspIP = "0.0.0.0";
		FOR_MAP(accept, string, Variant, m) {
			string rtspProtocol = MAP_VAL(m)["protocol"];
			if (rtspProtocol == CONF_PROTOCOL_INBOUND_RTSP) {
				rtspIP = (string)MAP_VAL(m)["ip"];
				break;
			}
		}

		 //pCarrier1 = UDPCarrier::Create("0.0.0.0", 0, 256, 256, "");
		pCarrier1 = UDPCarrier::Create(rtspIP, 0, 256, 256, "");
		if (pCarrier1 == NULL) {
			WARN("Unable to create UDP carrier for RTP");
			continue;
		}

		if ((pCarrier1->GetNearEndpointPort() % 2) == 0) {
			 //pCarrier2 = UDPCarrier::Create("0.0.0.0",
			pCarrier2 = UDPCarrier::Create(rtspIP,
					pCarrier1->GetNearEndpointPort() + 1, 256, 256, "");
		} else {
			// pCarrier2 = UDPCarrier::Create("0.0.0.0",
			pCarrier2 = UDPCarrier::Create(rtspIP,
					pCarrier1->GetNearEndpointPort() - 1, 256, 256, "");
		}

		if (pCarrier2 == NULL) {
			WARN("Unable to create UDP carrier for RTP");
			continue;
		}

		if (pCarrier1->GetNearEndpointPort() > pCarrier2->GetNearEndpointPort()) {
			//WARN("Switch carriers");
			UDPCarrier *pTemp = pCarrier1;
			pCarrier1 = pCarrier2;
			pCarrier2 = pTemp;
		}

		pCarrier1->SetProtocol(pRTP->GetFarEndpoint());
		pRTP->GetFarEndpoint()->SetIOHandler(pCarrier1);

		pCarrier2->SetProtocol(pRTCP->GetFarEndpoint());
		pRTCP->GetFarEndpoint()->SetIOHandler(pCarrier2);

		return pCarrier1->StartAccept() | pCarrier2->StartAccept();
	}

	if (pCarrier1 != NULL) {
		delete pCarrier1;
		pCarrier1 = NULL;
	}
	if (pCarrier2 != NULL) {
		delete pCarrier2;
		pCarrier2 = NULL;
	}

	return false;
}
#endif /* HAS_PROTOCOL_RTP */
