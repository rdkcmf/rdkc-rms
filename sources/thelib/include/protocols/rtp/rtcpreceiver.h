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
#ifndef _RTCPRECEIVER_H
#define	_RTCPRECEIVER_H
#endif	/* RTCPRECEIVER_H */

class BaseOutNetRTPUDPStream;

struct NackStats {
	uint32_t pRequested;
	uint32_t pTransmitted;
};

struct NackRequestFCI {
	uint16_t pid;
	uint16_t blp;
};

struct NackRequest {
	uint32_t ssrc;
	vector<NackRequestFCI> fciList;
};

class DLLEXP RTCPReceiver {
private:
	BaseOutNetRTPUDPStream *_pOutStream;

public:
	RTCPReceiver();
	~RTCPReceiver();
	void SetOutNetRTPStream(BaseOutNetRTPUDPStream *pStream);
	void ProcessRTCPPacket(IOBuffer &buffer);
	static NackStats GetNackStats() { return _nackStats; }
	static void ResetNackStats();
private:
	void UpdateNackStats(uint32_t pR, uint32_t pT);
	bool ProcessRTCPMessage(uint8_t *rtcpMessage, uint32_t messageLength);
	bool ProcessGenericNack(NackRequest nackRequest);
	static NackStats _nackStats;
};

#endif	/* HAS_PROTOCOL_RTP */
