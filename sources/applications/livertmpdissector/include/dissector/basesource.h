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


#ifndef _BASESOURCE_H
#define	_BASESOURCE_H

#include "common.h"

enum Direction {
	DIRECTION_UNKNOWN = 0,
	DIRECTION_CLIENT_TO_SERVER,
	DIRECTION_SERVER_TO_CLIENT
};

struct DataBlock {
	uint8_t *pPacketPayload;
	uint32_t packetPayloadLength;
	bool eof;
	timeval timestamp;
	Direction direction;

	DataBlock() {
		Reset();
	}

	void Reset() {
		pPacketPayload = NULL;
		packetPayloadLength = 0;
		eof = false;
		memset(&timestamp, 0, sizeof (timestamp));
		direction = DIRECTION_UNKNOWN;
	}

	operator string() {
		string result = format("%.6f - ", (double) timestamp.tv_sec + (double) timestamp.tv_usec / 1000000.0);
		switch (direction) {
			case DIRECTION_CLIENT_TO_SERVER:
			{
				result += "C->S: ";
				break;
			}
			case DIRECTION_SERVER_TO_CLIENT:
			{
				result += "S->C: ";
				break;
			}
			default:
			{
				result += "?->?: ";
				break;
			}
		}
		result += format("%8"PRIu32" bytes", packetPayloadLength);
		return result;
	}
};

class BaseSource {
public:
	BaseSource();
	virtual ~BaseSource();
	virtual bool Init(Variant &parameters) = 0;
	virtual bool GetData(DataBlock &destiantion) = 0;
};


#endif	/* _BASESOURCE_H */
