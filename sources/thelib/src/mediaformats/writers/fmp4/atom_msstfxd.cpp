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

#include "mediaformats/writers/fmp4/atom_msstfxd.h"

uint8_t Atom_MssTfxd::_userType[] = {
	0x6d, 0x1d, 0x9b, 0x05,
	0x42, 0xd5, 0x44, 0xe6,
	0x80, 0xe2, 0x14, 0x1d,
	0xaf, 0xf7, 0x57, 0xb2
};

Atom_MssTfxd::Atom_MssTfxd() : Atom(0x75756964, false) {
    _version = 1;
    _absoluteTimeStamp = 0;
    _fragmentDuration = 0;
}

Atom_MssTfxd::~Atom_MssTfxd() {

}

bool Atom_MssTfxd::WriteFields() {

	_pFile->WriteBuffer(_userType, sizeof(_userType));

    _buffer.ReadFromRepeat(0, 1);
    *(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 1) = _version;

    _buffer.ReadFromRepeat(0, 3);
    *(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 3) = 0x000000;

    _buffer.ReadFromRepeat(0, 8);
    EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, //Fragment absolute time stamp
            _absoluteTimeStamp);

    _buffer.ReadFromRepeat(0, 8);
    EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, //Fragment duration
            _fragmentDuration);

    return true;
}

uint64_t Atom_MssTfxd::GetFieldsSize() {
    uint64_t size = 0;
    size += sizeof(_userType);
    size += 4; //version and flags
    size += sizeof(_absoluteTimeStamp);
    size += sizeof(_fragmentDuration);
    return size;
}

bool Atom_MssTfxd::UpdateAbsoluteTimestamp() {

    if (_isDirectFileWrite) {
        uint64_t timeStampPos = _position + GetHeaderSize() + 20;

        if (!_pFile->SeekTo(timeStampPos)) {
            FATAL("Could not seek to desired file position!");
            return false;
        }

        if (!_pFile->WriteUI64(_absoluteTimeStamp)) {
            FATAL("Could not write updated fragment time stamp!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}

bool Atom_MssTfxd::UpdateFragmentDuration() {

    if (_isDirectFileWrite) {
        uint64_t durationPos = _position + GetHeaderSize() + 28;

        if (!_pFile->SeekTo(durationPos)) {
            FATAL("Could not seek to desired file position!");
            return false;
        }

        if (!_pFile->WriteUI64(_fragmentDuration)) {
            FATAL("Could not write updated fragment duration!");
            return false;
        }

    } else {
        ASSERT("Buffer support needs to be implemented!");
    }
    
    return true;
}

