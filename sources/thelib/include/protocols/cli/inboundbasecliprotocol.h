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


#ifdef HAS_PROTOCOL_CLI
#ifndef _INBOUNDBASECLIPROTOCOL_H
#define	_INBOUNDBASECLIPROTOCOL_H

#include "protocols/baseprotocol.h"

class BaseCLIAppProtocolHandler;

class DLLEXP InboundBaseCLIProtocol
: public BaseProtocol {
private:
	BaseCLIAppProtocolHandler *_pProtocolHandler;
protected:
	IOBuffer _outputBuffer;
public:
	InboundBaseCLIProtocol(uint64_t type);
	virtual ~InboundBaseCLIProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SendMessage(Variant &message) = 0;
	virtual bool SendRaw(string str);	// output this string
protected:
	bool ProcessMessage(Variant &message);
};


#endif	/* _BASEINBOUNDCLIPROTOCOL_H */
#endif /* HAS_PROTOCOL_CLI */
