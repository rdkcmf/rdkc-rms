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


#ifdef HAS_PROTOCOL_HLS
#include "streaming/hls/nettssegment.h"
#include "streaming/nalutypes.h"
#include "netio/netio.h"
#include "utils/udpsenderprotocol.h"
#include "protocols/http/httpadaptorprotocol.h"

#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7
//#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 1

NetTSSegment::NetTSSegment(IOBuffer &videoBuffer, IOBuffer &audioBuffer,
		IOBuffer &patPmtAndCounters, string const& drmType)
: BaseTSSegment(videoBuffer, audioBuffer, patPmtAndCounters, drmType) {
	_tsChunksCount = 0;
	_pSender = NULL;
	_pHTTPProtocol = NULL;
}

NetTSSegment::~NetTSSegment() {
	_pSender = NULL;
}

bool NetTSSegment::Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey, unsigned char *pIV) {
	return BaseTSSegment::Init(pCapabilities, pKey, pIV);
}

void NetTSSegment::SetUDPSender(UDPSenderProtocol *pSender) {
	_pSender = pSender;
}

void NetTSSegment::ResetUDPSender() {
	_pSender = NULL;
}

void NetTSSegment::SetHTTPProtocol(HTTPAdaptorProtocol *pProtocol) {
	_pHTTPProtocol = pProtocol;
}

void NetTSSegment::ResetHTTPProtocol() {
	_pHTTPProtocol = NULL;
}

uint16_t NetTSSegment::GetUdpBindPort() {
	if (_pSender == NULL)
		return 0;
	return _pSender->GetUdpBindPort();
}

bool NetTSSegment::WritePacket(uint8_t *pBuffer, uint32_t size) {
	if (_pHTTPProtocol != NULL) {
		if (!_pHTTPProtocol->SendBlock(pBuffer, size)) {
			FATAL("Unable to send data");
			return false;
		}
	} else {
		if (_pSender == NULL) {
			FATAL("No sender available");
			return false;
		}
		_buffer.ReadFromBuffer(pBuffer, size);
		_tsChunksCount++;
		//Check if Packet count is less than 7
		if (_tsChunksCount < TRANSPORT_PACKETS_PER_NETWORK_PACKET) {
			return true;
		}

		if (!_pSender->SendBlock(GETIBPOINTER(_buffer), GETAVAILABLEBYTESCOUNT(_buffer))) {
			WARN("unable to send udp packet...");
		}

		//Clear the buffer
		_buffer.IgnoreAll();
		_tsChunksCount = 0;
	}

	return true;
}

#endif /* HAS_PROTOCOL_HLS */
