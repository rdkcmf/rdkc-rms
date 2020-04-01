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

#include "mediaformats/writers/fmp4/atom_tfdt.h"

Atom_tfdt::Atom_tfdt()
: Atom(0x74666474, false),
_sequenceNo(0x01000000),
_baseMediaDecodeTime(0) {
}

Atom_tfdt::~Atom_tfdt() {

}

bool Atom_tfdt::WriteFields() {

    // Write to buffer the fields of this atom
    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
            _sequenceNo);

    _buffer.ReadFromRepeat(0, 8);
    EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
            _baseMediaDecodeTime);

    return true;
}

uint64_t Atom_tfdt::GetFieldsSize() {
    /*
     * Compute the size of the this atom.
     */
    uint64_t size = 0;

    size += 4; // _sequenceNo
    size += 8; // _baseMediaDecodeTime

    return size;
}

bool Atom_tfdt::UpdateSequenceNo() {

    if (_isDirectFileWrite) {
        uint64_t offset = GetHeaderSize() + GetFieldsSize() - 12;
        if (!_pFile->SeekTo(_position + offset)) {
            FATAL("Could not seek to desired file position!");
            return false;
        }

        if (!_pFile->WriteUI32(_sequenceNo)) {
            FATAL("Could not write updated fragment time stamp!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}

bool Atom_tfdt::UpdateFragmentDecodeTime() {

    if (_isDirectFileWrite) {
        uint64_t offset = GetHeaderSize() + GetFieldsSize() - 8;
        if (!_pFile->SeekTo(_position + offset)) {
            FATAL("Could not seek to desired file position!");
            return false;
        }

        if (!_pFile->WriteUI64(_baseMediaDecodeTime)) {
            FATAL("Could not write updated fragment duration!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}
