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
#include "mediaformats/readers/mp4/atomudta.h"
#include "mediaformats/readers/mp4/mp4document.h"
#include "mediaformats/readers/mp4/atommetafield.h"

AtomUDTA::AtomUDTA(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_metadata.IsArray(false);
}

AtomUDTA::~AtomUDTA() {
}

Variant &AtomUDTA::GetMetadata() {
	return _metadata;
}

bool AtomUDTA::AtomCreated(BaseAtom *pAtom) {
	if ((pAtom->GetTypeNumeric() >> 24) == 0xa9) {
		AtomMetaField *pField = (AtomMetaField *) pAtom;
		_metadata[pField->GetName()] = pField->GetValue();
		return true;
	}
	switch (pAtom->GetTypeNumeric()) {
		case A_META:
			return true;
		case A_NAME:
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
