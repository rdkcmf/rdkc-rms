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

// PT_WS_METADATA
// ST_OUT_META
// CONF_PROTOCOL_INBOUND_WS_JSON_META

#include "protocols/ws/wsmetadataprotocol.h"
#include "streaming/metadata/outmetadatastream.h"
#include "streaming/streamstypes.h"
#include "application/baseclientapplication.h"
//#include "application/originapplication.h"
//#include "../../../../applications/rdkcrouter/include/application/originapplication.h"

WsMetadataProtocol::WsMetadataProtocol()
: BaseProtocol(PT_WS_METADATA) {
	_pStream = NULL;
	_pManager = NULL;
}

WsMetadataProtocol::~WsMetadataProtocol() {
	// should I do this in some disconnect override?
	if (_pManager) {
		_pManager->RemovePushStream(_streamReq, _pStream);
	}
	// do I just delete the stream??
	if (_pStream) {
		_pStream->EnqueueForDelete();
		_pStream = NULL;
	}
}

//
// baseprotocol overrides
//

bool WsMetadataProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	//
    if (parameters.HasKey("streamname", false)) {
    	_streamName = (string) parameters.GetValue("streamname", false);
	}

	return true;
}

bool WsMetadataProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_INBOUND_WS;
}

bool WsMetadataProtocol::AllowNearProtocol(uint64_t type) {
	FATAL("This is an endpoint protocol");
	return false;
}

bool WsMetadataProtocol::SignalInputData(int32_t recvAmount) {
	FATAL("Operation not supported");
	return false;
}

bool WsMetadataProtocol::SignalInputData(IOBuffer &buffer) {
	FATAL("Operation not supported");
	return false;
}

bool WsMetadataProtocol::SendOutOfBandData(const IOBuffer &buffer, void *pUserData) {
	//DEBUG("$b2: WsMetadata Sending packet len: %d", GETAVAILABLEBYTESCOUNT(buffer));
	if (_pFarProtocol) {
		return _pFarProtocol->SendOutOfBandData(buffer, pUserData);
	} else {
		FATAL("WsMetadataProtocol has no Far Protocol (is orphaned)");
		return false;
	}
}

WSInterface* WsMetadataProtocol::GetWSInterface() {
	return this;
}

//
// WSInterface overrides
//

bool WsMetadataProtocol::SignalInboundConnection(Variant &request) {
	//get the output stream
	_streamReq = (string) request[HTTP_FIRST_LINE][HTTP_URL];
	WARN("WsMetadataProtocl - new request, stream: %s", STR(_streamReq));
	if (_streamReq[0] == '/') {
		_streamReq.erase(0,1);
	}
	if (!_streamReq.size()) {	// default
		_streamReq = _streamName;
	}
	// can't report our existence, handshake not yet complete
	//
	//string helloStr = "{\"RMS\":{\"status\":\"ok\"}}";
	//char world[] = "hello";
	//IOBuffer hello;
	//hello.ReadFromString(helloStr);
	//SendOutOfBandData(hello, world);
	// send it twice as a flushing test
	//SendOutOfBandData(hello, world);

	// find our Metadata Manager
	//get the application
	BaseClientApplication *pApp = GetApplication();
	if (pApp == NULL) {
		FATAL("Unable to get the application");
		return false;
	}
	_pManager = pApp->GetMetadataManager();
	if (!_pManager) {
		FATAL("WebSocket Metadata Protocol can't find MetadataManager!!");
		return false;
	}
	//
	// create our metadata stream module
	_pStream = new OutMetadataStream(this, ST_OUT_META, _streamReq);
	// $ToDo$ - i must need to call something to link this in!!

	// add our stream module to the manager
	_pManager->AddPushStream(_streamReq, _pStream);
	//
	return true;
}

bool WsMetadataProtocol::SignalTextData(const uint8_t *pBuffer, size_t length) {
	//FINEST("%s", STR(string((const char *) pBuffer, length)));
	//
	// route this to MetadataManager
	//
	string mdStr = string((char *)pBuffer, (size_t)length);
	//_pManager->SetMetadata(mdStr, _streamName);
	_pManager->SetMetadata(mdStr, _streamReq);	// streamName from Req above
	return true;
}

bool WsMetadataProtocol::SignalBinaryData(const uint8_t *pBuffer, size_t length) {
	//FINEST("Received %"PRIz"u bytes",length);
	return true;
}

bool WsMetadataProtocol::SignalPing(const uint8_t *pBuffer, size_t length) {
	//FINEST("ping: %s", STR(string((const char *) pBuffer, length)));
	return true;
}

void WsMetadataProtocol::SignalConnectionClose(uint16_t code, const uint8_t *pReason, size_t length) {
	FINEST("WsMetadata Connection Close! code: %"PRIu16"; reason: %s", code, STR(string((const char *) pReason, length)));
	GracefullyEnqueueForDelete();
}

bool WsMetadataProtocol::DemaskBuffer() {
	return true;
}



#endif // HAS_PROTOCOL_WS
