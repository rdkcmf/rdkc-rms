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


#ifndef _HTTPAUTHHELPER_H
#define	_HTTPAUTHHELPER_H

#include "common.h"

class HTTPAuthHelper {
public:
	HTTPAuthHelper();
	virtual ~HTTPAuthHelper();

	static bool GetAuthorizationHeader(string wwwAuthenticateHeader,
			string username, string password, string uri, string method,
			Variant &result);
	static string GetWWWAuthenticateHeader(string type, string realmName);
	static bool ValidateAuthRequest(string rawChallange, string rawResponse,
			string method, string requestUri, Variant &realm);
private:
	static bool ParseAuthLine(string challenge, Variant &result, bool isResponse);
	static bool ValidateChallenge(Variant &challenge);
	static bool ValidateResponse(Variant &response);
	static bool GetAuthorizationHeaderBasic(Variant &result);
	static bool GetAuthorizationHeaderDigest(Variant &result);
	static string ComputeResponseMD5(string username, string password,
			string realm, string method, string uri, string nonce);
};



#endif	/* _HTTPAUTHHELPER_H */

