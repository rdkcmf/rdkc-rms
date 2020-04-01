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

#ifdef HAS_PROTOCOL_WS
#ifndef _BASEWSPROTOCOL_H
#define	_BASEWSPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "utils/buffering/iobufferext.h"

#define WS_HEADERS_COMMON ""\
HTTP_HEADERS_SERVER": "HTTP_HEADERS_SERVER_US"\r\n"\
"Content-Type: application/octet-stream\r\n"\
"Access-Control-Allow-Origin: *\r\n"

class BaseWSProtocol
: public BaseProtocol {
protected:
	bool _wsHandshakeComplete;
	IOBufferExt _outputBuffer;
	IOBuffer _tempBuffer;
	IOBufferExt _metadataBuffer;
	bool _isText;
	bool _outIsBinary;
public:
	BaseWSProtocol(uint64_t type);
	virtual ~BaseWSProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SendOutOfBandData(const IOBuffer &buffer, void *pUserData);
protected:
	virtual bool ProcessWSHandshake(IOBuffer &buffer) = 0;
	virtual bool SignalWSInputData(IOBuffer &buffer);
	virtual bool SendBuffer(const IOBuffer &buffer, bool mask);
};

#endif	/* _BASEWSPROTOCOL_H */
#endif /* HAS_PROTOCOL_WS */
