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

#ifndef __x509CERTIFICATE_H
#define __x509CERTIFICATE_H

#include "common.h"

class X509Certificate {
private:
	std::string _certPath;
	std::string _keyPath;

	EVP_PKEY *_pKey;
	RSA *_pRSA;
	X509 *_pX509;
	std::string _sha1FingerprintString;
	std::string _sha256FingerprintString;
private:
	X509Certificate(const std::string &certPath, const std::string &keyPath);
	X509Certificate();
public:
	virtual ~X509Certificate();
	static X509Certificate *GetInstance(const std::string &certPath,
			const std::string &keyPath);
	static X509Certificate *GetInstance();
	const std::string &GetCertPath() const;
	const std::string &GetKeyPath() const;
	const std::string &GetSHA1FingerprintString() const;
	const std::string &GetSHA256FingerprintString() const;
	static bool ComputeFingerprints(X509 *pX509, std::string *pSha1, std::string *pSha256);

	X509 *GetCertificateDuplicate() const;
	EVP_PKEY *GetCertificateKeyDuplicate() const;

	bool GetCertificate(X509** x509p);
	bool GetKey(EVP_PKEY**  evpkeyp);

private:
	bool Load();
	bool Create();
	bool Generate();
	bool Save();
	void Clear();
};

#endif // __x509CERTIFICATE_H

