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


#ifdef HAS_MEDIA_TS
#ifndef _TSPARSER_H
#define	_TSPARSER_H

#include "common.h"
#include "mediaformats/readers/ts/piddescriptor.h"

class TSParserEventsSink;

#define TS_CHUNK_188 188
#define TS_CHUNK_204 204
#define TS_CHUNK_208 208

class TSParser {
private:
	TSParserEventsSink *_pEventSink;
	uint32_t _chunkSize;
	map<uint16_t, PIDDescriptor *> _pidMapping;
	map<uint16_t, uint16_t> _unknownPids;
	uint64_t _totalParsedBytes;
#ifdef HAS_MULTIPROGRAM_TS
	Variant _avFilters;
	bool _hasFilters;
#endif	/* HAS_MULTIPROGRAM_TS */
public:
	TSParser(TSParserEventsSink *pEventSink);
	virtual ~TSParser();

	void SetChunkSize(uint32_t chunkSize);
	bool ProcessPacket(uint32_t packetHeader, IOBuffer &buffer,
			uint32_t maxCursor);
	bool ProcessBuffer(IOBuffer &buffer, bool chunkByChunk);
	uint64_t GetTotalParsedBytes();
#ifdef HAS_MULTIPROGRAM_TS
	void SetAVFilters(Variant &filters);
#endif	/* HAS_MULTIPROGRAM_TS */
private:
	void FreePidDescriptor(PIDDescriptor *pPIDDescriptor);
	bool ProcessPidTypePAT(uint32_t packetHeader, PIDDescriptor &pidDescriptor,
			uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor);
	bool ProcessPidTypePMT(uint32_t packetHeader, PIDDescriptor &pidDescriptor,
			uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor);
	bool ProcessPidTypeAV(PIDDescriptor *pPIDDescriptor, uint8_t *pData,
			uint32_t length, bool packetStart, int8_t sequenceNumber);
#ifdef HAS_MULTIPROGRAM_TS
	bool HasProgramFilter();
	bool HasAVFilter();
	bool HasAudioFilter();
	bool HasVideoFilter();
#endif	/* HAS_MULTIPROGRAM_TS */
};


#endif	/* _TSPARSER_H */
#endif	/* HAS_MEDIA_TS */
