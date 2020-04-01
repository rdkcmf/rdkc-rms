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

#ifndef _ATOM_ASRT_H
#define	_ATOM_ASRT_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_asrt : public Atom {

	/*
	 * SEGMENTRUNENTRY structure
	 * See page 42 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _SegmentRunEntry {
		uint32_t _firstSegment;
		uint32_t _fragmentsPerSegment;
	} SegmentRunEntry;

public:
	/*
	 * ASRT atom/box (see 2.11.2.1 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_asrt();
	virtual ~Atom_asrt();

	/**
	 * Adds a segment run entry.
	 * See page 42 of Adobe Flash Video File Format Specification v10.1 for
	 * the parameters
	 *
	 * @param firstSegment
	 * @param fragmentsPerSegment
	 */
	void AddSegmentRunEntry(uint32_t firstSegment, uint32_t fragmentsPerSegment);

	/**
	 * Modifies the segment run entry based on the given index
	 * See page 42 of Adobe Flash Video File Format Specification v10.1 for
	 * the parameters except for index.
	 *
	 * @param index Value that represents the segment run entry
	 * @param firstSegment
	 * @param fragmentsPerSegment
	 * @return true on success, false otherwise
	 */
	bool ModifySegmentRunEntry(uint32_t index, uint32_t firstSegment,
			uint32_t fragmentsPerSegment);

	/**
	 * Adds a quality segment url modifiers represented through a string
	 *
	 * @param urlModifier Entry to be added
	 */
	void AddQualityEntry(string urlModifier);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	// Container for the quality modifiers
	vector<string > _qualitySegmentUrlModifiers;

	// Container for the segment run entries
	vector<SegmentRunEntry > _segmentRunEntryTable;
};
#endif	/* _ATOM_ASRT_H */
