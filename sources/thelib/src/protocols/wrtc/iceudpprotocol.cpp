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
#ifdef HAS_PROTOCOL_WEBRTC
#include "protocols/wrtc/iceudpprotocol.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/wrtc/stunmsg.h"
#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "netio/netio.h"

IceUdpProtocol::IceUdpProtocol() 
: BaseIceProtocol(PT_UDP) {
	_fd = (SOCKET)0;
	_isTcp = false;
}

IceUdpProtocol::~IceUdpProtocol() {
//#ifdef WEBRTC_DEBUG
	FINE("IceUdpProtocol deleted.");
//#endif
	if (_pCarrier != NULL) {
		UDPCarrier *pCarrier = _pCarrier;
		_pCarrier = NULL;
		pCarrier->SetProtocol(NULL);
		delete pCarrier;
	}
}

void IceUdpProtocol::Start(time_t ms) {
//#ifdef WEBRTC_DEBUG
	DEBUG("UDP STUN Start called for host: %s", STR(_bindIp));
//#endif
	// start prepping outgoing traffic
	// create our local host IP as a candidate
	_hostCan = Candidate::CreateHost(_hostIpStr);
	_pSig->SendCandidate(_hostCan);
	_hostCan->SetStatePierceSent();
	
	bool result;
	FastTick(false, result);	// kick send to Stun server
	// we set started here so above Tick() just does the Stun server calls
	_started = true;
}

bool IceUdpProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	return SignalInputData(_inputBuffer, pPeerAddress);
}

// SignalInputData needs to route to: STUN, DTLS, or RTP
// the following is from the RFC, (B = first byte of packet)
//             +----------------+
//             | 127 < B < 192 -+--> forward to RTP
//             |                |
// packet -->  |  19 < B < 64  -+--> forward to DTLS
//             |                |
//             |       B < 2   -+--> forward to STUN
//             +----------------+
// We also need to handle the TURN case,
// where the data is encapsulated within a DataIndication STUN message
// See HandleServerMsg() for the above re-check where DataIndication is unwrapped
//
bool IceUdpProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	string ipPortStr =  pPeerAddress->getIPwithPort();

//	WARN("BaseIceProtocol(%s) got %d bytes from: %s",
//		STR(_bindIp), GETAVAILABLEBYTESCOUNT(buffer), STR(ipPortStr));

	// first see if it is a Stun Message
	if (StunMsg::IsStun(buffer)) {
		// create a StunMsg object!
		StunMsg * pMsg = StunMsg::ParseBuffer(buffer, ipPortStr);
		int type = pMsg->GetType();

//		INFO("StunMsg parsed type: 0x%x, %d bytes",
//				type, pMsg->GetMessageLen());	//

		int err = pMsg->GetErrorCodeAttr();
		bool fromServer = pMsg->IsFromIpStr(_stunServerIpStr)
			|| pMsg->IsFromIpStr(_turnServerIpStr);
		if (fromServer) {
			//INFO("$b2$ BaseIceProtocol calling HandleServerMsg(type: %d, Err: %d)", type, err);
			HandleServerMsg(pMsg, type, err, buffer);
		}
		else {
			//INFO("$b2$ BaseIceProtocol calling HandlePeerMsg(type: %d, Err : %d)", type, err);
			HandlePeerMsg(pMsg, type, err, false);
		}
		delete pMsg;
		buffer.IgnoreAll();
		return true;
	} else {	// Not STUN
		// Bubble up any non-STUN messages to the upper protocol
		if (_pNearProtocol != NULL) {
			_pNearProtocol->SignalInputData(buffer, pPeerAddress);
		}
	}
	
	buffer.IgnoreAll();
	
	return true;
}

int IceUdpProtocol::CreateUdpSocket() {
	//create the UDP carrier used to receive UDP read signals
	_pCarrier = UDPCarrier::Create("::", 0, (BaseProtocol *)this, 256, 256, "");
	if (_pCarrier == NULL) {
		FATAL("Unable to spawn UDP carrier");
		return -1;
	}
	_pCarrier->SetSurviveError();	// avoid "network unreachable" killing the carrier
	//
	// get our port and form our _hostIpStr
	_bindPort = _pCarrier->GetNearEndpointPort();
	_bindAddress.setIPAddress(_bindIp, _bindPort);
	_hostIpStr = _bindAddress.getIPwithPort();
//#ifdef WEBRTC_DEBUG
	DEBUG("ICE UDP host: %s", STR(_hostIpStr));
//#endif
	//start accepting of packets process
	if (!_pCarrier->StartAccept()) {
		FATAL("Unable to start receiving data over UDP carrier");
		return -1;
	}
	// does the fd end up getting set in me??
	//get the resulted fd
	//int fd = pProtocol->GetFd();
	_fd = _pCarrier->GetOutboundFd();
	if (_fd < 0) {
		FATAL("Can't get outbound SOCKET");
		return -1;
	}
	if (_fd == 0) {
		WARN("Invalid STUN UDP socket.");
	}
	else {
//#ifdef WEBRTC_DEBUG
		DEBUG("STUN Opened UDP Port[%s]: _fd:0x%X", STR(_hostIpStr), (uint32_t)_fd);
//#endif
	}
	return (int)_fd;
}

bool IceUdpProtocol::SendTo(string ip, uint16_t port, uint8_t * pData, int len) {
	SocketAddress destIp;
	if (destIp.setIPAddress(ip, port)) {
		int sent = 0;
		int err = 0;
		do {
			sent = sendto(_fd, (char *)pData, len, 0, destIp.getSockAddr(), destIp.getLength());
			err = errno;
		} while ((sent == -1) && (err == 55));

		if (sent != len) {
			if (err == 55) {
				FATAL("Error 55");
				return true;
			}
			FATAL("Error (%d - %s) sending [%d of %d] to via FD:0x%x remote: %s:%" PRIu16,
				err, strerror(err), sent, len, (uint32_t)_fd, STR(ip), port);
			LOG_LATENCY("STUN Failed");
			return false;
		} else {
			//		INFO("$b2$ WrtcStun Sent %d bytes, to: 0x%x : 0x%x",
			//			sent, ip, (uint32_t)port);
			/**
			WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
			(int)pData[0], (int)pData[1], (int)pData[2], (int)pData[3],
			(int)pData[4], (int)pData[5], (int)pData[6], (int)pData[7]
			);
			WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
			(int)pData[8], (int)pData[9], (int)pData[10], (int)pData[11],
			(int)pData[12], (int)pData[13], (int)pData[14], (int)pData[15]
			);
			WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
			(int)pData[16], (int)pData[17], (int)pData[18], (int)pData[19],
			(int)pData[20], (int)pData[21], (int)pData[22], (int)pData[23]
			);
			**/
			LOG_LATENCY("STUN Delivery");
		}
		return true;
	} else {
		FATAL("Invalid IP(%s)/Port(%d)", STR(ip), port);
	}
	return false;
}

bool IceUdpProtocol::SendToV4(uint32_t ip, uint16_t port, uint8_t * pData, int len) {
#ifdef DEBUG_STUN
//	string ipStr;
//	IpIntToStr(ip, ipStr);
//	if (ipStr == _bindIp) {
//		return true;
//	}
//	if (ip != _turnServerIp) {
//		return true;
//	}
#endif
	
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
#ifdef WIN32
	sa.sin_addr.S_un.S_addr = EHTONL(ip);
#else
	sa.sin_addr.s_addr = EHTONL(ip);
#endif
	sa.sin_port = EHTONS(port);

	int sent = 0;
do {
	sent = sendto(_fd, (char *)pData, len, 0, (sockaddr *)&sa, sizeof(sa));
} while ((sent == -1) && (errno == 55));

	if (sent != len) {
		if (errno == 55) {
		FATAL("Error 55");
		return true;
		}
		string ipStr;
		ToIpStrV4(ip, port, ipStr);
		FATAL("Error %d sending [%d of %d] to via FD:0x%x remote: %s", 
				errno, sent, len, (uint32_t) _fd, STR(ipStr));
		LOG_LATENCY("STUN Failed");
		return false;
	} else {
//		INFO("$b2$ IceUdpProtocol Sent %d bytes, to: 0x%x : 0x%x",
//			sent, ip, (uint32_t)port);
		/**
		WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X", 
			(int)pData[0], (int)pData[1], (int)pData[2], (int)pData[3],
			(int)pData[4], (int)pData[5], (int)pData[6], (int)pData[7]
			);
		WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
			(int)pData[8], (int)pData[9], (int)pData[10], (int)pData[11],
			(int)pData[12], (int)pData[13], (int)pData[14], (int)pData[15]
			);
		WARN("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
			(int)pData[16], (int)pData[17], (int)pData[18], (int)pData[19],
			(int)pData[20], (int)pData[21], (int)pData[22], (int)pData[23]
			);
		**/
		LOG_LATENCY("STUN Delivery");
	}

	return true;
}

void IceUdpProtocol::FinishInitialization() {
	if (CreateUdpSocket() < 0) {
		FATAL("WebRTC unable to create STUN UDP Carrier");
		Terminate(false);
	}
}
#endif // HAS_PROTOCOL_WEBRTC
