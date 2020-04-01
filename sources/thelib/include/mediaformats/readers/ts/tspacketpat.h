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
#ifndef _TSPACKETPAT_H
#define	_TSPACKETPAT_H

#include "common.h"

//iso13818-1.pdf page 61/174
//Table 2-25 – Program association section

class TSPacketPAT {
private:
	//fields
	uint8_t _tableId;
	bool _sectionSyntaxIndicator;
	bool _reserved1;
	uint8_t _reserved2;
	uint16_t _sectionLength;
	uint16_t _transportStreamId;
	uint8_t _reserved3;
	uint8_t _versionNumber;
	bool _currentNextIndicator;
	uint8_t _sectionNumber;
	uint8_t _lastSectionNumber;
	uint32_t _crc;

	//internal variables
	uint32_t _patStart;
	uint32_t _patLength;
	uint32_t _entriesCount;
	map<uint16_t, uint16_t> _networkPids;
	map<uint16_t, uint16_t> _programPids;
public:
	TSPacketPAT();
	virtual ~TSPacketPAT();

	operator string();

	bool Read(uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor);

	map<uint16_t, uint16_t> &GetPMTs();
	map<uint16_t, uint16_t> &GetNITs();
	uint32_t GetCRC();

	static uint32_t PeekCRC(uint8_t *pBuffer, uint32_t cursor, uint32_t maxCursor);
};

#endif	/* _TSPACKETPAT_H */
#endif	/* HAS_MEDIA_TS */

