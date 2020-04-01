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


#ifdef HAS_PROTOCOL_DRM

#include "protocols/drm/verimatrixdrmprotocol.h"

VerimatrixDRMProtocol::VerimatrixDRMProtocol()
: BaseDRMProtocol(PT_DRM) {
}

VerimatrixDRMProtocol::~VerimatrixDRMProtocol() {
}

bool VerimatrixDRMProtocol::Serialize(string &rawData, Variant &variant) {
	return variant.SerializeToBin(rawData);
}

bool VerimatrixDRMProtocol::Deserialize(uint8_t *pBuffer, uint32_t bufferLength,
		Variant &result) {
	result["key"] = string((char *) pBuffer, bufferLength);
	return true;
}

#endif	/* HAS_PROTOCOL_DRM */
