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
#include "mediaformats/readers/mp4/atommetafield.h"
#include "mediaformats/readers/mp4/mp4document.h"
#include "mediaformats/readers/mp4/atomdata.h"

AtomMetaField::AtomMetaField(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {
}

AtomMetaField::~AtomMetaField() {
}

string &AtomMetaField::GetName() {
	return _name;
}

Variant &AtomMetaField::GetValue() {
	return _value;
}

bool AtomMetaField::Read() {
	if (GetSize() - 8 >= 8) {
		if (!GetDoc()->GetMediaFile().SeekAhead(4)) {
			FATAL("Unable to seek 4 bytes");
			return false;
		}
		uint32_t type;
		if (!ReadUInt32(type)) {
			FATAL("Unable to read type");
			return false;
		}
		if (type == A_DATA) {
			if (!GetDoc()->GetMediaFile().SeekBehind(8)) {
				FATAL("Unable to go back 8 bytes");
				return false;
			}
			AtomDATA *pAtom = (AtomDATA *) GetDoc()->ReadAtom(NULL);
			if (pAtom == NULL) {
				FATAL("Unable to read data atom");
				return false;
			}
			if ((GetTypeNumeric() >> 24) == 0xa9) {
				_name = GetTypeString().substr(1, 3);
			} else {
				_name = GetTypeString();
			}
			_value = pAtom->GetVariant();
			return GetDoc()->GetMediaFile().SeekTo(
					GetStart() + GetSize());
		} else {
			if (!GetDoc()->GetMediaFile().SeekBehind(8)) {
				FATAL("Unable to seek 8 bytes");
				return false;
			}
			return ReadSimpleString();
		}
	} else {
		return ReadSimpleString();
	}
}

string AtomMetaField::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}

bool AtomMetaField::ReadSimpleString() {
	if ((GetTypeNumeric() >> 24) == 0xa9) {
		//1. Read the size
		uint16_t size;
		if (!ReadUInt16(size)) {
			FATAL("Unable to read the size");
			return false;
		}

		//2. Skip the locale
		if (!SkipBytes(2)) {
			FATAL("Unable to skip 2 bytes");
			return false;
		}

		//3. Read the value
		string val;
		if (!ReadString(val, size)) {
			FATAL("Unable to read string");
			return false;
		}

		//4. Setup the name/value
		_name = GetTypeString().substr(1, 3);
		_value = val;

		//5. Done
		return true;
	} else {
		//3. Read the value
		string val;
		if (!ReadString(val, GetSize() - 8)) {
			FATAL("Unable to read string");
			return false;
		}

		//4. Setup the name/value
		_name = GetTypeString();
		_value = val;

		//5. Done
		return true;
	}
}

#endif /* HAS_MEDIA_MP4 */
