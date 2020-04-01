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
#include "protocols/rtp/rtcpprotocol.h"
#include "protocols/rtp/connectivity/inboundconnectivity.h"
#include "streaming/streamdebug.h"
#include "utils/misc/socketaddress.h"

RTCPProtocol::RTCPProtocol()
: BaseProtocol(PT_RTCP) {
	_pConnectivity = NULL;
	_lsr = 0;
	_buff[0] = 0x81; //V,P,RC
	_buff[1] = 0xc9; //PT
	_buff[2] = 0x00; //length
	_buff[3] = 0x07; //length
	EHTONLP(_buff + 4, GetId()); //SSRC of packet sender
	EHTONLP(_buff + 12, 0); //fraction lost/cumulative number of packets lost
	EHTONLP(_buff + 20, 0); //interarrival jitter
	EHTONLP(_buff + 28, 0); // delay since last SR (DLSR)
	_isAudio = false;
	_ssrc = rand();
	_ssrc ^= GetId();
	_validLastAddress = false;
#ifdef HAS_STREAM_DEBUG
	_pStreamDebugFile = new StreamDebugFile(format("./rtcp_%"PRIu32".bin", GetId()),
			sizeof (StreamDebugRTCP));
#endif /* HAS_STREAM_DEBUG */
}

RTCPProtocol::~RTCPProtocol() {
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL) {
		delete _pStreamDebugFile;
		_pStreamDebugFile = NULL;
	}
#endif /* HAS_STREAM_DEBUG */
}

bool RTCPProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool RTCPProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_UDP;
}

bool RTCPProtocol::AllowNearProtocol(uint64_t type) {
	NYIR;
}

bool RTCPProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("operation not supported");
	NYIR;
}

IOBuffer* RTCPProtocol::GetOutputBuffer() {
	return &_outputBuffer;
}

SocketAddress* RTCPProtocol::GetDestInfo() {
	SocketAddress* sa = new SocketAddress();
	sa->setIPAddress(_farIP, _farPort);
	return sa;
//
//	vector<string> ipf; // holds string for each ipAddr sub-field
//	split(_farIP, ".", ipf);
//	if (ipf.size() != 4) {
//		WARN("Wrong number of IP pieces: %"PRIu32, (uint32_t)ipf.size());
//		return NULL;
//	}
//	uint32_t ip = (atoi(STR(ipf[0])) << 24) | (atoi(STR(ipf[1])) << 16) | (atoi(STR(ipf[2])) << 8) | (atoi(STR(ipf[3])));
//	
//	struct sockaddr_in* sa = new sockaddr_in;
//	memset(sa, 0, sizeof(sockaddr_in));
//
//	sa->sin_family = AF_INET;
//#ifdef WIN32
//	sa->sin_addr.S_un.S_addr = EHTONL(ip);
//#else
//	sa->sin_addr.s_addr = EHTONL(ip);
//#endif
//	sa->sin_port = EHTONS(_farPort);
//
//	return sa;
}

void RTCPProtocol::SetOutboundPayload(string const& farIP, uint16_t farPort, string const& payload) {
	_outputBuffer.IgnoreAll();
	_outputBuffer.ReadFromString(payload);
	_farIP = farIP;
	_farPort = farPort;
}


bool RTCPProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	//0. Save the last known address
	if (&_lastAddress != pPeerAddress) {
		_lastAddress = *pPeerAddress;
		_validLastAddress = true;
	}

	//1. Parse the SR
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t bufferLength = GETAVAILABLEBYTESCOUNT(buffer);
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL)
		_pStreamDebugFile->PushRTCP(pBuffer, bufferLength);
#endif /* HAS_STREAM_DEBUG */
	while (bufferLength > 0) {
		if (bufferLength < 4) {
			buffer.IgnoreAll();
			return true;
		}

		uint8_t PT = pBuffer[1];
		uint16_t len = ENTOHSP(pBuffer + 2);
		len = (len + 1)*4;
		if (len > bufferLength) {
			buffer.IgnoreAll();
			return true;
		}

		switch (PT) {
			case 200: //SR
			{
				if (len < 28) {
					buffer.IgnoreAll();
					return true;
				}
				uint32_t ntpSec = ENTOHLP(pBuffer + 8) - 2208988800UL;
				uint32_t ntpFrac = ENTOHLP(pBuffer + 12);
				uint64_t ntpMicroseconds = (uint32_t) (((double) ntpFrac / (double) (0x100000000LL))*1000000.0);
				ntpMicroseconds += ((uint64_t) ntpSec)*1000000;
				uint32_t rtpTimestamp = ENTOHLP(pBuffer + 16);
				if (_pConnectivity == NULL) {
					FATAL("No connectivity, unable to send SR");
					return false;
				}
				_pConnectivity->ReportSR(ntpMicroseconds, rtpTimestamp, _isAudio);

				_lsr = ENTOHLP(pBuffer + 10);

				if (!_pConnectivity->SendRR(_isAudio)) {
					FATAL("Unable to send RR");
					_pConnectivity->EnqueueForDelete();
					_pConnectivity = NULL;
					return false;
				}
				break;
			}
			case 203: //BYE
			{
				if (_pConnectivity == NULL) {
					FATAL("No connectivity, BYE packet ignored");
					return false;
				}
				_pConnectivity->EnqueueForDelete();
				_pConnectivity = NULL;
				break;
			}
			default:
			{
				break;
			}
		}

		buffer.Ignore(len);

		pBuffer = GETIBPOINTER(buffer);
		bufferLength = GETAVAILABLEBYTESCOUNT(buffer);
	}

	return true;
}

bool RTCPProtocol::SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength, SocketAddress *pPeerAddress) {
	return SignalInputData(buffer, pPeerAddress);
}
bool RTCPProtocol::SignalInputData(IOBuffer &buffer) {
	return SignalInputData(buffer, &_lastAddress);
}

uint32_t RTCPProtocol::GetLastSenderReport() {
	return _lsr;
}

SocketAddress *RTCPProtocol::GetLastAddress() {
	if (_validLastAddress)
		return &_lastAddress;
	else
		return NULL;
}

uint32_t RTCPProtocol::GetSSRC() {
	return _ssrc;
}

void RTCPProtocol::SetInbboundConnectivity(InboundConnectivity *pConnectivity, bool isAudio) {
	_pConnectivity = pConnectivity;
	_isAudio = isAudio;
}
#endif /* HAS_PROTOCOL_RTP */
