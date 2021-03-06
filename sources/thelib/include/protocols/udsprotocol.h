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


#ifdef HAS_UDS
#ifndef _UDS_PROTOCOL_H
#define _UDS_PROTOCOL_H

#include "protocols/baseprotocol.h"

class IOHandler;

class DLLEXP UDSProtocol
: public BaseProtocol {
private:
	IOHandler *_pCarrier;
	IOBuffer _inputBuffer;
	uint64_t _decodedBytesCount;
public:
	UDSProtocol();
	virtual ~UDSProtocol();
	virtual bool Initialize(Variant &parameters);
	virtual IOHandler *GetIOHandler();
	virtual void SetIOHandler(IOHandler *pIOHandler);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetInputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool EnqueueForOutbound();
	virtual uint64_t GetDecodedBytesCount();
};

#endif	/* _UDS_PROTOCOL_H */
#endif	/* HAS_UDS */
