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
#ifndef _NETTSSEGMENT_H
#define	_NETTSSEGMENT_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#include "streaming/hls/basetssegment.h"

class UDPSenderProtocol;
class HTTPAdaptorProtocol;

class DLLEXP NetTSSegment
: public BaseTSSegment {
private:
	UDPSenderProtocol *_pSender;
	HTTPAdaptorProtocol *_pHTTPProtocol;
	IOBuffer _buffer;
	uint32_t _tsChunksCount;
public:
	NetTSSegment(IOBuffer &videoBuffer, IOBuffer &audioBuffer,
			IOBuffer &patPmtAndCounters, string const& drmType = "");
	virtual ~NetTSSegment();

	virtual bool Init(Variant &settings, StreamCapabilities *pCapabilities,
			unsigned char *pKey = NULL, unsigned char *pIV = NULL);

	void SetUDPSender(UDPSenderProtocol *pSender);
	void ResetUDPSender();
	void SetHTTPProtocol(HTTPAdaptorProtocol *pProtocol);
	void ResetHTTPProtocol();
	uint16_t GetUdpBindPort();
protected:
	virtual bool WritePacket(uint8_t *pBuffer, uint32_t size);
};
#endif	/* _NETTSSEGMENT_H */
#endif /* HAS_PROTOCOL_HLS */
