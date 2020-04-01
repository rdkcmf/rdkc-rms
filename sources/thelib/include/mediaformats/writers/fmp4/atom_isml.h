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

#ifndef _ATOM_ISML_H
#define _ATOM_ISML_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_isml : public Atom {

public:
    /*
     * UUID atom/box
     */
    Atom_isml();
    virtual ~Atom_isml();
    void SetXmlString(const string &xmlString);

private:
    bool WriteFields();
    uint64_t GetFieldsSize();

private:
    string _xmlString;
    uint32_t _reserved;
    static uint8_t _userType[16];
};
#endif /* _ATOM_ISML_H */
