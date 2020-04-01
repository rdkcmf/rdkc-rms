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
#include "mediaformats/readers/mp4/atomafrt.h"

AtomAFRT::AtomAFRT(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_timeScale = 0;
	_qualityEntryCount = 0;
	_fragmentRunEntryCount = 0;
}

AtomAFRT::~AtomAFRT() {
}

bool AtomAFRT::ReadData() {
		//FINEST("AFRT");
	if (!ReadUInt32(_timeScale)) {
		FATAL("Unable to read _timeScale");
		return false;
	}
		//FINEST("_timeScale: %"PRIu32, _timeScale);

	if (!ReadUInt8(_qualityEntryCount)) {
		FATAL("Unable to read _qualityEntryCount");
		return false;
	}
		//FINEST("_qualityEntryCount: %"PRIu8, _qualityEntryCount);

	for (uint8_t i = 0; i < _qualityEntryCount; i++) {
		string temp;
		if (!ReadNullTerminatedString(temp)) {
			FATAL("Unable to read QualitySegmentUrlModifiers");
			return false;
		}
		//FINEST("%"PRIu8": QualitySegmentUrlModifiers: %s", i, STR(temp));
		ADD_VECTOR_END(_qualitySegmentUrlModifiers, temp);
	}

	if (!ReadUInt32(_fragmentRunEntryCount)) {
		FATAL("Unable to read _fragmentRunEntryCount");
		return false;
	}
		//FINEST("_fragmentRunEntryCount: %"PRIu32, _fragmentRunEntryCount);

	for (uint32_t i = 0; i < _fragmentRunEntryCount; i++) {
		FRAGMENTRUNENTRY temp = {0, 0, 0, 0};
		if (!ReadUInt32(temp.firstFragment)) {
			FATAL("Unable to read FRAGMENTRUNENTRY.FirstFragment");
			return false;
		}
		if (!ReadUInt64(temp.firstFragmentTimestamp)) {
			FATAL("Unable to read FRAGMENTRUNENTRY.FirstFragmentTimestamp");
			return false;
		}
		if (!ReadUInt32(temp.fragmentDuration)) {
			FATAL("Unable to read FRAGMENTRUNENTRY.FragmentDuration");
			return false;
		}
		if (temp.fragmentDuration == 0) {
			if (!ReadUInt8(temp.discontinuityIndicator)) {
				FATAL("Unable to read FRAGMENTRUNENTRY.DiscontinuityIndicator");
				return false;
			}
		}
				//FINEST("%"PRIu32": FRAGMENTRUNENTRY.FirstFragment: %"PRIu32"; FRAGMENTRUNENTRY.FirstFragmentTimestamp: %"PRIu64"; FRAGMENTRUNENTRY.FragmentDuration: %"PRIu32"; FRAGMENTRUNENTRY.DiscontinuityIndicator: %"PRIu8,
				//		i, temp.firstFragment, temp.firstFragmentTimestamp, temp.fragmentDuration, temp.discontinuityIndicator);
		ADD_VECTOR_END(_fragmentRunEntryTable, temp);
	}

	return true;
}
#endif /* HAS_MEDIA_MP4 */
