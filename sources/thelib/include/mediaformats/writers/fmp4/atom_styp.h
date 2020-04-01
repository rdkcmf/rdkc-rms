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

#ifndef _ATOM_STYP_H
#define _ATOM_STYP_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_styp : public Atom {

public:
    /*
     * STYP atom/box (see 8.16.2 Segment Type Box of ISO_IEC_14496-12)
     */
	Atom_styp();
	virtual ~Atom_styp();

private:
    uint32_t _majorBrand;
    uint32_t _minorVersion;
    vector<uint32_t> _compatibleBrands;

    bool WriteFields();
    uint64_t GetFieldsSize();


};
#endif /* _ATOM_STYP_H */
