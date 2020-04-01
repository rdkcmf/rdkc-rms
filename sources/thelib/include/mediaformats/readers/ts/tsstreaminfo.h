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


#ifdef HAS_MEDIA_TS
#ifndef _TSSTREAMINFO_H
#define	_TSSTREAMINFO_H

#include "common.h"
#include "mediaformats/readers/ts/streamdescriptors.h"

struct TSStreamInfo {
	uint8_t streamType; //Table 2-29 : Stream type assignments
	uint16_t elementaryPID;
	uint16_t esInfoLength;
	vector<StreamDescriptor> esDescriptors;

	TSStreamInfo() {
		streamType = 0;
		elementaryPID = 0;
		esInfoLength = 0;
	}

	operator string() {
		return toString(0);
	}

	string toString(int32_t indent) {
		string result = format("%sstreamType: 0x%02"PRIx8"; elementaryPID: %hu; esInfoLength: %hu; descriptors count: %"PRIz"u\n",
				STR(string(indent, '\t')),
				streamType, elementaryPID, esInfoLength, esDescriptors.size());
		for (uint32_t i = 0; i < esDescriptors.size(); i++) {
			result += format("%s%s", STR(string(indent + 1, '\t')), STR(esDescriptors[i]));
			if (i != esDescriptors.size() - 1)
				result += "\n";
		}
		return result;
	}
};

#endif	/* _TSSTREAMINFO_H */
#endif	/* HAS_MEDIA_TS */
