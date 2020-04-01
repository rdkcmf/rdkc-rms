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

#include "mediaformats/writers/fmp4/atom_ftyp.h"

Atom_ftyp::Atom_ftyp() : Atom(0x66747970, false) {
    _majorBrand = 0X69736D6C; //isml
    _minorVersion = 0X00000200; //512
    _compatibleBrands.push_back(0X70696666); //piff
    _compatibleBrands.push_back(0X69736F32); //iso2
}

Atom_ftyp::~Atom_ftyp() {

}

bool Atom_ftyp::WriteFields() {

    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
            _majorBrand);

    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
            _minorVersion);

    for (uint32_t i=0; i < _compatibleBrands.size(); i++) {
        _buffer.ReadFromRepeat(0, 4);
        EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
                _compatibleBrands[i]);
    }

    return true;
}

uint64_t Atom_ftyp::GetFieldsSize() {
    uint64_t size = 0;

    size += sizeof(_majorBrand);
    size += sizeof(_minorVersion);
    size += _compatibleBrands.size() * 4;

    return size;
}

