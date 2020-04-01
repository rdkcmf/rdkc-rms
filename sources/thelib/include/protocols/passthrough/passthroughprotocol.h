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


#ifndef _PASSTHROUGHPROTOCOL_H
#define	_PASSTHROUGHPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "streaming/basestream.h"

class InNetPassThroughStream;
class OutNetPassThroughStream;
class SocketAddress;

class DLLEXP PassThroughProtocol
: public BaseProtocol {
private:
	InNetPassThroughStream *_pInStream;
	OutNetPassThroughStream *_pOutStream;
	BaseStream *_pDummyStream;
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
public:
	PassThroughProtocol();
	virtual ~PassThroughProtocol();

	virtual IOBuffer * GetOutputBuffer();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	void SetDummyStream(BaseStream *pDummyStream);
	virtual bool Initialize(Variant &parameters);
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	void SetOutStream(OutNetPassThroughStream *pOutStream);
	bool RegisterOutboundUDP(Variant streamConfig);
	bool SendTCPData(string &data);
	bool SendTCPData(const uint8_t *pBuffer, const uint32_t size);
private:
	void CloseStream();
};
#endif	/* _PASSTHROUGHPROTOCOL_H */
