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
#ifndef _BASEINBOUNDHTTPPROTOCOL_H
#define	_BASEINBOUNDHTTPPROTOCOL_H

#include "protocols/http/basehttpprotocol.h"

class InboundHTTPProtocol
: public BaseHTTPProtocol {
private:
	Variant _requestHeaders;
	uint16_t _statusCode;
public:
	InboundHTTPProtocol();
	virtual ~InboundHTTPProtocol();

	void SetStatusCode(uint16_t statusCode);

	virtual bool Initialize(Variant &parameters);
	virtual string GetOutputFirstLine();
	virtual bool ParseFirstLine(string &line, Variant &headers);
	virtual bool Authenticate();
private:
	bool SendAuthRequired(Variant &auth);
};


#endif	/* _BASEINBOUNDHTTPPROTOCOL_H */
#endif /* HAS_PROTOCOL_HTTP */

