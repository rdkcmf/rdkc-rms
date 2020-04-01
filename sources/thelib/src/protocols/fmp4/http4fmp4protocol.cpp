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


#include "protocols/fmp4/http4fmp4protocol.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "streaming/baseinstream.h"
#include "streaming/hls/outnethlsstream.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"//// FATAL(">>>
#include "protocols/ts/innettsstream.h"
#include "streaming/fmp4/outnetfmp4stream.h"
#include "streaming/mp4/outnetmp4stream.h"



HTTP4FMP4Protocol::HTTP4FMP4Protocol()
	: BaseProtocol(PT_INBOUND_HTTP_4_FMP4) {
	_available = 0;
	_returnCode = 200;
	_pOutStream = NULL;
}

HTTP4FMP4Protocol::~HTTP4FMP4Protocol() {
	if (_pOutStream != NULL) {
		delete _pOutStream;
		_pOutStream = NULL;
	}
}

bool HTTP4FMP4Protocol::Initialize(Variant &parameters) {
	_customParameters = parameters;
	return true;
}

bool HTTP4FMP4Protocol::AllowFarProtocol(uint64_t type) {
//	return (type == PT_INBOUND_HTTP2 || type == PT_OUTBOUND_HTTP2);
	return (type == PT_INBOUND_HTTP2);
}

bool HTTP4FMP4Protocol::AllowNearProtocol(uint64_t type) {
	return (type == PT_PASSTHROUGH);
}

bool HTTP4FMP4Protocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool HTTP4FMP4Protocol::SignalInputData(IOBuffer &buffer) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalInputData()");
	return _pNearProtocol != NULL
		&& (GETAVAILABLEBYTESCOUNT(buffer) >= _available)
		&& _inputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), _available)
		&& buffer.Ignore(_available)
		&& _pNearProtocol->SignalInputData(_inputBuffer);
}

int aaaax = 0;

bool HTTP4FMP4Protocol::SendBlock(uint8_t *pBuffer, uint32_t size) {
//// FATAL(">>> HTTP4FMP4Protocol::SendBlock(size=%d)",size);

	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) >= 8 * 1024 * 1024)
		return EnqueueForOutbound();
	return _outputBuffer.ReadFromBuffer(pBuffer, size)
		&& EnqueueForOutbound();
}

HTTPInterface* HTTP4FMP4Protocol::GetHTTPInterface() {
	return this;
}

IOBuffer * HTTP4FMP4Protocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

/* !
 * Implement the HTTPInterface::SignalRequestBegin
 * @brief Called by the framework when a request just begun
 */
bool HTTP4FMP4Protocol::SignalRequestBegin(	HttpMethod method, const char *pUri,
											const uint32_t uriLength, const char *pHttpVersion,
											const uint32_t httpVersionLength) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalRequestBegin");
	if (uriLength <= 1)
		return true;
	_streamName = string(pUri + 1, uriLength - 1);
	trim(_streamName);
	return true;
}

/*!
 * Implement the HTTPInterface::SignalResponseBegin
 * @brief Called by the framework when a response just arrived for a
 * previously made HTTP request
 */
bool HTTP4FMP4Protocol::SignalResponseBegin(const char *pHttpVersion,
											const uint32_t httpVersionLength, uint32_t code,
											const char *pDescription, const uint32_t descriptionLength) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalResponseBegin");
	_returnCode = code;
	switch (code) {
	case 200:
		//enable this to support link redirections
		//case 302:
		return true;
		break;
	case 404:
		FATAL("Not found");
	default:
		FATAL("HTTP error occured");
		return false;
	}
}

/*
 * Implement HTTPInterface::SignalHeader() 
 * @brief Called by the framework for each and every HTTP header encountered
 * in the HTTP request or response headers section
 */
bool HTTP4FMP4Protocol::SignalHeader(	const char *pName, const uint32_t nameLength,
										const char *pValue, const uint32_t valueLength) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalHeader (%s,%s)",pName, pValue);

	string headerName	= (string)pName;
	string headerValue	= (string)pValue;
	std::transform(headerName.begin(),	headerName.end(),	headerName.begin(),	 ::tolower);
	std::transform(headerValue.begin(), headerValue.end(),	headerValue.begin(), ::tolower);

	if (headerName == "content-type" &&
		(headerValue == "video/mp2t"
			|| headerValue == "application/octet-stream")) {
		InboundTSProtocol *pInTs = new InboundTSProtocol();
		if (!pInTs->Initialize(_customParameters)) {
			delete pInTs;
			return false;
		}
		SetNearProtocol(pInTs);
		_pNearProtocol->SetFarProtocol(this);
		_pNearProtocol->SetApplication(GetApplication());
	}

	return true;
}

/*
 * Implement HTTPInterface::SignalHeadersEnd() 
 */
bool HTTP4FMP4Protocol::SignalHeadersEnd() {
//// FATAL(">>> HTTP4FMP4Protocol::SignalHeadersEnd");
	return true;
}

/*
 * Implement HTTPInterface::SignalBodyData() 
 */
bool HTTP4FMP4Protocol::SignalBodyData(HttpBodyType bodyType, uint32_t length,
										uint32_t consumed, uint32_t available, uint32_t remaining) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalBodyData");
	_available = available;
	return true;
}

/*
 * Implement HTTPInterface::SignalBodyEnd()
 */
bool HTTP4FMP4Protocol::SignalBodyEnd() {
//// FATAL(">>> HTTP4FMP4Protocol::SignalBodyEnd");
	return true;
}

/*
 * Implement HTTPInterface::SignalRequestTransferComplete()
 */
bool HTTP4FMP4Protocol::SignalRequestTransferComplete() {
//// FATAL(">>> HTTP4FMP4Protocol::SignalTransferComplete, _streamName=%s", STR(_streamName));
	// Return an error 404 if there are no parameters provided.
	if (_streamName == "") {
		SetAck(404, "Not Found");
		return true;
	}

	// Check if the parameter is for an FMP4 (reserved ?FMP4=)
//#define FMP4REQUEST "?fmp4="
//	if (_streamName.find(FMP4REQUEST) == 0) {
//		string fmp4Name = _streamName.substr(strlen(FMP4REQUEST));
//		_streamName = fmp4Name;
//		return ProcessFMP4Request();
//	}
//	SetAck(404, "Not Found");
	return ProcessFMP4Request();  
}

bool HTTP4FMP4Protocol::ProcessFMP4Request() {
	bool progressive = false;
	bool useHttp = true;

	//get the application
	BaseClientApplication *pApp = NULL;
	if ((pApp = GetApplication()) == NULL) {
		FATAL("Unable to get the application");
		SetAck(404, "Not Found");
		return true;
	}

	uint64_t outputStreamType = ST_OUT_NET_FMP4;

	//get the private stream name
	string privateStreamName = pApp->GetStreamNameByAlias(_streamName, false);
	if (privateStreamName == "") {
		FATAL("Stream name alias value not found: %s", STR(_streamName));
		SetAck(404, "Not Found");
		return true;
	}

	// Create output stream
	StreamsManager *pSM = NULL;
	if ((pSM = pApp->GetStreamsManager()) == NULL) {
		FATAL("Unable to get the streams manager");
		SetAck(404, "Not Found");
		return true;
	}

	//and also try to bind it to the possibly present input stream
	BaseInStream *pInStream = NULL;
	map<uint32_t, BaseStream *> inStreams = pSM->FindByTypeByName(ST_IN_NET, privateStreamName, true, false);
	if (inStreams.size() != 0)
		pInStream = (BaseInStream *)MAP_VAL(inStreams.begin());
	if ((pInStream != NULL) && (pInStream->IsCompatibleWithType(outputStreamType))) {
		BaseOutStream *pOutNetStream = NULL;
		pOutNetStream = OutNetFMP4Stream::GetInstance(pApp, privateStreamName, progressive, useHttp);

		if (pOutNetStream == NULL) {
			FATAL("Unable to create the output stream");
			SetAck(404, "Not Found");
			return true;
		}

		if (!pInStream->Link(pOutNetStream)) {
			FATAL("Unable to link the in/out streams");
			pOutNetStream->EnqueueForDelete();
			SetAck(404, "Not Found");
			return true;
		} else {
			//WARN("$b2$ InboundWS4FMP4 linked stream: %s", STR(*pInStream));
		}
		((OutNetFMP4Stream *)pOutNetStream)->RegisterOutputProtocol(GetId(), NULL);
	} else {
		if (!pInStream) {
			FATAL("Can't find input stream: %s", STR(privateStreamName));
		} else if (!pInStream->IsCompatibleWithType(outputStreamType)) {
			FATAL("Input stream: %s Not compatible with %s!",
				STR(*pInStream), STR(tagToString(outputStreamType)));
		}
	}

//	((OutNetFMP4Stream *)pOutNetStream)->RegisterOutputProtocol(GetId(), NULL);

	//	return pOutNetStream->GetUniqueId();

	SetAck(200, "OK");
	return true;
}

/*
 * Implement HTTPInterface::SignalResponseTransferComplete()
 */
bool HTTP4FMP4Protocol::SignalResponseTransferComplete() {
	return true;
}

/*
 * Implement HTTPInterface::WriteFirstLine()
 */
bool HTTP4FMP4Protocol::WriteFirstLine(IOBuffer &outputBuffer) {
//// FATAL(">>> HTTP4FMP4Protocol::WriteFirstLine");
	//int size = GETAVAILABLEBYTESCOUNT(outputBuffer);
	//	if (size == 0) outputBuffer.IgnoreAll(); 
	if (_pFarProtocol->GetType() == PT_INBOUND_HTTP2) {
		string firstLine = format("HTTP/1.1 %"PRIu32" %s\r\n",
			_returnCode, STR(GetHTTPDescription(_returnCode)));
		outputBuffer.ReadFromString(firstLine);
	} else if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP2) {
		outputBuffer.ReadFromBuffer((const uint8_t *) "GET ", 4);
		outputBuffer.ReadFromString((string)_customParameters["customParameters"]["externalStreamConfig"]["uri"]["fullDocumentPathWithParameters"]);
		outputBuffer.ReadFromBuffer((const uint8_t *) " HTTP/1.1\r\n", 11);
	}

	return true;
}

/*
 * Implement HTTPInterface::WriteTargetHost()
 */
bool HTTP4FMP4Protocol::WriteTargetHost(IOBuffer &outputBuffer) {
//// FATAL(">>> HTTP4FMP4Protocol::WriteTargetHost");
	if (!_customParameters["customParameters"]["externalStreamConfig"]["uri"].HasKeyChain(V_STRING, false, 1, "host")) {
		FATAL("No host name detected on the output HTTP protocol");
		return false;
	}
	outputBuffer.ReadFromString((string)_customParameters["customParameters"]["externalStreamConfig"]["uri"].GetValue("host", false));
	return true;
}

/*
 * Implement HTTPInterface::WriteCustomHeaders()
 */
bool HTTP4FMP4Protocol::WriteCustomHeaders(IOBuffer &outputBuffer) {
//// FATAL(">>> HTTP4FMP4Protocol::WriteCustomHeaders");
	if (_pFarProtocol->GetType() == PT_INBOUND_HTTP2) {
		outputBuffer.ReadFromString("Content-type: video/mp4\r\n");
		outputBuffer.ReadFromString("Cache-Control: no-cache\r\n");
		outputBuffer.ReadFromString("Connection: keep-alive\r\n");
		outputBuffer.ReadFromString("x-frame-options: SAMEORIGIN\r\n");
	}
	return true;
}

/*
 * Implement HTTPInterface::GetOutputBodyType()
 */
HttpBodyType HTTP4FMP4Protocol::GetOutputBodyType() {
//// FATAL(">>> HTTP4FMP4Protocol::GetOutputBodyType");
	return HTTP_TRANSFER_CHUNKED;
}

/*
 * Implement HTTPInterface::GetContentLenth()
 */
int64_t HTTP4FMP4Protocol::GetContentLenth() {
	NYIA;
	return 0;
}

/*
 * Implement HTTPInterface::NeedsNewOutboundRequest()
 */
bool HTTP4FMP4Protocol::NeedsNewOutboundRequest() {
//// FATAL(">>> HTTP4FMP4Protocol::NeedsNewOutboundRequest");
	return false;
}

/*!
 * Implement HTTPInterface::HasMoreChunkedData()
 *
 * @brief Called by the framework when the connection is in chunked mode for
 * output. It will not be called for the rest of the transfer modes
 */
bool HTTP4FMP4Protocol::HasMoreChunkedData() {
	// return false;
//// FATAL(">>> HTTP4FMP4Protocol::HasMoreChunkedData");
	// how to know if we're at the end of the stream?
	return true;
}

bool HTTP4FMP4Protocol::SendRequest(Variant &variant) {
//// FATAL(">>> HTTP4FMP4Protocol::SendRequest");
	//1. Do we have a protocol?
	if (_pFarProtocol == NULL) {
		FATAL("This protocol is not linked");
		return false;
	}
#ifdef HAS_PROTOCOL_HTTP2
	if (_pFarProtocol->GetType() != PT_OUTBOUND_HTTP2) {
		FATAL("Far protocol is not PT_OUTBOUND_HTTP2");
		return false;
	}
#endif /* HAS_PROTOCOL_HTTP2 */

	return EnqueueForOutbound();
}

void HTTP4FMP4Protocol::SetAck(uint32_t code, string message) {
	_returnCode = code;
}

string HTTP4FMP4Protocol::GetHTTPDescription(uint32_t returnCode) {
	switch (returnCode) {
	case 200:
		return "OK";
	case 404:
		return "Not Found";
	case 500:
		return "Internal Server Error";
	default:
		return "";
	}
}

bool HTTP4FMP4Protocol::Connect(string ip, uint16_t port,
								Variant customParameters) {
//// FATAL(">>> HTTP4FMP4Protocol::Connect");
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
		customParameters[CONF_PROTOCOL]);
	if (chain.size() == 0) {
		FATAL("Unable to obtain protocol chain from settings: %s",
			STR(customParameters[CONF_PROTOCOL]));
		return false;
	}
	if (!TCPConnector<HTTP4FMP4Protocol>::Connect(ip, port, chain,
		customParameters)) {
		FATAL("Unable to connect to %s:%hu", STR(ip), port);
		return false;
	}
	return true;
}


bool HTTP4FMP4Protocol::SignalProtocolCreated(BaseProtocol *pProtocol,
											Variant customParameters) {
//// FATAL(">>> HTTP4FMP4Protocol::SignalProtocolCreated");
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

	//2. Trigger processing, including handshake
	HTTP4FMP4Protocol *pOutboundHTTPProtocol = (HTTP4FMP4Protocol *)pProtocol;
	pOutboundHTTPProtocol->SetApplication(pApplication);
	pOutboundHTTPProtocol->SetOutboundConnectParameters(customParameters);

	return pOutboundHTTPProtocol->SendRequest(customParameters);
}
