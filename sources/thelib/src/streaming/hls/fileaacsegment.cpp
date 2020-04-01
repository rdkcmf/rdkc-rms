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
#include "streaming/hls/fileaacsegment.h"
#include "streaming/nalutypes.h"

FileAACSegment::FileAACSegment(IOBuffer &audioBuffer, string const& drmType)
	: BaseAACSegment(audioBuffer, drmType) {
}

FileAACSegment::~FileAACSegment() {
}

bool FileAACSegment::Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey, unsigned char *pIV) {
	if (!FileSegment::Initialize(settings, pCapabilities, pKey, pIV))
		return false;
	return BaseAACSegment::Init(pCapabilities, pKey, pIV);
}

bool FileAACSegment::FlushPacket() {
	return BaseAACSegment::FlushPacket();
}

bool FileAACSegment::CreateKey(unsigned char * pKey, unsigned char * pIV) {
	return BaseAACSegment::CreateKey(pKey, pIV);
}

bool FileAACSegment::PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts) {
	return BaseAACSegment::PushAudioData(buffer, pts, dts);
}

bool FileAACSegment::PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts) {
	return true;
}

bool FileAACSegment::WritePacket(uint8_t *pBuffer, uint32_t size) {
	return FileSegment::WritePacket(pBuffer, size);
}


#endif /* HAS_PROTOCOL_HLS */
