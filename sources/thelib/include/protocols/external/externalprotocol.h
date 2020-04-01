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


#ifdef HAS_PROTOCOL_EXTERNAL
#ifndef _EXTERNALPROTOCOL_H
#define	_EXTERNALPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/external/rmsprotocol.h"

class InNetExternalStream;
class OutNetExternalStream;

class ExternalProtocol
: public BaseProtocol {
private:
	connection_t _connectionInterface;
	IOBuffer _outputBuffer;
	map<uint32_t, InNetExternalStream *> _inStreams;
	map<uint32_t, OutNetExternalStream *> _outStreams;
public:
	ExternalProtocol(uint64_t type, protocol_factory_t *pFactory);
	virtual ~ExternalProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual IOBuffer * GetOutputBuffer();
	virtual void ReadyForSend();
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);

	connection_t *GetInterface();
	bool SendData(const void *pData, uint32_t length);
	in_stream_t *CreateInStream(string name, void *pUserData);
	void CloseInStream(in_stream_t *pInStream);
	out_stream_t *CreateOutStream(string inStreamName, void *pUserData);
	void CloseOutStream(out_stream_t *pOutStream);
};
#endif	/* _EXTERNALPROTOCOL_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
