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


#ifndef ATOM_MDAT_H
#define	ATOM_MDAT_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_mdat : public Atom {
public:
	/*
	 * MDAT atom/box (see 2.13 Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_mdat();
	virtual ~Atom_mdat();

	// Populate the sample to MDAT
	bool AddSample(IOBuffer &sample);
	bool AddSample(IOBuffer &sample, uint8_t *pTagHeader);

	void HasFLVHeader(bool hasFLV);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	uint64_t _mdatSize;

	bool _hasFLV;
};
#endif	/* ATOM_MDAT_H */
