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



#ifdef HAS_PROTOCOL_VAR
#ifndef _BINVARIANTPROTOCOL_H
#define	_BINVARIANTPROTOCOL_H

#include "protocols/variant/basevariantprotocol.h"

class DLLEXP BinVariantProtocol
: public BaseVariantProtocol {
public:
	BinVariantProtocol();
	virtual ~BinVariantProtocol();
protected:
	virtual bool Serialize(string &rawData, Variant &variant);
	virtual bool Deserialize(uint8_t *pBuffer, uint32_t bufferLength,
			Variant &result);
};


#endif	/* _BINVARIANTPROTOCOL_H */
#endif	/* HAS_PROTOCOL_VAR */

