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
#include "mediaformats/readers/mp4/atomavc1.h"
#include "mediaformats/readers/mp4/mp4document.h"
#include "mediaformats/readers/mp4/atomavcc.h"

AtomAVC1::AtomAVC1(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_pAVCC = NULL;
}

AtomAVC1::~AtomAVC1() {
}

bool AtomAVC1::Read() {
	// See 'Sample Description Box' table in ISO/IEC 14496-12

	if (!SkipBytes(78)) {
		FATAL("Unable to skip 78 bytes");
		return false;
	}

	return BoxAtom::Read();
}

bool AtomAVC1::AtomCreated(BaseAtom *pAtom) {
	switch (pAtom->GetTypeNumeric()) {
			//TODO: What is the deal with all this order stuff!?
		case A_AVCC:
			_pAVCC = (AtomAVCC *) pAtom;
			return true;
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}

#endif /* HAS_MEDIA_MP4 */
