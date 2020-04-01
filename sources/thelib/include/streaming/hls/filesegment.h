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
#ifndef _FILESEGMENT_H
#define	_FILESEGMENT_H

#include "common.h"
#include "streaming/streamcapabilities.h"

class DLLEXP FileSegment {
private:
	File _file;
public:
	FileSegment();
	virtual ~FileSegment();
	string GetPath();
    inline uint64_t getCurrentFileOffset() {
        return _file.Cursor();
    }
    inline uint64_t getCurrentFileSize() {
        return _file.Size();
    }
	virtual bool FlushPacket() = 0;
	virtual bool CreateKey(unsigned char * pKey, unsigned char * pIV) = 0;
	virtual bool PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts) = 0;
	virtual bool PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts) = 0;
	virtual bool PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts, Variant& videoParamsSizes){ return false; }
	virtual bool PushMetaData(string const& vmfMetadata, int64_t pts) { return false; }
	virtual bool Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL) = 0;
protected:
	bool Initialize(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL);
	bool WritePacket(uint8_t *pBuffer, uint32_t size);
};
#endif	/* _FILESEGMENT_H */
#endif  /* HAS_PROTOCOL_HLS */
