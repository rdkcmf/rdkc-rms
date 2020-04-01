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
#include "mediaformats/readers/mp4/atomtfhd.h"

AtomTFHD::AtomTFHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_trackID = 0;
	_baseDataOffset = 0;
	_sampleDescriptionIndex = 0;
	_defaultSampleDuration = 0;
	_defaultSampleSize = 0;
	_defaultSampleFlags = 0;
}

AtomTFHD::~AtomTFHD() {
}

uint32_t AtomTFHD::GetTrackId() {
	return _trackID;
}

int64_t AtomTFHD::GetBaseDataOffset() {
	return _baseDataOffset;
}

bool AtomTFHD::HasBaseDataOffset() {
	return (_flags[2]&0x01) != 0;
}

bool AtomTFHD::HasSampleDescriptionIndex() {
	return (_flags[2]&0x02) != 0;
}

bool AtomTFHD::HasDefaultSampleDuration() {
	return (_flags[2]&0x08) != 0;
}

bool AtomTFHD::HasDefaultSampleSize() {
	return (_flags[2]&0x10) != 0;
}

bool AtomTFHD::HasDefaultSampleFlags() {
	return (_flags[2]&0x20) != 0;
}

bool AtomTFHD::DurationIsEmpty() {
	return (_flags[0]&0x01) != 0;
}

bool AtomTFHD::ReadData() {
	//FINEST("TFHD");
	if (!ReadInt32(_trackID)) {
		FATAL("Unable to read track ID");
		return false;
	}
	//FINEST("_trackID: %"PRIi32, _trackID);
	if (HasBaseDataOffset()) {
		if (!ReadInt64(_baseDataOffset)) {
			FATAL("Unable to read base data offset");
			return false;
		}
		//FINEST("_baseDataOffset: %"PRIi64, _baseDataOffset);
	}
	if (HasSampleDescriptionIndex()) {
		if (!ReadInt32(_sampleDescriptionIndex)) {
			FATAL("Unable to read sample description index");
			return false;
		}
		//FINEST("_sampleDescriptionIndex: %"PRIi32, _sampleDescriptionIndex);
	}
	if (HasDefaultSampleDuration()) {
		if (!ReadInt32(_defaultSampleDuration)) {
			FATAL("Unable to read default sample duration");
			return false;
		}
		//FINEST("_defaultSampleDuration: %"PRIi32, _defaultSampleDuration);
	}
	if (HasDefaultSampleSize()) {
		if (!ReadInt32(_defaultSampleSize)) {
			FATAL("Unable to read default sample size");
			return false;
		}
		//FINEST("_defaultSampleSize: %"PRIi32, _defaultSampleSize);
	}
	if (HasDefaultSampleFlags()) {
		if (!ReadInt32(_defaultSampleFlags)) {
			FATAL("Unable to read default sample flags");
			return false;
		}
		//FINEST("_defaultSampleFlags: %"PRIi32, _defaultSampleFlags);
	}
	return true;
}
#endif /* HAS_MEDIA_MP4 */
