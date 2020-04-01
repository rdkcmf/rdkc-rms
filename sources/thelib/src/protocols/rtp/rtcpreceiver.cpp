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
#include "protocols/rtp/streaming/baseoutnetrtpudpstream.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "protocols/rtp/rtcpreceiver.h"

#define RTCP_SR 200
#define RTCP_RR 201
#define RTCP_SDES 202
#define RTCP_BYE 203
#define RTCP_APP 204

/*	Name   | Value | Brief Description
	----------+-------+------------------------------------
	RTPFB  |  205  | Transport layer FB message
	PSFB   |  206  | Payload-specific FB message
*/
#define RTCP_RTPFB 205
#define RTCP_PSFB 206

string ptToString(uint8_t pt) {
	switch (pt) {
	case RTCP_SR:
		return "SR";
	case RTCP_RR:
		return "RR";
	case RTCP_SDES:
		return "SDES";
	case RTCP_BYE:
		return "BYE";
	case RTCP_APP:
		return "APP";
	case RTCP_RTPFB:
		return "RTPFB";
	case RTCP_PSFB:
		return "PSFB";
	default:
		return "(unknown)";
	}
}

NackStats RTCPReceiver::_nackStats = {0};

RTCPReceiver::RTCPReceiver() {
	memset(&_nackStats, 0, sizeof(NackStats));
}

RTCPReceiver::~RTCPReceiver() {

}

void RTCPReceiver::SetOutNetRTPStream(BaseOutNetRTPUDPStream *pStream) {
	INFO("Setting outstream to %s", pStream == NULL ? "NULL" : STR(pStream->GetName()));
	_pOutStream = pStream;
	if (_pOutStream != NULL)
		_pOutStream->EnablePacketCache(true);
}
void RTCPReceiver::ProcessRTCPPacket(IOBuffer &buffer) {
//	INFO("RTCP buffer: %s", STR(buffer.ToString()));
	uint32_t length = ENTOHSP(GETIBPOINTER(buffer) + 2) * 4 + 4;
	while (ProcessRTCPMessage(GETIBPOINTER(buffer), length)) { // include headerlength
//		WARN("Length = %"PRIu32, length);
		buffer.Ignore(length);
		if (GETAVAILABLEBYTESCOUNT(buffer) < 3)
			break;
		length = ENTOHSP(GETIBPOINTER(buffer) + 2) * 4 + 4;
	}
}

void RTCPReceiver::UpdateNackStats(uint32_t pR, uint32_t pT) {
	_nackStats.pRequested += pR;
	_nackStats.pTransmitted += pT;
}

void RTCPReceiver::ResetNackStats() {
	memset(&_nackStats, 0, sizeof(NackStats));
}

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|   FMT   |       PT      |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)                 :
   :                                                               :
*/

bool RTCPReceiver::ProcessRTCPMessage(uint8_t *rtcpMessage, uint32_t messageLength) {

	// Extract feedback message type
	uint8_t fmt = (rtcpMessage[0] & 0x1F);
	// Extract payload type
	uint8_t pt = rtcpMessage[1];
	uint8_t *pPayload = rtcpMessage + 4;
	uint32_t payloadLength = messageLength - 4;
	switch (pt) {
	case RTCP_SR:
		break;
	case RTCP_RR:
		break;
	case RTCP_SDES:
		break;
	case RTCP_BYE:
		break;
	case RTCP_APP:
		break;
	case RTCP_RTPFB:
		FINE("Found rtcpMessage %s. fmt=%"PRIu8, STR(ptToString(pt)), fmt);
		// The Generic NACK message is identified by PT=RTPFB and FMT=1.
		if (fmt == 1) {
			NackRequest nackRequest;
			pPayload += 4; // ignore packet sender SSRC
			nackRequest.ssrc = ENTOHLP(pPayload); // media SSRC
			pPayload += 4;
			payloadLength -= 8;
			// The FCI field MUST contain at least one and MAY contain more than one Generic NACK.
			while (payloadLength > 0) {
				NackRequestFCI nackFci;
				nackFci.pid = ENTOHSP(pPayload);
				pPayload += 2;
				nackFci.blp = ENTOHSP(pPayload);
				pPayload += 2;
				payloadLength -= 4;
				nackRequest.fciList.push_back(nackFci);
//				DEBUG("PID=%"PRIu16" BLP=%04"PRIx16, nackFci.pid, nackFci.blp);
			}
			return ProcessGenericNack(nackRequest);
		}
		break;
	case RTCP_PSFB:
		break;
	default:
		INFO("Unhandled rtcpMessage=%"PRIu32, pt);
		return false;
	}
	return true;
}

bool RTCPReceiver::ProcessGenericNack(NackRequest nackRequest) {
	if (_pOutStream == NULL)
		return true;
	if (!_pOutStream->HasCache(nackRequest.ssrc))
		return true;
	vector<uint16_t> pids;
	FOR_VECTOR(nackRequest.fciList, i) {
		uint16_t pid = nackRequest.fciList[i].pid;
		if (_pOutStream->PacketIsNewerThanCache(nackRequest.ssrc, pid)) {
			continue;
		}
		if (_pOutStream->PacketIsCached(nackRequest.ssrc, pid))
			pids.push_back(pid);
		uint16_t blp = nackRequest.fciList[i].blp;
		for (uint8_t i = 0; i < 16; i++) {
			if ((blp & 0x0001) == 1) {
				if (_pOutStream->PacketIsCached(nackRequest.ssrc, pid + i + 1))
					pids.push_back(pid + i + 1);
			}
			blp = blp >> 1;
			if (blp == 0) break;
		}
	}
	if (!pids.empty()) {
		FINE("Request to retransmit %"PRIu32" frames", (uint32_t)pids.size());
		uint32_t transmitted = 0;
		transmitted = _pOutStream->RetransmitFrames(nackRequest.ssrc, pids);
		FINE("Retransmited %"PRIu32" frames", transmitted);
		UpdateNackStats((uint32_t)pids.size(), transmitted);
	}
	return true;
}

#endif	/* HAS_PROTOCOL_RTP */
