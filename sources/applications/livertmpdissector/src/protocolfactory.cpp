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


#include "protocolfactory.h"
#include "protocols/protocoltypes.h"
#include "localdefines.h"
#include "rawtcpprotocol.h"
using namespace app_livertmpdissector;

ProtocolFactory::ProtocolFactory() {
}

ProtocolFactory::~ProtocolFactory() {
}

vector<uint64_t> ProtocolFactory::HandledProtocols() {
	vector<uint64_t> result;

	ADD_VECTOR_END(result, PT_RAW_TCP);

	return result;
}

vector<string> ProtocolFactory::HandledProtocolChains() {
	vector<string> result;

	ADD_VECTOR_END(result, CONF_PROTOCOL_RAW_TCP);

	return result;
}

vector<uint64_t> ProtocolFactory::ResolveProtocolChain(string name) {
	vector<uint64_t> result;
	if (name == CONF_PROTOCOL_RAW_TCP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_RAW_TCP);
	} else {
		FATAL("Invalid protocol chain: %s.", STR(name));
	}
	return result;
}

BaseProtocol *ProtocolFactory::SpawnProtocol(uint64_t type, Variant &parameters) {
	BaseProtocol *pResult = NULL;
	switch (type) {
		case PT_RAW_TCP:
			pResult = new RawTcpProtocol();
			break;
		default:
			FATAL("Spawning protocol %s not yet implemented",
					STR(tagToString(type)));
			break;
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
