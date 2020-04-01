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

#include "mediaformats/writers/fmp4/atom_tfhd.h"

Atom_tfhd::Atom_tfhd() : Atom(0x74666864, 0, 0x20) {
	_trackID = 1;
	_baseDataOffset = 0;
	_sampleDescriptionIndex = 1;
	_defaultSampleDuration = 0;
	_defaultSampleSize = 0;
	_defaultSampleFlagBits = 0;
}

Atom_tfhd::~Atom_tfhd() {

}

void Atom_tfhd::SetTrackID(uint32_t trackID) {
	_trackID = trackID;
}

uint32_t Atom_tfhd::GetTrackID() {
	return _trackID;
}

void Atom_tfhd::SetBaseDataOffset(uint64_t baseDataOffset) {

	_flags |= 0x000001;
	_baseDataOffset = baseDataOffset;
}

void Atom_tfhd::SetSampleDescriptionIndex(uint32_t sampleDescriptionIndex) {
    _flags |= 0x000002;
	_sampleDescriptionIndex = sampleDescriptionIndex;
}

void Atom_tfhd::SetDefaultSampleSize(uint32_t defaultSampleSize) {
    _flags |= 0x000010;
    _defaultSampleSize = defaultSampleSize;
}

void Atom_tfhd::SetDefaultSampleDuration(uint32_t defaultSampleDuration) {
    _flags |= 0x000008;
    _defaultSampleDuration = defaultSampleDuration;
}

void Atom_tfhd::SetDefaultSampleFlags(uint32_t sampleFlagBits) {
	_defaultSampleFlagBits = sampleFlagBits;
}

void Atom_tfhd::SetDefaultSampleFlags(SampleFlags &sampleFlags) {

	// Implement as necessary, should be straightforward enough
	NYIA;
}

bool Atom_tfhd::WriteFields() {

	// Write to buffer the fields of this atom
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4, _trackID);

	// Based on the default flag, write other necessary fields
	if (_flags & 0x000001) {
		_buffer.ReadFromRepeat(0, 8);
		EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
				_baseDataOffset);
	}
	if (_flags & 0x000002) {
		_buffer.ReadFromRepeat(0, 4);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_sampleDescriptionIndex);
	}
	if (_flags & 0x000008) {
		_buffer.ReadFromRepeat(0, 4);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_defaultSampleDuration);
	}
	if (_flags & 0x000010) {
		_buffer.ReadFromRepeat(0, 4);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_defaultSampleSize);
	}
	if (_flags & 0x000020) {
		_buffer.ReadFromRepeat(0, 4);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_defaultSampleFlagBits);
	}

	return true;
}

uint64_t Atom_tfhd::GetFieldsSize() {
	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	size += 4; // trackID

	// Increment the size based on the flags
	if (_flags & 0x000001) {
		size += 8; // baseDataOffset
	}
	if (_flags & 0x000002) {
		size += 4; // sampleDescriptionIndex
	}
	if (_flags & 0x000008) {
		size += 4; // defaultSampleDuration
	}
	if (_flags & 0x000010) {
		size += 4; // defaultSampleSize
	}
	if (_flags & 0x000020) {
		size += 4; // defaultSampleFlagBits
	}

	return size;
}
