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
#include "mediaformats/readers/mp4/atomavcc.h"

AtomAVCC::AtomAVCC(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {
	_configurationVersion = 0;
	_profile = 0;
	_profileCompatibility = 0;
	_level = 0;
	_naluLengthSize = 0;
}

AtomAVCC::~AtomAVCC() {

	FOR_VECTOR_ITERATOR(AVCCParameter, _seqParameters, i) {
		if (VECTOR_VAL(i).pData != NULL)
			delete[] VECTOR_VAL(i).pData;
	}

	FOR_VECTOR_ITERATOR(AVCCParameter, _picParameters, i) {
		if (VECTOR_VAL(i).pData != NULL)
			delete[] VECTOR_VAL(i).pData;
	}
}

uint64_t AtomAVCC::GetExtraDataStart() {
	return _start + 8;
}

uint64_t AtomAVCC::GetExtraDataLength() {
	return _size - 8;
}

bool AtomAVCC::Read() {
	uint8_t _seqCount;
	uint8_t _picCount;

	if (!ReadUInt8(_configurationVersion)) {
		FATAL("Unable to read _configurationVersion");
		return false;
	}

	if (!ReadUInt8(_profile)) {
		FATAL("Unable to read _profile");
		return false;
	}

	if (!ReadUInt8(_profileCompatibility)) {
		FATAL("Unable to read _profileCompatibility");
		return false;
	}

	if (!ReadUInt8(_level)) {
		FATAL("Unable to read _level");
		return false;
	}

	if (!ReadUInt8(_naluLengthSize)) {
		FATAL("Unable to read _naluLengthSize");
		return false;
	}
	_naluLengthSize = 1 + (_naluLengthSize & 0x03);

	if (!ReadUInt8(_seqCount)) {
		FATAL("Unable to read _seqCount");
		return false;
	}
	_seqCount = _seqCount & 0x1f;

	for (uint8_t i = 0; i < _seqCount; i++) {
		AVCCParameter parameter = {0};

		if (!ReadUInt16(parameter.size)) {
			FATAL("Unable to read parameter.size");
			return false;
		}

		if (parameter.size > 0) {
			parameter.pData = new uint8_t[parameter.size];
			if (!ReadArray(parameter.pData, parameter.size)) {
				FATAL("Unable to read parameter.pData");
				return false;
			}
		}

		ADD_VECTOR_END(_seqParameters, parameter);
	}



	if (!ReadUInt8(_picCount)) {
		FATAL("Unable to read _picCount");
		return false;
	}

	for (uint8_t i = 0; i < _picCount; i++) {
		AVCCParameter parameter = {0, 0};

		if (!ReadUInt16(parameter.size)) {
			FATAL("Unable to read parameter.size");
			return false;
		}

		if (parameter.size > 0) {
			parameter.pData = new uint8_t[parameter.size];
			if (!ReadArray(parameter.pData, parameter.size)) {
				FATAL("Unable to read parameter.pData");
				return false;
			}
		}

		ADD_VECTOR_END(_picParameters, parameter);
	}
	return true;
}

string AtomAVCC::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}


#endif /* HAS_MEDIA_MP4 */
