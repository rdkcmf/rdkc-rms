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
#include "mediaformats/readers/mp4/atommdia.h"
#include "mediaformats/readers/mp4/mp4document.h"

AtomMDIA::AtomMDIA(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_pMDHD = NULL;
	_pHDLR = NULL;
	_pMINF = NULL;
	_pDINF = NULL;
	_pSTBL = NULL;
}

AtomMDIA::~AtomMDIA() {
}

bool AtomMDIA::AtomCreated(BaseAtom *pAtom) {
	switch (pAtom->GetTypeNumeric()) {
		case A_MDHD:
			_pMDHD = (AtomMDHD *) pAtom;
			return true;
		case A_HDLR:
			_pHDLR = (AtomHDLR *) pAtom;
			return true;
		case A_MINF:
			_pMINF = (AtomMINF *) pAtom;
			return true;
		case A_DINF:
			_pDINF = (AtomDINF *) pAtom;
			return true;
		case A_STBL:
			_pSTBL = (AtomSTBL *) pAtom;
			return true;
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}


#endif /* HAS_MEDIA_MP4 */
