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



#ifndef _CRYPTO_H
#define	_CRYPTO_H

#include "platform/platform.h"
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/rc4.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/rsa.h>

/*!
	@class DHWrapper
	@brief Class that handles the DH wrapper
 */
class DLLEXP DHWrapper {
private:
	int32_t _bitsCount;
	DH *_pDH;
	uint8_t *_pSharedKey;
	int32_t _sharedKeyLength;
	BIGNUM *_peerPublickey;
public:
	DHWrapper(int32_t bitsCount);
	virtual ~DHWrapper();

	/*!
		@brief Initializes the DH wrapper
	 */
	bool Initialize();

	/*!
		@brief Copies the public key.
		@param pDst - Where the copied key is stored
		@param dstLength
	 */
	bool CopyPublicKey(uint8_t *pDst, int32_t dstLength);

	/*!
		@brief Copies the private key.
		@param pDst - Where the copied key is stored
		@param dstLength
	 */
	bool CopyPrivateKey(uint8_t *pDst, int32_t dstLength);

	/*!
		@brief Creates a shared secret key
		@param pPeerPublicKey
		@param length
	 */
	bool CreateSharedKey(uint8_t *pPeerPublicKey, int32_t length);

	/*!
		@brief Copies the shared secret key.
		@param pDst - Where the copied key is stored
		@param dstLength
	 */
	bool CopySharedKey(uint8_t *pDst, int32_t dstLength);
private:
	void Cleanup();
	bool CopyKey(BIGNUM *pNum, uint8_t *pDst, int32_t dstLength);
};

DLLEXP void InitRC4Encryption(uint8_t *secretKey, uint8_t *pubKeyIn, uint8_t *pubKeyOut,
		RC4_KEY *rc4keyIn, RC4_KEY *rc4keyOut);
DLLEXP string md5(string source, bool textResult);
DLLEXP string md5(uint8_t *pBuffer, uint32_t length, bool textResult);
DLLEXP void HMACsha256(const void *pData, uint32_t dataLength, const void *pKey,
		uint32_t keyLength, void *pResult);
DLLEXP void HMACsha1(const void *pData, uint32_t dataLength, const void *pKey,
		uint32_t keyLength, void *pResult);
DLLEXP string sha256(string source);
DLLEXP string b64(string source);
DLLEXP string b64(uint8_t *pBuffer, uint32_t length);
DLLEXP string unb64(string source);
DLLEXP string unb64(uint8_t *pBuffer, uint32_t length);
DLLEXP string hex(string source);
DLLEXP string hex(const uint8_t *pBuffer, uint32_t length);
DLLEXP string bits(string source);
DLLEXP string bits(const uint8_t *pBuffer, uint32_t length);
DLLEXP string unhex(string source);
DLLEXP string unhex(const uint8_t *pBuffer, uint32_t length);
DLLEXP void CleanupSSL();
DLLEXP uint32_t calcCrc32(const uint8_t * pBuf, uint32_t len);

#endif /* _CRYPTO_H */

