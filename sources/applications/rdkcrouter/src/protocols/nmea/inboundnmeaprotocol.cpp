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
#include "protocols/nmea/inboundnmeaprotocol.h"
#include "streaming/basestream.h"
#include "application/clientapplicationmanager.h"
#include "application/originapplication.h"
#include "streaming/baseoutstream.h"

InboundNMEAProtocol::InboundNMEAProtocol()
: BaseProtocol(PT_INBOUND_NMEA) {

    _pProtocolHandler = NULL;
}

InboundNMEAProtocol::~InboundNMEAProtocol() {

}

bool InboundNMEAProtocol::Initialize(Variant &parameters) {
    GetCustomParameters() = parameters;

    return true;
}

bool InboundNMEAProtocol::Connect(string ip, uint16_t port, Variant customParameters) {
    WARN("Entered: InboundNMEAProtocol::Connect");
    vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			customParameters[CONF_PROTOCOL]);
    if (chain.size() == 0) {
	FATAL("Unable to obtain protocol chain from settings: %s",
			STR(customParameters[CONF_PROTOCOL]));
	return false;
    }
    if (!TCPConnector<InboundNMEAProtocol>::Connect(ip, port, chain,
			customParameters)) {
	FATAL("Unable to connect to %s:%hu", STR(ip), port);
	return false;
    }
    return true;
}

bool InboundNMEAProtocol::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant customParameters) {
	//1. Get the application  designated for the newly created connection
	if (customParameters[CONF_APPLICATION_NAME] != V_STRING) {
		FATAL("connect parameters must have an application name");
		return false;
	}
	BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
			customParameters[CONF_APPLICATION_NAME]);
	if (pApplication == NULL) {
		FATAL("Application %s not found", STR(customParameters[CONF_APPLICATION_NAME]));
		return false;
	}

	if (pProtocol == NULL) {
		FATAL("Connection failed:\n%s", STR(customParameters.ToString()));
		return pApplication->OutboundConnectionFailed(customParameters);
	}

	//2. Set the application
	pProtocol->SetApplication(pApplication);


	//3. Trigger processing, including handshake
	InboundNMEAProtocol *pInboundNMEAProtocol = (InboundNMEAProtocol *) pProtocol;
	pInboundNMEAProtocol->SetOutboundConnectParameters(customParameters);
	IOBuffer dummy;
	return pInboundNMEAProtocol->SignalInputData(dummy);
}

void InboundNMEAProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pProtocolHandler = (NMEAAppProtocolHandler *)
				pApplication->GetProtocolHandler(this);
	} else {
		_pProtocolHandler = NULL;
	}
}

bool InboundNMEAProtocol::AllowFarProtocol(uint64_t type) {
    return true;
}

bool InboundNMEAProtocol::AllowNearProtocol(uint64_t type) {
    FATAL("This protocol doesn't allow any near protocols");
    return false;
}

bool InboundNMEAProtocol::SignalInputData(int32_t recvAmount) {
    ASSERT("OPERATION NOT SUPPORTED");
    return false;
}

bool InboundNMEAProtocol::SignalInputData(IOBuffer &buffer, sockaddr_in *pPeerAddress) {
    return SignalInputData(buffer);
}

#define MAX_SENTENCE_LENGTH 80

bool InboundNMEAProtocol::SignalInputData(IOBuffer &buffer) {
    //1. Get the buffer and the length
    uint8_t *pBuffer = GETIBPOINTER(buffer);
    uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
    WARN("InboundNMEAProtocol::SignalInputData! Len: %d", length);
    if (length == 0)
        return true;

    //2. Walk through the buffer and parse out nmea sentences
    string sentence = "";
    int parseCount = 0;
    for (uint32_t i = 0; i < length; i++) {
        if ((pBuffer[i] == 0x0d) || (pBuffer[i] == 0x0a)) {
            if (sentence != "") {
                //INFO("%s", STR(sentence));

                if (ParseNMEASentence(sentence, _nmeaData)) {
                    ++parseCount;
                    //INFO("NMEA Parsed: \n %s", _nmeaData.ToString().c_str());
                }
            }
            sentence = "";
            buffer.Ignore(i);
            pBuffer = GETIBPOINTER(buffer);
            length = GETAVAILABLEBYTESCOUNT(buffer);
            i = 0;
            continue;
        }
        sentence += (char) pBuffer[i];
        if (sentence.length() >= MAX_SENTENCE_LENGTH) {
            FATAL("NMEA Sentence too long");
            return false;
        }
    }

    if (parseCount > 0) {
        INFO("NMEA Parsed: \n %s", _nmeaData.ToString().c_str());
        //app_rdkcrouter::OriginApplication * app = (app_rdkcrouter::OriginApplication *)this->GetApplication();
        //app->AddLocationMetadata( _nmeaData );
        FATAL("NEED TO IMPLEMENT SENDIGN THIS TO THE NEW VMF WRAPPER");
    }

    //3. Done
    return true;
}


BaseAppProtocolHandler *InboundNMEAProtocol::GetProtocolHandler() {
    return _pProtocolHandler;
}

bool InboundNMEAProtocol::ParseNMEASentence( const string & sentence, Variant & nmeaData ) {
    if( !( sentence[0] == '$' &&
           sentence[1] == 'G' &&
           sentence[2] == 'P' )) {
        WARN( "Malformed NMEA Sentence: %s", sentence.c_str() );
        return false;
    }
    
    if( sentence[3] == 'G' && sentence[4] == 'G' && sentence[5] == 'A' ) {
        return ParseGGA( sentence, nmeaData );
    }
    else if( sentence[3] == 'R' && sentence[4] == 'M' && sentence[5] == 'C' ) {
        return ParseRMC( sentence, nmeaData );
    }
    
    INFO("Not parsing %c%c%c", sentence[3],sentence[4],sentence[5]);
    return false;
}



bool InboundNMEAProtocol::ParseGGA( const string & sentence, Variant & nmeaData ) {
    // build the tools to parse the sentence
    int tokenIdx = 1;
    int cursor = 6;
    
    // Loop over the individual tokens in the sentence
    while( cursor < MAX_SENTENCE_LENGTH ) {
        // Get the next token from the sentence
        string token("");
        do {
            token += sentence[cursor];
            ++cursor;
        }
        while( sentence[cursor] != ',' );
        
        //Skip the comma
        ++cursor;
        
        // Now that we have a token, save it or toss it, depending on what it is.
        switch( tokenIdx++ ) {
            case 1: {
                nmeaData["fixTime"] = token;
                break;
            }
            case 2: {
                nmeaData["latitude"] = token;
                break;
            }
            case 3: {
                nmeaData["latDir"] = token;
                break;
            }
            case 4: {
                nmeaData["longitude"] = token;
                break;
            }
            case 5: {
                nmeaData["longDir"] = token;
                break;
            }
            case 6: {
                nmeaData["ggaFixQuality"] = token;
                break;
            }
            case 8: {
                nmeaData["altitude"] = token;
                break;
            }
            case 9: {
                //Once we get here return out so that we dont have to parse the rest of the sentence
                // because we dont care about the rest of the data
                return true;
            }
            default:
                break;
        }        
    }
    
    return nmeaData.MapSize() > 0;    
}

bool InboundNMEAProtocol::ParseRMC( const string & sentence, Variant & nmeaData ){
    // build the tools to parse the sentence
    int tokenIdx = 1;
    int cursor = 6;
    
    // Loop over the individual tokens in the sentence
    while( cursor < MAX_SENTENCE_LENGTH ) {
        // Get the next token from the sentence
        string token("");
        do {
            token += sentence[cursor];
            ++cursor;
        }
        while( sentence[cursor] != ',' );
        
        //Skip the comma
        ++cursor;
        
        // Now that we have a token, save it or toss it, depending on what it is.
        switch( tokenIdx++ ) {
            case 1: {
                nmeaData["fixTime"] = token;
                break;
            }
            case 2: {
                // V = Void, meaning the data is not valid, so we might as well just stop parsing
                if( token == "V" || token == "v" )
                    return false;
                break;
            }
            case 3: {
                nmeaData["latitude"] = token;
                break;
            }
            case 4: {
                nmeaData["latDir"] = token;
                break;
            }
            case 5: {
                nmeaData["longitude"] = token;
                break;
            }
            case 6: {
                nmeaData["longDir"] = token;
                break;
            }
            case 7: {
                nmeaData["speed"] = token;
                break;
            }
            case 8: {
                nmeaData["angDegDirection"] = token;
                break;
            }
            case 9: {
                //Once we get here return out so that we dont have to parse the rest of the sentence
                // because we dont care about the rest of the data
                return true;
            }
            default:
                break;
        }        
    }
    
    return nmeaData.MapSize() > 0;
}



#endif	/* defined HAS_PROTOCOL_NMEA */


