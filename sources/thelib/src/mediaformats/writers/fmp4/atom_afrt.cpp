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

#include "mediaformats/writers/fmp4/atom_afrt.h"

Atom_afrt::Atom_afrt() : Atom(0x61667274, true) {

	_timeScale = 1000;
}

Atom_afrt::~Atom_afrt() {

	_qualitySegmentUrlModifiers.clear();
	_fragmentRunEntryTable.clear();
}

void Atom_afrt::AddFragmentRunEntry(FragmentRunEntry &fragmentRunEntry) {

	ADD_VECTOR_END(_fragmentRunEntryTable, fragmentRunEntry);
}

void Atom_afrt::AddFragmentRunEntry(uint32_t firstFragment,
		uint64_t firstFragmentTimestamp,
		uint32_t fragmentDuration,
		uint8_t discontinuityIndicator) {

	// Create and add the new fragment run entry
	FragmentRunEntry newEntry;
	newEntry._firstFragment = firstFragment;
	newEntry._firstFragmentTimestamp = firstFragmentTimestamp;
	newEntry._fragmentDuration = fragmentDuration;
	newEntry._discontinuityIndicator = discontinuityIndicator;

	ADD_VECTOR_END(_fragmentRunEntryTable, newEntry);
}

bool Atom_afrt::ModifyFragmentRunEntry(uint32_t index, uint32_t firstFragment,
		uint64_t firstFragmentTimestamp, uint32_t fragmentDuration,
		uint8_t discontinuityIndicator) {

	// Return false if this is an invalid index
	if (index >= _fragmentRunEntryTable.size()) return false;

	_fragmentRunEntryTable[index]._firstFragment = firstFragment;
	_fragmentRunEntryTable[index]._firstFragmentTimestamp = firstFragmentTimestamp;
	_fragmentRunEntryTable[index]._fragmentDuration = fragmentDuration;
	_fragmentRunEntryTable[index]._discontinuityIndicator = discontinuityIndicator;

	return true;
}

void Atom_afrt::SetTimeScale(uint32_t timeScale) {
	_timeScale = timeScale;
}

void Atom_afrt::AddQualityEntry(string uriModifier) {

	ADD_VECTOR_END(_qualitySegmentUrlModifiers, uriModifier);
}

uint32_t Atom_afrt::GetEntryCount() {
	return (uint32_t) _fragmentRunEntryTable.size();
}

bool Atom_afrt::WriteFields() {
	/*
	 * Special case for ASRT, AFRT and ABST: data are in buffer since they are
	 * common to all fragments.
	 */

	// timescale
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4, _timeScale);

	// Quality Entry Count;
	_buffer.ReadFromByte((uint8_t) _qualitySegmentUrlModifiers.size());

	/*
	 * For each quality segment url modifier, transfer it to the buffer
	 */
	FOR_VECTOR(_qualitySegmentUrlModifiers, i) {
		WriteString(_qualitySegmentUrlModifiers[i]);
	}

	// Fragment Run Entry Count;
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
			((uint32_t) _fragmentRunEntryTable.size()));

	/*
	 * For each fragment run entry, transfer it to the buffer
	 */
	FOR_VECTOR(_fragmentRunEntryTable, j) {

		_buffer.ReadFromRepeat(0, 16);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 16,
				_fragmentRunEntryTable[j]._firstFragment);

		EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 12,
				_fragmentRunEntryTable[j]._firstFragmentTimestamp);

		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_fragmentRunEntryTable[j]._fragmentDuration);

		// If we have a zero duration, there will be a discontinuity indicator
		if (_fragmentRunEntryTable[j]._fragmentDuration == 0) {
			_buffer.ReadFromByte(_fragmentRunEntryTable[j]._discontinuityIndicator);
		}
	}

	return true;
}

uint64_t Atom_afrt::GetFieldsSize() {

	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	// timescale
	size += 4;
	//EHTONLP(GETIBPOINTER(_buffer) + 12, _timeScale);

	// Quality Entry Count;
	size += 1;
	//_buffer.ReadFromByte((uint8_t) _qualitySegmentUrlModifiers.size());

	/*
	 * For each quality segment url modifier, compute the size
	 */
	FOR_VECTOR(_qualitySegmentUrlModifiers, i) {
		size += GetStringSize(_qualitySegmentUrlModifiers[i]);
	}

	// Fragment Run Entry Count;
	size += 4;

	/*
	 * For each fragment run entry, transfer it to the buffer
	 */
	FOR_VECTOR(_fragmentRunEntryTable, j) {

		size += 16;

		// If we have a zero duration, there will be a discontinuity indicator
		if (_fragmentRunEntryTable[j]._fragmentDuration == 0) {
			//_buffer.ReadFromByte(_fragmentRunEntryTable[j]._discontinuityIndicator);
			size += 1;
		}
	}

	return size;
}
