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
#ifndef _RTMPEPROTOCOL_H
#define	_RTMPEPROTOCOL_H

#include "protocols/baseprotocol.h"

class DLLEXP RTMPEProtocol
: public BaseProtocol {
private:
	IOBuffer _outputBuffer;
	IOBuffer _inputBuffer;
	RC4_KEY *_pKeyIn;
	RC4_KEY *_pKeyOut;
	uint32_t _skipBytes;
public:
	RTMPEProtocol(RC4_KEY *pKeyIn, RC4_KEY *pKeyOut, uint32_t skipBytes = 0);
	virtual ~RTMPEProtocol();

	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetInputBuffer();
	virtual IOBuffer * GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool EnqueueForOutbound();

};


#endif	/* _RTMPEPROTOCOL_H */

#endif /* HAS_PROTOCOL_RTMP */

