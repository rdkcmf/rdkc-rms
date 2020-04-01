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


#ifndef _URI_H
#define	_URI_H

#include "platform/platform.h"
#include "utils/misc/variant.h"

class DLLEXP URI
: public Variant {
private:
	//static Variant _dummy;
public:
	VARIANT_GETSET(string, originalUri, "");
	VARIANT_GETSET(string, fullUri, "");
	VARIANT_GETSET(string, fullUriWithAuth, "");
	VARIANT_GETSET(string, scheme, "");
	VARIANT_GETSET(string, userName, "");
	VARIANT_GETSET(string, password, "");
	VARIANT_GETSET(string, host, "");
	VARIANT_GETSET(string, ip, "");
	VARIANT_GETSET(uint16_t, port, 0);
	VARIANT_GETSET(bool, portSpecified, false);
	VARIANT_GETSET(string, fullDocumentPathWithParameters, "");
	VARIANT_GETSET(string, fullDocumentPath, "");
	VARIANT_GETSET(string, fullParameters, "");
	VARIANT_GETSET(string, documentPath, "");
	VARIANT_GETSET(string, document, "");
	VARIANT_GETSET(string, documentWithFullParameters, "");
	VARIANT_GETSET(Variant, parameters, Variant());
	string baseURI();
	string derivedURI(string relativePath, bool includeParams);

	static bool FromVariant(Variant & variant, URI &uri);
	static bool FromString(string stringUri, bool resolveHost, URI &uri);
};

#endif	/* _URI_H */

