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
#include "mediaformats/readers/mp4/atomasrt.h"

AtomASRT::AtomASRT(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_qualityEntryCount = 0;
	_segmentRunEntryCount = 0;
}

AtomASRT::~AtomASRT() {
}

bool AtomASRT::ReadData() {
		//FINEST("ASRT");
	if (!ReadUInt8(_qualityEntryCount)) {
		FATAL("Unable to read _qualityEntryCount");
		return false;
	}
		//FINEST("_qualityEntryCount: %"PRIu8, _qualityEntryCount);

	for (uint8_t i = 0; i < _qualityEntryCount; i++) {
		string temp;
		if (!ReadNullTerminatedString(temp)) {
			FATAL("Unable to read _qualitySegmentUrlModifiers");
			return false;
		}
		//		FINEST("%"PRIu8": _qualitySegmentUrlModifiers: %s", i, STR(temp));
		ADD_VECTOR_END(_qualitySegmentUrlModifiers, temp);
	}

	if (!ReadUInt32(_segmentRunEntryCount)) {
		FATAL("Unable to read _segmentRunEntryCount");
		return false;
	}
		//FINEST("_segmentRunEntryCount: %"PRIu32, _segmentRunEntryCount);

	for (uint32_t i = 0; i < _segmentRunEntryCount; i++) {
		SEGMENTRUNENTRY temp;
		if (!ReadUInt32(temp.firstSegment)) {
			FATAL("Unable to read SEGMENTRUNENTRY.FirstSegment");
			return false;
		}
		if (!ReadUInt32(temp.fragmentsPerSegment)) {
			FATAL("Unable to read SEGMENTRUNENTRY.FragmentsPerSegment");
			return false;
		}
				//FINEST("%"PRIu32": SEGMENTRUNENTRY.FirstSegment: %"PRIu32"; SEGMENTRUNENTRY.FragmentsPerSegment: %"PRIu32,
				//		i, temp.firstSegment, temp.fragmentsPerSegment);
		ADD_VECTOR_END(_segmentRunEntryTable, temp);
	}

	return true;
}
#endif /* HAS_MEDIA_MP4 */
