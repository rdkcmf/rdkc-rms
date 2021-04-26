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


#include "protocols/ssl/basesslprotocol.h"

#define MAX_SSL_READ_BUFFER 65536

map<string, SSL_CTX *> BaseSSLProtocol::_pGlobalContexts;
bool BaseSSLProtocol::_libraryInitialized = false;
X509Certificate* BaseSSLProtocol::_pCertificate = NULL;

BaseSSLProtocol::BaseSSLProtocol(uint64_t type)
: BaseProtocol(type) {
	_pGlobalSSLContext = NULL;
	_pSSL = NULL;
	_sslHandshakeCompleted = false;
	_pReadBuffer = new uint8_t[MAX_SSL_READ_BUFFER];
	_type = type;
	_hash = "";
	_evpcert = NULL;
	_evpkey = NULL;
}

BaseSSLProtocol::~BaseSSLProtocol() {
	if (_pSSL != NULL) {
		SSL_free(_pSSL);
		_pSSL = NULL;
	}

	delete[] _pReadBuffer;
}

bool BaseSSLProtocol::Initialize(Variant &parameters) {
	//Initialize certificate
	if( NULL == _pCertificate) {
		INFO("Initializing certificate in BaseSSLProtocol");
		_pCertificate = X509Certificate::GetInstance();
	}
	_pCertificate->GetKey(&_evpkey);
	_pCertificate->GetCertificate(&_evpcert);

	//1. Initialize the SSL library
	if (!_libraryInitialized) {
		//3. This is the first time we use the library. So we have to
		//initialize it first
		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		SSL_library_init();
		//init readable error messages
		SSL_load_error_strings();
		ERR_load_SSL_strings();
		ERR_load_CRYPTO_strings();
		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();
		OpenSSL_add_all_ciphers();
		OpenSSL_add_all_digests();
		#else
		OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
		#endif

#ifdef HAS_PROTOCOL_WEBRTC
		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		// Make sure that we are using openssl 1.0.1p or higher!
		uint64_t version = SSLeay();
		INFO("OpenSSL version %lu", version);
		if (version != 0x01000207f) {
			ASSERT("OpenSSL version found: %s (%"PRIx64")! It should be 1.0.2g!", SSLeay_version(SSLEAY_VERSION), version);
		}
		#else
		uint64_t version = OpenSSL_version_num();
		INFO("OpenSSL version %lu", version);
		if (version < 0x10100000L) {
			ASSERT("OpenSSL version found: %s (%"PRIx64")! It should be at least 1.1.x!", OpenSSL_version(OPENSSL_VERSION), version);
		}
		#endif
#endif

		//initialize the random numbers generator
		InitRandGenerator();
		_libraryInitialized = true; //This was false previously, why???
	}

	//2. Initialize the global context
	if (!InitGlobalContext(parameters)) {
		FATAL("Unable to initialize global context");
		return false;
	}

	//3. create connection SSL context
	_pSSL = SSL_new(_pGlobalSSLContext);
	if (_pSSL == NULL) {
		FATAL("Unable to create SSL connection context");
		return false;
	}

	//4. setup the I/O buffers
	SSL_set_bio(_pSSL, BIO_new(BIO_s_mem()), BIO_new(BIO_s_mem()));

	INFO("Do ssl handshake as base ssl protocol has been initialized");
	return DoHandshake();
}

bool BaseSSLProtocol::AllowFarProtocol(uint64_t type) {
	if (((_type == PT_OUTBOUND_DTLS) || (_type == PT_INBOUND_DTLS))
			&& ((type == PT_UDP) || (type == PT_ICE_TCP))) {
		return true;
	}

	if (type == PT_TCP) 
		return true;

	return false;
}

bool BaseSSLProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

bool BaseSSLProtocol::EnqueueForOutbound() {
	//1. Is the SSL handshake completed?
	if (!_sslHandshakeCompleted) {
		INFO("Do ssl handshake");
		return DoHandshake();
	}

	//2. Do we have some outstanding data?
	IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();

	if (pBuffer == NULL)
		return true;

	//3. Encrypt the outstanding data
	/*
	if (SSL_write(_pSSL, GETIBPOINTER(*pBuffer), GETAVAILABLEBYTESCOUNT(*pBuffer))
			!= (int32_t) GETAVAILABLEBYTESCOUNT(*pBuffer)) {
		FATAL("Unable to write %u bytes", GETAVAILABLEBYTESCOUNT(*pBuffer));
		return false;
	}
	*/
	int32_t result = SSL_write(_pSSL, GETIBPOINTER(*pBuffer), GETAVAILABLEBYTESCOUNT(*pBuffer));
	if (result <= 0) {
		int32_t error = SSL_get_error(_pSSL, result);
		FATAL("Unable to write %u bytes. Error: %"PRId32, GETAVAILABLEBYTESCOUNT(*pBuffer), error);
		return false;
	}
	pBuffer->IgnoreAll();

	//4. Do the actual I/O
	return PerformIO();
}

IOBuffer * BaseSSLProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;
	return NULL;
}

IOBuffer * BaseSSLProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool BaseSSLProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool BaseSSLProtocol::SignalInputData(IOBuffer &buffer) {
	//1. get the SSL input buffer
	BIO *pInBio = SSL_get_rbio(_pSSL);

	//2. dump all the data from the network inside the ssl input
	BIO_write(pInBio, GETIBPOINTER(buffer),
			GETAVAILABLEBYTESCOUNT(buffer));
	buffer.IgnoreAll();

	//3. Do we have to do some handshake?
	if (!_sslHandshakeCompleted) {
		INFO("Do ssl handshake as ssl protocol has been signalled with data");
		if (!DoHandshake()) {
			FATAL("Unable to do the SSL handshake");
			return false;
		}
		if (!_sslHandshakeCompleted) {
			return true;
		}
	}
	
	//4. Read the actual data and put it in the descrypted input buffer
	int32_t read = 0;
	while ((read = SSL_read(_pSSL, _pReadBuffer, MAX_SSL_READ_BUFFER)) > 0) {
		_inputBuffer.ReadFromBuffer(_pReadBuffer, (uint32_t) read);
	}
	if (read < 0) {
		int32_t error = SSL_get_error(_pSSL, read);
		if (error != SSL_ERROR_WANT_READ &&
				error != SSL_ERROR_WANT_WRITE) {
			FATAL("Unable to read data: %d", error);
			return false;
		}
	}

	//6. If we have pending data inside the decrypted buffer, bubble it up on the protocol stack
	if (GETAVAILABLEBYTESCOUNT(_inputBuffer) > 0) {
		if (_pNearProtocol != NULL) {
			if (!_pNearProtocol->SignalInputData(_inputBuffer)) {
				FATAL("Unable to signal near protocol for new data");
				return false;
			}
		}
	}
	
	//7. After the data was sent on the upper layers, we might have outstanding
	//data that needs to be sent.
	return PerformIO();
}

bool BaseSSLProtocol::PerformIO() {
	//1. Put the data from SSL output buffer inside our protocol output buffer
	if (!_outputBuffer.ReadFromBIO(SSL_get_wbio(_pSSL))) {
		FATAL("Unable to transfer data from outBIO to outputBuffer");
		return false;
	}

	//2. Enqueue the protocol for outbound if we have data that needs to be sent
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0) {
		if (_pFarProtocol != NULL) {
			return _pFarProtocol->EnqueueForOutbound();
		}
	}

	//3. Done
	return true;
}

string BaseSSLProtocol::GetSSLErrors() {
	string result = "";
	uint32_t errorCode;
	char *pTempBuffer = new char[4096];
	while ((errorCode = ERR_get_error()) != 0) {
		memset(pTempBuffer, 0, 4096);
		ERR_error_string_n(errorCode, pTempBuffer, 4095);
		result += "\n";
		result += pTempBuffer;
	}
	delete[] pTempBuffer;
	return result;
}

string BaseSSLProtocol::DumpBIO(BIO *pBIO) {
	string formatString;
	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	formatString = "method: %p\n";
	formatString += "callback: %p\n";
	formatString += "cb_arg: %p\n";
	formatString += "init: %d\n";
	formatString += "shutdown: %d\n";
	formatString += "flags: %d\n";
	formatString += "retry_reason: %d\n";
	formatString += "num: %d\n";
	formatString += "ptr: %p\n";
	formatString += "next_bio: %p\n";
	formatString += "prev_bio: %p\n";
	formatString += "references: %d\n";
	formatString += "num_read: %"PRId64"\n";
	formatString += "num_write: %"PRId64;
	return format(STR(formatString),
			pBIO->method,
			pBIO->callback,
			pBIO->cb_arg,
			pBIO->init,
			pBIO->shutdown,
			pBIO->flags,
			pBIO->retry_reason,
			pBIO->num,
			pBIO->ptr,
			pBIO->next_bio,
			pBIO->prev_bio,
			pBIO->references,
			(int64_t) pBIO->num_read,
			(int64_t) pBIO->num_write);
	#else
	return formatString;
	#endif
}

void BaseSSLProtocol::InitRandGenerator() {
	uint32_t length = 16;
	uint32_t *pBuffer = new uint32_t[length];

	while (RAND_status() == 0) {
		for (uint32_t i = 0; i < length; i++) {
			pBuffer[i] = rand();
		}

		RAND_seed(pBuffer, length * 4);
	}

	delete[] pBuffer;
}

bool BaseSSLProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	FATAL("This should have been overridden!");
	return false;
}

bool BaseSSLProtocol::Start() {
	FATAL("This should have been overridden!");
	return false;
}

int8_t BaseSSLProtocol::GetHandshakeState() {
	if (_pSSL != NULL) {
		return (int8_t)SSL_get_state(_pSSL);
	}
	else {
		return -1;
	}
}
