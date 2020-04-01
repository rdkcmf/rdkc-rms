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

#ifndef ATOM_AFRT_H
#define	ATOM_AFRT_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_afrt : public Atom {
public:

	/*
	 * FRAGMENTRUNENTRY structure
	 * See page 44 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _FragmentRunEntry {
		uint32_t _firstFragment;
		uint64_t _firstFragmentTimestamp;
		uint32_t _fragmentDuration;
		uint8_t _discontinuityIndicator; // if fragmentduration == 0
	} FragmentRunEntry;

	/*
	 * AFRT atom/box (see 2.11.2.2 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_afrt();
	virtual ~Atom_afrt();

	/**
	 * Adds a fragment run entry to this atom/box
	 *
	 * @param fragmentRunEntry Entry to be added
	 */
	void AddFragmentRunEntry(FragmentRunEntry &fragmentRunEntry);

	/**
	 * Adds a fragment run entry to this atom/box
	 * See page 44 of Adobe Flash Video File Format Specification v10.1 for
	 * the parameters
	 *
	 * @param firstFragment
	 * @param firstFragmentTimestamp
	 * @param fragmentDuration
	 * @param discontinuityIndicator
	 */
	void AddFragmentRunEntry(uint32_t firstFragment, uint64_t firstFragmentTimestamp,
			uint32_t fragmentDuration, uint8_t discontinuityIndicator);

	/**
	 * Modify the fragment run entry based on the given index
	 * See page 44 of Adobe Flash Video File Format Specification v10.1 for
	 * the parameters except for index.
	 *
	 * @param index Value to indicate the entry to be modified
	 * @param firstFragment
	 * @param firstFragmentTimestamp
	 * @param fragmentDuration
	 * @param discontinuityIndicator
	 * @return true on success, false otherwise
	 */
	bool ModifyFragmentRunEntry(uint32_t index, uint32_t firstFragment,
			uint64_t firstFragmentTimestamp, uint32_t fragmentDuration,
			uint8_t discontinuityIndicator);

	/**
	 * Sets the timescale field of this atom/box
	 *
	 * @param timeScale Value to set
	 */
	void SetTimeScale(uint32_t timeScale);

	/**
	 * Adds a quality segment url modifiers represented through a string
	 *
	 * @param uriModifier Entry to be added
	 */
	void AddQualityEntry(string uriModifier);
	
	/**
	 * Returns the number of fragment run entries
	 * 
     * @return The number of entries available
     */
	uint32_t GetEntryCount();

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	uint32_t _timeScale;

	// Container for the quality modifiers
	vector<string > _qualitySegmentUrlModifiers;

	// Container for the fragment run entries
	vector<FragmentRunEntry > _fragmentRunEntryTable;
};
#endif	/* ATOM_AFRT_H */
