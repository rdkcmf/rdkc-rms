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


#ifdef HAS_MEDIA_MP4
#ifndef _ATOMTRAK_H
#define _ATOMTRAK_H

#include "mediaformats/readers/mp4/boxatom.h"

class AtomTKHD;
class AtomMDIA;
class AtomHDLR;
class AtomMINF;
class AtomDINF;
class AtomSTBL;
class AtomUDTA;
class AtomMETA;
class AtomEDTS;

class AtomTRAK
: public BoxAtom {
private:
	AtomTKHD *_pTKHD;
	AtomMDIA *_pMDIA;
	AtomHDLR *_pHDLR;
	AtomMINF *_pMINF;
	AtomDINF *_pDINF;
	AtomSTBL *_pSTBL;
	AtomUDTA *_pUDTA;
	AtomMETA *_pMETA;
	AtomEDTS *_pEDTS;
public:
	AtomTRAK(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomTRAK();
	uint32_t GetId();
protected:
	virtual bool AtomCreated(BaseAtom *pAtom);
};

#endif	/* _ATOMTRAK_H */


#endif /* HAS_MEDIA_MP4 */
