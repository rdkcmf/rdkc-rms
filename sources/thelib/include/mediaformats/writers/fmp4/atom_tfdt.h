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


#ifndef _ATOM_TFDT_H
#define	_ATOM_TFDT_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_tfdt : public Atom {
private:
    uint32_t _sequenceNo;
    uint64_t _baseMediaDecodeTime;

private:
    bool WriteFields();
    uint64_t GetFieldsSize();

public:
    Atom_tfdt();
    virtual ~Atom_tfdt();

    bool UpdateSequenceNo();
    bool UpdateFragmentDecodeTime();

    inline void SetFragmentDecodeTime(uint64_t fragmentDecodeTime) {
        _baseMediaDecodeTime = fragmentDecodeTime;
    }
};

#endif	/* _ATOM_TFDT_H */
