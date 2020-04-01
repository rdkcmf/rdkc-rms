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
#ifndef _BASEAACSEGMENT_H
#define	_BASEAACSEGMENT_H

#include "streaming/hls/basesegment.h"

class DLLEXP BaseAACSegment : public BaseSegment {
public:
	BaseAACSegment(IOBuffer &audioBuffer, string const& drmType);
	virtual ~BaseAACSegment();

	virtual bool Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL) = 0;
	bool Init(StreamCapabilities *pCapabilities, unsigned char *pKey = NULL, unsigned char *pIV = NULL);
protected:
	virtual bool WritePacket(uint8_t *pBuffer, uint32_t size) = 0;
private:
	uint16_t AddStream(PID_TYPE pidType);
	bool WritePayload(uint8_t *pBuffer, uint32_t length, uint16_t pid, int64_t pts, int64_t dts);
};
#endif	/* _BASEAACSEGMENT_H */
#endif  /* HAS_PROTOCOL_HLS */
