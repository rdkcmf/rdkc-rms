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

#include "mediaformats/writers/fmp4/atom_sidx.h"

Atom_sidx::Atom_sidx() : Atom(0x73696478, false) {
	_referenceId = 0;
	_timeScale = 0;
	_earliestPresentationTime = 0;
	_firstOffset = 0;
	_reserved = 0;
	_reference.clear();
	AddReference(0, 0, 0); // add a dummy value during init
}

Atom_sidx::~Atom_sidx() {

}

bool Atom_sidx::WriteFields() {

    // Write to buffer the fields of this atom
	//reference_ID
    _buffer.ReadFromRepeat(0, 8);
    EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
		_referenceId);
	//timescale
    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
		_timeScale);
	//earliest_presentation_time
	_buffer.ReadFromRepeat(0, 8);
	EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
		_earliestPresentationTime);
	//first_offset
	_buffer.ReadFromRepeat(0, 8);
	EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
		_firstOffset);
	//reserved
	_buffer.ReadFromRepeat(0, 2);
	EHTONSP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 2,
		_reserved);

	if (_reference.size() > 0) {
		//reference_count
		_buffer.ReadFromRepeat(0, 2);
		EHTONSP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 2,
			(uint16_t)_reference.size());

		for (size_t i = 0; i < _reference.size(); ++i) {
			//reference_type, reference_size
			_buffer.ReadFromRepeat(0, 4);
			EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_reference[i].referenceTypeSize);
			//subsegment_duration
			_buffer.ReadFromRepeat(0, 4);
			EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_reference[i].subsegmentDuration);
			//SAP flags
			_buffer.ReadFromRepeat(0, 4);
			EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
				_reference[i].sapFlags);
		}
	}

    return true;
}

uint64_t Atom_sidx::GetFieldsSize() {
    /*
     * Compute the size of the this atom.
     */
    uint64_t size = 0;

    size += 8; // reference_ID
    size += 4; // time_scale
	size += 8; // earliest_presentation_time
	size += 8; // first_offset
	size += 2; // reserved
	if (_reference.size() > 0) {
		size += 2; //reference_count
		//reference_type(1 bit), reference_size(31 bit), subsegment_duration(32 bit), 
		//starts_with_SAP(1 bit), SAP_type(3 bit), SAP_delta_time(28 bit)
		size += _reference.size() * 12;
	}

    return size;
}

bool Atom_sidx::UpdateTimeScale() {

    if (_isDirectFileWrite) {
		uint64_t referenceOffset = 0;
		if (_reference.size() > 0) {
			referenceOffset += 2; //reference_count
			referenceOffset += _reference.size() * 12; //reference
		}
		uint64_t offset = GetHeaderSize() + GetFieldsSize() - (22 + referenceOffset);
        if (!_pFile->SeekTo(_position + offset)) {
            FATAL("Could not seek to timescale position!");
            return false;
        }

        if (!_pFile->WriteUI32(_timeScale)) {
            FATAL("Could not write updated timescale!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}

bool Atom_sidx::UpdateEarliestPresentationTime() {

    if (_isDirectFileWrite) {
		uint64_t referenceOffset = 0;
		if (_reference.size() > 0) {
			referenceOffset += 2; //reference_count
			referenceOffset += _reference.size() * 12; //reference
		}
        uint64_t offset = GetHeaderSize() + GetFieldsSize() - (18 + referenceOffset);
        if (!_pFile->SeekTo(_position + offset)) {
            FATAL("Could not seek to earliest_presentation_time position!");
            return false;
        }

        if (!_pFile->WriteUI64(_earliestPresentationTime)) {
            FATAL("Could not write updated earliest_presentation_time!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}

bool Atom_sidx::UpdateFirstOffset() {

	if (_isDirectFileWrite) {
		uint64_t referenceOffset = 0;
		if (_reference.size() > 0) {
			referenceOffset += 2; //reference_count
			referenceOffset += _reference.size() * 12; //reference
		}
		uint64_t offset = GetHeaderSize() + GetFieldsSize() - (10 + referenceOffset);
		if (!_pFile->SeekTo(_position + offset)) {
			FATAL("Could not seek to first_offset position!");
			return false;
		}

		if (!_pFile->WriteUI64(_firstOffset)) {
			FATAL("Could not write updated first_offset!");
			return false;
		}

	}
	else {
		ASSERT("Buffer support needs to be implemented!");
	}

	return true;
}

bool Atom_sidx::UpdateReference() {

	if (_isDirectFileWrite) {
		uint64_t referenceOffset = 0;
		if (_reference.size() > 0) {
			referenceOffset += 2; //reference_count
			referenceOffset += _reference.size() * 12; //reference
		}
		uint64_t offset = GetHeaderSize() + GetFieldsSize() - referenceOffset;
		if (_reference.size() > 0) {
			//update the reference_count
			if (!_pFile->SeekTo(_position + offset)) {
				FATAL("Could not seek to reference_count position!");
				return false;
			}

			if (!_pFile->WriteUI16((uint16_t)_reference.size())) {
				FATAL("Could not write updated reference_count!");
				return false;
			}
			for (size_t i = 0; i < _reference.size(); ++i) {
				//update reference_type/reference_size
				if (!_pFile->SeekTo(_position + offset + 2)) {
					FATAL("Could not seek to reference_type position!");
					return false;
				}

				if (!_pFile->WriteUI32(_reference[i].referenceTypeSize)) {
					FATAL("Could not write updated reference_type, reference_size!");
					return false;
				}
				//update subsegment_duration
				if (!_pFile->SeekTo(_position + offset + 2 + 4)) {
					FATAL("Could not seek to subsegment_duration position!");
					return false;
				}

				if (!_pFile->WriteUI32(_reference[i].subsegmentDuration)) {
					FATAL("Could not write updated subsegment_duration!");
					return false;
				}
				//update SAP flags information
				if (!_pFile->SeekTo(_position + offset + 2 + 4 + 4)) {
					FATAL("Could not seek to SAP position!");
					return false;
				}

				if (!_pFile->WriteUI32(_reference[i].sapFlags)) {
					FATAL("Could not write updated starts_with_SAP, SAP_type, SAP_delta_time!");
					return false;
				}
			}
		}

	}
	else {
		ASSERT("Buffer support needs to be implemented!");
	}

	return true;
}
bool Atom_sidx::Update() {
	if (!UpdateTimeScale()) {
		FATAL("unable to update timescale");
		return false;
	}

	if (!UpdateEarliestPresentationTime()) {
		FATAL("unable to update timescale");
		return false;
	}

	if (!UpdateFirstOffset()) {
		FATAL("unable to update timescale");
		return false;
	}

	if (!UpdateReference()) {
		FATAL("unable to update timescale");
		return false;
	}

	return true;
}


void Atom_sidx::AddReference(uint32_t referenceTypeSize, uint32_t subsegmentDuration, uint32_t sapFlags) {
	SidxReference newReference = { referenceTypeSize, subsegmentDuration, sapFlags };
	_reference.push_back(newReference);
}

