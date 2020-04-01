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
#include "streaming/hls/filetssegment.h"
#include "streaming/nalutypes.h"

FileTSSegment::FileTSSegment(IOBuffer &videoBuffer, IOBuffer &audioBuffer,
	IOBuffer &patPmtAndCounters, string const& drmType)
	: BaseTSSegment(videoBuffer, audioBuffer, patPmtAndCounters, drmType) {
}

FileTSSegment::~FileTSSegment() {
	if (_file.IsOpen()) {
		_file.Close();
	}
}

bool FileTSSegment::Init(Variant &settings, StreamCapabilities *pCapabilities,
	unsigned char *pKey, unsigned char *pIV) {
		if (!FileSegment::Initialize(settings, pCapabilities, pKey, pIV))
			return false;
        settings["segmentStartOffset"] = FileSegment::getCurrentFileOffset();
		return BaseTSSegment::Init(pCapabilities, pKey, pIV);
}

bool FileTSSegment::FlushPacket() {
	return BaseTSSegment::FlushPacket();
}

bool FileTSSegment::CreateKey(unsigned char * pKey, unsigned char * pIV) {
	return BaseTSSegment::CreateKey(pKey, pIV);
}

bool FileTSSegment::PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts) {
	return BaseTSSegment::PushAudioData(buffer, pts, dts);
}

bool FileTSSegment::PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts, Variant& videoParamsSizes) {
	return BaseTSSegment::PushVideoData(buffer, pts, dts, videoParamsSizes);
}

bool FileTSSegment::PushMetaData(string const& vmfMetadata, int64_t pts) {
	return BaseTSSegment::PushMetaData(vmfMetadata, pts);
}

bool FileTSSegment::WritePacket(uint8_t *pBuffer, uint32_t size) {
	return FileSegment::WritePacket(pBuffer, size);
}


#endif /* HAS_PROTOCOL_HLS */
