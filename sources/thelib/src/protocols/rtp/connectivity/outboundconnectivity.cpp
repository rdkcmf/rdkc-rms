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
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "protocols/rtp/streaming/baseoutnetrtpudpstream.h"
#include "protocols/protocolmanager.h"
#include "protocols/baseprotocol.h"
#include "protocols/rtp/rtspprotocol.h"
#include "protocols/rtp/nattraversalprotocol.h"
#include "protocols/ssl/inbounddtlsprotocol.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "netio/netio.h"
#include "protocols/udpprotocol.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"

OutboundConnectivity::OutboundConnectivity(bool forceTcp, BaseProtocol *pProtocol)
: BaseConnectivity() {
	_forceTcp = forceTcp;
	_pProtocol = pProtocol;
	_multicastIP = "0.0.0.0";
	_pOutStream = NULL;

	memset(&_dataMessage, 0, sizeof (_dataMessage));
	_dataMessage.MSGHDR_MSG_IOV = new IOVEC[1];
	_dataMessage.MSGHDR_MSG_IOVLEN = 1;
	_dataMessage.MSGHDR_MSG_NAMELEN = sizeof (sockaddr_in);

	memset(&_rtcpMessage, 0, sizeof (_rtcpMessage));
	_rtcpMessage.MSGHDR_MSG_IOV = new IOVEC[1];
	_rtcpMessage.MSGHDR_MSG_IOVLEN = 1;
	_rtcpMessage.MSGHDR_MSG_NAMELEN = sizeof (sockaddr_in);
	_rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN = 28;
	_rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE = new IOVEC_IOV_BASE_TYPE[_rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN];
	((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[0] = 0x80; //V,P,RC
	((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE)[1] = 0xc8; //PT=SR=200
	EHTONSP(((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 2, 6); //length
	EHTONLP(((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 4, 0); //SSRC
	_pRTCPNTP = ((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 8;
	_pRTCPRTP = ((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 16;
	_pRTCPSPC = ((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 20;
	_pRTCPSOC = ((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 24;

	_hasVideo = false;
	_videoDataFd = (SOCKET) (-1);
	_videoDataPort = 0;
	_videoRTCPFd = (SOCKET) (-1);
	_videoRTCPPort = 0;
	_videoNATDataId = 0;
	_videoNATRTCPId = 0;
	_videoSampleRate = 0;

	_hasAudio = false;
	_audioDataFd = (SOCKET) (-1);
	_audioDataPort = 0;
	_audioRTCPFd = (SOCKET) (-1);
	_audioRTCPPort = 0;
	_audioNATDataId = 0;
	_audioNATRTCPId = 0;
	_audioSampleRate = 0;

	_startupTime = (uint64_t) time(NULL);

	_amountSent = 0;
	_dummyValue = 0;

	_rtpClient.isSRTP = pProtocol != NULL ? pProtocol->GetType() == PT_WRTC_CNX : false;
	_rtpClient.isUdp = _rtpClient.isSRTP;

	_firstFrameSent = false;
	_firstFrameSentPostResume = true;
}

OutboundConnectivity::~OutboundConnectivity() {
	delete[] _dataMessage.MSGHDR_MSG_IOV;
	delete[] ((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE);
	delete[] _rtcpMessage.MSGHDR_MSG_IOV;
	if (_pOutStream != NULL) {
		delete _pOutStream;
	}
	NATTraversalProtocol *pTemp = NULL;
	pTemp = (NATTraversalProtocol *) ProtocolManager::GetProtocol(_videoNATDataId);
	if (pTemp != NULL)
		pTemp->EnqueueForDelete();

	pTemp = (NATTraversalProtocol *) ProtocolManager::GetProtocol(_videoNATRTCPId);
	if (pTemp != NULL)
		pTemp->EnqueueForDelete();

	pTemp = (NATTraversalProtocol *) ProtocolManager::GetProtocol(_audioNATDataId);
	if (pTemp != NULL)
		pTemp->EnqueueForDelete();

	pTemp = (NATTraversalProtocol *) ProtocolManager::GetProtocol(_audioNATRTCPId);
	if (pTemp != NULL)
		pTemp->EnqueueForDelete();
}

bool OutboundConnectivity::Initialize() {
	if (_forceTcp) {
		_rtpClient.audioDataChannel = 0;
		_rtpClient.audioRtcpChannel = 1;
		_rtpClient.videoDataChannel = 2;
		_rtpClient.videoRtcpChannel = 3;
	}
	else {
		if (_rtpClient.isSRTP) {
			Variant custParams = _pProtocol->GetCustomParameters();
			if (custParams.HasKeyChain(V_BOOL, false, 1, "hasAudio")) {
				_rtpClient.hasAudio = (bool) custParams.GetValue("hasAudio", false);
			}
			if (custParams.HasKeyChain(V_BOOL, false, 1, "hasVideo")) {
				_rtpClient.hasVideo = (bool)custParams.GetValue("hasVideo", false);
			}
		} else {
			if (!InitializePorts(_videoDataFd, _videoDataPort, _videoNATDataId,
				_videoRTCPFd, _videoRTCPPort, _videoNATRTCPId)) {
				FATAL("Unable to initialize video ports");
				return false;
			}
			if (!InitializePorts(_audioDataFd, _audioDataPort, _audioNATDataId,
				_audioRTCPFd, _audioRTCPPort, _audioNATRTCPId)) {
				FATAL("Unable to initialize audio ports");
				return false;
			}
		}
	}
	return true;
}

void OutboundConnectivity::SetRTPBlockSize(uint32_t blockSize) {
	if (_pOutStream != NULL) {
		_pOutStream->SetRTPBlockSize(blockSize);
	}
}

void OutboundConnectivity::Enable(bool value) {
	if (_pOutStream != NULL)
		_pOutStream->Enable(value);
}

void OutboundConnectivity::SetOutStream(BaseOutNetRTPUDPStream *pOutStream) {
	_pOutStream = pOutStream;
}

string OutboundConnectivity::GetVideoPorts() {
	return format("%"PRIu16"-%"PRIu16, _videoDataPort, _videoRTCPPort);
}

string OutboundConnectivity::GetAudioPorts() {
	return format("%"PRIu16"-%"PRIu16, _audioDataPort, _audioRTCPPort);
}

string OutboundConnectivity::GetVideoChannels() {
	return format("%"PRIu8"-%"PRIu8, _rtpClient.videoDataChannel,
			_rtpClient.videoRtcpChannel);
}

string OutboundConnectivity::GetAudioChannels() {
	return format("%"PRIu8"-%"PRIu8, _rtpClient.audioDataChannel,
			_rtpClient.audioRtcpChannel);
}

uint32_t OutboundConnectivity::GetAudioSSRC() {
	if (_pOutStream != NULL)
		return _pOutStream->AudioSSRC();
	return 0;
}

uint32_t OutboundConnectivity::GetVideoSSRC() {
	if (_pOutStream != NULL)
		return _pOutStream->VideoSSRC();
	return 0;
}

uint16_t OutboundConnectivity::GetLastVideoSequence() {
	return _pOutStream->VideoCounter();
}

uint16_t OutboundConnectivity::GetLastAudioSequence() {
	return _pOutStream->AudioCounter();
}

void OutboundConnectivity::HasAudio(bool value) {
	_hasAudio = value;
	_pOutStream->HasAudioVideo(_hasAudio, _hasVideo);
}

void OutboundConnectivity::HasVideo(bool value) {
	_hasVideo = value;
	_pOutStream->HasAudioVideo(_hasAudio, _hasVideo);
}

bool OutboundConnectivity::RegisterUDPVideoClient(uint32_t rtspProtocolId,
		SocketAddress& data, SocketAddress& rtcp) {
	if (_rtpClient.hasVideo) {
		//WARN("Client already registered for video feed");
		return true;
	}
	_rtpClient.hasVideo = true;
	_rtpClient.isUdp = true;
	_rtpClient.videoDataAddress = data;
	_rtpClient.videoRtcpAddress = rtcp;
	_rtpClient.protocolId = rtspProtocolId;
	NATTraversalProtocol *pVideoNATData =
			(NATTraversalProtocol *) ProtocolManager::GetProtocol(_videoNATDataId);
	NATTraversalProtocol *pVideoNATRTCP =
			(NATTraversalProtocol *) ProtocolManager::GetProtocol(_videoNATRTCPId);
	bool result = true;
	if (pVideoNATData != NULL) {
		pVideoNATData->SetOutboundAddress(&_rtpClient.videoDataAddress);
		result &= ((UDPCarrier *) pVideoNATData->GetIOHandler())->StartAccept();
	}
	if (pVideoNATRTCP != NULL) {
		pVideoNATRTCP->SetOutboundAddress(&_rtpClient.videoRtcpAddress);
		result &= ((UDPCarrier *) pVideoNATRTCP->GetIOHandler())->StartAccept();
	}

	return result;
}


bool OutboundConnectivity::RegisterUDPVideoClient(uint32_t protocolId,
	SocketAddress &data) {
	if (_rtpClient.hasVideo) {
		//WARN("Client already registered for video feed");
		return true;
	}
	_rtpClient.hasVideo = true;
	_rtpClient.isUdp = true;
	_rtpClient.videoDataAddress = data;
	_rtpClient.protocolId = protocolId;
	NATTraversalProtocol *pVideoNATData =
		(NATTraversalProtocol *)ProtocolManager::GetProtocol(_videoNATDataId);
	bool result = true;
	if (pVideoNATData != NULL) {
		pVideoNATData->SetOutboundAddress(&_rtpClient.videoDataAddress);
		result &= ((UDPCarrier *)pVideoNATData->GetIOHandler())->StartAccept();
	}
	return result;
}

bool OutboundConnectivity::RegisterUDPAudioClient(uint32_t rtspProtocolId,
		SocketAddress& data, SocketAddress& rtcp) {
	if (_rtpClient.hasAudio) {
		//WARN("Client already registered for audio feed");
		return true;
	}
	_rtpClient.hasAudio = true;
	_rtpClient.isUdp = true;
	_rtpClient.audioDataAddress = data;
	_rtpClient.audioRtcpAddress = rtcp;
	_rtpClient.protocolId = rtspProtocolId;

	NATTraversalProtocol *pAudioNATData =
			(NATTraversalProtocol *) ProtocolManager::GetProtocol(_audioNATDataId);

	NATTraversalProtocol *pAudioNATRTCP =
			(NATTraversalProtocol *) ProtocolManager::GetProtocol(_audioNATRTCPId);

	bool result = true;
	if (pAudioNATData != NULL) {
		pAudioNATData->SetOutboundAddress(&_rtpClient.audioDataAddress);
		result &= ((UDPCarrier *) pAudioNATData->GetIOHandler())->StartAccept();
	}
	if (pAudioNATRTCP != NULL) {
		pAudioNATRTCP->SetOutboundAddress(&_rtpClient.audioRtcpAddress);
		result &= ((UDPCarrier *) pAudioNATRTCP->GetIOHandler())->StartAccept();
	}
	return result;
}

bool OutboundConnectivity::RegisterUDPAudioClient(uint32_t protocolId,
	SocketAddress &data) {
	if (_rtpClient.hasAudio) {
		//WARN("Client already registered for audio feed");
		return true;
	}
	_rtpClient.hasAudio = true;
	_rtpClient.isUdp = true;
	_rtpClient.audioDataAddress = data;
	_rtpClient.protocolId = protocolId;

	NATTraversalProtocol *pAudioNATData =
		(NATTraversalProtocol *)ProtocolManager::GetProtocol(_audioNATDataId);

	bool result = true;
	if (pAudioNATData != NULL) {
		pAudioNATData->SetOutboundAddress(&_rtpClient.audioDataAddress);
		result &= ((UDPCarrier *)pAudioNATData->GetIOHandler())->StartAccept();
	}
	return result;
}

bool OutboundConnectivity::RegisterTCPVideoClient(uint32_t rtspProtocolId,
		uint8_t data, uint8_t rtcp) {
	if (_rtpClient.hasVideo) {
		//WARN("Client already registered for video feed");
		return true;
	}
	_rtpClient.hasVideo = true;
	_rtpClient.isUdp = false;
	_rtpClient.videoDataChannel = data;
	_rtpClient.videoRtcpChannel = rtcp;
	_rtpClient.protocolId = rtspProtocolId;
	return true;
}

bool OutboundConnectivity::RegisterTCPAudioClient(uint32_t rtspProtocolId,
		uint8_t data, uint8_t rtcp) {
	if (_rtpClient.hasAudio) {
		//WARN("Client already registered for audio feed");
		return true;
	}
	_rtpClient.hasAudio = true;
	_rtpClient.isUdp = false;
	_rtpClient.audioDataChannel = data;
	_rtpClient.audioRtcpChannel = rtcp;
	_rtpClient.protocolId = rtspProtocolId;
	return true;
}

void OutboundConnectivity::SignalDetachedFromInStream() {
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(_rtpClient.protocolId);
	if (pProtocol != NULL) {
		pProtocol->EnqueueForDelete();
	}
	_pProtocol = NULL;
}

bool OutboundConnectivity::FeedVideoData(MSGHDR &message,
		double pts, double dts) {
	if (!FeedData(message, pts, dts, false)) {
		FATAL("Unable to feed video UDP clients");
		return false;
	}
	return true;
}

bool OutboundConnectivity::FeedAudioData(MSGHDR &message,
		double pts, double dts) {
	if (!FeedData(message, pts, dts, true)) {
		FATAL("Unable to feed audio UDP clients");
		return false;
	}
	return true;
}

void OutboundConnectivity::ReadyForSend() {
	if (_pProtocol != NULL)
		_pProtocol->ReadyForSend();
}

bool OutboundConnectivity::InitializeRTPPort(SOCKET &rtpFd, uint16_t &rtpPort, uint32_t &natRtpId) {
	UDPCarrier *pCarrier = NULL;
	NATTraversalProtocol *pNATRtp = NULL;
	pCarrier = UDPCarrier::Create(_multicastIP, 0, 256, 256, "");
	if (pCarrier == NULL) {
		WARN("Unable to create UDP carrier for RTP");
		return false;
	}

	Variant dummy;

	rtpFd = pCarrier->GetInboundFd();
	rtpPort = pCarrier->GetNearEndpointPort();
	pNATRtp = (NATTraversalProtocol *)ProtocolFactoryManager::CreateProtocolChain(
		CONF_PROTOCOL_RTP_NAT_TRAVERSAL, dummy);
	if (pNATRtp == NULL) {
		FATAL("Unable to create the protocol chain %s", CONF_PROTOCOL_RTP_NAT_TRAVERSAL);
		return false;
	}
	pCarrier->SetProtocol((pNATRtp->GetFarEndpoint()));
	pNATRtp->GetFarEndpoint()->SetIOHandler(pCarrier);
	natRtpId = pNATRtp->GetId();
	return true;
}

bool OutboundConnectivity::PushMetaData(string const& vmfMetadata, int64_t pts) {
	switch (_pProtocol->GetType()) {
	case PT_WRTC_CNX:
	{
		WrtcConnection *pWrtc = (WrtcConnection *)_pProtocol;
		if (pWrtc->GetCommandChannelId() == 0)
			return false;
		Variant payload;
		payload["timestamp"] = pts;
		payload["data"] = vmfMetadata;
		pWrtc->PushMetaData(payload);
		return true;;
	}
	default:
		return true; // returning true so that the metadata will be erased from the config
	}
}

bool OutboundConnectivity::InitializePorts(SOCKET &dataFd, uint16_t &dataPort,
		uint32_t &natDataId, SOCKET &RTCPFd, uint16_t &RTCPPort,
		uint32_t &natRTCPId) {
	UDPCarrier *pCarrier1 = NULL;
	UDPCarrier *pCarrier2 = NULL;
	NATTraversalProtocol *pNATData = NULL;
	NATTraversalProtocol *pNATRTCP = NULL;
	for (uint32_t i = 0; i < 10; i++) {
		if (pCarrier1 != NULL) {
			delete pCarrier1;
			pCarrier1 = NULL;
		}
		if (pCarrier2 != NULL) {
			delete pCarrier2;
			pCarrier2 = NULL;
		}

		pCarrier1 = UDPCarrier::Create(_multicastIP, 0, 256, 256, "");
		if (pCarrier1 == NULL) {
			WARN("Unable to create UDP carrier for RTP");
			continue;
		}

		if ((pCarrier1->GetNearEndpointPort() % 2) == 0) {
			pCarrier2 = UDPCarrier::Create(_multicastIP,
					pCarrier1->GetNearEndpointPort() + 1, 256, 256, "");
		} else {
			pCarrier2 = UDPCarrier::Create(_multicastIP,
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

		Variant dummy;
		//data


		dataFd = pCarrier1->GetInboundFd();
		dataPort = pCarrier1->GetNearEndpointPort();
		pNATData = (NATTraversalProtocol *) ProtocolFactoryManager::CreateProtocolChain(
				CONF_PROTOCOL_RTP_NAT_TRAVERSAL, dummy);
		if (pNATData == NULL) {
			FATAL("Unable to create the protocol chain %s", CONF_PROTOCOL_RTP_NAT_TRAVERSAL);
			return false;
		}
		pCarrier1->SetProtocol((pNATData->GetFarEndpoint()));
		pNATData->GetFarEndpoint()->SetIOHandler(pCarrier1);

		//RTCP
		RTCPFd = pCarrier2->GetInboundFd();
		RTCPPort = pCarrier2->GetNearEndpointPort();
		pNATRTCP = (NATTraversalProtocol *) ProtocolFactoryManager::CreateProtocolChain(
				CONF_PROTOCOL_RTP_NAT_TRAVERSAL, dummy);
		if (pNATRTCP == NULL) {
			FATAL("Unable to create the protocol chain %s", CONF_PROTOCOL_RTP_NAT_TRAVERSAL);
			pNATData->EnqueueForDelete();
			pNATData = NULL;
			return false;
		}
		pCarrier2->SetProtocol(pNATRTCP->GetFarEndpoint());
		pNATRTCP->GetFarEndpoint()->SetIOHandler(pCarrier2);

		natDataId = pNATData->GetId();
		natRTCPId = pNATRTCP->GetId();

		//return pCarrier1->StartAccept() & pCarrier2->StartAccept();
		return true;
	}

	if (pNATData != NULL)
		pNATData->EnqueueForDelete();

	if (pNATRTCP != NULL)
		pNATRTCP->EnqueueForDelete();


	return false;
}

bool OutboundConnectivity::FeedData(MSGHDR &message, double pts, double dts,
		bool isAudio) {
	if (pts < 0 || dts < 0)
		return true;

	//	uint8_t *pDbg = (uint8_t*) message.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE;
	//	uint16_t flags = ENTOHSP(pDbg);
	//	uint16_t seq = ENTOHSP(pDbg + 2);
	//	uint32_t ts = ENTOHLP(pDbg + 4);
	//	uint32_t csrc = ENTOHLP(pDbg + 8);
	//	string dbg = format("%c: flags: %04"PRIx16"; seq: %5"PRIu16"; rtp: %10"PRIu32"; csrc: %08"PRIx32"; %.2f/%.2f",
	//			isAudio ? 'A' : 'V', flags, seq, ts, csrc, ((double) ts) / (isAudio ? 44.100 : 90.0), pts);
	//	if (isAudio)
	//		WARN("%s", STR(dbg));
	//	else
	//		FINEST("%s", STR(dbg));


	double &rate = isAudio ? _audioSampleRate : _videoSampleRate;
	if (rate == 0) {
		if (isAudio) {
			StreamCapabilities *pCapabilities = _pOutStream->GetCapabilities();
			AudioCodecInfo *pInfo = NULL;
#ifdef HAS_G711
			uint64_t codecType = pCapabilities->GetAudioCodecType();
			if ((pCapabilities != NULL)
					&& (codecType == CODEC_AUDIO_AAC
					|| ((codecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
					) && ((pInfo = pCapabilities->GetAudioCodec()) != NULL)) {
#else
			if ((pCapabilities != NULL)
					&& (pCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC)
					&& ((pInfo = pCapabilities->GetAudioCodec<AudioCodecInfo > ()) != NULL)) {
#endif	/* HAS_G711 */
				rate = pInfo->_samplingRate;
			} else {
				rate = 1;
			}
		} else {
			StreamCapabilities *pCapabilities = _pOutStream->GetCapabilities();
			VideoCodecInfo *pInfo = NULL;
			if ((pCapabilities != NULL)
					&& (pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)
					&& ((pInfo = pCapabilities->GetVideoCodec<VideoCodecInfo > ()) != NULL)) {
				rate = pInfo->_samplingRate;
			} else {
				rate = 1;
			}
		}
	}
	uint32_t ssrc = isAudio ? _pOutStream->AudioSSRC() : _pOutStream->VideoSSRC();
	uint16_t messageLength = 0;
	for (uint32_t i = 0; i < (uint32_t) message.MSGHDR_MSG_IOVLEN; i++) {
		messageLength += (uint16_t) message.MSGHDR_MSG_IOV[i].IOVEC_IOV_LEN;
	}

	bool &hasTrack = isAudio ? _rtpClient.hasAudio : _rtpClient.hasVideo;
	uint32_t &packetsCount = isAudio ? _rtpClient.audioPacketsCount : _rtpClient.videoPacketsCount;
	uint32_t &bytesCount = isAudio ? _rtpClient.audioBytesCount : _rtpClient.videoBytesCount;
	if (!hasTrack && !_rtpClient.isSRTP) {
		return true;
	}

	if ((packetsCount % 500) == 0) {
		//FINEST("Send %c RTCP: %u", isAudio ? 'A' : 'V', packetsCount);
		EHTONLP(((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE) + 4, ssrc); //SSRC

		//NTP
		uint32_t integerValue = (uint32_t) (pts / 1000.0);
		double fractionValue = (pts / 1000.0 - ((uint32_t) (pts / 1000.0)))*4294967296.0;
		uint64_t ntpVal = (_startupTime + integerValue + 2208988800ULL) << 32;
		ntpVal |= (uint32_t) fractionValue;
		EHTONLLP(_pRTCPNTP, ntpVal);

		//RTP
		uint64_t rtp = (uint64_t) (((double) (integerValue) + fractionValue / 4294967296.0) * rate);
		rtp &= 0xffffffff;
		EHTONLP(_pRTCPRTP, (uint32_t) rtp);

		//packet count
		EHTONLP(_pRTCPSPC, packetsCount);

		//octet count
		EHTONLP(_pRTCPSOC, bytesCount);
		//			FINEST("\n%s", STR(IOBuffer::DumpBuffer(((uint8_t *) _rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE),
		//					_rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN)));

		if (_rtpClient.isUdp) {
			if (_rtpClient.isSRTP) {
				//InboundDTLSProtocol *pDtls = (InboundDTLSProtocol *)_pProtocol;
				//pDtls->FeedRtcpData(_rtcpMessage, ssrc);
				WriteToBuffer(*_pProtocol->GetOutputBuffer(), _rtcpMessage);
				_pProtocol->EnqueueForOutbound();
			}
			else {
				SOCKET rtcpFd = isAudio ? _audioRTCPFd : _videoRTCPFd;
//				sockaddr_in &rtcpAddress = isAudio ? _rtpClient.audioRtcpAddress : _rtpClient.videoRtcpAddress;
				SocketAddress &rtcpAddress = isAudio ? _rtpClient.audioRtcpAddress : _rtpClient.videoRtcpAddress;
				_rtcpMessage.MSGHDR_MSG_NAME = rtcpAddress.getSockAddr();
				_rtcpMessage.MSGHDR_MSG_NAMELEN = rtcpAddress.getLength();
//				_rtcpMessage.MSGHDR_MSG_NAME = (sockaddr *)& rtcpAddress;  
				//WARN("===Outbound: %" PRIu64, clock() / CLOCKS_PER_SEC);
				if (!_rtpClient.isSRTP) {
					_amountSent = SENDMSG(rtcpFd, &_rtcpMessage, 0, &_dummyValue);
					if (_amountSent < 0) {
						FATAL("Unable to send message");
						return false;
					}
					ADD_OUT_BYTES_MANAGED(IOHT_UDP_CARRIER, _amountSent);
				}
			}
		} else {
			RTSPProtocol *pRTSPProtocol = (RTSPProtocol *)_pProtocol;
			if (pRTSPProtocol != NULL) {
				if (!pRTSPProtocol->SendRaw(&_rtcpMessage,
						(uint16_t) (_rtcpMessage.MSGHDR_MSG_IOV[0].IOVEC_IOV_LEN), &_rtpClient, isAudio, false, true)) {
					FATAL("Unable to send raw rtcp audio data");
					return false;
				}
			}
		}
	}


	if (_rtpClient.isUdp) {
		if (_rtpClient.isSRTP) {
			//InboundDTLSProtocol *pDtls = (InboundDTLSProtocol *)_pProtocol;
			//pDtls->FeedRtpData(message, ssrc);
			//uint8_t *pDbg = (uint8_t*)message.MSGHDR_MSG_IOV[0].IOVEC_IOV_BASE;
			//uint16_t seq = ENTOHSP(pDbg + 2);
			//LOG_LATENCY("Sending to DTLS|SSRC: %"PRIu32"|RTP seq num: %"PRIu16, ssrc, seq);
			WriteToBuffer(*_pProtocol->GetOutputBuffer(), message);
			_pProtocol->EnqueueForOutbound();

			//RTP data has been sent
			if(!_firstFrameSent && _pOutStream){
				// client only needs a debug log for this now...
				//_pOutStream->GetEventLogger()->LogOutStreamFirstFrameSent(_pOutStream);

				if (_pProtocol->GetType() == PT_WRTC_CNX) {
					WrtcConnection *pWrtc = (WrtcConnection *)_pProtocol;
					if (pWrtc->GetPeerState() != "")
						INFO("Peer State Transition: %s-active", STR(pWrtc->GetPeerState()));

					if (pWrtc->SetPeerState(WRTC_PEER_ACTIVE)) {
						INFO( "Peer State has been set to \"active\"" );
					}
					else {
						WARN( "Unable to move the Peer State to \"active\"" );
					}
				}
				else {
					WARN( "Wrtc Connection unreachable. Unable to move the Peer State to \"active\"" );
				}

				INFO( "First frame sent to player (RTP)" );
				//Output the time it took to get to this point
				std::stringstream sstrm;
				sstrm << _pProtocol->GetId();
				string probSess("wrtcplayback");
				probSess += sstrm.str();
				uint64_t ts = PROBE_TRACK_TIME_MS(STR(probSess) );
				INFO("First frame sent event:%"PRId64"", ts);
				// We need to go get the other data we stored earlier to report here
				probSess += "wrtcConn";
				uint64_t wconTs = PROBE_GET_STORED_TIME(probSess,false);
				INFO("Session statistics:%"PRId32",%"PRId64",%"PRId64"", _pProtocol->GetId(), wconTs, ts );
				// Store the time taken to send the first frame as we need to log at the end of the session
				probSess += "firstFrame";
				PROBE_STORE_TIME(STR(probSess),ts);
				_firstFrameSent = true;
			}

      //RTP data has been sent post Resume command from the client
      if(!_firstFrameSentPostResume && _pOutStream){
        INFO( "First frame sent to player (RTP) post Resume Command from client" );
        //Output the time it took to get to this point
        std::stringstream sstrm;
        sstrm << _pProtocol->GetId();
        string probSess("wrtcplayback");
        probSess += sstrm.str();
        uint64_t ts = PROBE_TRACK_TIME_MS(STR(probSess) );
        INFO("First frame sent event post Resume command from client:%"PRId64"", ts);
        // Store the time taken to send the first frame as we need to log at the end of the session
        probSess += "firstFramePostResume";
        PROBE_STORE_TIME(STR(probSess),ts);
        _firstFrameSentPostResume = true;

			}
		}
		else {
			SOCKET dataFd = isAudio ? _audioDataFd : _videoDataFd;
//			sockaddr_in &dataAddress = isAudio ? _rtpClient.audioDataAddress : _rtpClient.videoDataAddress;
			SocketAddress &dataAddress = isAudio ? _rtpClient.audioDataAddress : _rtpClient.videoDataAddress;
			message.MSGHDR_MSG_NAME = dataAddress.getSockAddr();
			message.MSGHDR_MSG_NAMELEN = dataAddress.getLength();
//			message.MSGHDR_MSG_NAME = (sockaddr *)& dataAddress;
			_amountSent = SENDMSG(dataFd, &message, 0, &_dummyValue);
			if (_amountSent < 0) {
				int err = LASTSOCKETERROR;
				FATAL("Unable to send message: %d", err);
				return false;
			}
			ADD_OUT_BYTES_MANAGED(IOHT_UDP_CARRIER, _amountSent);
		}
	} else {
		RTSPProtocol *pRTSPProtocol = (RTSPProtocol *)_pProtocol;
		if (pRTSPProtocol != NULL) {
			if (!pRTSPProtocol->SendRaw(&message, messageLength, &_rtpClient,
					isAudio, true, true)) {
				FATAL("Unable to send raw rtcp audio data");
				return false;
			}
		}
	}

	packetsCount++;
	bytesCount += messageLength;

	return true;
}

void OutboundConnectivity::SetMulticastIP(string ip) {
	_multicastIP = ip;
}
void OutboundConnectivity::WriteToBuffer(IOBuffer &dest, MSGHDR src) {
	for (uint8_t i = 0; i < src.MSGHDR_MSG_IOVLEN; i++) {
		dest.ReadFromBuffer((uint8_t *)src.MSGHDR_MSG_IOV[i].IOVEC_IOV_BASE, src.MSGHDR_MSG_IOV[i].IOVEC_IOV_LEN);
	}
}
void OutboundConnectivity::SendDirect(IOBuffer &data) {
	if (_rtpClient.isSRTP) { // enable only for SRTP for now
		_pProtocol->GetOutputBuffer()->ReadFromBuffer(GETIBPOINTER(data), GETAVAILABLEBYTESCOUNT(data));
		_pProtocol->EnqueueForOutbound();
	}
}
#endif /* HAS_PROTOCOL_RTP */
