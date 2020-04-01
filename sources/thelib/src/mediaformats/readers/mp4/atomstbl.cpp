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
#include "mediaformats/readers/mp4/atomstbl.h"
#include "mediaformats/readers/mp4/mp4document.h"

AtomSTBL::AtomSTBL(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_pSTSD = NULL;
	_pSTTS = NULL;
	_pSTSC = NULL;
	_pSTSZ = NULL;
	_pSTCO = NULL;
	_pCTTS = NULL;
	_pSTSS = NULL;
}

AtomSTBL::~AtomSTBL() {
}

bool AtomSTBL::AtomCreated(BaseAtom *pAtom) {
	switch (pAtom->GetTypeNumeric()) {
		case A_STSD:
			_pSTSD = (AtomSTSD *) pAtom;
			return true;
		case A_STTS:
			_pSTTS = (AtomSTTS *) pAtom;
			return true;
		case A_STSC:
			_pSTSC = (AtomSTSC *) pAtom;
			return true;
		case A_STSZ:
			_pSTSZ = (AtomSTSZ *) pAtom;
			return true;
		case A_STCO:
			_pSTCO = (AtomSTCO *) pAtom;
			return true;
		case A_CO64:
			_pCO64 = (AtomCO64 *) pAtom;
			return true;
		case A_CTTS:
			_pCTTS = (AtomCTTS *) pAtom;
			return true;
		case A_STSS:
			_pSTSS = (AtomSTSS *) pAtom;
			return true;
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}

#endif /* HAS_MEDIA_MP4 */
