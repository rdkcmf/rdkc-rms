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


#if defined HAS_PROTOCOL_NMEA
#ifndef _INBOUNDNMEAPROTOCOL_H
#define	_INBOUNDNMEAPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/nmea/nmeaappprotocolhandler.h"

class TSParser;
class BaseAppProtocolHandler;

class DLLEXP InboundNMEAProtocol
: public BaseProtocol {
private:
	NMEAAppProtocolHandler *_pProtocolHandler;
        Variant _nmeaData;
        
public:
	InboundNMEAProtocol();
	virtual ~InboundNMEAProtocol();

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
	
private:
    bool ParseNMEASentence( const string & sentence, Variant & nmeaData );
    bool ParseGGA( const string & sentence, Variant & nmeaData );
    bool ParseRMC( const string & sentence, Variant & nmeaData );
};


#endif	/* _INBOUNDNMEAPROTOCOL_H */
#endif	/* defined HAS_PROTOCOL_NMEA */

