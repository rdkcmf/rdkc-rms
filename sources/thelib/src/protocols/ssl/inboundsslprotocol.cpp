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



#include "protocols/ssl/inboundsslprotocol.h"

InboundSSLProtocol::InboundSSLProtocol()
: BaseSSLProtocol(PT_INBOUND_SSL) {

}

InboundSSLProtocol::~InboundSSLProtocol() {
}

bool InboundSSLProtocol::InitGlobalContext(Variant &parameters) {
	//1. Comput the hash on key/cert pair and see
	//if we have a global context with that hash
	string hash = "";
	if (parameters["hash"] != V_STRING) {
		if (parameters[CONF_SSL_KEY] != V_STRING
				|| parameters[CONF_SSL_CERT] != V_STRING) {
			FATAL("No key/cert provided");
			return false;
		}
		hash = md5((string) parameters[CONF_SSL_KEY]
				+ (string) parameters[CONF_SSL_CERT], true);
		parameters["hash"] = hash;
	} else {
		hash = (string) parameters["hash"];
	}
	string key = parameters[CONF_SSL_KEY];
	string cert = parameters[CONF_SSL_CERT];
	_pGlobalSSLContext = _pGlobalContexts[hash];

	//2. Initialize the global context based on the specified
	//key/cert pair if we don't have it
	if (_pGlobalSSLContext == NULL) {
		//3. prepare the global ssl context
		_pGlobalSSLContext = SSL_CTX_new(SSLv23_method());
		if (_pGlobalSSLContext == NULL) {
			FATAL("Unable to create global SSL context");
			return false;
		}

		//4. setup the certificate
		if (SSL_CTX_use_certificate_file(_pGlobalSSLContext, STR(cert),
				SSL_FILETYPE_PEM) <= 0) {
			FATAL("Unable to load certificate %s; Error(s) was: %s",
					STR(cert),
					STR(GetSSLErrors()));
			SSL_CTX_free(_pGlobalSSLContext);
			_pGlobalSSLContext = NULL;
			return false;
		}

		//5. setup the private key
		if (SSL_CTX_use_PrivateKey_file(_pGlobalSSLContext, STR(key),
				SSL_FILETYPE_PEM) <= 0) {
			FATAL("Unable to load key %s; Error(s) was: %s",
					STR(key),
					STR(GetSSLErrors()));
			SSL_CTX_free(_pGlobalSSLContext);
			_pGlobalSSLContext = NULL;
			return false;
		}

		//6. disable client certificate authentication
		SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_NONE, NULL);

		string cipherSuite = "";
		if (parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_CIPHERSUITE))
			cipherSuite = (string) parameters[CONF_SSL_CIPHERSUITE];
		trim(cipherSuite);
		if (cipherSuite != "") {
			INFO("Apply cipher suite `%s` on %s %s:%"PRIu16,
					STR(cipherSuite),
					STR(parameters[CONF_PROTOCOL]),
					STR(parameters[CONF_IP]),
					(uint16_t) parameters[CONF_PORT]
					);
			if (SSL_CTX_set_cipher_list(_pGlobalSSLContext, STR(cipherSuite)) == 0) {
				FATAL("Unable to apply cipher suite `%s`: Error was: `%s`",
						STR(cipherSuite),
						STR(GetSSLErrors()));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}
		}

		//7. Store the global context for later usage
		_pGlobalContexts[hash] = _pGlobalSSLContext;
		INFO("SSL server context initialized");
	}

	return true;
}

bool InboundSSLProtocol::DoHandshake() {
	if (_sslHandshakeCompleted)
		return true;

	int32_t errorCode = SSL_ERROR_NONE;

	errorCode = SSL_accept(_pSSL);
	if (errorCode < 0) {
		int32_t error = SSL_get_error(_pSSL, errorCode);
		if (error != SSL_ERROR_WANT_READ &&
				error != SSL_ERROR_WANT_WRITE) {
			FATAL("Unable to accept SSL connection: %d; %s",
					error, STR(GetSSLErrors()));
			return false;
		}
	}

	if (!PerformIO()) {
		FATAL("Unable to perform I/O");
		return false;
	}

	_sslHandshakeCompleted = SSL_is_init_finished(_pSSL);

	return true;
}

