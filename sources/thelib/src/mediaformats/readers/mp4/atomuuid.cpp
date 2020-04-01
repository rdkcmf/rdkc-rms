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


#include "mediaformats/readers/mp4/atomuuid.h"

AtomUUID::AtomUUID(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {

}

AtomUUID::~AtomUUID() {
}

bool AtomUUID::Read() {
	uint8_t buffer[16];
	if (!ReadArray(buffer, 16)) {
		FATAL("Unable to read UUID");
		return false;
	}
	_metadata["uuid"] = format("%02"PRIx8"%02"PRIx8"%02"PRIx8"%02"PRIx8"-%02"PRIx8"%02"PRIx8"-%02"PRIx8"%02"PRIx8"-%02"PRIx8"%02"PRIx8"-%02"PRIx8"%02"PRIx8"%02"PRIx8"%02"PRIx8"%02"PRIx8"%02"PRIx8,
			buffer[0],
			buffer[1],
			buffer[2],
			buffer[3],
			buffer[4],
			buffer[5],
			buffer[6],
			buffer[7],
			buffer[8],
			buffer[9],
			buffer[10],
			buffer[11],
			buffer[12],
			buffer[13],
			buffer[14],
			buffer[15]
			);
	if (_metadata["uuid"] == "be7acfcb-97a9-42e8-9c71-999491e3afac") {
		//http://www.adobe.com/content/dam/Adobe/en/devnet/xmp/pdfs/XMPSpecificationPart3.pdf
		string payload;
		if (!ReadString(payload, _size - 16 - 8)) {
			FATAL("Unable to read XMP");
			return false;
		}
		_metadata["payload"] = payload;
	} else {
		if ((_size - 16 - 8) > 0) {
			uint32_t tempSize = (uint32_t) _size - 16 - 8;
			uint8_t *pTemp = new uint8_t[tempSize];
			if (!ReadArray(pTemp, tempSize)) {
				FATAL("Unable to read UUID raw content");
				delete[] pTemp;
				return false;
			}
			_metadata["payload"] = Variant(pTemp, tempSize);
			delete[] pTemp;
		} else {
			_metadata["payload"] = Variant();
		}
	}
	return true;
}

string AtomUUID::Hierarchy(uint32_t indent) {

	return string(4 * indent, ' ') + GetTypeString();
}

Variant &AtomUUID::GetMetadata() {
	return _metadata;
}
