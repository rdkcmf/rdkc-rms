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

#include "mediaformats/writers/fmp4/atom_mdat.h"

Atom_mdat::Atom_mdat() : Atom(0x6D646174, false) {

	_mdatSize = 0;
	_hasFLV = false;
}

Atom_mdat::~Atom_mdat() {

}

bool Atom_mdat::AddSample(IOBuffer &sample) {

	uint32_t count = GETAVAILABLEBYTESCOUNT(sample);

	if (_isDirectFileWrite) {
		// Go to the end of file
		if (!_pFile->SeekEnd()) {
			FATAL("Could not seek to end of file!");
			return false;
		}

		
		// Write the content to file
		if (!_pFile->WriteBuffer(GETIBPOINTER(sample), count)) {
			FATAL("Could not write content to file!");
			return false;
		}

	} else {
		//TODO, in case we want to support buffering
		ASSERT("Support for buffering needs to be implemented!");
	}

	_mdatSize += count;
	
	UpdateSize();

	return true;
}

bool Atom_mdat::AddSample(IOBuffer &sample, uint8_t *pTagHeader) {

	uint32_t count = GETAVAILABLEBYTESCOUNT(sample);

	if (_isDirectFileWrite) {
		// Go to the end of file
		if (!_pFile->SeekEnd()) {
			FATAL("Could not seek to end of file!");
			return false;
		}

		if (!_pFile->WriteBuffer(pTagHeader, 11)) {
			FATAL("Could not write content to file!");
			return false;
		}

		// Write the content to file
		if (!_pFile->WriteBuffer(GETIBPOINTER(sample), count)) {
			FATAL("Could not write content to file!");
			return false;
		}

		if (!_pFile->WriteBuffer(pTagHeader + 11, 4)) {
			FATAL("Could not write content to file!");
			return false;
		}
	} else {
		//TODO, in case we want to support buffering
		ASSERT("Support for buffering needs to be implemented!");
	}

	_mdatSize += count + 11 + 4;

	return true;
}

bool Atom_mdat::WriteFields() {

	// No fields to write...

	return true;
}

uint64_t Atom_mdat::GetFieldsSize() {
	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = _mdatSize;

	if (_hasFLV) {
		size += 13; //FLV header size is 13 bytes
	}

	return size;
}

void Atom_mdat::HasFLVHeader(bool hasFLV) {

	_hasFLV = hasFLV;
}
