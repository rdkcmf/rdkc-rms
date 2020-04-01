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



#ifdef HAS_PROTOCOL_HTTP
#include "protocols/http/outboundhttpprotocol.h"

OutboundHTTPProtocol::OutboundHTTPProtocol()
: BaseHTTPProtocol(PT_OUTBOUND_HTTP) {
}

OutboundHTTPProtocol::~OutboundHTTPProtocol() {
}

string OutboundHTTPProtocol::Method() {
	return _method;
}

void OutboundHTTPProtocol::Method(string method) {
	_method = method;
}

string OutboundHTTPProtocol::Document() {
	return _document;
}

void OutboundHTTPProtocol::Document(string document) {
	_document = document;
}

string OutboundHTTPProtocol::Host() {
	return _host;
}

void OutboundHTTPProtocol::Host(string host) {
	_host = host;
}

bool OutboundHTTPProtocol::EnqueueForOutbound() {
	SetOutboundHeader(HTTP_HEADERS_HOST, _host);
	return BaseHTTPProtocol::EnqueueForOutbound();
}

bool OutboundHTTPProtocol::Is200OK() {
	return _headers[HTTP_FIRST_LINE][HTTP_STATUS_CODE] == HTTP_STATUS_CODE_200;
}

string OutboundHTTPProtocol::GetOutputFirstLine() {
	return format("%s %s HTTP/1.1", STR(_method), STR(_document));
}

bool OutboundHTTPProtocol::ParseFirstLine(string &line, Variant &firstLineHeader) {
	vector<string> parts;
	split(line, " ", parts);
	if (parts.size() < 3) {
		FATAL("Incorrect first line: %s", STR(line));
		return false;
	}

	if ((parts[0] != HTTP_VERSION_1_1)
			&& (parts[0] != HTTP_VERSION_1_0)) {
		FATAL("Http version not supported: %s", STR(parts[0]));
		return false;
	}

	if (!isNumeric(parts[1])) {
		FATAL("Invalid HTTP code: %s", STR(parts[1]));
		return false;
	}

	string reason = "";
	for (uint32_t i = 2; i < parts.size(); i++) {
		reason += parts[i];
		if (i != parts.size() - 1)
			reason += " ";
	}

	firstLineHeader[HTTP_VERSION] = parts[0];
	firstLineHeader[HTTP_STATUS_CODE] = parts[1];
	firstLineHeader[HTTP_STATUS_CODE_REASON] = reason;

	return true;
}

bool OutboundHTTPProtocol::Authenticate() {
	return true;
}
#endif /* HAS_PROTOCOL_HTTP */

