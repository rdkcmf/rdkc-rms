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

#include "mediaformats/writers/fmp4/atom_mfhd.h"

Atom_mfhd::Atom_mfhd() : Atom(0x6D666864, true) {
	_sequenceNumber = 1; // fragment numbering starts at 1
}

Atom_mfhd::~Atom_mfhd() {

}

void Atom_mfhd::SetSequenceNumber(uint32_t seqNum) {

	_sequenceNumber = seqNum;
}

bool Atom_mfhd::WriteFields() {

	// Transfer this field to the box/atom's buffer
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
			_sequenceNumber);

	return true;
}

uint64_t Atom_mfhd::GetFieldsSize() {
	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	size += 4; // sequenceNumber

	return size;
}
