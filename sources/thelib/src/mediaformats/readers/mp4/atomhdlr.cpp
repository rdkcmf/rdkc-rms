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
#include "mediaformats/readers/mp4/atomhdlr.h"

AtomHDLR::AtomHDLR(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_componentType = 0;
	_componentSubType = 0;
	_componentManufacturer = 0;
	_componentFlags = 0;
	_componentFlagsMask = 0;
	_componentName = "";
}

AtomHDLR::~AtomHDLR() {
}

uint32_t AtomHDLR::GetComponentSubType() {
	return _componentSubType;
}

bool AtomHDLR::ReadData() {
	if (!ReadUInt32(_componentType)) {
		FATAL("Unable to read component type");
		return false;
	}

	if (!ReadUInt32(_componentSubType)) {
		FATAL("Unable to read component sub type");
		return false;
	}

	if (!ReadUInt32(_componentManufacturer)) {
		FATAL("Unable to read component manufacturer");
		return false;
	}

	if (!ReadUInt32(_componentFlags)) {
		FATAL("Unable to read component flags");
		return false;
	}

	if (!ReadUInt32(_componentFlagsMask)) {
		FATAL("Unable to read component flags mask");
		return false;
	}

	if (!ReadString(_componentName, _size - 32)) {
		FATAL("Unable to read component name");
		return false;
	}

	return true;
}

string AtomHDLR::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString() + "(" + U32TOS(_componentSubType) + ")";
}


#endif /* HAS_MEDIA_MP4 */
