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


#ifndef _BASEDEVICEPROTOCOL_H
#define _BASEDEVICEPROTOCOL_H
#include "protocols/baseprotocol.h"

class BaseStream;
class DLLEXP BaseDeviceProtocol
: public BaseProtocol {
private:
	BaseStream *_pStream;
public:
	BaseDeviceProtocol(uint64_t type);
	virtual ~BaseDeviceProtocol();
	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
        void SetStream(BaseStream *pStream);
};

#endif	/* _BASEDEVICEPROTOCOL_H */

