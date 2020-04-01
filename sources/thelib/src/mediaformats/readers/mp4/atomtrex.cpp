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
#include "mediaformats/readers/mp4/atomtrex.h"

AtomTREX::AtomTREX(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
	_trackID = 0;
	_defaultSampleDescriptionIndex = 0;
	_defaultSampleDuration = 0;
	_defaultSampleSize = 0;
	_defaultSampleFlags = 0;
}

AtomTREX::~AtomTREX() {
}

uint32_t AtomTREX::GetTrackID() {
	return _trackID;
}

uint32_t AtomTREX::GetDefaultSampleDescriptionIndex() {
	return _defaultSampleDescriptionIndex;
}

uint32_t AtomTREX::GetDefaultSampleDuration() {
	return _defaultSampleDuration;
}

uint32_t AtomTREX::GetDefaultSampleSize() {
	return _defaultSampleSize;
}

uint32_t AtomTREX::GetDefaultSampleFlags() {
	return _defaultSampleFlags;
}

bool AtomTREX::ReadData() {
	if (!ReadUInt32(_trackID)) {
		FATAL("Unable to read count");
		return false;
	}
	if (!ReadUInt32(_defaultSampleDescriptionIndex)) {
		FATAL("Unable to read count");
		return false;
	}
	if (!ReadUInt32(_defaultSampleDuration)) {
		FATAL("Unable to read count");
		return false;
	}
	if (!ReadUInt32(_defaultSampleSize)) {
		FATAL("Unable to read count");
		return false;
	}
	if (!ReadUInt32(_defaultSampleFlags)) {
		FATAL("Unable to read count");
		return false;
	}
	return true;
}
#endif /* HAS_MEDIA_MP4 */
