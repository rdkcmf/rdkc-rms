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

#include "mediaformats/writers/fmp4/atom_free.h"

Atom_free::Atom_free() : Atom(0x66726565, false) {
	_free = 2;
}

Atom_free::~Atom_free() {

}

bool Atom_free::SetFreeSpace(uint32_t free) {

	if (free < MINIMUM) {
		ASSERT("FREE/SKIP box should be at least %"PRIu8" bytes in size, not %"PRIu32"!", MINIMUM, free);
	}

	/*
	 * Remove the box header size and type. From the user's perspective the
	 * free space already includes the header size and type of this box
	 *
	 * TODO: support in case FREE needs to use extended size!
	 */
	_free = free - 8;

	return true;
}

bool Atom_free::WriteFields() {

	// Write here fill data
	_buffer.ReadFromRepeat(0x77, _free);

	return true;
}

uint64_t Atom_free::GetFieldsSize() {

	// Return the content of this box/atom
	return _free;
}
