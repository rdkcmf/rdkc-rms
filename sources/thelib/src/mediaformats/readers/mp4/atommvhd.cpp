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
#include "mediaformats/readers/mp4/atommvhd.h"

AtomMVHD::AtomMVHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_creationTime = 0;
	_modificationTime = 0;
	_timeScale = 0;
	_duration = 0;
	_preferredRate = 0;
	_preferredVolume = 0;
	memset(_reserved, 0, 10);
	memset(_matrixStructure, 0, 36);
	_previewTime = 0;
	_previewDuration = 0;
	_posterTime = 0;
	_selectionTime = 0;
	_selectionDuration = 0;
	_currentTime = 0;
	_nextTrakId = 0;
}

AtomMVHD::~AtomMVHD() {
}

bool AtomMVHD::ReadData() {
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
		uint32_t temp;
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

	if (!ReadUInt32(_timeScale)) {
		FATAL("Unable to read time scale");
		return false;
	}
	if (_version == 1) {
		if (!ReadUInt64(_duration)) {
			FATAL("Unable to read duration");
			return false;
		}
	} else {
		uint32_t temp;
		if (!ReadUInt32(temp)) {
			FATAL("Unable to read duration");
			return false;
		}
		_duration = temp;
	}

	if (!ReadUInt32(_preferredRate)) {
		FATAL("Unable to read preferred rate");
		return false;
	}

	if (!ReadUInt16(_preferredVolume)) {
		FATAL("Unable to read preferred volume");
		return false;
	}

	if (!ReadArray(_reserved, 10)) {
		FATAL("Unable to read reserved");
		return false;
	}

	if (!ReadArray((uint8_t *) _matrixStructure, 36)) {
		FATAL("Unable to read matrix structure");
		return false;
	}

	if (!ReadUInt32(_previewTime)) {
		FATAL("Unable to read preview time");
		return false;
	}

	if (!ReadUInt32(_previewDuration)) {
		FATAL("Unable to read preview duration");
		return false;
	}

	if (!ReadUInt32(_posterTime)) {
		FATAL("Unable to read poster time");
		return false;
	}

	if (!ReadUInt32(_selectionTime)) {
		FATAL("Unable to read selection time");
		return false;
	}

	if (!ReadUInt32(_selectionDuration)) {
		FATAL("Unable to read selection duration");
		return false;
	}

	if (!ReadUInt32(_currentTime)) {
		FATAL("Unable to read current time");
		return false;
	}

	if (!ReadUInt32(_nextTrakId)) {
		FATAL("Unable to read next track ID");
		return false;
	}

	return true;
}


#endif /* HAS_MEDIA_MP4 */
