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

#include "mediaformats/writers/fmp4/atom_isml.h"

uint8_t Atom_isml::_userType[] = {
	0xA5, 0xD4, 0x0B, 0x30, 
	0xE8, 0x14, 0x11, 0xDD,
	0xBA, 0x2F, 0x08, 0x00,
	0x20, 0x0C, 0x9A, 0x66
};

Atom_isml::Atom_isml() : Atom(0x75756964, false) {
    _reserved = 0;
}

Atom_isml::~Atom_isml() {

}

bool Atom_isml::WriteFields() {

	_pFile->WriteBuffer(_userType, sizeof(_userType));

    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
            _reserved);

    WriteString(_xmlString);

    return true;
}

uint64_t Atom_isml::GetFieldsSize() {

    uint64_t size = 0;

    size += sizeof(_userType);
    size += sizeof(_reserved);
    size += GetStringSize(_xmlString);

    return size;
}

void Atom_isml::SetXmlString(const string &xmlString) {
    _xmlString = xmlString;
}

