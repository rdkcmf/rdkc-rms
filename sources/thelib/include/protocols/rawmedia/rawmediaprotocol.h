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

#ifndef _RAWMEDIAPROTOCOL_H
#define _RAWMEDIAPROTOCOL_H
#include "protocols/baseprotocol.h"
#include "streaming/basestream.h"

class InNetRawStream;

class DLLEXP RawMediaProtocol
: public BaseProtocol {
private:
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
	InNetRawStream *_pStream;
public:
	RawMediaProtocol();
	virtual ~RawMediaProtocol();

	virtual IOBuffer * GetOutputBuffer();
//	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
private:
	void CloseStream();
};
#endif /* _RAWMEDIAPROTOCOL_H */
