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
#include "mediaformats/readers/mp4/atomtkhd.h"

AtomTKHD::AtomTKHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_creationTime = 0;
	_modificationTime = 0;
	_trackId = 0;
	memset(_reserved1, 0, 4);
	_duration = 0;
	memset(_reserved2, 0, 8);
	_layer = 0;
	_alternateGroup = 0;
	_volume = 0;
	memset(_reserved3, 0, 2);
	memset(_matrixStructure, 0, 36);
	_trackWidth = 0;
	_trackHeight = 0;
}

AtomTKHD::~AtomTKHD() {
}

uint32_t AtomTKHD::GetTrackId() {
	return _trackId;
}

uint32_t AtomTKHD::GetWidth() {
	return _trackWidth >> 16;
}

uint32_t AtomTKHD::GetHeight() {
	return _trackHeight >> 16;
}

bool AtomTKHD::ReadData() {
	if (_version == 1) {
		if (!ReadUInt64(_creationTime)) {
			FATAL("Unable to read creation time");
			return false;
		}

		if (!ReadUInt64(_modificationTime)) {
			FATAL("Unable to read modification time");
			return false;
		}
	} else {
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
	}

	if (!ReadUInt32(_trackId)) {
		FATAL("Unable to read track id");
		return false;
	}

	if (!ReadArray(_reserved1, 4)) {
		FATAL("Unable to read reserved 1");
		return false;
	}

	if (_version == 1) {
		if (!ReadUInt64(_duration)) {
			FATAL("Unable to read duration");
			return false;
		}
	} else {
		uint32_t temp = 0;
		if (!ReadUInt32(temp)) {
			FATAL("Unable to read duration");
			return false;
		}
		_duration = temp;
	}

	if (!ReadArray(_reserved2, 8)) {
		FATAL("Unable to read reserved 2");
		return false;
	}

	if (!ReadUInt16(_layer)) {
		FATAL("Unable to read layer");
		return false;
	}

	if (!ReadUInt16(_alternateGroup)) {
		FATAL("Unable to read alternate group");
		return false;
	}

	if (!ReadUInt16(_volume)) {
		FATAL("Unable to read volume");
		return false;
	}

	if (!ReadArray(_reserved3, 2)) {
		FATAL("Unable to read reserved 3");
		return false;
	}

	if (!ReadArray(_matrixStructure, 36)) {
		FATAL("Unable to read matrix structure");
		return false;
	}

	if (!ReadUInt32(_trackWidth)) {
		FATAL("Unable to read track width");
		return false;
	}

	if (!ReadUInt32(_trackHeight)) {
		FATAL("Unable to read track height");
		return false;
	}

	return true;
}


#endif /* HAS_MEDIA_MP4 */
