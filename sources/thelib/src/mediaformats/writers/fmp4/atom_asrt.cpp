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

#include "mediaformats/writers/fmp4/atom_asrt.h"

Atom_asrt::Atom_asrt() : Atom(0x61737274, true) {

}

Atom_asrt::~Atom_asrt() {
	_qualitySegmentUrlModifiers.clear();
	_segmentRunEntryTable.clear();
}

void Atom_asrt::AddSegmentRunEntry(uint32_t firstSegment, uint32_t fragmentsPerSegment) {

	// Create and add a segment entry
	SegmentRunEntry newEntry;
	newEntry._firstSegment = firstSegment;
	newEntry._fragmentsPerSegment = fragmentsPerSegment;

	ADD_VECTOR_END(_segmentRunEntryTable, newEntry);
}

bool Atom_asrt::ModifySegmentRunEntry(uint32_t index, uint32_t firstSegment,
		uint32_t fragmentsPerSegment) {

	// Return false if this is an invalid index
	if (index >= _segmentRunEntryTable.size()) return false;

	_segmentRunEntryTable[index]._firstSegment = firstSegment;
	_segmentRunEntryTable[index]._fragmentsPerSegment = fragmentsPerSegment;

	return true;
}

void Atom_asrt::AddQualityEntry(string urlModifier) {

	ADD_VECTOR_END(_qualitySegmentUrlModifiers, urlModifier);
}

bool Atom_asrt::WriteFields() {
	/*
	 * Special case for ASRT, AFRT and ABST: data are in buffer since they are
	 * common to all fragments.
	 */

	// Quality Entry Count;
	_buffer.ReadFromByte((uint8_t) _qualitySegmentUrlModifiers.size());

	/*
	 * For each quality segment url modifier, transfer it to the buffer
	 */
	FOR_VECTOR(_qualitySegmentUrlModifiers, i) {
		WriteString(_qualitySegmentUrlModifiers[i]);
	}

	// Segment Run Entry Count;
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
			((uint32_t) _segmentRunEntryTable.size()));

	/*
	 * For each segment run entry, transfer it to the buffer
	 */
	FOR_VECTOR(_segmentRunEntryTable, j) {
		_buffer.ReadFromRepeat(0, 8);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
				_segmentRunEntryTable[j]._firstSegment);

		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_segmentRunEntryTable[j]._fragmentsPerSegment);
	}

	return true;
}

uint64_t Atom_asrt::GetFieldsSize() {
	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	// Quality Entry Count;
	size += 1;

	/*
	 * For each quality segment url modifier, compute the size
	 */
	FOR_VECTOR(_qualitySegmentUrlModifiers, i) {
		size += GetStringSize(_qualitySegmentUrlModifiers[i]);
	}

	// Segment Run Entry Count;
	size += 4;

	/*
	 * For each segment run entry, transfer it to the buffer
	 */
	FOR_VECTOR(_segmentRunEntryTable, j) {
		size += 8;
	}

	return size;
}
