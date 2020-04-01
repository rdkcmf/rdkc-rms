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


#include "protocols/protocolfactory.h"
#include "protocols/protocoltypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "protocols/external/externalprotocolmodule.h"
#include "protocols/external/externalprotocol.h"
#include "protocols/nmea/inboundnmeaprotocol.h"
#include "protocols/metadata/jsonmetadataprotocol.h"
#include "protocols/vmf/outvmfprotocol.h"
#ifdef HAS_VMF
#include "protocols/vmfschemaprotocol.h"
#endif

using namespace app_rdkcrouter;

ProtocolFactory *ProtocolFactory::_pThis = NULL;

ProtocolFactory::ProtocolFactory() {
	_pThis = this;

	_handledProtocols[PT_PASSTHROUGH] = NULL;
    _handledProtocols[PT_INBOUND_NMEA] = NULL;

    _handledProtocols[PT_JSONMETADATA] = NULL;
#ifdef HAS_VMF
    _handledProtocols[PT_INBOUND_VMFSCHEMA] = NULL;
#endif
    _handledProtocols[PT_OUTBOUND_VMF] = NULL;

	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_UDP_PASSTHROUGH], PT_UDP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_UDP_PASSTHROUGH], PT_PASSTHROUGH);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_TCP_PASSTHROUGH], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_TCP_PASSTHROUGH], PT_PASSTHROUGH);
#ifdef HAS_PROTOCOL_NMEA
    ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_NMEA], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_NMEA], PT_INBOUND_NMEA);
#endif /* HAS_PROTOCOL_NMEA */        
#ifdef HAS_PROTOCOL_METADATA 
        ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_JSONMETADATA], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_JSONMETADATA], PT_JSONMETADATA);
        
        ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_HTTP_JSONMETADATA], PT_TCP);
        ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_HTTP_JSONMETADATA], PT_INBOUND_HTTP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_HTTP_JSONMETADATA], PT_JSONMETADATA);
        
#endif
#ifdef HAS_VMF
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_VMFSCHEMA], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_INBOUND_VMFSCHEMA], PT_INBOUND_VMFSCHEMA);
#endif /* HAS_PROTOCOL_METADATA  */        
    ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_OUTBOUND_VMF], PT_TCP);
	ADD_VECTOR_END(_resolvedChains[CONF_PROTOCOL_OUTBOUND_VMF], PT_OUTBOUND_VMF);
}

ProtocolFactory::~ProtocolFactory() {
#ifdef HAS_PROTOCOL_EXTERNAL
	for (uint32_t i = 0; i < _modules.size(); i++) {
		delete _modules[i];
	}
	_modules.clear();
#endif /* HAS_PROTOCOL_EXTERNAL */
}

vector<uint64_t> ProtocolFactory::HandledProtocols() {
	vector<uint64_t> result;

	FOR_MAP(_handledProtocols, uint64_t, __HPP, i) {
		ADD_VECTOR_END(result, MAP_KEY(i));
	}

	return result;
}

vector<string> ProtocolFactory::HandledProtocolChains() {
	vector<string> result;

	FOR_MAP(_resolvedChains, string, vector<uint64_t>, i) {
		ADD_VECTOR_END(result, MAP_KEY(i));
	}
	return result;
}

vector<uint64_t> ProtocolFactory::ResolveProtocolChain(string name) {
	if (!MAP_HAS1(_resolvedChains, name)) {
		FATAL("Invalid protocol chain: %s.", STR(name));
		return vector<uint64_t > ();
	}
	return _resolvedChains[name];
}

BaseProtocol *ProtocolFactory::SpawnProtocol(uint64_t type,
		Variant &parameters) {
	BaseProtocol *pResult = NULL;
	switch (type) {
	case PT_PASSTHROUGH:
		pResult = new PassThroughProtocol();
		break;
#ifdef HAS_PROTOCOL_NMEA
	case PT_INBOUND_NMEA:
		pResult = new InboundNMEAProtocol();
		break;
#endif //HAS_PROTOCOL_NMEA
#ifdef HAS_PROTOCOL_METADATA
	case PT_JSONMETADATA:
		pResult = new JsonMetadataProtocol();
		INFO("*b2*:ProtocolFactory: created JsonMetadataProtocol!");
		break;
#endif
#ifdef HAS_VMF
	case PT_INBOUND_VMFSCHEMA:
		pResult = new VmfSchemaProtocol();
		INFO("*b2*:ProtocolFactory: created VmfSchemaProtocol!");
		break;
#endif // HAS_PROTOCOL_METADATA
	case PT_OUTBOUND_VMF:
		pResult = new OutVmfProtocol();
		INFO("*b2*:ProtocolFactory: created OutVmfProtocol!");
		break;
	default: {
		__HPP pModule = _handledProtocols[type];
		if (pModule == NULL) {
			FATAL("Spawning protocol %s not yet implemented",
					STR(tagToString(type)));
			break;
		}
#ifdef HAS_PROTOCOL_EXTERNAL
		pResult = pModule->SpawnProtocol(type);
#endif /* HAS_PROTOCOL_EXTERNAL */
		break;
	}//end Switch
}
	if (pResult != NULL) {
		if (!pResult->Initialize(parameters)) {
			FATAL("Unable to initialize protocol %s", STR(tagToString(type)));
			delete pResult;
			pResult = NULL;
		}
	}
	return pResult;
}
#ifdef HAS_PROTOCOL_EXTERNAL

bool ProtocolFactory::LoadModule(Variant &config) {
	ExternalProtocolModule *pModule = new ExternalProtocolModule(config);
	if (!pModule->Init()) {
		delete pModule;
		return false;
	}

	vector<uint64_t> handledProtocols = pModule->GetHandledProtocols();
	string exportedProtocols;
	for (uint32_t i = 0; i < handledProtocols.size(); i++) {
		if (MAP_HAS1(_handledProtocols, handledProtocols[i])) {
			FATAL("Protocol %s already handled by someone else",
					STR(tagToString(handledProtocols[i])));
			return false;
		}
		_handledProtocols[handledProtocols[i]] = pModule;
		exportedProtocols += tagToString(handledProtocols[i]) + ",";
	}

	map<string, vector<uint64_t> > handledProtocolChains = pModule->GetHandledProtocolChains();
	string exportedProtocolChains;

	FOR_MAP(handledProtocolChains, string, vector<uint64_t>, i) {
		if (MAP_HAS1(_resolvedChains, MAP_KEY(i))) {
			FATAL("Protocol chain %s already handled by someone else",
					STR(MAP_KEY(i)));
			return false;
		}
		_resolvedChains[MAP_KEY(i)] = MAP_VAL(i);
		exportedProtocolChains += MAP_KEY(i) + ",";
	}

	INFO("Module %s loaded.\nExported protocols: %s\nExported protocol chains: %s",
			STR(pModule->GetFile()),
			STR(exportedProtocols),
			STR(exportedProtocolChains));

	ADD_VECTOR_END(_modules, pModule);

	return true;
}

bool ProtocolFactory::FinishInitModules(ExternalModuleAPI *pAPI,
		BaseClientApplication *pApp) {
	for (uint32_t i = 0; i < _pThis->_modules.size(); i++) {
		if (!_pThis->_modules[i]->FinishInit(pAPI, pApp)) {
			FATAL("Module failed");
			return false;
		}
	}
	return true;
}

void ProtocolFactory::UnRegisterExternalModuleProtocolHandlers(BaseClientApplication *pApp) {
	for (uint32_t i = 0; i < _pThis->_modules.size(); i++) {
		_pThis->_modules[i]->UnRegisterExternalModuleProtocolHandlers(pApp);
	}
}

BaseAppProtocolHandler *ProtocolFactory::GetProtocolHandler(string &scheme) {
	for (uint32_t i = 0; i < _pThis->_modules.size(); i++) {
		BaseAppProtocolHandler *pResult = _pThis->_modules[i]->GetProtocolHandler(scheme);
		if (pResult != NULL) {
			return pResult;
		}
	}
	return NULL;
}
#endif /* HAS_PROTOCOL_EXTERNAL */
