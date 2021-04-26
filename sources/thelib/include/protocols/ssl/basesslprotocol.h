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



#ifndef _SSLPROTOCOL_H
#define	_SSLPROTOCOL_H

#include "protocols/baseprotocol.h"

class BaseSSLProtocol
: public BaseProtocol {
private:
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
protected:
	static map<string, SSL_CTX *> _pGlobalContexts;
	static bool _libraryInitialized;
	SSL_CTX *_pGlobalSSLContext;
	SSL *_pSSL;
	bool _sslHandshakeCompleted;
	uint8_t *_pReadBuffer;
	uint32_t _maxOutBufferSize;
	string _hash;
	EVP_PKEY* _evpkey;
	X509* _evpcert;
	static X509Certificate* _pCertificate;
public:
	BaseSSLProtocol(uint64_t type);
	virtual ~BaseSSLProtocol();
	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool EnqueueForOutbound();
	virtual IOBuffer * GetOutputBuffer();
	virtual IOBuffer * GetInputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool Start();
	int8_t GetHandshakeState();
protected:
	virtual bool DoHandshake() = 0;
	virtual bool InitGlobalContext(Variant &parameters) = 0;
	bool PerformIO();
	string GetSSLErrors();
private:
	string DumpBIO(BIO *pBIO);
	void InitRandGenerator();
};


#endif	/* _SSLPROTOCOL_H */


