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

#include "utils/misc/x509certificate.h"

#define X509_KEY_SIZE 1024

X509Certificate::X509Certificate(const std::string &certPath,
		const std::string &keyPath) {
	_certPath = certPath;
	_keyPath = keyPath;
	_pKey = NULL;
	_pRSA = NULL;
	_pX509 = NULL;
}

X509Certificate::X509Certificate() {
	_certPath = "";
	_keyPath = "";
	_pKey = NULL;
	_pRSA = NULL;
	_pX509 = NULL;
}


X509Certificate::~X509Certificate() {
	Clear();
}

X509Certificate *X509Certificate::GetInstance(const std::string &certPath,
		const std::string &keyPath) {
	X509Certificate *pResult = new X509Certificate(certPath, keyPath);
	if (pResult->Load()) {
		INFO("X509Certificate cert Loaded");
		return pResult;
	}
	if (!pResult->Create()) {
		FATAL("Unable to create X509 certificate");
		delete pResult;
		return NULL;
	}
	INFO("X509Certificate cert created");
	return pResult;
}

X509Certificate *X509Certificate::GetInstance() {
	X509Certificate *pResult = new X509Certificate();
	if (!pResult->Generate()) {
		FATAL("Unable to Generate X509 certificate");
		delete pResult;
		return NULL;
	}
	INFO("X509Certificate cert generated");
	return pResult;
}

const std::string &X509Certificate::GetCertPath() const {
	return _certPath;
}

const std::string &X509Certificate::GetKeyPath() const {
	return _keyPath;
}

const std::string &X509Certificate::GetSHA1FingerprintString() const {
	return _sha1FingerprintString;
}

const std::string &X509Certificate::GetSHA256FingerprintString() const {
	return _sha256FingerprintString;
}

bool X509Certificate::ComputeFingerprints(X509 *pX509, std::string *pSha1, std::string *pSha256) {
	if ((pSha1 == NULL)&&(pSha256 == NULL))
		return false;

	//get the DER format in memory
	BIO *pBIO = BIO_new(BIO_s_mem());
	if (i2d_X509_bio(pBIO, pX509) != 1) {
		DEBUG("Unable to save the X509 certificate to memory in DER format");
		BIO_free(pBIO);
		return false;
	}
	BUF_MEM *pBuffer;
	BIO_get_mem_ptr(pBIO, &pBuffer);

	//compute the sha1 and sha256 fingerprints for the certificate
	char byte[3];
	if (pSha1 != NULL) {
		*pSha1 = "";
		uint8_t _sha1Fingerprint[SHA_DIGEST_LENGTH];
		SHA1((unsigned char *) pBuffer->data, pBuffer->length, _sha1Fingerprint);
		for (size_t i = 0; i<sizeof (_sha1Fingerprint); i++) {
			sprintf(byte, "%02"PRIX8, _sha1Fingerprint[i]);
			*pSha1 += byte;
			if (i != (sizeof (_sha1Fingerprint) - 1))
				*pSha1 += ":";
		}
	}
	if (pSha256 != NULL) {
		*pSha256 = "";
		uint8_t _sha256Fingerprint[SHA256_DIGEST_LENGTH];
		SHA256((unsigned char *) pBuffer->data, pBuffer->length, _sha256Fingerprint);
		for (size_t i = 0; i<sizeof (_sha256Fingerprint); i++) {
			sprintf(byte, "%02"PRIX8, _sha256Fingerprint[i]);
			*pSha256 += byte;
			if (i != (sizeof (_sha256Fingerprint) - 1))
				*pSha256 += ":";
		}
	}

	BIO_free(pBIO);

	return true;
}

bool X509Certificate::GetCertificate(X509** x509p)
{
	if( NULL == _pX509){
		FATAL("_pX509 is NULL");
		return false;
	}
	*x509p = _pX509;
	return true;
}

bool X509Certificate::GetKey(EVP_PKEY**  evpkeyp)
{
	if( NULL == _pKey){
		FATAL("_pKey is NULL");
		return false;
	}
	*evpkeyp = _pKey;
	return true;
}

X509 *X509Certificate::GetCertificateDuplicate() const {
	if (_pX509 == NULL) {
		FATAL("_pX509 is NULL ");
		return NULL;
	}
	return X509_dup(_pX509);
}

EVP_PKEY *X509Certificate::GetCertificateKeyDuplicate() const {
	if (_pKey == NULL) {
		FATAL("_pKey is NULL ");
		return NULL;
	}

	//create a new BIO
	BIO *pBio = BIO_new(BIO_s_mem());

	//serialize the key
	if (PEM_write_bio_PrivateKey(pBio, _pKey, NULL, NULL, 0, NULL, NULL) != 1) {
		DEBUG("Unable to serialize key to BIO");
		BIO_free(pBio);
		return NULL;
	}

	//deserialize it into a new key
	EVP_PKEY *pResult = NULL;
	if ((PEM_read_bio_PrivateKey(pBio, &pResult, NULL, NULL) == NULL)
			|| (pResult == NULL)
			) {
		DEBUG("Unable to deserialize key from BIO");
		BIO_free(pBio);
		return NULL;
	}

	//free the BIO
	BIO_free(pBio);

	//done
	return pResult;
}

bool X509Certificate::Load() {
	//clear everything first
	Clear();

	//see if we need to save anything
	if ((_keyPath == "") || (_certPath == ""))
		return false;

	//open the file to read the key
	FILE *pFile = fopen(_keyPath.c_str(), "r");
	if (pFile == NULL) {
		DEBUG("Unable to open `%s` to read the X509 certificate key", _keyPath.c_str());
		return false;
	}

	//read the key
	void *pRes = PEM_read_PrivateKey(pFile, &_pKey, NULL, NULL);
	fclose(pFile);
	if (pRes == NULL) {
		DEBUG("Unable to read the X509 certificate key to `%s`", _keyPath.c_str());
		return false;
	}

	//open the file to read the certificate
	pFile = fopen(_certPath.c_str(), "r");
	if (pFile == NULL) {
		DEBUG("Unable to open `%s` to read the X509 certificate", _certPath.c_str());
		return false;
	}

	//read the certificate
	pRes = PEM_read_X509(pFile, &_pX509, NULL, NULL);
	fclose(pFile);
	if (pRes == NULL) {
		DEBUG("Unable to read the X509 certificate to `%s`", _certPath.c_str());
		return false;
	}

	//compute the fingerprints
	if (!ComputeFingerprints(_pX509, &_sha1FingerprintString, &_sha256FingerprintString)) {
		DEBUG("Unable to compute the X509 certificate fingerprints");
		return false;
	}

	return true;
}

bool X509Certificate::Generate() {
	if ( ( NULL != _pKey) && ( NULL != _pX509) ) {
		DEBUG("X509Certificate is already created");
		return true;
	}

	//create the keys container
	_pKey = EVP_PKEY_new();
	if (_pKey == NULL) {
		DEBUG("Unable to create the X509 certificate key container");
		return false;
	}

	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x00908000L
	//create the key itself which will be RSA
	_pRSA = RSA_generate_key(
			X509_KEY_SIZE, /* number of bits for the key - 2048 is a sensible value */
			RSA_F4, /* exponent - RSA_F4 is defined as 0x10001L */
			NULL, /* callback - can be NULL if we aren't displaying progress */
			NULL /* callback argument - not needed in this case */
			);
	if (_pRSA == NULL) {
		DEBUG("Unable to create the X509 RSA certificate key");
		return false;
	}
	#else
	BIGNUM* bne = BN_new();
    int ret = BN_set_word(bne, RSA_F4);
    if(!ret){
        DEBUG("Unable to seed the X509 RSA certificate key");
		return false;
    }

	_pRSA = RSA_new();
    ret = RSA_generate_key_ex(_pRSA, X509_KEY_SIZE, bne, NULL);
    if(!ret){
        DEBUG("Unable to create the X509 RSA certificate key");
		return false;
    }
	#endif

	//assign the key to the keys container
	if (EVP_PKEY_assign_RSA(_pKey, _pRSA) != 1) {
		DEBUG("Unable to assign the RSA key to the key container");
		return false;
	}

	//now we can make the _pRSA because it is safely inside the container
	//and it will be cleaned up along with the container
	_pRSA = NULL;

	//create the certificate
	_pX509 = X509_new();
	if (_pX509 == NULL) {
		DEBUG("Unable to create the X509 certificate");
		return false;
	}

	//set the public key
	if (X509_set_pubkey(_pX509, _pKey) != 1) {
		DEBUG("Unable to set the X509 certificate key");
		return false;
	}

	//set the serial number
	if (ASN1_INTEGER_set(X509_get_serialNumber(_pX509), 1) != 1) {
		DEBUG("Unable to set the X509 certificate serial number");
		return false;
	}

	//Redmine 2269
	//set validity period
	#if OPENSSL_API_COMPAT < 0x10100000L
	if ((X509_gmtime_adj(X509_get_notBefore(_pX509), -1 * 24 * 3600) == NULL)
			|| (X509_gmtime_adj(X509_get_notAfter(_pX509), 315360000L) == NULL)) {
		DEBUG("Unable to set the X509 certificate validity time period");
		return false;
	}
	#else
	if ((X509_gmtime_adj(X509_getm_notBefore(_pX509), -1 * 24 * 3600) == NULL)
			|| (X509_gmtime_adj(X509_getm_notAfter(_pX509), 315360000L) == NULL)) {
		DEBUG("Unable to set the X509 certificate validity time period");
		return false;
	}
	#endif

	//get the subject for the x509 cert
	X509_NAME *pSubjectProperties = X509_get_subject_name(_pX509);
	if (pSubjectProperties == NULL) {
		DEBUG("Unable to obtain the subject properties from the X509 certificate");
		return false;
	}

	//set the relevant properties on the subject
	if ((X509_NAME_add_entry_by_txt(pSubjectProperties, "C", MBSTRING_ASC, (unsigned char *) "CA", -1, -1, 0) != 1)
			|| (X509_NAME_add_entry_by_txt(pSubjectProperties, "O", MBSTRING_ASC, (unsigned char *) "RDKC Inc.", -1, -1, 0) != 1)
			|| (X509_NAME_add_entry_by_txt(pSubjectProperties, "CN", MBSTRING_ASC, (unsigned char *) "WebRTC", -1, -1, 0) != 1)) {
		DEBUG("Unable to set subject properties on the X509 certificate");
		return false;
	}

	//this will be self signed. So issuer == subject
	if (X509_set_issuer_name(_pX509, pSubjectProperties) != 1) {
		DEBUG("Unable to set issuer properties on the X509 certificate");
		return false;
	}

	//sign the certificate
	if (X509_sign(_pX509, _pKey, EVP_sha1()) == 0) {
		DEBUG("Unable to sign the X509 certificate");
		return false;
	}

	//compute the fingerprints
	if (!ComputeFingerprints(_pX509, &_sha1FingerprintString, &_sha256FingerprintString)) {
		DEBUG("Unable to compute the X509 certificate fingerprints");
		return false;
	}
	//PEM_write_PrivateKey(stdout,_pKey,NULL,NULL,0,0,NULL);
        //PEM_write_X509(stdout,_pX509);

	return true;
}

bool X509Certificate::Create() {
	//clear everything first
	Clear();

	//create the keys container
	_pKey = EVP_PKEY_new();
	if (_pKey == NULL) {
		DEBUG("Unable to create the X509 certificate key container");
		return false;
	}

	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x00908000L
	//create the key itself which will be RSA
	_pRSA = RSA_generate_key(
			X509_KEY_SIZE, /* number of bits for the key - 2048 is a sensible value */
			RSA_F4, /* exponent - RSA_F4 is defined as 0x10001L */
			NULL, /* callback - can be NULL if we aren't displaying progress */
			NULL /* callback argument - not needed in this case */
			);
	if (_pRSA == NULL) {
		DEBUG("Unable to create the X509 RSA certificate key");
		return false;
	}
	#else
	BIGNUM* bne = BN_new();
    int ret = BN_set_word(bne, RSA_F4);
    if(!ret){
        DEBUG("Unable to seed the X509 RSA certificate key");
		return false;
    }

	_pRSA = RSA_new();
    ret = RSA_generate_key_ex(_pRSA, X509_KEY_SIZE, bne, NULL);
    if(!ret){
        DEBUG("Unable to create the X509 RSA certificate key");
		return false;
    }
	#endif

	//assign the key to the keys container
	if (EVP_PKEY_assign_RSA(_pKey, _pRSA) != 1) {
		DEBUG("Unable to assign the RSA key to the key container");
		return false;
	}

	//now we can make the _pRSA because it is safely inside the container
	//and it will be cleaned up along with the container
	_pRSA = NULL;

	//create the certificate
	_pX509 = X509_new();
	if (_pX509 == NULL) {
		DEBUG("Unable to create the X509 certificate");
		return false;
	}

	//set the public key
	if (X509_set_pubkey(_pX509, _pKey) != 1) {
		DEBUG("Unable to set the X509 certificate key");
		return false;
	}

	//set the serial number
	if (ASN1_INTEGER_set(X509_get_serialNumber(_pX509), 1) != 1) {
		DEBUG("Unable to set the X509 certificate serial number");
		return false;
	}

	//Redmine 2269
	//set validity period
	#if OPENSSL_API_COMPAT < 0x10100000L
	if ((X509_gmtime_adj(X509_get_notBefore(_pX509), -1 * 24 * 3600) == NULL)
			|| (X509_gmtime_adj(X509_get_notAfter(_pX509), 31536000L) == NULL)) {
		DEBUG("Unable to set the X509 certificate validity time period");
		return false;
	}
	#else
	if ((X509_gmtime_adj(X509_getm_notBefore(_pX509), -1 * 24 * 3600) == NULL)
			|| (X509_gmtime_adj(X509_getm_notAfter(_pX509), 31536000L) == NULL)) {
		DEBUG("Unable to set the X509 certificate validity time period");
		return false;
	}
	#endif

	//get the subject for the x509 cert
	X509_NAME *pSubjectProperties = X509_get_subject_name(_pX509);
	if (pSubjectProperties == NULL) {
		DEBUG("Unable to obtain the subject properties from the X509 certificate");
		return false;
	}

	//set the relevant properties on the subject
	if ((X509_NAME_add_entry_by_txt(pSubjectProperties, "C", MBSTRING_ASC, (unsigned char *) "CA", -1, -1, 0) != 1)
			|| (X509_NAME_add_entry_by_txt(pSubjectProperties, "O", MBSTRING_ASC, (unsigned char *) "RDKC Inc.", -1, -1, 0) != 1)
			|| (X509_NAME_add_entry_by_txt(pSubjectProperties, "CN", MBSTRING_ASC, (unsigned char *) "WebRTC", -1, -1, 0) != 1)) {
		DEBUG("Unable to set subject properties on the X509 certificate");
		return false;
	}

	//this will be self signed. So issuer == subject
	if (X509_set_issuer_name(_pX509, pSubjectProperties) != 1) {
		DEBUG("Unable to set issuer properties on the X509 certificate");
		return false;
	}

	//sign the certificate
	if (X509_sign(_pX509, _pKey, EVP_sha1()) == 0) {
		DEBUG("Unable to sign the X509 certificate");
		return false;
	}

	//compute the fingerprints
	if (!ComputeFingerprints(_pX509, &_sha1FingerprintString, &_sha256FingerprintString)) {
		DEBUG("Unable to compute the X509 certificate fingerprints");
		return false;
	}

	//save the key/cert pair
	return Save();
}

bool X509Certificate::Save() {
	//see if we need to save anything
	if ((_keyPath == "")&&(_certPath == ""))
		return true;

	//ok, they are non-empty. They must be present both than
	if ((_keyPath == "") || (_certPath == "")) {
		DEBUG("Both key and cert path must be present");
		return false;
	}

	//open a file to save the key
	FILE *pFile = fopen(_keyPath.c_str(), "wb");
	if (pFile == NULL) {
		DEBUG("Unable to open `%s` to save the X509 certificate key", _keyPath.c_str());
		return false;
	}

	//save the key
	int res = PEM_write_PrivateKey(pFile, _pKey, NULL, NULL, 0, NULL, NULL);
	fclose(pFile);
	if (res != 1) {
		DEBUG("Unable to save the X509 certificate key to `%s`", _keyPath.c_str());
		return false;
	}

	//open a file to save the certificate
	pFile = fopen(_certPath.c_str(), "wb");
	if (pFile == NULL) {
		DEBUG("Unable to open `%s` to save the X509 certificate", _certPath.c_str());
		return false;
	}

	//save the certificate
	res = PEM_write_X509(pFile, _pX509);
	fclose(pFile);
	if (res != 1) {
		DEBUG("Unable to save the X509 certificate to `%s`", _certPath.c_str());
		return false;
	}

	return true;
}

void X509Certificate::Clear() {
	if (_pKey != NULL) {
		EVP_PKEY_free(_pKey);
		_pKey = NULL;
	}

	if (_pRSA != NULL) {
		RSA_free(_pRSA);
		_pRSA = NULL;
	}

	if (_pX509 != NULL) {
		X509_free(_pX509);
		_pX509 = NULL;
	}

	_sha1FingerprintString = "";
	_sha256FingerprintString = "";
	INFO("X509Certificate cert cleared");
}
