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

#include "mediaformats/writers/fmp4/atom_afra.h"

Atom_afra::Atom_afra() : Atom(0x61667261, true) {
	// default settings
	_longIds = true; //1 bit
	_longOffset = true; //1 bit
	_globalEntries = false; //1 bit
	//_reserved = 0; //5 bits
	_timeScale = 1000;

	// time (8) + offset (8)
	_afraEntrySize = 16;

	// time (8) + segment (4) + fragment (4) + afraOffset (8) + offsetFromAfra (8)
	_globalAfraEntrySize = 32;
}

Atom_afra::~Atom_afra() {
	_localAccessEntries.clear();
	_globalAccessEntries.clear();
}

void Atom_afra::SetBitFlags(bool longID, bool longOffset, bool globalEntries) {

	_longIds = longID;
	_longOffset = longOffset;
	_globalEntries = globalEntries;
}

void Atom_afra::SetTimeScale(uint32_t timeScale) {
	_timeScale = timeScale;
}

void Atom_afra::AddAfraEntry(uint64_t time, uint64_t offset) {

	// Create and add an AFRA entry
	AfraEntry localEntry;
	localEntry._time = time;
	localEntry._offset = offset;

	ADD_VECTOR_END(_localAccessEntries, localEntry);
}

bool Atom_afra::ModifyAfraRunEntry(uint32_t index, uint64_t time, uint64_t offset) {

	// Return false if this is an invalid index
	if (index >= _localAccessEntries.size()) return false;

	if (_isDirectFileWrite) {
		// Consider the header
		uint32_t pos = (uint32_t) (_position + GetHeaderSize());

		// 1 (bitFlags) + 4 (timeScale) + 4 (localAccessEntries.size())
		pos += 9;

		// In case we have multiple AFRA local access entries
		pos += (index * _afraEntrySize);

		// Locate this position
		if (!_pFile->SeekTo(pos)) {
			FATAL("Could not seek to desired file position!");
			return false;
		}

		// Modify the time
		_pFile->WriteUI64(time);

		// Modify the offset
		if (_longOffset) {
			_pFile->WriteUI64(offset);
		} else {
			_pFile->WriteUI32((uint32_t) offset);
		}
	} else {
		// Just modify contents on the buffer if this is not a direct file write
		_localAccessEntries[index]._time = time;
		_localAccessEntries[index]._offset = offset;
	}

	return true;
}

bool Atom_afra::WriteFields() {

	// set bit flags
	// 1110 0000
	uint8_t bitFlags = 0;
	if (_longIds) bitFlags |= 0x80;
	if (_longOffset) bitFlags |= 0x40;
	if (_globalEntries) bitFlags |= 0x20;

	// Transfer to buffer contents of the bit flags
	_buffer.ReadFromByte(bitFlags);

	// timescale
	_buffer.ReadFromRepeat(0, 8);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, _timeScale);

	// local access entries
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
			(uint32_t) _localAccessEntries.size());

	FOR_VECTOR(_localAccessEntries, i) {
		// Copy to buffer the correct size depending on the _longOffset flag
		if (_longOffset) {
			_buffer.ReadFromRepeat(0, 16);
			EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 16,
					_localAccessEntries[i]._time);
			EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
					_localAccessEntries[i]._offset);
		} else {
			_buffer.ReadFromRepeat(0, 12);
			EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 12,
					_localAccessEntries[i]._time);
			EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
					(uint32_t) _localAccessEntries[i]._offset);
		}
	}

	if (_globalEntries) {
		// global access entries
		_buffer.ReadFromRepeat(0, 4);
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				(uint32_t) _globalAccessEntries.size());

		/*
		 * Transfer to buffer if we have global entries
		 */
		FOR_VECTOR(_globalAccessEntries, j) {
			// time
			_buffer.ReadFromRepeat(0, 8);
			EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
					_globalAccessEntries[j]._time);

			// segment and fragment
			if (_longIds) {
				_buffer.ReadFromRepeat(0, 8);
				EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
						_globalAccessEntries[j]._segment);
				EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
						_globalAccessEntries[j]._fragment);
			} else {
				_buffer.ReadFromRepeat(0, 4);
				EHTONSP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
						(uint16_t) _globalAccessEntries[j]._segment);
				EHTONSP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 2,
						(uint16_t) _globalAccessEntries[j]._fragment);
			}

			// offsets
			if (_longOffset) {
				_buffer.ReadFromRepeat(0, 16);
				EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 16,
						_globalAccessEntries[j]._afraOffset);
				EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
						_globalAccessEntries[j]._offsetFromAfra);
			} else {
				_buffer.ReadFromRepeat(0, 8);
				EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
						(uint32_t) _globalAccessEntries[j]._afraOffset);
				EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
						(uint32_t) _globalAccessEntries[j]._offsetFromAfra);
			}
		}
	}

	return true;
}

uint64_t Atom_afra::GetFieldsSize() {
	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	// (longIds (1 bit), longOffset (1 bit), globalEntries (1 bit), reserved (5 bits))
	size += 1;

	// timescale
	size += 4;

	// entrycount
	size += 4;

	// Adjust local and globa afra entry sizes, start at _longIds = _longOffset = false
	// time (8) + offset (4)
	_afraEntrySize = 12;

	// time (8) + segment (2) + fragment (2) + afraOffset (4) + offsetFromAfra (4)
	_globalAfraEntrySize = 20;


	if (_longIds) {
		// if longIds is true, add 4 bytes (fragment (2) + segment (2))
		_globalAfraEntrySize += 4;
	}

	if (_longOffset) {
		// if _longOffset is true, double the offsets
		_afraEntrySize += 4;
		_globalAfraEntrySize += 8;
	}

	// add the local AFRA entry size
	size += (_localAccessEntries.size() * _afraEntrySize);

	if (_globalEntries) {
		// add the global AFRA entry size
		size += (_globalAccessEntries.size() * _globalAfraEntrySize);
	}

	return size;
}
