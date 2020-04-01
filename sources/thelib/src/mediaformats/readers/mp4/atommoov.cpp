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
#include "mediaformats/readers/mp4/atommoov.h"
#include "mediaformats/readers/mp4/mp4document.h"
#include "mediaformats/readers/mp4/atommvhd.h"

AtomMOOV::AtomMOOV(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_pMVEX = NULL;
	_pMVHD = NULL;
	_pUDTA = NULL;
	_pMETA = NULL;
}

AtomMOOV::~AtomMOOV() {
}

vector<AtomTRAK *> AtomMOOV::GetTracks() {
	return _tracks;
}

bool AtomMOOV::AtomCreated(BaseAtom *pAtom) {
	switch (pAtom->GetTypeNumeric()) {
		case A_MVEX:
			_pMVEX = (AtomMVEX *) pAtom;
			return true;
		case A_MVHD:
			_pMVHD = (AtomMVHD *) pAtom;
			return true;
		case A_UDTA:
			_pUDTA = (AtomUDTA *) pAtom;
			return true;
		case A_TRAK:
			ADD_VECTOR_END(_tracks, (AtomTRAK *) pAtom);
			return true;
		case A_META:
			_pMETA = (AtomMETA *) pAtom;
			return true;
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}


#endif /* HAS_MEDIA_MP4 */
