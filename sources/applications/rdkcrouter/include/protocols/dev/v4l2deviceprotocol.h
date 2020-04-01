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


#ifndef _V4L2DEVICEPROTOCOL_H
#define _V4L2DEVICEPROTOCOL_H
#include "protocols/dev/basedeviceprotocol.h"

#define PT_V4L2 MAKE_TAG4('V','4','L','2')

namespace app_rdkcrouter {
	class DLLEXP V4L2DeviceProtocol
	: public BaseDeviceProtocol {
	public:
		V4L2DeviceProtocol();
		virtual ~V4L2DeviceProtocol();
		virtual bool Initialize(Variant &parameters);
	};
}

#endif	/* _V4L2DEVICEPROTOCOL_H */

