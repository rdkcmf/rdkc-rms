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


#ifdef HAS_PROTOCOL_RPC
#ifndef _BASERPCPROTOCOL_H
#define	_BASERPCPROTOCOL_H

#include "protocols/baseprotocol.h"

class BaseRPCSerializer;

class BaseRPCProtocol
: public BaseProtocol {
private:
	IOBuffer _outputBuffer;
protected:
	uint32_t _available;
	BaseRPCSerializer *_pInputSerializer;
	BaseRPCSerializer *_pOutputSerializer;
public:
	BaseRPCProtocol(uint64_t type);
	virtual ~BaseRPCProtocol();

	bool SerializeOutput(Variant &source);

	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual IOBuffer * GetOutputBuffer();

};

#endif	/* _BASERPCPROTOCOL_H */
#endif /* HAS_PROTOCOL_RPC */

