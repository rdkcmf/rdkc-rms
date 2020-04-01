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

#include "mediaformats/readers/ts/tsparser.h"
#include "mediaformats/readers/ts/tspacketheader.h"
#include "mediaformats/readers/ts/tsboundscheck.h"
#include "mediaformats/readers/ts/avcontext.h"
#include "mediaformats/readers/ts/tspacketpat.h"
#include "mediaformats/readers/ts/tspacketpmt.h"
#include "mediaformats/readers/ts/tsparsereventsink.h"
#include "streaming/baseinstream.h"
#define TS_CLOCK_FREQ       90000   //ticks per second
#define TS_PTS_ROLLOVER     60      //60 seconds

TSParser::TSParser(TSParserEventsSink *pEventSink) {
	_pEventSink = pEventSink;
	_chunkSize = 188;

	//2. Setup the PAT pid
	PIDDescriptor *pPAT = new PIDDescriptor;
	pPAT->type = PID_TYPE_PAT;
	pPAT->pid = 0;
	pPAT->crc = 0;
	pPAT->pAVContext = NULL;
	_pidMapping[0] = pPAT;

	//3. Setup CAT table
	PIDDescriptor *pCAT = new PIDDescriptor;
	pCAT->type = PID_TYPE_CAT;
	pCAT->pid = 1;
	pCAT->crc = 0;
	pCAT->pAVContext = NULL;
	_pidMapping[1] = pCAT;

	//4. Setup TSDT table
	PIDDescriptor *pTSDT = new PIDDescriptor;
	pTSDT->type = PID_TYPE_TSDT;
	pTSDT->pid = 2;
	pTSDT->crc = 0;
	pTSDT->pAVContext = NULL;
	_pidMapping[2] = pTSDT;

	//5. Setup reserved tables
	for (uint16_t i = 3; i < 16; i++) {
		PIDDescriptor *pReserved = new PIDDescriptor;
		pReserved->type = PID_TYPE_RESERVED;
		pReserved->pid = i;
		pReserved->crc = 0;
		pReserved->pAVContext = NULL;
		_pidMapping[i] = pReserved;
	}

	//6. Setup the NULL pid
	PIDDescriptor *pNULL = new PIDDescriptor;
	pNULL->type = PID_TYPE_NULL;
	pNULL->pid = 0x1fff;
	pNULL->crc = 0;
	pNULL->pAVContext = NULL;
	_pidMapping[0x1fff] = pNULL;

	_totalParsedBytes = 0;
#ifdef HAS_MULTIPROGRAM_TS
	_hasFilters = false;
#endif	/* HAS_MULTIPROGRAM_TS */
}

TSParser::~TSParser() {
	//1. Cleanup the pid mappings
	map<uint16_t, PIDDescriptor*>::iterator removeIterator;
	for (removeIterator = _pidMapping.begin(); removeIterator != _pidMapping.end();) {
		FreePidDescriptor(removeIterator->second);
		_pidMapping.erase(removeIterator++);
	}
	_pEventSink = NULL;
}

void TSParser::SetChunkSize(uint32_t chunkSize) {
	_chunkSize = chunkSize;
}

bool TSParser::ProcessPacket(uint32_t packetHeader, IOBuffer &buffer,
		uint32_t maxCursor) {
	//1. Get the PID descriptor or create it if absent
	PIDDescriptor *pPIDDescriptor = NULL;
	if (!_pidMapping.empty() && MAP_HAS1(_pidMapping, TS_TRANSPORT_PACKET_PID(packetHeader))) {
		pPIDDescriptor = _pidMapping[TS_TRANSPORT_PACKET_PID(packetHeader)];
	} else {
		pPIDDescriptor = new PIDDescriptor;
		pPIDDescriptor->type = PID_TYPE_UNKNOWN;
		pPIDDescriptor->pAVContext = NULL;
		pPIDDescriptor->pid = TS_TRANSPORT_PACKET_PID(packetHeader);
		_pidMapping[pPIDDescriptor->pid] = pPIDDescriptor;
	}

	//2. Skip the transport packet structure
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t cursor = 4;

	if (TS_TRANSPORT_PACKET_HAS_ADAPTATION_FIELD(packetHeader)) {
		TS_CHECK_BOUNDS(1);
		TS_CHECK_BOUNDS(pBuffer[cursor]);
		cursor += pBuffer[cursor] + 1;
	}
	if (!TS_TRANSPORT_PACKET_HAS_PAYLOAD(packetHeader)) {
		return true;
	}

	switch (pPIDDescriptor->type) {
		case PID_TYPE_PAT:
		{
			return ProcessPidTypePAT(packetHeader, *pPIDDescriptor, pBuffer, cursor, maxCursor);
		}
		case PID_TYPE_NIT:
		case PID_TYPE_CAT:
		case PID_TYPE_TSDT:
		{
			//			FINEST("%s", STR(IOBuffer::DumpBuffer(pBuffer + cursor,
			//					_chunkSize - cursor)));
			return true;
		}
		case PID_TYPE_PMT:
		{
#ifdef HAS_MULTIPROGRAM_TS
			if (HasProgramFilter()) {
				uint16_t programPID = _avFilters.GetValue("programPID", false);
				if (programPID != pPIDDescriptor->pid) {
					FINE("ProgramPID (%"PRIu16") ignored.", pPIDDescriptor->pid);
					return true;
				} else {
					FINEST("Found selected ProgramPID (%"PRIu16").", pPIDDescriptor->pid);
				}
			}
#endif	/* HAS_MULTIPROGRAM_TS */
			return ProcessPidTypePMT(packetHeader, *pPIDDescriptor, pBuffer,
					cursor, maxCursor);
		}
		case PID_TYPE_AUDIOSTREAM:
		case PID_TYPE_VIDEOSTREAM:
		{
#ifdef HAS_MULTIPROGRAM_TS
			if (_hasFilters) {
				uint16_t videoPID = 0;
				uint16_t audioPID = 0;
				if (HasVideoFilter()) {
					videoPID = _avFilters.GetValue("videoPID", false);
				}
				if (HasAudioFilter()) {
					audioPID = _avFilters.GetValue("audioPID", false);
				}
				if (pPIDDescriptor->type == PID_TYPE_AUDIOSTREAM && HasAudioFilter()) {
					if (pPIDDescriptor->pid != audioPID) {
						FINE("AudioPID (%"PRIu16") ignored.", pPIDDescriptor->pid);
						return true;
					} else {
						FINE("AudioPID (%"PRIu16") found.", pPIDDescriptor->pid);
					}
				}
				if (pPIDDescriptor->type == PID_TYPE_VIDEOSTREAM && HasVideoFilter()) {
					if (pPIDDescriptor->pid != videoPID) {
						FINE("VideoPID (%"PRIu16") ignored.", pPIDDescriptor->pid);
						return true;
					} else {
						FINE("VideoPID (%"PRIu16") found.", pPIDDescriptor->pid);
					}
				}
			}
#endif	/* HAS_MULTIPROGRAM_TS */
			return ProcessPidTypeAV(pPIDDescriptor,
					pBuffer + cursor, _chunkSize - cursor,
					TS_TRANSPORT_PACKET_IS_PAYLOAD_START(packetHeader),
					(int8_t) packetHeader & 0x0f);
		}
			//		case PID_TYPE_AUDIOSTREAM:
			//		{
			//			if (_pEventSink != NULL)
			//				return _pEventSink->ProcessData(pPIDDescriptor,
			//					pBuffer + cursor, _chunkSize - cursor,
			//					TS_TRANSPORT_PACKET_IS_PAYLOAD_START(packetHeader), true,
			//					(int8_t) packetHeader & 0x0f);
			//			return true;
			//		}
			//		case PID_TYPE_VIDEOSTREAM:
			//		{
			//			if (_pEventSink != NULL)
			//				return _pEventSink->ProcessData(pPIDDescriptor,
			//					pBuffer + cursor, _chunkSize - cursor,
			//					TS_TRANSPORT_PACKET_IS_PAYLOAD_START(packetHeader), false,
			//					(int8_t) packetHeader & 0x0f);
			//			return true;
			//		}
		case PID_TYPE_RESERVED:
		{
			WARN("This PID %hu should not be used because is reserved according to iso13818-1.pdf", pPIDDescriptor->pid);
			return true;
		}
		case PID_TYPE_UNKNOWN:
		{
			if (_unknownPids.empty() || !MAP_HAS1(_unknownPids, pPIDDescriptor->pid)) {
				//WARN("PID %"PRIu16" not known yet", pPIDDescriptor->pid);
				_unknownPids[pPIDDescriptor->pid] = pPIDDescriptor->pid;
			}
			return true;

		}
		case PID_TYPE_NULL:
		{
			//Ignoring NULL pids
			return true;
		}
		default:
		{
			WARN("PID type not implemented: %d. Pid number: %"PRIu16,
					pPIDDescriptor->type, pPIDDescriptor->pid);
			return false;
		}
	}
}

bool TSParser::ProcessBuffer(IOBuffer &buffer, bool chunkByChunk) {
	while (GETAVAILABLEBYTESCOUNT(buffer) >= _chunkSize) {
		if (GETIBPOINTER(buffer)[0] != 0x47) {
			WARN("Bogus chunk detected");
			if (_pEventSink != NULL) {
				_pEventSink->SignalResetChunkSize();
			}
			return true;
		}

		uint32_t packetHeader = ENTOHLP(GETIBPOINTER(buffer));

		if (!ProcessPacket(packetHeader, buffer, _chunkSize)) {
			FATAL("Unable to process packet");
			return false;
		}

		_totalParsedBytes += _chunkSize;

		if (!buffer.Ignore(_chunkSize)) {
			FATAL("Unable to ignore %u bytes", _chunkSize);
			return false;
		}

		if (chunkByChunk)
			break;
	}

	if (!chunkByChunk)
		buffer.MoveData();

	return true;
}

uint64_t TSParser::GetTotalParsedBytes() {
	return _totalParsedBytes;
}

void TSParser::FreePidDescriptor(PIDDescriptor *pPIDDescriptor) {
	if (pPIDDescriptor != NULL) {
		if (pPIDDescriptor->pAVContext != NULL) {
			delete pPIDDescriptor->pAVContext;
			pPIDDescriptor->pAVContext = NULL;
		}
		delete pPIDDescriptor;
		pPIDDescriptor = NULL;
	}
}

bool TSParser::ProcessPidTypePAT(uint32_t packetHeader,
		PIDDescriptor &pidDescriptor, uint8_t *pBuffer, uint32_t &cursor,
		uint32_t maxCursor) {
	//1. Advance the pointer field
	if (TS_TRANSPORT_PACKET_IS_PAYLOAD_START(packetHeader)) {
		TS_CHECK_BOUNDS(1);
		TS_CHECK_BOUNDS(pBuffer[cursor]);
		cursor += pBuffer[cursor] + 1;
	}

	//1. Get the crc from the packet and compare it with the last crc.
	//if it is the same, ignore this packet
	uint32_t crc = TSPacketPAT::PeekCRC(pBuffer, cursor, maxCursor);
	if (crc == 0) {
		FATAL("Unable to read crc");
		return false;
	}
	if (pidDescriptor.crc == crc) {
		if (_pEventSink != NULL) {
			_pEventSink->SignalPAT(NULL);
		}
		return true;
	}

	//2. read the packet
	TSPacketPAT packetPAT;
	if (!packetPAT.Read(pBuffer, cursor, maxCursor)) {
		FATAL("Unable to read PAT");
		return false;
	}

	//3. Store the crc
	pidDescriptor.crc = packetPAT.GetCRC();


	//4. Store the pid types found in PAT

	FOR_MAP(packetPAT.GetPMTs(), uint16_t, uint16_t, i) {
		PIDDescriptor *pDescr = new PIDDescriptor;
		pDescr->pid = MAP_VAL(i);
		pDescr->type = PID_TYPE_PMT;
		pDescr->crc = 0;
		pDescr->pAVContext = NULL;
		_pidMapping[pDescr->pid] = pDescr;
	}

	FOR_MAP(packetPAT.GetNITs(), uint16_t, uint16_t, i) {
		PIDDescriptor *pDescr = new PIDDescriptor;
		pDescr->pid = MAP_VAL(i);
		pDescr->type = PID_TYPE_NIT;
		pDescr->pAVContext = NULL;
		_pidMapping[pDescr->pid] = pDescr;
	}

	if (_pEventSink != NULL) {
		_pEventSink->SignalPAT(&packetPAT);
	}

	//5. Done
	return true;
}

bool TSParser::ProcessPidTypePMT(uint32_t packetHeader,
		PIDDescriptor &pidDescriptor, uint8_t *pBuffer, uint32_t &cursor,
		uint32_t maxCursor) {
	//1. Advance the pointer field
	if (TS_TRANSPORT_PACKET_IS_PAYLOAD_START(packetHeader)) {
		TS_CHECK_BOUNDS(1);
		TS_CHECK_BOUNDS(pBuffer[cursor]);
		cursor += pBuffer[cursor] + 1;
	}

	//1. Get the crc from the packet and compare it with the last crc.
	//if it is the same, ignore this packet. Also test if we have a protocol handler
	uint32_t crc = TSPacketPMT::PeekCRC(pBuffer, cursor, maxCursor);
	if (crc == 0) {
		FATAL("Unable to read crc");
		return false;
	}
	if (pidDescriptor.crc == crc) {
		if (_pEventSink != NULL) {
			_pEventSink->SignalPMT(NULL);
		}
		return true;
	}

	//2. read the packet
	TSPacketPMT packetPMT;
	if (!packetPMT.Read(pBuffer, cursor, maxCursor)) {
		FATAL("Unable to read PAT");
		return false;
	}
	if (_pEventSink != NULL) {
		_pEventSink->SignalPMT(&packetPMT);
	}

	//3. Store the CRC
	pidDescriptor.crc = packetPMT.GetCRC();

	//4. Gather the info about the streams present inside the program
	//videoPid will contain the selected video stream
	//audioPid will contain the selected audio stream
	//unknownPids will contain the rest of the streams that will be ignored
	map<uint16_t, uint16_t> unknownPids;
	vector<PIDDescriptor *> pidDescriptors;
	PIDDescriptor *pPIDDescriptor = NULL;

	map<uint16_t, TSStreamInfo> streamsInfo = packetPMT.GetStreamsInfo();
	if (_pEventSink != NULL) {
		if (!_pEventSink->SignalStreamsPIDSChanged(streamsInfo)) {
			FATAL("SignalStreamsPIDSChanged failed");
			return false;
		}
	}

	bool ignore = true;
	PIDType pidType = PID_TYPE_NULL;
	BaseAVContext *pAVContext = NULL;

	FOR_MAP(streamsInfo, uint16_t, TSStreamInfo, i) {
		ignore = true;
		pidType = PID_TYPE_NULL;
		pAVContext = NULL;
		switch (MAP_VAL(i).streamType) {
			case TS_STREAMTYPE_VIDEO_H264:
			{
				pidType = PID_TYPE_VIDEOSTREAM;
				pAVContext = new H264AVContext();
				if (_pEventSink != NULL) {
					if (!_pEventSink->SignalStreamPIDDetected(MAP_VAL(i),
							pAVContext, pidType, ignore)) {
						FATAL("SignalStreamsPIDSChanged failed");
						return false;
					}
				}
#ifdef HAS_MULTIPROGRAM_TS
				if (_hasFilters) {
					if (!HasVideoFilter()) {
						FINE("VideoPID (%"PRIu16") selected" , MAP_VAL(i).elementaryPID);
						_avFilters["videoPID"] = MAP_VAL(i).elementaryPID;
					} else {
						ignore = ((uint16_t)_avFilters.GetValue("videoPID", false) != MAP_VAL(i).elementaryPID);
					}
				}
#endif	/* HAS_MULTIPROGRAM_TS */
				break;
			}
			case TS_STREAMTYPE_AUDIO_AAC:
			{
				pidType = PID_TYPE_AUDIOSTREAM;
				pAVContext = new AACAVContext();
				if (_pEventSink != NULL) {
					if (!_pEventSink->SignalStreamPIDDetected(MAP_VAL(i),
							pAVContext, pidType, ignore)) {
						FATAL("SignalStreamsPIDSChanged failed");
						return false;
					}
				}
#ifdef HAS_MULTIPROGRAM_TS
				if (_hasFilters) {
					if (!HasAudioFilter()) {
						ignore = true;
						if (_avFilters.HasKeyChain(V_STRING, false, 1, "audioLanguage")) {
							FOR_VECTOR(MAP_VAL(i).esDescriptors, index) {
								if (MAP_VAL(i).esDescriptors[index].type == DESCRIPTOR_TYPE_ISO_639_LANGUAGE) {
									StreamDescriptor * esDesc = &MAP_VAL(i).esDescriptors[index];
									string lang = string((char *) esDesc->payload.iso_639_descriptor.language_code, 3);
							
									if (lang == (string)_avFilters.GetValue("audioLanguage", false)) {
										_avFilters["audioPID"] = MAP_VAL(i).elementaryPID;
										ignore = false;
										FINE("Selected AudioLanguage (%s) found on AudioPID=%"PRIu16".", STR(lang), MAP_VAL(i).elementaryPID);
										break;
									}
								}
							}
						}
					} else {
						ignore = ((uint16_t)_avFilters.GetValue("audioPID", false) != MAP_VAL(i).elementaryPID);
					}
				}
#endif	/* HAS_MULTIPROGRAM_TS */
				break;
			}
			default:
			{
				if (_pEventSink != NULL)
					_pEventSink->SignalUnknownPIDDetected(MAP_VAL(i));
				break;
			}
		}
		if (ignore) {
			pidType = PID_TYPE_NULL;
			if (pAVContext != NULL) {
				delete pAVContext;
				pAVContext = NULL;
			}
		}
		pPIDDescriptor = new PIDDescriptor();
		pPIDDescriptor->pid = MAP_KEY(i);
		pPIDDescriptor->type = pidType;
		pPIDDescriptor->pAVContext = pAVContext;
		if (pPIDDescriptor->pAVContext != NULL)
			pPIDDescriptor->pAVContext->_pEventsSink = _pEventSink;
		ADD_VECTOR_END(pidDescriptors, pPIDDescriptor);
	}

	//7. Mount the newly created pids

	FOR_VECTOR(pidDescriptors, i) {
		if (!_pidMapping.empty() && MAP_HAS1(_pidMapping, pidDescriptors[i]->pid)) {
			FreePidDescriptor(_pidMapping[pidDescriptors[i]->pid]);
		}
		_pidMapping[pidDescriptors[i]->pid] = pidDescriptors[i];
	}

	if (_pEventSink != NULL) {
		_pEventSink->SignalPMTComplete();
	}

	return true;
}

bool TSParser::ProcessPidTypeAV(PIDDescriptor *pPIDDescriptor, uint8_t *pData,
	uint32_t length, bool packetStart, int8_t sequenceNumber) {
	if (pPIDDescriptor->pAVContext == NULL) {
		FATAL("No AVContext created");
		return false;
	}

	if (pPIDDescriptor->pAVContext->_sequenceNumber == -1) {
		pPIDDescriptor->pAVContext->_sequenceNumber = sequenceNumber;
	} else {
		if (((pPIDDescriptor->pAVContext->_sequenceNumber + 1)&0x0f) != sequenceNumber) {
			pPIDDescriptor->pAVContext->_sequenceNumber = sequenceNumber;
			pPIDDescriptor->pAVContext->DropPacket();
			return true;
		} else {
			pPIDDescriptor->pAVContext->_sequenceNumber = sequenceNumber;
		}
	}

	if (packetStart) {
		if (!pPIDDescriptor->pAVContext->HandleData()) {
			FATAL("Unable to handle AV data");
			return false;
		}
		if (length > 8) {
			if (((pData[3]&0xE0) != 0xE0)&&((pData[3]&0xC0) != 0xC0)) {
				BaseInStream *pTemp = pPIDDescriptor->pAVContext->GetInStream();
				WARN("PID %"PRIu16" from %s is not h264/aac. The type is 0x%02"PRIx8,
						pPIDDescriptor->pid,
						pTemp != NULL ? STR(*pTemp) : "",
						pData[3]);
				pPIDDescriptor->type = PID_TYPE_NULL;
				return true;
			}

			uint32_t pesHeaderLength = pData[8];
			if (pesHeaderLength + 9 > length) {
				WARN("Not enough data");
				pPIDDescriptor->pAVContext->DropPacket();
				return true;
			}

			//compute the time:
			//http://dvd.sourceforge.net/dvdinfo/pes-hdr.html
			uint8_t *pPTS = NULL;
			uint8_t *pDTS = NULL;

			uint8_t ptsdstflags = pData[7] >> 6;

			if (ptsdstflags == 2) {
				if (pesHeaderLength >= 5)
					pPTS = pData + 9;
			} else if (ptsdstflags == 3) {
				if (pesHeaderLength >= 5)
					pPTS = pData + 9;
				if (pesHeaderLength >= 10)
					pDTS = pData + 14;
			}

			if (pPTS != NULL) {
				uint64_t value = (pPTS[0]&0x0f) >> 1;
				value = (value << 8) | pPTS[1];
				value = (value << 7) | (pPTS[2] >> 1);
				value = (value << 8) | pPTS[3];
				value = (value << 7) | (pPTS[4] >> 1);
				value &= 0x1FFFFFFFFLL;

                if (pPIDDescriptor->pAVContext->_pts.lastRaw > value) {
                    uint64_t ptsDiff = pPIDDescriptor->pAVContext->_pts.lastRaw - value;
                    if ((ptsDiff / TS_CLOCK_FREQ) >= TS_PTS_ROLLOVER)
                        pPIDDescriptor->pAVContext->_pts.rollOverCount++;
                }

				pPIDDescriptor->pAVContext->_pts.lastRaw = value;
				value += (pPIDDescriptor->pAVContext->_pts.rollOverCount * 0x200000000LL);
				pPIDDescriptor->pAVContext->_pts.time = (double) value / 90.00;
			} else {
				WARN("No PTS!");
				pPIDDescriptor->pAVContext->DropPacket();
				return true;
			}

			double tempDtsTime = 0;
			if (pDTS != NULL) {
				uint64_t value = (pDTS[0]&0x0f) >> 1;
				value = (value << 8) | pDTS[1];
				value = (value << 7) | (pDTS[2] >> 1);
				value = (value << 8) | pDTS[3];
				value = (value << 7) | (pDTS[4] >> 1);
				value &= 0x1FFFFFFFFLL;
				if ((pPIDDescriptor->pAVContext->_dts.lastRaw >= value) &&
					(pPIDDescriptor->pAVContext->_dts.rollOverCount != pPIDDescriptor->pAVContext->_pts.rollOverCount)) {
					pPIDDescriptor->pAVContext->_dts.rollOverCount = pPIDDescriptor->pAVContext->_pts.rollOverCount;
				}
				pPIDDescriptor->pAVContext->_dts.lastRaw = value;
				value += (pPIDDescriptor->pAVContext->_dts.rollOverCount * 0x200000000LL);
				tempDtsTime = (double) value / 90.00;
				// try to correct dts firsthand
				if ((pPIDDescriptor->pAVContext->_dts.time > tempDtsTime) && 
						(pPIDDescriptor->pAVContext->_dts.rollOverCount != pPIDDescriptor->pAVContext->_pts.rollOverCount)) {
					value = pPIDDescriptor->pAVContext->_dts.lastRaw + (pPIDDescriptor->pAVContext->_pts.rollOverCount * 0x200000000LL);
					tempDtsTime = (double) value / 90.00;
				}
			} else {
				tempDtsTime = pPIDDescriptor->pAVContext->_pts.time;
			}

			if (pPIDDescriptor->pAVContext->_dts.time > tempDtsTime) {
				WARN("Back timestamp: %.2f -> %.2f on pid %"PRIu16,
						pPIDDescriptor->pAVContext->_dts.time, tempDtsTime, pPIDDescriptor->pid);
				pPIDDescriptor->pAVContext->DropPacket();
				return true;
			}
			pPIDDescriptor->pAVContext->_dts.time = tempDtsTime;

			pData += 9 + pesHeaderLength;
			length -= (9 + pesHeaderLength);

		} else {
			WARN("Not enough data");
			pPIDDescriptor->pAVContext->DropPacket();
			return true;
		}
	}

	pPIDDescriptor->pAVContext->_bucket.ReadFromBuffer(pData, length);

	return true;
}
#ifdef HAS_MULTIPROGRAM_TS
void TSParser::SetAVFilters(Variant &filters) {
	_avFilters = filters;
	_hasFilters = true;
}
bool TSParser::HasProgramFilter() {
	return _avFilters.HasKeyChain(_V_NUMERIC, false, 1, "programPID");
}
bool TSParser::HasAVFilter() {
	return (_avFilters.HasKeyChain(_V_NUMERIC, false, 1, "videoPID") || 
		_avFilters.HasKeyChain(_V_NUMERIC, false, 1, "audioPID"));
}
bool TSParser::HasAudioFilter() {
	return _avFilters.HasKeyChain(_V_NUMERIC, false, 1, "audioPID");
}
bool TSParser::HasVideoFilter() {
	return _avFilters.HasKeyChain(_V_NUMERIC, false, 1, "videoPID");
}
#endif	/* HAS_MULTIPROGRAM_TS */

#endif	/* HAS_MEDIA_TS */
