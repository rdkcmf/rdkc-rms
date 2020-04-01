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


#ifndef _MEDIAFRAME_H
#define	_MEDIAFRAME_H

#include "common.h"

#define MEDIAFRAME_TYPE_AUDIO 0
#define MEDIAFRAME_TYPE_VIDEO 1
#define MEDIAFRAME_TYPE_DATA 2

typedef struct _MediaFrame {
	uint64_t start;
	uint64_t length;
	uint8_t type;
	bool isKeyFrame;
	double pts;
	double dts;
	double cts;
	bool isBinaryHeader;

	operator string() {
		return format("s: %16"PRIx64"; l: %6"PRIx64"; t: %c; kf: %"PRIu8"; pts: %8.2f; dts: %8.2f; cts: %6.2f; bh: %"PRIu8,
				start, length,
				type == MEDIAFRAME_TYPE_AUDIO ? 'A' : (type == MEDIAFRAME_TYPE_VIDEO ? 'V' : (type == MEDIAFRAME_TYPE_DATA ? 'D' : 'U')),
				isKeyFrame, pts, dts, cts, isBinaryHeader);
	};
} MediaFrame;

#endif	/* _MEDIAFRAME_H */

