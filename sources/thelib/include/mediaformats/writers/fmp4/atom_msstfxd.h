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

#ifndef _ATOM_MSSTFXD_H
#define _ATOM_MSSTFXD_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_MssTfxd : public Atom {

public:
    /*
     * Tfxd atom/box (see 2.2.4.4 of MS-SSTR.pdf
     * Smooth Streaming Protocol Specification)
     */
    Atom_MssTfxd();
    virtual ~Atom_MssTfxd();
    bool UpdateAbsoluteTimestamp();
    bool UpdateFragmentDuration();
    inline void SetAbsoluteTimestamp(uint64_t value) { _absoluteTimeStamp = value; }
    inline void SetFragmentDuration(uint64_t value) { _fragmentDuration = value; }
private:
    bool WriteFields();
    uint64_t GetFieldsSize();

private:
	static uint8_t _userType[16];
    uint64_t _absoluteTimeStamp;
    uint64_t _fragmentDuration;
};
#endif /* _ATOM_MSSTFXD_H */
