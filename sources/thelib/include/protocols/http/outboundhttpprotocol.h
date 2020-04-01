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
#ifndef _BASEOUTBOUNDHTTPPROTOCOL_H
#define	_BASEOUTBOUNDHTTPPROTOCOL_H

#include "protocols/http/basehttpprotocol.h"

class DLLEXP OutboundHTTPProtocol
: public BaseHTTPProtocol {
private:
	string _method;
	string _host;
	string _document;
public:
	OutboundHTTPProtocol();
	virtual ~OutboundHTTPProtocol();

	string Method();
	void Method(string method);
	string Document();
	void Document(string document);
	string Host();
	void Host(string host);
	virtual bool EnqueueForOutbound();

	virtual bool Is200OK();
protected:
	virtual string GetOutputFirstLine();
	virtual bool ParseFirstLine(string &line, Variant &firstLineHeader);
	virtual bool Authenticate();
};


#endif	/* _BASEOUTBOUNDHTTPPROTOCOL_H */
#endif /* HAS_PROTOCOL_HTTP */

