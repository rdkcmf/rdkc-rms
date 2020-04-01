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

#include "mediaformats/readers/ts/tspacketpat.h"
#include "mediaformats/readers/ts/tsboundscheck.h"

TSPacketPAT::TSPacketPAT() {
	//fields
	_tableId = 0;
	_sectionSyntaxIndicator = false;
	_reserved1 = false;
	_reserved2 = 0;
	_sectionLength = 0;
	_transportStreamId = 0;
	_reserved3 = 0;
	_versionNumber = 0;
	_currentNextIndicator = false;
	_sectionNumber = 0;
	_lastSectionNumber = 0;
	_crc = 0;

	//internal variables
	_patStart = 0;
	_patLength = 0;
	_entriesCount = 0;
}

TSPacketPAT::~TSPacketPAT() {
}

TSPacketPAT::operator string() {
	string result = "";
	result += format("tableId:                %hhu\n", _tableId);
	result += format("sectionSyntaxIndicator: %hhu\n", _sectionSyntaxIndicator);
	result += format("reserved1:              %hhu\n", _reserved1);
	result += format("reserved2:              %hhu\n", _reserved2);
	result += format("sectionLength:          %hu\n", _sectionLength);
	result += format("transportStreamId:      %hu\n", _transportStreamId);
	result += format("reserved3:              %hhu\n", _reserved3);
	result += format("versionNumber:          %hhu\n", _versionNumber);
	result += format("currentNextIndicator:   %hhu\n", _currentNextIndicator);
	result += format("sectionNumber:          %hhu\n", _sectionNumber);
	result += format("lastSectionNumber:      %hhu\n", _lastSectionNumber);
	result += format("crc:                    %x\n", _crc);
	result += format("entriesCount:           %u\n", _entriesCount);
	result += format("NIT count:              %"PRIz"u\n", _networkPids.size());
	if (_networkPids.size() > 0) {

		FOR_MAP(_networkPids, uint16_t, uint16_t, i) {
			result += format("\tNIT %hu: %hu\n", MAP_KEY(i), MAP_VAL(i));
		}
	}
	result += format("PMT count:              %"PRIz"u\n", _programPids.size());
	if (_programPids.size() > 0) {

		FOR_MAP(_programPids, uint16_t, uint16_t, i) {
			result += format("\tPMT %hu: %hu\n", MAP_KEY(i), MAP_VAL(i));
		}
	}
	return result;
}

bool TSPacketPAT::Read(uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor) {
	//1. Read table id
	TS_CHECK_BOUNDS(1);
	_tableId = pBuffer[cursor++];
	if (_tableId != 0) {
		FATAL("Invalid table id");
		return false;
	}

	//2. read section length and syntax indicator
	TS_CHECK_BOUNDS(2);
	_sectionSyntaxIndicator = (pBuffer[cursor]&0x80) != 0;
	_reserved1 = (pBuffer[cursor]&0x40) != 0;
	_reserved2 = (pBuffer[cursor] >> 4)&0x03;
	_sectionLength = ENTOHSP((pBuffer + cursor))&0x0fff;
	cursor += 2;

	//4. Compute entries count
	_entriesCount = (_sectionLength - 9) / 4;

	//5. Read transport stream id
	TS_CHECK_BOUNDS(2);
	_transportStreamId = ENTOHSP((pBuffer + cursor));
	cursor += 2;

	//6. read current next indicator and version
	TS_CHECK_BOUNDS(1);
	_reserved3 = pBuffer[cursor] >> 6;
	_versionNumber = (pBuffer[cursor] >> 1)&0x1f;
	_currentNextIndicator = (pBuffer[cursor]&0x01) != 0;
	cursor++;

	//7. read section number
	TS_CHECK_BOUNDS(1);
	_sectionNumber = pBuffer[cursor++];

	//8. read last section number
	TS_CHECK_BOUNDS(1);
	_lastSectionNumber = pBuffer[cursor++];

	//9. read the table
	for (uint32_t i = 0; i < _entriesCount; i++) {
		//9.1. read the program number
		TS_CHECK_BOUNDS(2);
		uint16_t programNumber = ENTOHSP((pBuffer + cursor)); //----MARKED-SHORT----
		cursor += 2;

		//9.2. read the network or program map pid
		TS_CHECK_BOUNDS(2);
		uint16_t temp = ENTOHSP((pBuffer + cursor)); //----MARKED-SHORT----
		cursor += 2;
		temp &= 0x1fff;

		if (programNumber == 0)
			_networkPids[programNumber] = temp;
		else
			_programPids[programNumber] = temp;
	}

	//10. read the CRC
	TS_CHECK_BOUNDS(4);
	_crc = ENTOHLP((pBuffer + cursor));
	cursor += 4;

	//12. done
	return true;
}

map<uint16_t, uint16_t> &TSPacketPAT::GetPMTs() {

	return _programPids;
}

map<uint16_t, uint16_t> &TSPacketPAT::GetNITs() {

	return _networkPids;
}

uint32_t TSPacketPAT::GetCRC() {

	return _crc;
}

uint32_t TSPacketPAT::PeekCRC(uint8_t *pBuffer, uint32_t cursor, uint32_t maxCursor) {
	//1. ignore table id
	TS_CHECK_BOUNDS(1);
	cursor++;

	//3. read section length
	TS_CHECK_BOUNDS(2);
	uint16_t length = ENTOHSP((pBuffer + cursor)); //----MARKED-SHORT----
	length &= 0x0fff;
	cursor += 2;

	//4. Move to the crc position
	TS_CHECK_BOUNDS(length - 4);
	cursor += (length - 4);

	//5. return the crc
	TS_CHECK_BOUNDS(4);
	return ENTOHLP((pBuffer + cursor)); //----MARKED-LONG---
}
#endif	/* HAS_MEDIA_TS */

