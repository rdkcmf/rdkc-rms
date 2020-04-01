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
*/

#include "protocols/http/httpadaptorprotocol.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "streaming/baseinstream.h"
#include "streaming/hls/outnethlsstream.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/ts/innettsstream.h"

HTTPAdaptorProtocol::HTTPAdaptorProtocol()
: BaseProtocol(PT_HTTP_ADAPTOR) {
	_available = 0;
	_returnCode = 200;
	_pOutStream = NULL;
}

HTTPAdaptorProtocol::~HTTPAdaptorProtocol() {
	if (_pOutStream != NULL) {
		delete _pOutStream;
		_pOutStream = NULL;
	}
}

bool HTTPAdaptorProtocol::Initialize(Variant &parameters) {
	_customParameters = parameters;
	return true;
}

bool HTTPAdaptorProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_INBOUND_HTTP2 || type == PT_OUTBOUND_HTTP2);
}

bool HTTPAdaptorProtocol::AllowNearProtocol(uint64_t type) {
	return (type == PT_PASSTHROUGH || type == PT_INBOUND_TS);
}

bool HTTPAdaptorProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool HTTPAdaptorProtocol::SignalInputData(IOBuffer &buffer) {
	return _pNearProtocol != NULL
			&& (GETAVAILABLEBYTESCOUNT(buffer) >= _available)
			&& _inputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), _available)
			&& buffer.Ignore(_available)
			&& _pNearProtocol->SignalInputData(_inputBuffer);
}

int aaaa = 0;

bool HTTPAdaptorProtocol::SendBlock(uint8_t *pBuffer, uint32_t size) {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) >= 8 * 1024 * 1024)
		return EnqueueForOutbound();
	return _outputBuffer.ReadFromBuffer(pBuffer, size)
			&& EnqueueForOutbound();
}

HTTPInterface* HTTPAdaptorProtocol::GetHTTPInterface() {
	return this;
}

IOBuffer * HTTPAdaptorProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

bool HTTPAdaptorProtocol::SignalRequestBegin(HttpMethod method, const char *pUri,
		const uint32_t uriLength, const char *pHttpVersion,
		const uint32_t httpVersionLength) {
	if (uriLength <= 1)
		return true;
	_streamName = string(pUri + 1, uriLength - 1);
	trim(_streamName);
	return true;
}

bool HTTPAdaptorProtocol::SignalResponseBegin(const char *pHttpVersion,
		const uint32_t httpVersionLength, uint32_t code,
		const char *pDescription, const uint32_t descriptionLength) {
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

bool HTTPAdaptorProtocol::SignalHeader(const char *pName, const uint32_t nameLength,
		const char *pValue, const uint32_t valueLength) {
	string headerName = (string) pName;
	string headerValue = (string) pValue;
	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
	std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);

	//todo: rms ts/http does not support redirection yet. code below somewhat works but fails to pull the stream
	//initial redirection code
	//if (_returnCode == 302 && headerName == "location") {
	//	bool resolveHost = (!_customParameters["customParameters"]["externalStreamConfig"].HasKeyChain(V_STRING, true, 1, "httpProxy"))
	//		|| (_customParameters["customParameters"]["externalStreamConfig"]["httpProxy"] == "")
	//		|| (_customParameters["customParameters"]["externalStreamConfig"]["httpProxy"] == "self");

	//	//2. Split the URI
	//	URI uri;
	//	if (!URI::FromString(headerValue, resolveHost, uri)) {
	//		FATAL("Invalid URI: %s", STR(headerValue));
	//		return false;
	//	}

	//	_customParameters["customParameters"]["externalStreamConfig"]["uri"] = uri;
	//	if (GetNearProtocol() != NULL)
	//		GetNearProtocol()->EnqueueForDelete();
	//	this->EnqueueForDelete();

	//	return HTTPAdaptorProtocol::Connect(uri.ip(), uri.port(), _customParameters);
	//}

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

bool HTTPAdaptorProtocol::SignalHeadersEnd() {
	return true;
}

bool HTTPAdaptorProtocol::SignalBodyData(HttpBodyType bodyType, uint32_t length,
		uint32_t consumed, uint32_t available, uint32_t remaining) {
	_available = available;
	return true;
}

bool HTTPAdaptorProtocol::SignalBodyEnd() {
	return true;
}

bool HTTPAdaptorProtocol::SignalRequestTransferComplete() {
	if (_streamName == "") {
		SetAck(404, "Not Found");
		return true;
	}

	StreamsManager *pStreamsMgr = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pStreamsMgr->FindByTypeByName(
			ST_IN_NET, _streamName, true, false); //name must match
	if (streams.size() == 0) {
		FATAL("Stream %s not found", STR(_streamName));
		SetAck(404, "Not Found");
		return true;
	}

	BaseInStream *pInStream = NULL;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_PASSTHROUGH)
				|| MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_TS)) {
			pInStream = (BaseInStream *) MAP_VAL(i);
			break;
		}
	}

	if (pInStream == NULL) {
		FATAL("Stream %s not compatible to TS", STR(_streamName));
		SetAck(415, "Unsupported Media Type");
		return true;
	}

	Variant customParameters;

	_pOutStream = new OutNetHLSStream(this, _streamName, customParameters);
	if (!_pOutStream->SetStreamsManager(pStreamsMgr)) {
		FATAL("Unable to set the streams manager");
		delete _pOutStream;
		_pOutStream = NULL;
		SetAck(404, "Not Found");
		return true;
	}
	if (!pInStream->Link(_pOutStream)) {
		delete _pOutStream;
		_pOutStream = NULL;
		FATAL("Source stream %s not compatible with TS streaming", STR(_streamName));
		SetAck(404, "Not Found");
		return true;
	}
	SetAck(200, "OK");

	_pOutStream->Enable(true);
	return true;
}

bool HTTPAdaptorProtocol::SignalResponseTransferComplete() {
	return true;
}

bool HTTPAdaptorProtocol::WriteFirstLine(IOBuffer &outputBuffer) {
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

bool HTTPAdaptorProtocol::WriteTargetHost(IOBuffer &outputBuffer) {
	if (!_customParameters["customParameters"]["externalStreamConfig"]["uri"].HasKeyChain(V_STRING, false, 1, "host")) {
		FATAL("No host name detected on the output HTTP protocol");
		return false;
	}
	outputBuffer.ReadFromString((string) _customParameters["customParameters"]["externalStreamConfig"]["uri"].GetValue("host", false));
	return true;
}

bool HTTPAdaptorProtocol::WriteCustomHeaders(IOBuffer &outputBuffer) {
	if (_pFarProtocol->GetType() == PT_INBOUND_HTTP2) {
		outputBuffer.ReadFromString("Content-type: video/MP2T\r\n");
		outputBuffer.ReadFromString("Cache-Control: no-cache\r\n");
	} else if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP2) {
		outputBuffer.ReadFromString("Accept: video/mp2t,application/octet-stream\r\n");
		outputBuffer.ReadFromString("Connection: keep-alive\r\n");
	}
	return true;
}

HttpBodyType HTTPAdaptorProtocol::GetOutputBodyType() {
	return HTTP_TRANSFER_CHUNKED;
}

int64_t HTTPAdaptorProtocol::GetContentLenth() {
	NYIA;
	return 0;
}

bool HTTPAdaptorProtocol::NeedsNewOutboundRequest() {
	return false;
}

bool HTTPAdaptorProtocol::HasMoreChunkedData() {
	return false;
}

bool HTTPAdaptorProtocol::SendRequest(Variant &variant) {
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

void HTTPAdaptorProtocol::SetAck(uint32_t code, string message) {
	_returnCode = code;
}

string HTTPAdaptorProtocol::GetHTTPDescription(uint32_t returnCode) {
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

bool HTTPAdaptorProtocol::Connect(string ip, uint16_t port,
		Variant customParameters) {

	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			customParameters[CONF_PROTOCOL]);
	if (chain.size() == 0) {
		FATAL("Unable to obtain protocol chain from settings: %s",
				STR(customParameters[CONF_PROTOCOL]));
		return false;
	}
	if (!TCPConnector<HTTPAdaptorProtocol>::Connect(ip, port, chain,
			customParameters)) {
		FATAL("Unable to connect to %s:%hu", STR(ip), port);
		return false;
	}
	return true;
}

bool HTTPAdaptorProtocol::SignalProtocolCreated(BaseProtocol *pProtocol,
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

	//2. Trigger processing, including handshake
	HTTPAdaptorProtocol *pOutboundHTTPProtocol = (HTTPAdaptorProtocol *) pProtocol;
	pOutboundHTTPProtocol->SetApplication(pApplication);
	pOutboundHTTPProtocol->SetOutboundConnectParameters(customParameters);

	return pOutboundHTTPProtocol->SendRequest(customParameters);
}
