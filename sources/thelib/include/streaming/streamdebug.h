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


#ifdef HAS_STREAM_DEBUG
#ifndef _STREAMDEBUG_H
#define	_STREAMDEBUG_H

#include "common.h"

struct __attribute__((packed)) StreamDebugRTP {
	uint64_t serial;
	uint8_t headerLength;
	uint8_t rtpHeader[8];
	uint8_t firstPayloadByte;
};

struct __attribute__((packed)) StreamDebugRTCP {
	uint64_t serial;
	uint8_t length;
	uint8_t payload[255];
};

struct __attribute__((packed)) StreamDebugHLS {
	uint64_t serial;
	uint8_t isAudio;
	double pts;
	double dts;
	uint32_t timestamp;
};

class StreamDebugFile {
private:
	static uint64_t _streamDebugIdGenerator;
	File *_pFile;
	uint8_t *_pData;
	uint32_t _structSize;
	uint32_t _dataCount;
	uint32_t _dataCountCursor;
public:
	StreamDebugFile(string path, size_t structSize, uint32_t chunkSize = 128 * 1024);
	virtual ~StreamDebugFile();

	void PushRTP(uint8_t *pBuffer, uint32_t length);
	void PushRTCP(uint8_t *pBuffer, uint32_t length);
	void PushHLS(bool isAudio, double pts, double dts, uint32_t timestamp);
private:
	void Flush();
};

#endif	/* _STREAMDEBUG_H */
#endif /* HAS_STREAM_DEBUG */
