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

// #if defined HAS_PROTOCOL_JSONMETADATA

#ifndef _JSONMETADATAPROTOCOL_H
#define	_JSONMETADATAPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/metadata/jsonmetadataappprotocolhandler.h"

//class BaseAppProtocolHandler;

/**
 * JsonMetadataProtocol implements the transport of Metadata represented in JSON
 * It attempts to represent both in and outbound cases.
 * The primary processing is to ensure a full JSON object is transported.
 * $ToDo: add params to dictate if data is to be parsed into a variant
 */

class OrignApplication;
class StreamsManager;

class DLLEXP JsonMetadataProtocol
: public BaseProtocol {
private:
	JsonMetadataAppProtocolHandler *_pProtocolHandler;
        
public:
	JsonMetadataProtocol();
	virtual ~JsonMetadataProtocol();

	virtual bool Initialize(Variant &parameters);
        
        static bool Connect(string ip, uint16_t port, Variant customParameters);
        static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters);
        
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer, sockaddr_in *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer);

	BaseAppProtocolHandler *GetProtocolHandler();
protected:
	app_rdkcrouter::OriginApplication * _pApp;
	StreamsManager * _pSM;
	string _streamName;	// assigned streamName for this acceptor

	virtual void DistributeMetadata(string & mdStr, string & streamName);
private:
};


#endif	/* _JSONMETADATAPROTOCOL_H */

// #endif	/* defined HAS_PROTOCOL_JSONMETADATA */

