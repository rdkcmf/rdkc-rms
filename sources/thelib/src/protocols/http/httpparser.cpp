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
#include "protocols/http/httpparser.h"

#define DEBUG_HTTP(...)
//#define DEBUG_HTTP(...) FINEST(__VA_ARGS__)

void StringBounds::GetVariant(Variant& result, const uint8_t* pBuffer) const {
	result = string((const char *) pBuffer + startPos, length);
}

void StringBounds::GetVariant(string& result, const uint8_t* pBuffer) const {
	result = string((const char *) pBuffer + startPos, length);
}

HTTPRequest::HTTPRequest() {
	Clear();
}

HTTPRequest::~HTTPRequest() {
}

void HTTPRequest::Clear() {
	_state = HS_METHOD;
	_cursor = 0;
	memset(&_method, 0, sizeof (_method));
	memset(&_uri, 0, sizeof (_uri));
	memset(&_version, 0, sizeof (_version));
	_kvpCount = 0;
	memset(&_kvp, 0, sizeof (_kvp));
	_headersLength = 0;
	_variant.Reset();
}

bool HTTPRequest::Parse(const IOBuffer &raw) {
	return Parse((const char *) GETIBPOINTER(raw), GETAVAILABLEBYTESCOUNT(raw));
}

bool HTTPRequest::Parse(const char *pRaw, size_t length) {
	DEBUG_HTTP("Parsing %"PRIz"u to %"PRIz"u", _cursor, length);
	int result = 0;
	while (_cursor < length) {
		switch (_state) {
			case HS_METHOD:
			{
				result = ParseHTTPRequestMethod(pRaw, length, HS_URI);
				break;
			}
			case HS_URI:
			{
				result = ParseHTTPRequestURI(pRaw, length, HS_VERSION);
				break;
			}
			case HS_VERSION:
			{
				result = ParseHTTPRequestVersion(pRaw, length, HS_HEADER_KVP);
				break;
			}
			case HS_HEADER_KVP:
			{
				result = ParseHTTPRequestKVP(pRaw, length, HS_COMPLETE);
				break;
			}
			case HS_COMPLETE:
			{
				break;
			}
			case HS_ERROR:
			default:
			{
				FATAL("Invalid state: %d", _state);
				ChangeState(HS_ERROR);
				return false;
			}
		}
		if (result < 0) {
			ChangeState(HS_ERROR);
			return false;
		}
		_cursor += result;
		if ((_state == HS_COMPLETE)&&(_headersLength == 0))
			_headersLength = _cursor;
		if (result == 0)
			return true;
	}
	return true;
}

string StringBoundsToString(const StringBounds *pString, const char *pRaw) {
	if (pRaw == NULL)
		return format("#(%"PRIz"u - %"PRIz"u)#", pString->startPos, pString->length);
	else
		return string(pRaw + pString->startPos, pString->length);
}

HTTPHeadersState HTTPRequest::GetState() const {
	return _state;
}

bool HTTPRequest::IsComplete() const {
	return _state == HS_COMPLETE;
}

size_t HTTPRequest::BytesCount() const {
	return _headersLength;
}

string HTTPRequest::ToString(const IOBuffer &raw) const {
	return ToString((const char *) GETIBPOINTER(raw));
}

string HTTPRequest::ToString(const char *pRaw) const {
	string result;
	result += format("   state: %d\n", _state);
	result += format("  curosr: %"PRIz"u\n", _cursor);
	result += format("  method: \"%s\"\n", STR(StringBoundsToString(&_method, pRaw)));
	result += format("     uri: \"%s\"\n", STR(StringBoundsToString(&_uri, pRaw)));
	result += format(" version: \"%s\"\n", STR(StringBoundsToString(&_version, pRaw)));
	result += format("kvpCount: %"PRIz"u\n", _kvpCount);
	for (size_t i = 0; i < _kvpCount; i++) {
		result += format("\t\"%s\" (`%s`:`%s`)\n",
				STR(StringBoundsToString(&_kvp[i].full, pRaw)),
				STR(StringBoundsToString(&_kvp[i].key, pRaw)),
				STR(StringBoundsToString(&_kvp[i].value, pRaw))
				);
	}
	result += format("headersLength: %"PRIz"u", _headersLength);
	return result;
}

Variant &HTTPRequest::GetVariant(const IOBuffer &buffer) {
	return GetVariant(GETIBPOINTER(buffer));
}

Variant &HTTPRequest::GetVariant(const uint8_t *pBuffer) {
	//if the request is not complete, reset the variant and return it
	if (_state != HS_COMPLETE) {
		_variant.Reset();
		return _variant;
	}

	//if we already have a completed HTTP request and also a variant, than just
	//return it
	if (_variant != V_NULL)
		return _variant;

	//otherwise, we build it
	_method.GetVariant(_variant[HTTP_FIRST_LINE][HTTP_METHOD], pBuffer);
	_uri.GetVariant(_variant[HTTP_FIRST_LINE][HTTP_URL], pBuffer);
	_version.GetVariant(_variant[HTTP_FIRST_LINE][HTTP_VERSION], pBuffer);
	string key = "";
	string value = "";
	for (size_t i = 0; i < _kvpCount; i++) {
		_kvp[i].key.GetVariant(key, pBuffer);
		trim(key);
		_kvp[i].value.GetVariant(value, pBuffer);
		trim(value);
		_variant[HTTP_HEADERS][key] = value;
	}
	return _variant;
}

void HTTPRequest::ChangeState(HTTPHeadersState targetState) {
	DEBUG_HTTP("%d -> %d", _state, targetState);
	_state = targetState;
}

size_t HTTPRequest::GetURIPos() const {
	return _uri.startPos;
}

size_t HTTPRequest::GetURILength() const {
	return _uri.length;
}

size_t HTTPRequest::GetVersionPos() const {
	return _version.startPos;
}

size_t HTTPRequest::GetVersionLength() const {
	return _version.length;
}

/*
 * - all have at least 1 character, because they are called
 * - they do not alter the cursor
 * - they do alter the state
 * - they setup the relevant string bounds
 */
int HTTPRequest::ParseHTTPRequestMethod(const char *pRaw, size_t length, HTTPHeadersState targetState) {
	switch (pRaw[_cursor ]) {
		case 'G':
		{
			//`GET `
			if (_cursor + 4 > length)
				return 0;
			if ((pRaw[_cursor + 1] != 'E')
					|| (pRaw[_cursor + 2] != 'T')
					|| (pRaw[_cursor + 3] != ' '))
				return -1;
			_method.startPos = _cursor;
			_method.length = 3;
			ChangeState(targetState);
			return 4;
		}
		default:
		{
			FATAL("Invalid HTTP method");
			return -1;
		}
	}
}

int HTTPRequest::ParseHTTPRequestURI(const char *pRaw, size_t length, HTTPHeadersState targetState) {
	for (size_t i = _cursor; i < length; i++) {
		if ((i >= MAX_HTTP_LINE_LENGTH) || (pRaw[i] == '\r') || (pRaw[i] == '\n'))
			return -1;
		if (pRaw[i] == ' ') {
			_uri.startPos = _cursor;
			_uri.length = i - _cursor;
			if (_uri.length != 0) {
				ChangeState(targetState);
				return (int) (_uri.length + 1);
			} else {
				return -1;
			}
		}
	}
	return 0;
}

int HTTPRequest::ParseHTTPRequestVersion(const char *pRaw, size_t length, HTTPHeadersState targetState) {
	//`HTTP/1.1\r\n`
	if ((_cursor + 10) > length)
		return 0;
	if ((pRaw[_cursor + 0] != 'H')
			|| (pRaw[_cursor + 1] != 'T')
			|| (pRaw[_cursor + 2] != 'T')
			|| (pRaw[_cursor + 3] != 'P')
			|| (pRaw[_cursor + 4] != '/')
			|| (pRaw[_cursor + 5] != '1')
			|| (pRaw[_cursor + 6] != '.')
			|| ((pRaw[_cursor + 7] != '0')&&(pRaw[_cursor + 7] != '1'))
			|| (pRaw[_cursor + 8] != '\r')
			|| (pRaw[_cursor + 9] != '\n')
			)
		return -1;
	_version.startPos = _cursor;
	_version.length = 8;
	ChangeState(targetState);
	return 10;
}

int HTTPRequest::ParseHTTPRequestKVP(const char *pRaw, size_t length, HTTPHeadersState targetState) {
	size_t colPos = 0;
	for (size_t i = _cursor; i <= length - 2; i++) {
		if ((i - _cursor) >= MAX_HTTP_LINE_LENGTH)
			return -1;
		if ((pRaw[i] == ':')&&(colPos == 0))
			colPos = i;
		if ((pRaw[i] == '\r') || (pRaw[i + 1] == '\n')) {
			if (_kvpCount + 1 > MAX_HTTP_KVP_COUNT)
				return -1;
			_kvp[_kvpCount].full.startPos = _cursor;
			_kvp[_kvpCount].full.length = i - _cursor;
			if (colPos != 0) {
				_kvp[_kvpCount].key.startPos = _cursor;
				_kvp[_kvpCount].key.length = colPos - _cursor;
				_kvp[_kvpCount].value.startPos = _kvp[_kvpCount].key.startPos + _kvp[_kvpCount].key.length + 1;
				_kvp[_kvpCount].value.length = _kvp[_kvpCount].full.length - _kvp[_kvpCount].key.length - 1;
			}
			if (_kvp[_kvpCount].full.length != 0) {
				_kvpCount++;
			} else {
				ChangeState(targetState);
			}
			return (int) (i - _cursor + 2);
		}
	}
	return 0;
}
