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
#include "mediaformats/readers/mp4/atomftyp.h"

AtomFTYP::AtomFTYP(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {
	_majorBrand = 0;
	_minorVersion = 0;
}

AtomFTYP::~AtomFTYP() {
}

bool AtomFTYP::Read() {
	if (!ReadUInt32(_majorBrand, false)) {
		FATAL("Unable to read major brand");
		return false;
	}

	if (!ReadUInt32(_minorVersion, false)) {
		FATAL("Unable to read minor version");
		return false;
	}

	for (uint64_t i = 16; i < _size; i += 4) {
		uint32_t val = 0;
		if (!ReadUInt32(val, false)) {
			FATAL("Unable to read compatible brand");
			return false;
		}
		ADD_VECTOR_END(_compatibleBrands, val);
	}
	return true;
}

string AtomFTYP::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}


#endif /* HAS_MEDIA_MP4 */
