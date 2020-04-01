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


#include "protocols/axislicenseprotocolfactory.h"
#include "protocols/protocoltypes.h"
#include "protocols/axislicenseinterface/axislicenseinterfaceprotocol.h"

using namespace app_axislicenseinterface;

AxisLicenseProtocolFactory::AxisLicenseProtocolFactory() {
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE], PT_OUTBOUND_SSL);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE], PT_OUTBOUND_HTTP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTPS_AXIS_LICENSE_INTERFACE], PT_AXIS_LICENSE_INTERFACE);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTP_AXIS_LICENSE_INTERFACE], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTP_AXIS_LICENSE_INTERFACE], PT_OUTBOUND_HTTP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_HTTP_AXIS_LICENSE_INTERFACE], PT_AXIS_LICENSE_INTERFACE);
}

AxisLicenseProtocolFactory::~AxisLicenseProtocolFactory() {
}

vector<uint64_t> AxisLicenseProtocolFactory::HandledProtocols() {
	vector<uint64_t> result;
	ADD_VECTOR_END(result, PT_AXIS_LICENSE_INTERFACE);
	return result;
}

vector<string> AxisLicenseProtocolFactory::HandledProtocolChains() {
	vector<string> result;

	FOR_MAP(_resolvedChains, string, vector<uint64_t>, i) {
		ADD_VECTOR_END(result, MAP_KEY(i));
	}
	return result;
}

vector<uint64_t> AxisLicenseProtocolFactory::ResolveProtocolChain(string name) {
	if (!MAP_HAS1(_resolvedChains, name)) {
		FATAL("Invalid protocol chain: %s.", STR(name));
		return vector<uint64_t > ();
	}
	return _resolvedChains[name];
}

BaseProtocol *AxisLicenseProtocolFactory::SpawnProtocol(uint64_t type, Variant &parameters) {
	BaseProtocol *pResult = NULL;
	switch (type) {
		case PT_AXIS_LICENSE_INTERFACE:
			pResult = new AxisLicenseInterfaceProtocol();
			break;
		default:
		{
			FATAL("Spawning protocol %s not yet implemented",
					STR(tagToString(type)));
			break;
		}
	}
	if (pResult != NULL) {
		if (!pResult->Initialize(parameters)) {
			FATAL("Unable to initialize protocol %s",
					STR(tagToString(type)));
			delete pResult;
			pResult = NULL;
		}
	}
	return pResult;
}
