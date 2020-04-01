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

#include "protocols/http/httpmediareceiver.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"

HTTPMediaReceiver::HTTPMediaReceiver()
	: BaseProtocol(PT_HTTP_MEDIA_RECV) {
	_available = 0;
	_consumedBytes = 0;
	_returnCode = 200;
}

HTTPMediaReceiver::~HTTPMediaReceiver() {
}

bool HTTPMediaReceiver::Initialize(Variant &parameters) {
	_customParameters = parameters;
	return true;
}

bool HTTPMediaReceiver::AllowFarProtocol(uint64_t type) {
	return (type == PT_INBOUND_HTTP2);
}

bool HTTPMediaReceiver::AllowNearProtocol(uint64_t type) {
	return false;
}

bool HTTPMediaReceiver::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool HTTPMediaReceiver::SignalInputData(IOBuffer &buffer) {
	if ((GETAVAILABLEBYTESCOUNT(buffer) >= _available)
		&& _inputBuffer.IgnoreAll()
		&& _inputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), _available)
		&& buffer.Ignore(_available)) {
		string targetFolder = (string)_customParameters["targetFolder"];
		if (targetFolder == "") {
			FATAL("Record folder is not specified");
			return false;
		}
		if (!fileExists(targetFolder)) {
			if (!createFolder(targetFolder, true)) {
				FATAL("Unable to create targetFolder: %s", targetFolder.c_str());
				return false;
			}
		}
		string delimiter;
#ifdef WIN32
		delimiter = "\\";
#else
		delimiter = "/";
#endif
		if (!_mediaWriter.IsOpen() && !_mediaWriter.Initialize(format("%s%s%s.mp4", STR(targetFolder),
			STR(delimiter), STR(_streamName)), FILE_OPEN_MODE_APPEND)) {
			FATAL("Unable to open media file for writing");
			return false;
		}
		if (!_mediaWriter.WriteBuffer(GETIBPOINTER(_inputBuffer), _available)) {
			FATAL("Unable to write stream to media file");
			return false;
		}	
	}
	return true;
}

HTTPInterface* HTTPMediaReceiver::GetHTTPInterface() {
	return this;
}

IOBuffer * HTTPMediaReceiver::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) != 0)
		return &_outputBuffer;
	return NULL;
}

bool HTTPMediaReceiver::SignalRequestBegin(HttpMethod method, const char *pUri,
		const uint32_t uriLength, const char *pHttpVersion,
		const uint32_t httpVersionLength) {
	if (uriLength <= 1) {
		FATAL("No streamname detected from POST headers");
		return false;
	}
	_streamName = string(pUri + 1, uriLength - 1);
	trim(_streamName);

	return true;
}

bool HTTPMediaReceiver::SignalResponseBegin(const char *pHttpVersion,
		const uint32_t httpVersionLength, uint32_t code,
		const char *pDescription, const uint32_t descriptionLength) {
	return true;
}

bool HTTPMediaReceiver::SignalHeader(const char *pName, const uint32_t nameLength,
		const char *pValue, const uint32_t valueLength) {
	string headerName = (string) pName;
	string headerValue = (string) pValue;
	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
	std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);
	
	if (headerName == "content-type" &&
			(headerValue != "video/mp4" && headerValue != "application/octet-stream")) {
		return false;
	}

	return true;
}

bool HTTPMediaReceiver::SignalHeadersEnd() {
	return true;
}

bool HTTPMediaReceiver::SignalBodyData(HttpBodyType bodyType, uint32_t length,
		uint32_t consumed, uint32_t available, uint32_t remaining) {
	_available = available;
	_consumedBytes += available;
	return true;
}

bool HTTPMediaReceiver::SignalBodyEnd() {
	return true;
}

bool HTTPMediaReceiver::SignalRequestTransferComplete() {
	SetAck(200, "OK");
	if (_mediaWriter.IsOpen()) {
		_mediaWriter.Flush();
		_mediaWriter.Close();
	}

	return true;
}

bool HTTPMediaReceiver::SignalResponseTransferComplete() {
	return true;
}

bool HTTPMediaReceiver::WriteFirstLine(IOBuffer &outputBuffer) {
	string firstLine = format("HTTP/1.1 %"PRIu32" %s\r\n",
			_returnCode, STR(GetHTTPDescription(_returnCode)));
	outputBuffer.ReadFromString(firstLine);

	return true;
}

bool HTTPMediaReceiver::WriteTargetHost(IOBuffer &outputBuffer) {
	return true;
}

bool HTTPMediaReceiver::WriteCustomHeaders(IOBuffer &outputBuffer) {
	outputBuffer.ReadFromString("Content-type: video/mp4\r\n");
	outputBuffer.ReadFromString("Cache-Control: no-cache\r\n");

	return true;
}

HttpBodyType HTTPMediaReceiver::GetOutputBodyType() {
	return HTTP_TRANSFER_CONTENT_LENGTH;
}

int64_t HTTPMediaReceiver::GetContentLenth() {
	return _consumedBytes;
}

bool HTTPMediaReceiver::NeedsNewOutboundRequest() {
	return false;
}

bool HTTPMediaReceiver::HasMoreChunkedData() {
	return false;
}

void HTTPMediaReceiver::SetAck(uint32_t code, string message) {
	_returnCode = code;
}

string HTTPMediaReceiver::GetHTTPDescription(uint32_t returnCode) {
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
