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
#include "mediaformats/readers/mp4/atomilst.h"
#include "mediaformats/readers/mp4/mp4document.h"
#include "mediaformats/readers/mp4/atommetafield.h"

AtomILST::AtomILST(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_metadata.IsArray(false);
}

AtomILST::~AtomILST() {
}

Variant &AtomILST::GetMetadata() {
	return _metadata;
}

bool AtomILST::AtomCreated(BaseAtom *pAtom) {
	if ((pAtom->GetTypeNumeric() >> 24) == 0xa9) {
		AtomMetaField *pField = (AtomMetaField *) pAtom;
		_metadata[pField->GetName()] = pField->GetValue();
		return true;
	}
	switch (pAtom->GetTypeNumeric()) {
		case A_AART:
		case A_COVR:
		case A_CPIL:
		case A_DESC:
		case A_DISK:
		case A_GNRE:
		case A_PGAP:
		case A_TMPO:
		case A_TRKN:
		case A_TVEN:
		case A_TVES:
		case A_TVSH:
		case A_TVSN:
		case A_SONM:
		case A_SOAL:
		case A_SOAR:
		case A_SOAA:
		case A_SOCO:
		case A_SOSN:
		{
			AtomMetaField *pField = (AtomMetaField *) pAtom;
			_metadata[pField->GetName()] = pField->GetValue();
			return true;
		}
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}

#endif /* HAS_MEDIA_MP4 */
