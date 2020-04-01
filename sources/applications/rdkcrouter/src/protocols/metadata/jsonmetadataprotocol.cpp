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

//#if defined HAS_PROTOCOL_JSONMETADATA

#include "protocols/metadata/jsonmetadataprotocol.h"
#include "protocols/baseprotocol.h"
#include "streaming/basestream.h"
#include "streaming/baseinstream.h"
#include "streaming/streamsmanager.h"
#include "streaming/streamstypes.h"
#include "application/clientapplicationmanager.h"
#include "application/originapplication.h"
#include "streaming/baseoutstream.h"
#include "streaming/hls/baseouthlsstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/messagefactories/genericmessagefactory.h"

JsonMetadataProtocol::JsonMetadataProtocol()
: BaseProtocol(PT_JSONMETADATA) {

    _pProtocolHandler = NULL;
    _pApp = NULL;
    _pSM = NULL;
}

JsonMetadataProtocol::~JsonMetadataProtocol() {

}

bool JsonMetadataProtocol::Initialize(Variant &parameters) {
	//FATAL("$b2$: JsonMetadataProtocol::Initialize, parms=%s", STR(parameters.ToString()));
    GetCustomParameters() = parameters;
    if (parameters.HasKey("streamname", false)) {
    	string temp = (string) parameters.GetValue("streamname", false);
    	_streamName = temp;
    	//INFO("inboundJsonMeta Protocol, streamname: %s", STR(_streamName));
    }else{
    	//WARN("inboundJsonMeta Protocol, no streamname!");
    }

    return true;
}

/// Connect - static method
///@param ip - ip address in uri (to connect to)
///@param port - port number to connect to
///@param customParameters - don't know what this is yet
//
bool JsonMetadataProtocol::Connect(string ip, uint16_t port, Variant customParameters) {
    WARN("Entered: JsonMetadataProtocol::Connect");
    vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			customParameters[CONF_PROTOCOL]);
    if (chain.size() == 0) {
	FATAL("Unable to obtain protocol chain from settings: %s",
			STR(customParameters[CONF_PROTOCOL]));
	return false;
    }
    if (!TCPConnector<JsonMetadataProtocol>::Connect(ip, port, chain,
			customParameters)) {
	FATAL("Unable to connect to %s:%hu", STR(ip), port);
	return false;
    }
    return true;
}

bool JsonMetadataProtocol::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant customParameters) {
	//INFO("$b2$: JsonMetadataProtocol::SignalProtocolCreated, parms are: %s", STR(customParameters.ToString()));
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
    JsonMetadataProtocol *pJsonMetadataProtocol = (JsonMetadataProtocol *) pProtocol;
    pJsonMetadataProtocol->SetOutboundConnectParameters(customParameters);
    //4 - get the default streamName (false == not case sensitive)
    if (customParameters.HasKey("streamname", false)) {
    	string temp = (string) customParameters.GetValue("streamname", false);
    	pJsonMetadataProtocol->_streamName = temp;
    	INFO("inboundJsonMeta Protocol, streamname: %s", STR(pJsonMetadataProtocol->_streamName));
    }else{
    	WARN("inboundJsonMeta Protocol, no streamname!");
    }
    //5 do this
    IOBuffer dummy;
    return pJsonMetadataProtocol->SignalInputData(dummy);
}

void JsonMetadataProtocol::SetApplication(BaseClientApplication *pApplication) {
    BaseProtocol::SetApplication(pApplication);
    _pApp = (app_rdkcrouter::OriginApplication *)pApplication;
    if (pApplication != NULL) {
        _pProtocolHandler = (JsonMetadataAppProtocolHandler *)
                            pApplication->GetProtocolHandler(this);
        _pSM = _pApp->GetStreamsManager();
    } else {
        _pProtocolHandler = NULL;
        _pSM = NULL;
    }
}

bool JsonMetadataProtocol::AllowFarProtocol(uint64_t type) {
    return true;
}

bool JsonMetadataProtocol::AllowNearProtocol(uint64_t type) {
    FATAL("This protocol doesn't allow any near protocols");
    return false;
}

bool JsonMetadataProtocol::SignalInputData(int32_t recvAmount) {
    ASSERT("OPERATION NOT SUPPORTED");
    return false;
}

bool JsonMetadataProtocol::SignalInputData(IOBuffer &buffer, sockaddr_in *pPeerAddress) {
    return SignalInputData(buffer);
}

/*!
 * SignalInputData receives data from below.
 * We parse out the JSon structure(s) from the stream, assuming it has clutter around it
 * but handling multiple and partial JSONs per buffer
 */
bool JsonMetadataProtocol::SignalInputData(IOBuffer &buffer) {
    // Get the buffer and the length
	uint8_t *pBuffer;
	uint32_t length;
	uint32_t i;	// i is used below the for loop to "discard" buffer bytes
	do {
		pBuffer = GETIBPOINTER(buffer);
		length = GETAVAILABLEBYTESCOUNT(buffer);
		//
		string jmdObj;
		int braceCount = 0;   // brace nesting counter, start looking for 1

		for (i = 0; i < length; i++) {
			char c = pBuffer[i];
			if (!c) continue;	// allow for 16bit chars, sort of
			if (!braceCount) {  // not even started grabbing the obect
				if (c != '{') { // loop back and ignore this
					continue;
				}
			}
			jmdObj += c;
			if (c == '{') {
				braceCount++;
			} else if (c == '}') {
				braceCount--;
			}
			if (!braceCount) { // then we've matched the first one
				break;         // sneak out the bottom of the loop
			}
		}

		if (!jmdObj.length()) {  // zero chars captured?
			WARN("JSONMETADATA: Got empty Object");
		}else if (braceCount) {  // didn't fully match braces?
			WARN("JSONMETADATA: incomplete object received, discarding");
		}else{
			// good object, enthrone it into the object
			//INFO("JSONMETADATA: Good JsonMetadata Received!! len: %d, streamname:>%s<", jmdObj.length(), STR(_streamName));
			//
			DistributeMetadata(jmdObj, _streamName);

			buffer.Ignore(i+1);	// i is an index, add 1 to get the count
		}
		// i counts off what we looked at - length is the buffer size
		// we may have hit length and not matched "{", so we exit hoping I/O will fetch the rest
	} while ((length - i) > 10);	// while we have a count of some significance

    return true;
}

/*!
 * DistributeMetadata via the named inStream's associated outStreams (plural)
 * - we form a message Variant with RM_INVOKE_FUNCTION
 * - and add the metadata as a string in PARAM 1
 */
void JsonMetadataProtocol::DistributeMetadata(string & mdStr, string & streamName) {
    //INFO("$b2$Enter JsonMetadataProtocol::DistributeMetadata, len:%d, streamName:>%s<", (uint32_t)mdStr.size(), STR(streamName));
    _pApp->GetMetadataManager()->SetMetadata(mdStr, streamName);		// save object in Manager
}


BaseAppProtocolHandler *JsonMetadataProtocol::GetProtocolHandler() {
    return (BaseAppProtocolHandler *)_pProtocolHandler;
}

/*************************************************
* old code which parses streamName
string temp;
for (uint32_t iii = 0; iii<length; iii++) {
	if (pBuffer[iii]) {
		temp += pBuffer[iii];
	}
}
WARN("JsonMetadataProtocol::SignalInputData! Len: %d, JSON: %s",
		length, STR(temp));


// grab the JSON Metadata Object from the incoming buffer
// $ToDo - handle Objects that cross calls to us, and multiple objects
//       - for now, barf if buffer doesn't hold a complete object
//       - and ignore more than one object
// jmd is JsonMetaData
//
// grab the header from the stream
// data:  { "streamname" : "TheName", "vmf" :
//state: 01              2 3       4 (4 is done)
string streamName = "";
int state = 0;
uint32_t i;
for (i = 0; (i < length) && (state < 4); i++) {
    char c = pBuffer[i];
    if (!c) continue;	// allow for 16bit chars, sort of
	switch (state) {
	case 0: // looking for first brace
		if (c == '{') state++;  //
		break;
	case 1: // looking for the colon
		if (c == ':') state++;  //
		break;
	case 2: // looking for opening quote
		if (c == '"') state++;  //
		break;
	case 3: // stashing chars looking for closing quote
		if (c == '"') state++;  //
		else streamName += c;
		break;
	}
}
//INFO("Parsed stream name: %s", STR(streamName));
// so we have streamName (or we don't and i > length)
//string jmdObj ="{ \"vmf\" : ";    // use strings like the old code
******************************************/

// #endif	/* defined HAS_PROTOCOL_JSONMETADATA */


