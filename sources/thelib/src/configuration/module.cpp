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


#include "configuration/module.h"
#include "protocols/baseprotocolfactory.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "eventlogger/eventlogger.h"
#include "netio/netio.h"

Module::Module() {
	getApplication = NULL;
	getFactory = NULL;
	pApplication = NULL;
	pFactory = NULL;
	libHandler = NULL;
}

void Module::Release() {
	config.Reset();

	if (pFactory != NULL) {
		ProtocolFactoryManager::UnRegisterProtocolFactory(pFactory);
		delete pFactory;
		pFactory = NULL;
	}

	if (libHandler != NULL) {
		FREE_LIBRARY(libHandler);
		libHandler = NULL;
	}
}

bool Module::Load() {
	if (getApplication == NULL) {
		if (!LoadLibrary()) {
			FATAL("Unable to load module library");
			return false;
		}
	}

	return true;
}

bool Module::LoadLibrary() {
	string path = config[CONF_APPLICATION_LIBRARY];
	libHandler = LOAD_LIBRARY(STR(path), LOAD_LIBRARY_FLAGS);
	if (libHandler == NULL) {
		string strError = OPEN_LIBRARY_ERROR;
		FATAL("Unable to open library %s. Error was: %s", STR(path),
				STR(strError));
		return false;
	}

	string functionName = (string) config[CONF_APPLICATION_INIT_APPLICATION_FUNCTION];
	getApplication = (GetApplicationFunction_t) GET_PROC_ADDRESS(libHandler,
			STR(functionName));
	if (getApplication == NULL) {
		string strError = OPEN_LIBRARY_ERROR;
		FATAL("Unable to find %s function. Error was: %s", STR(functionName),
				STR(strError));
		return false;
	}

	functionName = (string) config[CONF_APPLICATION_INIT_FACTORY_FUNCTION];
	getFactory = (GetFactoryFunction_t) GET_PROC_ADDRESS(libHandler,
			STR(functionName));

	INFO("Module %s loaded", STR(path));
	return true;
}

bool Module::ConfigFactory() {
	if (getFactory != NULL) {
		pFactory = getFactory(config);
		if (pFactory != NULL) {
			if (!ProtocolFactoryManager::RegisterProtocolFactory(pFactory)) {
				FATAL("Unable to register factory exported by application %s",
						STR(config[CONF_APPLICATION_NAME]));
				return false;
			}
			INFO("Loaded factory from application %s", STR(config[CONF_APPLICATION_NAME]));
		}
	}
	return true;
}

bool Module::BindAcceptors() {

	FOR_MAP(config[CONF_ACCEPTORS], string, Variant, i) {
		if (!BindAcceptor(MAP_VAL(i))) {
			FATAL("Unable to configure acceptor:\n%s", STR(MAP_VAL(i).ToString()));
			return false;
		}
	}
	return true;
}

bool Module::BindAcceptor(Variant &node) {
	//1. Get the chain
	vector<uint64_t> chain;
	chain = ProtocolFactoryManager::ResolveProtocolChain(node[CONF_PROTOCOL]);
	if (chain.size() == 0) {
		WARN("Invalid protocol chain: %s", STR(node[CONF_PROTOCOL]));
		return true;
	}

	//2. Is it TCP or UDP based?
	if (chain[0] == PT_TCP) {
		//3. This is a tcp acceptor. Instantiate it and start accepting connections
		TCPAcceptor *pAcceptor = new TCPAcceptor(node[CONF_IP],
				node[CONF_PORT], node, chain);
		if (!pAcceptor->Bind()) {
			FATAL("Unable to fire up acceptor from this config node: %s",
					STR(node.ToString()));
			return false;
		}
		ADD_VECTOR_END(acceptors, pAcceptor);
		return true;
	} else if (chain[0] == PT_UDP) {
		//4. Ok, this is an UDP acceptor. Because of that, we can instantiate
		//the full stack. Get the stack first
		BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(
				chain, node);
		if (pProtocol == NULL) {
			FATAL("Unable to instantiate protocol stack %s", STR(node[CONF_PROTOCOL]));
			return false;
		}

		//5. Create the carrier and bind it
		UDPCarrier *pUDPCarrier = UDPCarrier::Create(node[CONF_IP], node[CONF_PORT],
				pProtocol, 256, 256, "");
		if (pUDPCarrier == NULL) {
			FATAL("Unable to instantiate UDP carrier on %s:%hu",
					STR(node[CONF_IP]), (uint16_t) node[CONF_PORT]);
			pProtocol->EnqueueForDelete();
			return false;
		}
		pUDPCarrier->SetParameters(node);
		ADD_VECTOR_END(acceptors, pUDPCarrier);

		//6. We are done
		return true;
#ifdef HAS_UDS
	} else if (chain[0] == PT_UDS) {
		UDSAcceptor *pAcceptor = new UDSAcceptor(node[CONF_PATH], node, chain);
		if (!pAcceptor->Bind()) {
			FATAL("Unable to fire up acceptor from this config node: %s",
					STR(node.ToString()));
			return false;
		}
		ADD_VECTOR_END(acceptors, pAcceptor);
		return true;
#endif	/* HAS_UDS */
	} else {
		FATAL("Invalid carrier type");
		return false;
	}
}

bool Module::ConfigApplication() {
	string path = (string) config[CONF_APPLICATION_LIBRARY];
	if (getApplication == NULL) {
		WARN("Module %s doesn't export any applications", STR(path));
		return true;
	}

	pApplication = getApplication(config);
	if (pApplication == NULL) {
		FATAL("Unable to load application %s.", STR(config[CONF_APPLICATION_NAME]));
		return false;
	}
	INFO("Application %s instantiated", STR(pApplication->GetName()));
	if (!ClientApplicationManager::RegisterApplication(pApplication)) {
		FATAL("Unable to register application %s", STR(pApplication->GetName()));
		delete pApplication;
		pApplication = NULL;
		return false;
	}
	if (!pApplication->Initialize()) {
		FATAL("Unable to initialize the application: %s", STR(pApplication->GetName()));
		return false;
	} else {
		EventLogger *pEvtLog = pApplication->GetEventLogger();
		if (pEvtLog != NULL)
			pEvtLog->LogApplicationStart(pApplication);
	}

	if (!pApplication->ParseAuthentication()) {
		FATAL("Unable to parse authetication for application %s", STR(pApplication->GetName()));
		return false;
	}

	if (!pApplication->ActivateAcceptors(acceptors)) {
		FATAL("Unable to activate acceptors for application %s", STR(pApplication->GetName()));
		return false;
	}

	return true;
}

