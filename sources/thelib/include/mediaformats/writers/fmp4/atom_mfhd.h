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


#ifndef _ATOM_MFHD_H
#define	_ATOM_MFHD_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_mfhd : public Atom {
public:
	/*
	 * MFHD atom/box (see 2.12.1 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_mfhd();
	virtual ~Atom_mfhd();

	/**
	 * Sets the sequence number field, which in this case is the fragment
	 * number, starts at 1.
	 *
	 * @param seqNum Number to set
	 */
	void SetSequenceNumber(uint32_t seqNum);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	// Basically equates to the fragment number of this atom
	uint32_t _sequenceNumber;
};
#endif	/* _ATOM_MFHD_H */
