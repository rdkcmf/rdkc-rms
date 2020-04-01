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

#ifndef _AXISLICENSEINTERFACEPROTOCOL_H
#define _AXISLICENSEINTERFACEPROTOCOL_H
#include "protocols/http/httpinterface.h"
#include "protocols/baseprotocol.h"

#define PT_AXIS_LICENSE_INTERFACE	MAKE_TAG4('A','X','I','S')
namespace app_axislicenseinterface {

	class AxisLicenseInterfaceProtocol
	: public BaseProtocol {
	private:
		IOBuffer _outputBuffer;
	public:
		AxisLicenseInterfaceProtocol();
		~AxisLicenseInterfaceProtocol();

		virtual bool AllowFarProtocol(uint64_t type);
		virtual bool AllowNearProtocol(uint64_t type);
		virtual bool Initialize(Variant &config);
		virtual bool SignalInputData(int32_t recvAmount);
		virtual bool SignalInputData(IOBuffer &buffer);
		virtual IOBuffer *GetOutputBuffer();

		virtual bool Send(Variant &variant);
	};
}
#endif /* _AXISLICENSEINTERFACEPROTOCOL_H */
