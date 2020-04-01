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





#include "protocols/protocolfactorymanager.h"
#include "protocols/baseprotocolfactory.h"
#include "protocols/baseprotocol.h"

map<uint32_t, BaseProtocolFactory *> ProtocolFactoryManager::_factoriesById;
map<uint64_t, BaseProtocolFactory *> ProtocolFactoryManager::_factoriesByProtocolId;
map<string, BaseProtocolFactory *> ProtocolFactoryManager::_factoriesByChainName;

bool ProtocolFactoryManager::RegisterProtocolFactory(BaseProtocolFactory *pFactory) {

	//1. Test to see if this factory is already registered
	if (MAP_HAS1(_factoriesById, pFactory->GetId())) {
		FATAL("Factory id %u already registered", pFactory->GetId());
		return false;
	}

	//2. Test to see if the protocol chains exported by this factory are already in use
	vector<string> protocolChains = pFactory->HandledProtocolChains();

	FOR_VECTOR(protocolChains, i) {
		if (MAP_HAS1(_factoriesByChainName, protocolChains[i])) {
			FATAL("protocol chain %s already handled by factory %u",
					STR(protocolChains[i]),
					_factoriesByChainName[protocolChains[i]]->GetId());
			return false;
		}
	}

	//3. Test to see if the protocols exported by this factory are already in use
	vector<uint64_t> protocols = pFactory->HandledProtocols();

	FOR_VECTOR(protocols, i) {
		if (MAP_HAS1(_factoriesByProtocolId, protocols[i])) {
			FATAL("protocol %"PRIx64" already handled by factory %u", protocols[i],
					_factoriesByProtocolId[protocols[i]]->GetId());
			return false;
		}
	}

	//4. Register everything

	FOR_VECTOR(protocolChains, i) {
		_factoriesByChainName[protocolChains[i]] = pFactory;
	}

	FOR_VECTOR(protocols, i) {
		_factoriesByProtocolId[protocols[i]] = pFactory;
	}

	_factoriesById[pFactory->GetId()] = pFactory;

	return true;
}

bool ProtocolFactoryManager::UnRegisterProtocolFactory(uint32_t factoryId) {
	if (!MAP_HAS1(_factoriesById, factoryId)) {
		WARN("Factory id not found: %u", factoryId);
		return true;
	}
	return UnRegisterProtocolFactory(_factoriesById[factoryId]);
}

bool ProtocolFactoryManager::UnRegisterProtocolFactory(BaseProtocolFactory *pFactory) {
	if (pFactory == NULL) {
		WARN("pFactory is NULL");
		return true;
	}

	if (!MAP_HAS1(_factoriesById, pFactory->GetId())) {
		WARN("Factory id not found: %u", pFactory->GetId());
		return true;
	}

	vector<string> protocolChains = pFactory->HandledProtocolChains();
	vector<uint64_t> protocols = pFactory->HandledProtocols();

	FOR_VECTOR(protocolChains, i) {
		_factoriesByChainName.erase(protocolChains[i]);
	}

	FOR_VECTOR(protocols, i) {
		_factoriesByProtocolId.erase(protocols[i]);
	}

	_factoriesById.erase(pFactory->GetId());

	return true;
}

vector<uint64_t> ProtocolFactoryManager::ResolveProtocolChain(string name) {
	if (!MAP_HAS1(_factoriesByChainName, name)) {
		FATAL("chain %s not registered by any protocol factory", STR(name));
		//FOR_MAP(_factoriesByChainName, string, BaseProtocolFactory*, i) {
		//	FATAL("$b2$: Have chain: %s", STR(MAP_KEY(i)));
		//}
		return vector<uint64_t > ();
	}

	return _factoriesByChainName[name]->ResolveProtocolChain(name);
}

BaseProtocol *ProtocolFactoryManager::CreateProtocolChain(string name,
		Variant &parameters) {
	vector<uint64_t> chain = ResolveProtocolChain(name);
	if (chain.size() == 0) {
		FATAL("Unable to create protocol chain");
		return NULL;
	}

	return CreateProtocolChain(chain, parameters);
}

BaseProtocol *ProtocolFactoryManager::CreateProtocolChain(vector<uint64_t> &chain,
		Variant &parameters) {
#ifdef HAS_LICENSE
#ifdef HAS_APP_LMINTERFACE
	//-1. First, do some verification with the license
	License * license = License::GetLicenseInstance();

	if (license->IsExpired()) {
		FATAL("YOUR SERVER LICENSE HAS EXPIRED! Please contact "BRANDING_COMPANY_NAME" for a new license! ");
		exit(1);
	}

	static int RMS_DEMO(200); //for demos, only allow this many protocols to be created
	static int hidden(0);
	if (license->IsDemo()) {
		static bool once(true);
		if (once) {
			SET_CONSOLE_TEXT_COLOR(FATAL_COLOR);
			fprintf(stdout, "This is a DEMO version of the "BRANDING_PRODUCT_NAME" !\n");
			fprintf(stdout, "You will be allowed approximately %"PRIu32" connections\n", (uint32_t) (RMS_DEMO / 2));
			fprintf(stdout, "We hope you enjoy the evaluation and consider buying a copy!\n");
			SET_CONSOLE_TEXT_COLOR(NORMAL_COLOR);
			once = false;
		}
		static int demo_prot_count(0);
		++demo_prot_count;
		++hidden;
		if (demo_prot_count >= RMS_DEMO) {
			SET_CONSOLE_TEXT_COLOR(FATAL_COLOR);
			fprintf(stdout, "You have reached the maximum number of connections for this DEMO version!\n");
			fprintf(stdout, "We hope that you have enjoyed using the "BRANDING_COMPANY_NAME"\n");
			fprintf(stdout, "Please consider purchasing a full copy!\n");
			SET_CONSOLE_TEXT_COLOR(NORMAL_COLOR);
			exit(1);
		}
	}
#endif /* HAS_APP_LMINTERFACE */
#endif /* HAS_LICENSE */

	//0. License checks out, continue on
	BaseProtocol *pResult = NULL;

	//1. Check and see if all the protocols are handled by a factory

	FOR_VECTOR(chain, i) {
		if (!MAP_HAS1(_factoriesByProtocolId, chain[i])) {
			//FATAL("protocol %"PRIx64" not handled by anyone", chain[i]);
			union {
				uint64_t xx;
				char c[9];
			}yy;
			yy.xx = EHTONLL(chain[i]);
			yy.c[8] = 0;	// terminate the string
			FATAL("protocol '%s' not handled!!", yy.c);
			return NULL;
		}
	}

	//2. Spawn the protocols

	bool failed = false;

	FOR_VECTOR(chain, i) {
		BaseProtocol *pProtocol = _factoriesByProtocolId[chain[i]]->SpawnProtocol(
				chain[i], parameters);
		if (pProtocol == NULL) {
			FATAL("Unable to spawn protocol %s handled by factory %u",
					STR(tagToString(chain[i])),
					_factoriesByProtocolId[chain[i]]->GetId());
			failed = true;
			break;
		}
		if (pResult != NULL)
			pResult->SetNearProtocol(pProtocol);

		pResult = pProtocol;
	}

	if (failed) {
		if (pResult != NULL)
			delete pResult->GetFarEndpoint();
		pResult = NULL;
	} else {
		pResult = pResult->GetNearEndpoint();
	}

#ifdef HAS_LICENSE
#ifdef HAS_APP_LMINTERFACE
	// Crash horribly if the demo limit has expired or if the license is expired.
	// This should NEVER happen unless someone is hacking our stuff!
	if (hidden >= RMS_DEMO || license->IsExpired()) {
		pResult = (BaseProtocol *) (((uint8_t *) pResult) + hidden);
	}
#endif /* HAS_APP_LMINTERFACE */
#endif /* HAS_LICENSE */
	return pResult;
}

string ProtocolFactoryManager::Dump() {
	string result = "Factories by id\n";

	FOR_MAP(_factoriesById, uint32_t, BaseProtocolFactory *, i) {
		result += format("\t%u\t%p\n", MAP_KEY(i), MAP_VAL(i));
	}

	result += "Factories by protocol id\n";

	FOR_MAP(_factoriesByProtocolId, uint64_t, BaseProtocolFactory *, i) {
		result += format("\t%s\t%p\n",
				STR(tagToString(MAP_KEY(i))),
				MAP_VAL(i));
	}

	result += "Factories by chain name\n";

	FOR_MAP(_factoriesByChainName, string, BaseProtocolFactory *, i) {
		result += format("\t%s\t%p\n",
				STR(MAP_KEY(i)),
				MAP_VAL(i));
	}

	return result;
}

