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

#ifndef __WSMETADATAPROTOCOL_H__
#define __WSMETADATAPROTOCOL_H__

#include "protocols/baseprotocol.h"
#include "protocols/ws/wsinterface.h"

class OutMetadataStream;
class MetadataManager;

class WsMetadataProtocol
: public BaseProtocol, WSInterface {

public:
	WsMetadataProtocol();
	virtual ~WsMetadataProtocol();

	//BaseProtocol
	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SendOutOfBandData(const IOBuffer &buffer, void *pUserData);

	//WSInterface
	virtual WSInterface* GetWSInterface();
	virtual bool SignalInboundConnection(Variant &request);
	virtual bool SignalTextData(const uint8_t *pBuffer, size_t length);
	virtual bool SignalBinaryData(const uint8_t *pBuffer, size_t length);
	virtual bool SignalPing(const uint8_t *pBuffer, size_t length);
	virtual void SignalConnectionClose(uint16_t code, const uint8_t *pReason, size_t length);
	virtual bool DemaskBuffer();
	virtual bool IsOutBinary() { return false;};	// Metadata is Text

private:
	OutMetadataStream * _pStream;
	MetadataManager * _pManager;
	string _streamName;	// configured optional streamname
	string _streamReq;	// requested stream name, we send this to metadata manager
};

#endif // __WSMETADATAPROTOCOL_H__
#endif // HAS_PROTOCOL_WS

