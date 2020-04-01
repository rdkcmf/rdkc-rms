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
#ifndef _BASETSSEGMENT_H
#define	_BASETSSEGMENT_H

#include "streaming/hls/basesegment.h"

#define PAT_PMT_INTERVAL 500.0

//#define DEBUG_DUMP_TS_HEADER

class DLLEXP BaseTSSegment : public BaseSegment {
private:
	uint8_t *_pCounters;
	uint8_t *_pPatPacket;
	uint8_t _patPacketLength;
	uint8_t *_pPmtPacket;
	uint8_t _pmtPacketLength;

	uint16_t _pmtPid;
	uint16_t _pcrPid;

	uint16_t _videoPid;

	uint8_t *_pPacket;

	IOBuffer &_videoBuffer;
	IOBuffer &_patPmtAndCounters;

	uint32_t _pcrCount;

	uint64_t _frameCount;

	uint16_t _metadataPid;
public:
	BaseTSSegment(IOBuffer &videoBuffer, IOBuffer &audioBuffer,
		IOBuffer &patPmtAndCounters, string const& drmType);
	virtual ~BaseTSSegment();

	virtual bool Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL) = 0;
	bool WritePATPMT();
	bool PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts);
	bool PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts, Variant& videoParamsSizes);
	bool PushMetaData(string const& vmfMetadata, int64_t pts);
	bool Init(StreamCapabilities *pCapabilities, unsigned char *pKey = NULL, unsigned char *pIV = NULL);
	bool FlushPacket();
	uint64_t GetFrameCount();
protected:
	virtual bool WritePacket(uint8_t *pBuffer, uint32_t size) = 0;
private:
	uint16_t AddStream(PID_TYPE pidType);
	bool WritePayload(uint8_t* pBuffer, uint32_t length, int64_t pts);
	bool WritePayload(uint8_t* pBuffer, uint32_t length, uint16_t pid, int64_t pts, int64_t dts);
	bool WritePayload(uint8_t *pBuffer, uint32_t length, uint16_t pid, int64_t pts, int64_t dts, Variant& videoParamsSizes);
	bool WritePAT();
	bool WritePMT();
	bool BuildPATPacket();
	bool BuildPMTPacket();
	bool BuildPacketHeader(uint8_t *pBuffer, bool payloadStart, uint16_t pid,
		bool hasAdaptationField, bool hasPayload);
	uint32_t synchsafe(uint32_t realNumber);
#ifdef DEBUG_DUMP_TS_HEADER
	void DumpTS4BytesHeader(uint8_t *pBuffer);
#endif /* DEBUG_DUMP_TS_HEADER */
};
#endif	/* _BASETSSEGMENT_H */
#endif  /* HAS_PROTOCOL_HLS */
