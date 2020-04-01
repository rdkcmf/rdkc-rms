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
#include "mediaformats/readers/mp4/atomstsz.h"

AtomSTSZ::AtomSTSZ(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {

}

AtomSTSZ::~AtomSTSZ() {
}

vector<uint64_t> AtomSTSZ::GetEntries() {
	return _entries;
}

bool AtomSTSZ::ReadData() {
	if (!ReadUInt32(_sampleSize)) {
		FATAL("Unable to read sample size");
		return false;
	}

	if (!ReadUInt32(_sampleCount)) {
		FATAL("Unable to read sample count");
		return false;
	}

	if (_sampleSize != 0) {
		for (uint32_t i = 0; i < _sampleCount; i++) {
			ADD_VECTOR_END(_entries, _sampleSize);
		}
		return true;
	} else {
		for (uint32_t i = 0; i < _sampleCount; i++) {
			uint32_t size;
			if (!ReadUInt32(size)) {
				FATAL("Unable to read size");
				return false;
			}
			ADD_VECTOR_END(_entries, size);
		}
		return true;
	}
}

#endif /* HAS_MEDIA_MP4 */
