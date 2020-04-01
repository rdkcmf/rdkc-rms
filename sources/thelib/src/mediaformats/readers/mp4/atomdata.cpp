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
#include "mediaformats/readers/mp4/atomdata.h"

AtomDATA::AtomDATA(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {

}

AtomDATA::~AtomDATA() {
}

Variant AtomDATA::GetVariant() {
	switch (_type) {
		case 1:
		{
			//Single string
			return _dataString;
		}
		case 0:
		{
			//many uint16_t
			Variant result;

			FOR_VECTOR(_dataUI16, i) {
				result[i] = _dataUI16[i];
			}
			result.IsArray(true);
			return result;
		}
		case 21:
		{
			//many uint8_t
			Variant result;

			FOR_VECTOR(_dataUI8, i) {
				result[i] = _dataUI8[i];
			}
			result.IsArray(true);
			return result;
		}
		case 13: //JPEG
		case 14: //PNG
		case 15: //? (looks like this is an image as well... Don't know the type)
		case 27: //BMP
		{
			Variant result = _dataImg;
			result.IsByteArray(true);
			return result;
		}
		default:
		{
			FATAL("Type %"PRIu32" not yet implemented", _type);
			return false;
		}
	}
}

bool AtomDATA::Read() {
	//1. Read the type
	if (!ReadUInt32(_type)) {
		FATAL("Unable to read type");
		return false;
	}

	//2. Read unknown 4 bytes
	if (!ReadUInt32(_unknown)) {
		FATAL("Unable to read type");
		return false;
	}

	switch (_type) {
		case 1:
		{
			//Single string
			if (!ReadString(_dataString, GetSize() - 8 - 8)) {
				FATAL("Unable to read string");
				return false;
			}
			return true;
		}
		case 0:
		{
			//many uint16_t
			uint64_t count = (GetSize() - 8 - 8) / 2;
			for (uint64_t i = 0; i < count; i++) {
				uint16_t val;
				if (!ReadUInt16(val)) {
					FATAL("Unable to read value");
					return false;
				}
				ADD_VECTOR_END(_dataUI16, val);
			}
			return true;
		}
		case 21:
		{
			//many uint8_t
			uint64_t count = GetSize() - 8 - 8;
			for (uint64_t i = 0; i < count; i++) {
				uint8_t val;
				if (!ReadUInt8(val)) {
					FATAL("Unable to read value");
					return false;
				}
				ADD_VECTOR_END(_dataUI8, val);
			}
			return true;
		}
		case 13:
		case 14:
		case 15:
		{
			if (!ReadString(_dataImg, GetSize() - 8 - 8)) {
				FATAL("Unable to read data");
				return false;
			}
			return true;
		}
		default:
		{
			FATAL("Type %u not yet implemented", _type);
			return false;
		}
	}
}

string AtomDATA::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}


#endif /* HAS_MEDIA_MP4 */
