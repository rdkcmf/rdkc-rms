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


#include "protocols/ssl/outboundsslprotocol.h"
#include <openssl/x509v3.h>
#define BEGIN_CERT "-----BEGIN CERTIFICATE-----"
#define BEGIN_CERT_LEN 27
#define END_CERT "-----END CERTIFICATE-----"
#define END_CERT_LEN 25

uint32_t OutboundSSLProtocol::_clientConCounter = 0;

OutboundSSLProtocol::OutboundSSLProtocol()
: BaseSSLProtocol(PT_OUTBOUND_SSL) {

}

OutboundSSLProtocol::~OutboundSSLProtocol() {
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

bool OutboundSSLProtocol::InitGlobalContext(Variant &parameters) {
	//1. get the hash which is always the same for client connetions
	++_clientConCounter;
	_hash = format("clientConnection%" PRIu32, _clientConCounter);
	if (_clientConCounter == 0xffffffff)
		_clientConCounter = 0;

	//_pGlobalSSLContext = _pGlobalContexts[_hash];
	if (_pGlobalSSLContext == NULL) {
		//2. prepare the global ssl context

		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		_pGlobalSSLContext = SSL_CTX_new(TLSv1_method());
		#else
		_pGlobalSSLContext = SSL_CTX_new(TLS_method());
		#endif

		if (_pGlobalSSLContext == NULL) {
			FATAL("Unable to create global SSL context");
			return false;
		}

		//3. Set verify location for server certificate authentication, if any
		if (parameters.HasKeyChain(V_STRING, false, 1, "trustedCerts")) {
			string tmp;
			File certsFile;
			certsFile.Initialize((string)parameters.GetValue("trustedCerts", false), FILE_OPEN_MODE_READ);
			certsFile.ReadAll(tmp);
			certsFile.Close();
			if (tmp.size() > 0) {
				Variant serverCerts;
				size_t found = tmp.find(BEGIN_CERT);
				while (found != string::npos) {
					size_t end = tmp.find(END_CERT, found);
					if (end == string::npos) {
						break;
					}
					size_t begin = found + BEGIN_CERT_LEN;
					string cert = tmp.substr(begin, end - begin);
					replace(cert, "\r", "");
					replace(cert, "\n", "");
					serverCerts.PushToArray(unb64(cert));
					found = tmp.find(BEGIN_CERT, end);
				}
				if (serverCerts.MapSize() > 0) {
					parameters["serverCerts"] = serverCerts;
				}
			}
		}
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
					return false;
				}
				X509_free(x509ServerCert);
				BIO_free_all(bio);

				SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_PEER, NULL);
			}
		} else if (parameters.HasKeyChain(V_MAP, false, 1, "serverCerts")) {
			Variant certs = parameters["serverCerts"];
			X509_STORE * x509Ctx = SSL_CTX_get_cert_store(_pGlobalSSLContext);
			string hostname = parameters.HasKey("host", false) ? parameters["host"] : "";
			if (certs.IsArray() && x509Ctx != NULL) {
				X509_VERIFY_PARAM *param = SSL_CTX_get0_param(_pGlobalSSLContext);
				X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
				X509_VERIFY_PARAM_set1_host(param, STR(hostname), 0);
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
				SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_PEER, NULL);
			}
		}
		
		string key;
		string cert;
		if ((parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_KEY))
				&&(parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_CERT))) {
			key = (string) parameters.GetValue(CONF_SSL_KEY, false);
			cert = (string) parameters.GetValue(CONF_SSL_CERT, false);
		}

		// Flag to disable client side certs
		bool ignoreCerts = false;
		if (parameters.HasKeyChain(V_BOOL, false, 1, "ignoreCertsForOutbound")) {
			ignoreCerts = (bool) (parameters.GetValue("ignoreCertsForOutbound", false));
		}

		//4. Load client key and certificate, if any
		if ((key != "")&&(cert != "") && (ignoreCerts == false)) {
			//5. Setup certificate from string
			if (SSL_CTX_use_certificate_ASN1(_pGlobalSSLContext, (int) cert.size(),
					(unsigned char *) cert.data()) <= 0) {
				FATAL("Unable to load certificate %s; Error(s) was: %s",
						STR(cert),
						STR(GetSSLErrors()));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}

			//6. Setup private key
			if (SSL_CTX_use_RSAPrivateKey_ASN1(_pGlobalSSLContext,
					(unsigned char *) key.data(), (long) key.size()) <= 0) {
				FATAL("Unable to load key %s; Error(s) was: %s",
						STR(key),
						STR(GetSSLErrors()));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}

		}
		//7. Store the global context for later usage
		//_pGlobalContexts[_hash] = _pGlobalSSLContext;
	}
	return true;
}

bool OutboundSSLProtocol::DoHandshake() {
	if (_sslHandshakeCompleted)
		return true;
	int32_t errorCode = SSL_ERROR_NONE;
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

	if (_sslHandshakeCompleted)
		return EnqueueForOutbound();

	return true;
}
