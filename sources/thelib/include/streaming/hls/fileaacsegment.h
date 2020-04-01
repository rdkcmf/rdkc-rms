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


#ifdef HAS_PROTOCOL_HLS
#ifndef _FILEAACSEGMENT_H
#define	_FILEAACSEGMENT_H

#include "common.h"
#include "streaming/hls/filesegment.h"
#include "streaming/hls/baseaacsegment.h"

class DLLEXP FileAACSegment
	: public BaseAACSegment, public FileSegment {
public:
	FileAACSegment(IOBuffer &audioBuffer, string const& drmType);
	virtual ~FileAACSegment();
	bool WritePacket(uint8_t *pBuffer, uint32_t size);
	bool FlushPacket();
	bool CreateKey(unsigned char * pKey, unsigned char * pIV);
	bool PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts);
	bool PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts);
	bool Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL);
};
#endif	/* _FILEAACSEGMENT_H */
#endif  /* HAS_PROTOCOL_HLS */
