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
#ifndef _BASEHTTPPROTOCOL_H
#define	_BASEHTTPPROTOCOL_H

#include "protocols/baseprotocol.h"

class IOBuffer;
class IOBuffer;

class DLLEXP BaseHTTPProtocol
: public BaseProtocol {
protected:
	uint32_t _state;
	Variant _headers;
	bool _chunkedContent;
	bool _lastChunk;
	uint32_t _contentLength;
	uint32_t _sessionDecodedBytesCount;
	uint64_t _decodedBytesCount;
	bool _disconnectAfterTransfer;
	bool _autoFlush;
	IOBuffer _outputBuffer;
	IOBuffer _inputBuffer;
	bool _hasAuth;
	Variant _outboundHeaders;
	bool _continueAfterParseHeaders;
public:
	BaseHTTPProtocol(uint64_t protocolType);
	virtual ~BaseHTTPProtocol();

	virtual IOBuffer * GetOutputBuffer();
	virtual IOBuffer * GetInputBuffer();
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual uint64_t GetDecodedBytesCount();
	virtual bool Initialize(Variant &parameters);
	virtual bool EnqueueForOutbound();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);

	bool GetDisconnectAfterTransfer();
	void SetDisconnectAfterTransfer(bool disconnectAfterTransfer);
	bool IsChunkedContent();
	bool IsLastChunk();
	uint32_t GetContentLength();
	uint32_t GetSessionDecodedBytesCount();
	bool TransferCompleted();
	Variant GetHeaders();
	bool Flush();

	void SetOutboundHeader(string name, string value);
protected:
	virtual string GetOutputFirstLine() = 0;
	virtual bool ParseFirstLine(string &line, Variant &headers) = 0;
	virtual bool Authenticate() = 0;
private:
	string DumpState();
	bool ParseHeaders(IOBuffer &buffer);
	bool HandleChunkedContent(IOBuffer &buffer);
	bool HandleFixedLengthContent(IOBuffer &buffer);

};


#endif	/* _BASEHTTPPROTOCOL_H */
#endif /* HAS_PROTOCOL_HTTP */

