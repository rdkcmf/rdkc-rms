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


#ifdef HAS_MEDIA_MP4
#include "mediaformats/readers/mp4/atommdhd.h"

AtomMDHD::AtomMDHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_creationTime = 0;
	_modificationTime = 0;
	_timeScale = 0;
	_duration = 0;
	_language = 0;
	_quality = 0;
}

AtomMDHD::~AtomMDHD() {
}

uint32_t AtomMDHD::GetTimeScale() {
	return _timeScale;
}

bool AtomMDHD::ReadData() {
	if (_version == 1) {
		return ReadDataVersion1();
	} else {
		return ReadDataVersion0();
	}
}

bool AtomMDHD::ReadDataVersion0() {
	uint32_t temp = 0;
	if (!ReadUInt32(temp)) {
		FATAL("Unable to read creation time");
		return false;
	}
	_creationTime = temp;

	if (!ReadUInt32(temp)) {
		FATAL("Unable to read modification time");
		return false;
	}
	_modificationTime = temp;

	if (!ReadUInt32(_timeScale)) {
		FATAL("Unable to read time scale");
		return false;
	}

	if (!ReadUInt32(temp)) {
		FATAL("Unable to read duration");
		return false;
	}
	_duration = temp;

	if (!ReadUInt16(_language)) {
		FATAL("Unable to read language");
		return false;
	}

	if (!ReadUInt16(_quality)) {
		FATAL("Unable to read quality");
		return false;
	}

	return true;
}

bool AtomMDHD::ReadDataVersion1() {
	if (!ReadUInt64(_creationTime)) {
		FATAL("Unable to read creation time");
		return false;
	}

	if (!ReadUInt64(_modificationTime)) {
		FATAL("Unable to read modification time");
		return false;
	}

	if (!ReadUInt32(_timeScale)) {
		FATAL("Unable to read time scale");
		return false;
	}

	if (!ReadUInt64(_duration)) {
		FATAL("Unable to read duration");
		return false;
	}

	if (!ReadUInt16(_language)) {
		FATAL("Unable to read language");
		return false;
	}

	if (!ReadUInt16(_quality)) {
		FATAL("Unable to read quality");
		return false;
	}

	return true;
}

#endif /* HAS_MEDIA_MP4 */
