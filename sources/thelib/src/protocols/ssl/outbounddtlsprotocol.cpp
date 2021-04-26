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

#include "protocols/ssl/outbounddtlsprotocol.h"


OutboundDTLSProtocol::OutboundDTLSProtocol()
: BaseSSLProtocol(PT_OUTBOUND_DTLS) {
	_isStarted = false;
}

OutboundDTLSProtocol::OutboundDTLSProtocol(X509Certificate *pCertificate)
: BaseSSLProtocol(PT_OUTBOUND_DTLS) {
	_isStarted = false;
	_pCertificate = pCertificate;
}

OutboundDTLSProtocol::~OutboundDTLSProtocol() {
	map<string, SSL_CTX*>::iterator it = _pGlobalContexts.find(_hash);
	if(it != _pGlobalContexts.end()){
		_pGlobalContexts.erase(it);
	}

	if(_pGlobalSSLContext != NULL) {
		SSL_CTX_free(_pGlobalSSLContext);
		_pGlobalSSLContext = NULL;
	}
	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	ERR_remove_state(0);
	#endif
}

bool OutboundDTLSProtocol::InitGlobalContext(Variant &parameters) {
	//1. get the hash which is always the same for client connections
	bool useprebuiltcert = false;
	_hash = "DtlsClientConnection";
	useprebuiltcert = (bool) parameters["usePrebuiltCert"];
	_pGlobalSSLContext = _pGlobalContexts[_hash];

	if (_pGlobalSSLContext == NULL) {
		//2. prepare the global ssl context

		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		_pGlobalSSLContext = SSL_CTX_new(DTLSv1_client_method());
		#else
		_pGlobalSSLContext = SSL_CTX_new(DTLS_client_method());
		#endif

		if (_pGlobalSSLContext == NULL) {
			FATAL("Unable to create global SSL context");
			return false;
		}

		// Enable/disable the extensions
		SSL_CTX_clear_options(_pGlobalSSLContext, SSL_CTX_get_options(_pGlobalSSLContext));
		// Disable tickets, ssl versions, etc.
		//TODO
		//const long flags = SSL_OP_NO_TICKET | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_ALL | SSL_OP_CIPHER_SERVER_PREFERENCE;
		const long flags = SSL_OP_NO_TICKET;
		SSL_CTX_set_options(_pGlobalSSLContext, flags);
		// Enable use_srtp
		SSL_CTX_set_tlsext_use_srtp(_pGlobalSSLContext, "SRTP_AES128_CM_SHA1_80");

		//3. Set verify location for server certificate authentication, if any
		if (parameters.HasKeyChain(V_STRING, false, 1, "serverCert")) {
			string serverCertData = (string) parameters.GetValue("serverCert", true);
			X509_STORE * x509Ctx = SSL_CTX_get_cert_store(_pGlobalSSLContext);
			if ((x509Ctx != NULL)&&(serverCertData.size() != 0)) {
				BIO * bio = BIO_new(BIO_s_mem());
				BIO_write(bio, (const void *) serverCertData.data(),
						(int) serverCertData.length());
				X509 * x509ServerCert = d2i_X509_bio(bio, NULL);
				if (X509_STORE_add_cert(x509Ctx, x509ServerCert) == 0) {
					FATAL("Unable to load Server CA. Error(s): %s", STR(GetSSLErrors()));
					X509_free(x509ServerCert);
					BIO_free_all(bio);
					return false;
				}
				X509_free(x509ServerCert);
				BIO_free_all(bio);
				SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_PEER, NULL);
			}
		} else if (parameters.HasKeyChain(V_MAP, false, 1, "serverCerts")) {
			Variant certs = parameters["serverCerts"];
			X509_STORE * x509Ctx = SSL_CTX_get_cert_store(_pGlobalSSLContext);

			if (certs.IsArray() && x509Ctx != NULL) {
				for (uint32_t i = 0; i < certs.MapSize(); i++) {
					string servCert = certs[i];
					if (servCert.size() != 0) {
						BIO * bio = BIO_new(BIO_s_mem());
						BIO_write(bio, (const void *) servCert.data(),
								(int) servCert.length());
						X509 * x509ServerCert = d2i_X509_bio(bio, NULL);
						if (X509_STORE_add_cert(x509Ctx, x509ServerCert) == 0) {
							FATAL("Unable to load Server CA. Error(s): %s", STR(GetSSLErrors()));
						}
						X509_free(x509ServerCert);
						BIO_free_all(bio);
					}
				}
			}
		}

		if ( useprebuiltcert ) {
			string key = "";
			string cert = "";

			if ((parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_KEY))
					&&(parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_CERT))) {
				key = (string) parameters.GetValue(CONF_SSL_KEY, false);
				cert = (string) parameters.GetValue(CONF_SSL_CERT, false);
			}

			//4. Load client key and certificate, if any
			if ((key != "") && (cert != "")) {
				// Load the certificate and key from file
				if (SSL_CTX_use_certificate_file(_pGlobalSSLContext, STR(cert), SSL_FILETYPE_PEM) <= 0) {
					FATAL("Unable to load certificate from file (%s)!", STR(cert));
					SSL_CTX_free(_pGlobalSSLContext);
					_pGlobalSSLContext = NULL;
					return false;
				}

				if (SSL_CTX_use_PrivateKey_file(_pGlobalSSLContext, STR(key), SSL_FILETYPE_PEM) <= 0) {
					FATAL("Unable to load private key from file (%s)!", STR(key));
					SSL_CTX_free(_pGlobalSSLContext);
					_pGlobalSSLContext = NULL;
					return false;
				}
			}
		} else {
                        if (SSL_CTX_use_certificate(_pGlobalSSLContext, _evpcert) != 1) {
                                FATAL("SSL_CTX_use_certificate(): %s.", ERR_error_string(ERR_get_error(), NULL));
                                SSL_CTX_free(_pGlobalSSLContext);
                                _pGlobalSSLContext = NULL;
                                return false;
                        }

			if (SSL_CTX_use_PrivateKey(_pGlobalSSLContext, _evpkey) != 1) {
				FATAL("SSL_CTX_use_PrivateKey(): %s.", ERR_error_string(ERR_get_error(), NULL));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}
		}

		//7. Store the global context for later usage
		_pGlobalContexts[_hash] = _pGlobalSSLContext;

		//TODO: Needed?
		// Set verification depth
		//SSL_CTX_set_verify_depth (_pGlobalSSLContext, 2);

		// Set cipher list
		SSL_CTX_set_cipher_list(_pGlobalSSLContext, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	}

	return true;
}

bool OutboundDTLSProtocol::DoHandshake() {
	if (_sslHandshakeCompleted || !_isStarted)
		return true;

	int32_t errorCode = SSL_ERROR_NONE;

	//TODO: needed?
	//SSL_set_tlsext_heartbeat_no_requests(_pSSL, 0);

	errorCode = SSL_connect(_pSSL);
	if (errorCode < 0) {
		int32_t error = SSL_get_error(_pSSL, errorCode);
		if (error != SSL_ERROR_WANT_READ &&
				error != SSL_ERROR_WANT_WRITE) {
			FATAL("Unable to connect SSL: %d; %s", error, STR(GetSSLErrors()));
			return false;
		}
	}

	_sslHandshakeCompleted = SSL_is_init_finished(_pSSL);
	
	if (!PerformIO()) {
		FATAL("Unable to perform I/O");
		return false;
	}

	if (_sslHandshakeCompleted) {
		FINE("DTLS handshake complete.");
		return EnqueueForOutbound();
	}

	return true;
}

bool OutboundDTLSProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	//TODO: Probably need to take note of the peer address?
	return BaseSSLProtocol::SignalInputData(buffer);
}

bool OutboundDTLSProtocol::Start() {
	_isStarted = true;
	return DoHandshake();
}
