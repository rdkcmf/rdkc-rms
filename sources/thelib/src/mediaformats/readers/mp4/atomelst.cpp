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


#include "mediaformats/readers/mp4/atomelst.h"

AtomELST::AtomELST(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {

}

AtomELST::~AtomELST() {
}

bool AtomELST::ReadData() {
	uint32_t count = 0;
	if (!ReadUInt32(count)) {
		FATAL("Unable to read elst entries count");
		return false;
	}
	//FINEST("-------");
	ELSTEntry entry;
	for (uint32_t i = 0; i < count; i++) {
		if (_version == 1) {
			if (!ReadUInt64(entry.value._64.segmentDuration)) {
				FATAL("Unable to read elst atom");
				return false;
			}
			if (!ReadUInt64(entry.value._64.mediaTime)) {
				FATAL("Unable to read elst atom");
				return false;
			}
		} else {
			if (!ReadUInt32(entry.value._32.segmentDuration)) {
				FATAL("Unable to read elst atom");
				return false;
			}
			if (!ReadUInt32(entry.value._32.mediaTime)) {
				FATAL("Unable to read elst atom");
				return false;
			}
		}
		if (!ReadUInt16(entry.mediaRateInteger)) {
			FATAL("Unable to read elst atom");
			return false;
		}
		if (!ReadUInt16(entry.mediaRateFraction)) {
			FATAL("Unable to read elst atom");
			return false;
		}
		//		FINEST("version: %"PRIu8"; segmentDuration: %"PRIu32"; mediaTime: %"PRIu32"; mediaRateInteger: %"PRIu16"; mediaRateFraction: %"PRIu16,
		//				_version,
		//				entry.value._32.segmentDuration,
		//				entry.value._32.mediaTime,
		//				entry.mediaRateInteger,
		//				entry.mediaRateFraction);
		_entries.push_back(entry);
	}
	return SkipRead(false);
}
