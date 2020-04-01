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
#include "mediaformats/readers/mp4/versionedatom.h"

VersionedAtom::VersionedAtom(MP4Document *pDocument, uint32_t type,
		uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {
	_version = 0;
	memset(_flags, 0, 3);
}

VersionedAtom::~VersionedAtom() {
}

bool VersionedAtom::Read() {
	if (!ReadUInt8(_version)) {
		FATAL("Unable to read version");
		return false;
	}
	//FINEST("_version: %"PRIu8, _version);

	if (!ReadArray(_flags, 3)) {
		FATAL("Unable to read flags");
		return false;
	}
	//FINEST("_flags: %"PRIx8"%"PRIx8"%"PRIx8, _flags[0], _flags[1], _flags[2]);

	return ReadData();
}

string VersionedAtom::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}

#endif /* HAS_MEDIA_MP4 */
