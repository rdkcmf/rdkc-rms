/*
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
 */

#ifndef ATOM_FREE_H
#define	ATOM_FREE_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_free : public Atom {
public:
	/*
	 * FREE atom/box (see 2.15 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_free();
	virtual ~Atom_free();

	/**
	 * Sets the number of free spaces this box will contain. The free space
	 * to be set should be at least 10 bytes (8 byte header size) + 2 space bytes,
	 * since a single byte space content can be interpreted as an extended size.
	 *
	 * @param free Size of this free/skip box
	 */
	bool SetFreeSpace(uint32_t free);

	static const uint8_t MINIMUM = 10;

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	// Free space/filler to be used by this atom
	uint32_t _free;
};
#endif	/* ATOM_FREE_H */
