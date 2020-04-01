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


#include "protocols/axislicenseinterface/axislicenseinterfaceprotocol.h"
#include "application/axislicenseinterfaceapplication.h"
#include "protocols/http/outboundhttpprotocol.h"

using namespace app_axislicenseinterface;

AxisLicenseInterfaceProtocol::AxisLicenseInterfaceProtocol()
: BaseProtocol(PT_AXIS_LICENSE_INTERFACE) {
}

AxisLicenseInterfaceProtocol::~AxisLicenseInterfaceProtocol() {
}

bool AxisLicenseInterfaceProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_OUTBOUND_HTTP);
}

bool AxisLicenseInterfaceProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool AxisLicenseInterfaceProtocol::Initialize(Variant &config) {
	_customParameters = config;
	return true;
}

IOBuffer *AxisLicenseInterfaceProtocol::GetOutputBuffer() {
	return &_outputBuffer;
}

bool AxisLicenseInterfaceProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool AxisLicenseInterfaceProtocol::SignalInputData(IOBuffer &buffer) {
	string payload = string((char *) GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
	trim(payload);
	if (payload != "valid=yes") {
		FATAL("Invalid Credentials");
		((AxisLicenseInterfaceApplication *) GetApplication())->ShutdownServer();
	}
	EnqueueForDelete();
	return true;
}

bool AxisLicenseInterfaceProtocol::Send(Variant &variant) {
	OutboundHTTPProtocol *pHTTP = (OutboundHTTPProtocol *) _pFarProtocol;

	//8. We wish to disconnect after the transfer is complete
	pHTTP->SetDisconnectAfterTransfer(true);

	//9. This will always be a POST
	pHTTP->Method(HTTP_METHOD_GET);

	//10. Our document and the host
	pHTTP->Document(variant["document"]);
	pHTTP->Host(variant["host"]);

	return EnqueueForOutbound();
}
