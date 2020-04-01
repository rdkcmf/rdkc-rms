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
#ifndef _HTTPPARSER_H
#define	_HTTPPARSER_H

#include "common.h"

typedef enum {
	HS_METHOD = 0,
	HS_URI,
	HS_VERSION,
	HS_HEADER_KVP,
	HS_COMPLETE,
	HS_ERROR
} HTTPHeadersState;

typedef struct {
	size_t startPos;
	size_t length;
	void GetVariant(Variant &result, const uint8_t *pBuffer) const;
	void GetVariant(string &result, const uint8_t *pBuffer) const;
} StringBounds;

typedef struct {
	StringBounds full;
	StringBounds key;
	StringBounds value;
} HTTPHeaderKVP;

#define MAX_HTTP_KVP_COUNT 64
#define MAX_HTTP_LINE_LENGTH 16384

class HTTPRequest {
private:
	HTTPHeadersState _state;
	size_t _cursor;
	StringBounds _method;
	StringBounds _uri;
	StringBounds _version;
	size_t _kvpCount;
	HTTPHeaderKVP _kvp[MAX_HTTP_KVP_COUNT];
	size_t _headersLength;
	Variant _variant;
public:
	HTTPRequest();
	virtual ~HTTPRequest();

	void Clear();
	bool Parse(const IOBuffer &raw);
	bool Parse(const char *pRaw, size_t length);
	HTTPHeadersState GetState() const;
	bool IsComplete() const;
	size_t BytesCount() const;
	string ToString(const IOBuffer &raw) const;
	string ToString(const char *pRaw) const;

	size_t GetURIPos() const;
	size_t GetURILength() const;
	size_t GetVersionPos() const;
	size_t GetVersionLength() const;

	Variant &GetVariant(const IOBuffer &buffer);
	Variant &GetVariant(const uint8_t *pBuffer);
private:
	void ChangeState(HTTPHeadersState targetState);
	int ParseHTTPRequestMethod(const char *pRaw, size_t length, HTTPHeadersState targetState);
	int ParseHTTPRequestURI(const char *pRaw, size_t length, HTTPHeadersState targetState);
	int ParseHTTPRequestVersion(const char *pRaw, size_t length, HTTPHeadersState targetState);
	int ParseHTTPRequestKVP(const char *pRaw, size_t length, HTTPHeadersState targetState);

};

#endif	/* _HTTPPARSER_H */
