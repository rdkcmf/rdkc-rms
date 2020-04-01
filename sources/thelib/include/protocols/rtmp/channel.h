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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _CHANNEL_H
#define	_CHANNEL_H

#include "protocols/rtmp/header.h"

#define CS_HEADER 0
#define CS_PAYLOAD 1

typedef struct _Channel {
	uint32_t id;
	uint32_t state;
	IOBuffer inputData;

	Header lastInHeader;
	int8_t lastInHeaderType;
	uint32_t lastInProcBytes;
	double lastInAbsTs;
	uint32_t lastInStreamId;

	Header lastOutHeader;
	int8_t lastOutHeaderType;
	uint32_t lastOutProcBytes;
	double lastOutAbsTs;
	uint32_t lastOutStreamId;

	void Reset() {
		state = CS_HEADER;
		inputData.IgnoreAll();

		memset(&lastInHeader, 0, sizeof (lastInHeader));
		lastInHeaderType = 0;
		lastInProcBytes = 0;
		lastInAbsTs = 0;
		lastInStreamId = 0xffffffff;

		memset(&lastOutHeader, 0, sizeof (lastOutHeader));
		lastOutHeaderType = 0;
		lastOutProcBytes = 0;
		lastOutAbsTs = 0;
		lastOutStreamId = 0xffffffff;
	}
} Channel;


#endif	/* _CHANNEL_H */

#endif /* HAS_PROTOCOL_RTMP */

