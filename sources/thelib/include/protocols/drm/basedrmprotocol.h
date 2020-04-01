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


#ifdef HAS_PROTOCOL_DRM
#ifndef _BASEDRMPROTOCOL_H
#define	_BASEDRMPROTOCOL_H

#include "protocols/baseprotocol.h"

class BaseClientApplication;
class BaseDRMAppProtocolHandler;

class DLLEXP BaseDRMProtocol
: public BaseProtocol {
private:
	IOBuffer _outputBuffer;
	BaseDRMAppProtocolHandler *_pProtocolHandler;
protected:
	Variant _lastSent;
	Variant _lastReceived;
	Variant _headers;
public:
	BaseDRMProtocol(uint64_t type);
	virtual ~BaseDRMProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual IOBuffer *GetOutputBuffer();
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	bool Send(Variant &variant);
protected:
	virtual bool Serialize(string &rawData, Variant &variant) = 0;
	virtual bool Deserialize(uint8_t *pBuffer, uint32_t bufferLength,
			Variant &result) = 0;
};

#endif	/* _BASEDRMPROTOCOL_H */
#endif	/* HAS_PROTOCOL_DRM */
