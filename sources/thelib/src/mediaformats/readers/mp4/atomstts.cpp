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
#include "mediaformats/readers/mp4/atomstts.h"

AtomSTTS::AtomSTTS(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
}

AtomSTTS::~AtomSTTS() {
}

vector<uint32_t> AtomSTTS::GetEntries() {
	if (_normalizedEntries.size() != 0)
		return _normalizedEntries;

	FOR_VECTOR_ITERATOR(STTSEntry, _sttsEntries, i) {
		for (uint32_t j = 0; j < VECTOR_VAL(i).count; j++) {
			ADD_VECTOR_END(_normalizedEntries, VECTOR_VAL(i).delta);
		}
	}
	return _normalizedEntries;
}

bool AtomSTTS::ReadData() {
	uint32_t entryCount;
	if (!ReadUInt32(entryCount)) {
		FATAL("Unable to read entry count");
		return false;
	}

	for (uint32_t i = 0; i < entryCount; i++) {
		STTSEntry entry;

		if (!ReadUInt32(entry.count)) {
			FATAL("Unable to read count");
			return false;
		}

		if (!ReadUInt32(entry.delta)) {
			FATAL("Unable to read delta");
			return false;
		}

		ADD_VECTOR_END(_sttsEntries, entry);
	}
	return true;
}


#endif /* HAS_MEDIA_MP4 */
