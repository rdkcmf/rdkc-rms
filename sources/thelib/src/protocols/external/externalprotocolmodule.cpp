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


#ifdef HAS_PROTOCOL_EXTERNAL
#include "protocols/external/externalprotocolmodule.h"
#include "protocols/external/externalmoduleapi.h"
#include "protocols/external/externalprotocol.h"
#include "application/baseclientapplication.h"
#include "protocols/external/externalappprotocolhandler.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"

ExternalProtocolModule::ExternalProtocolModule(Variant &config) {
	_config = config;
	_libHandler = NULL;
	_init = NULL;
	_getProtocolFactory = NULL;
	_releaseProtocolFactory = NULL;
	_release = NULL;
	_pFactory = NULL;
	_initCalled = false;
	_pHandler = NULL;
}

ExternalProtocolModule::~ExternalProtocolModule() {
	_init = NULL;
	_getProtocolFactory = NULL;
	if (_libHandler == NULL) {
		_releaseProtocolFactory = NULL;
		_release = NULL;
		_pFactory = NULL;
		_initCalled = false;
		return;
	}

	if ((_pFactory != NULL) && (_releaseProtocolFactory != NULL)) {
		_releaseProtocolFactory(_pFactory);
		delete (BaseIC *) _pFactory->context.pOpaque;
		delete _pFactory;
	}
	_pFactory = NULL;
	_releaseProtocolFactory = NULL;

	if ((_release != NULL) && _initCalled) {
		_release();
	}
	_release = NULL;
	_initCalled = false;


	FREE_LIBRARY(_libHandler);
	_libHandler = NULL;
}

void ExternalProtocolModule::UnRegisterExternalModuleProtocolHandlers(
		BaseClientApplication *pApp) {
	if (_pFactory == NULL)
		return;
	for (uint32_t i = 0; i < _pFactory->supportedProtocolSize; i++) {
		pApp->UnRegisterAppProtocolHandler(_pFactory->pSupportedProtocols[i]);
	}
	delete _pHandler;
	_pHandler = NULL;
}

bool ExternalProtocolModule::Init() {
	//1. Minimal validation
	if ((!_config.HasKeyChain(V_STRING, false, 1, "file"))
			|| (!_config.HasKeyChain(V_STRING, false, 1, "getProtocolFactoryFunctionName"))
			|| (!_config.HasKeyChain(V_STRING, false, 1, "initFunctionName"))
			|| (!_config.HasKeyChain(V_STRING, false, 1, "releaseFunctionName"))
			|| (!_config.HasKeyChain(V_STRING, false, 1, "releaseProtocolFactoryFunctionName"))) {
		FATAL("Invalid module config: %s", STR(_config.ToString()));
		return false;
	}

	//2. Get the file name
	_file = (string) _config.GetValue("file", false);
	if (!fileExists(_file)) {
		FATAL("Module file not found: %s", STR(_file));
		return false;
	}

	//3. Get the function names
	string initFunctionName = (string) _config.GetValue("initFunctionName", false);
	string getProtocolFactoryFunctionName = (string) _config.GetValue("getProtocolFactoryFunctionName", false);
	string releaseProtocolFactoryFunctionName = (string) _config.GetValue("releaseProtocolFactoryFunctionName", false);
	string releaseFunctionName = (string) _config.GetValue("releaseFunctionName", false);

	//4. Load the library
	_libHandler = LOAD_LIBRARY(STR(_file), LOAD_LIBRARY_FLAGS);
	if (_libHandler == NULL) {
		string strError = OPEN_LIBRARY_ERROR;
		FATAL("Unable to open library %s. Error was: %s", STR(_file),
				STR(strError));
		return false;
	}

	//5. Load init function
	if (!LoadFunction(_file, initFunctionName, (void **) &_init)) {
		FATAL("Unable to load init function named %s", STR(initFunctionName));
		return false;
	}
	if (!LoadFunction(_file, getProtocolFactoryFunctionName, (void **) &_getProtocolFactory)) {
		FATAL("Unable to load getProtocolFactory function named %s", STR(getProtocolFactoryFunctionName));
		return false;
	}
	if (!LoadFunction(_file, releaseProtocolFactoryFunctionName, (void **) &_releaseProtocolFactory)) {
		FATAL("Unable to load releaseProtocolFactory function named %s", STR(releaseProtocolFactoryFunctionName));
		return false;
	}
	if (!LoadFunction(_file, releaseFunctionName, (void **) &_release)) {
		FATAL("Unable to load release function named %s", STR(releaseFunctionName));
		return false;
	}

	_init();
	_initCalled = true;

	_pFactory = new protocol_factory_t();
	memset(_pFactory, 0, sizeof (protocol_factory_t));

	_pFactory->version = 1;

	_getProtocolFactory(_pFactory);

	bool result = true;
	if (_pFactory->events.lib.initCompleted == NULL) {
		FATAL("Event protocol_factory_t.events.lib.initCompleted is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.connectSucceeded == NULL) {
		FATAL("Event protocol_factory_t.events.connection.connectSucceeded is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.connectFailed == NULL) {
		FATAL("Event protocol_factory_t.events.connection.connectFailed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.connectionAccepted == NULL) {
		FATAL("Event protocol_factory_t.events.connection.connectionAccepted is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.dataAvailable == NULL) {
		FATAL("Event protocol_factory_t.events.connection.dataAvailable is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.outputBufferCompletelySent == NULL) {
		FATAL("Event protocol_factory_t.events.connection.outputBufferCompletelySent is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.connection.connectionClosed == NULL) {
		FATAL("Event protocol_factory_t.events.connection.connectionClosed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.inStreamCreated == NULL) {
		FATAL("Event protocol_factory_t.events.stream.inStreamCreated is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.inStreamCreateFailed == NULL) {
		FATAL("Event protocol_factory_t.events.stream.inStreamCreateFailed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.inStreamClosed == NULL) {
		FATAL("Event protocol_factory_t.events.stream.inStreamClosed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.inStreamAttached == NULL) {
		FATAL("Event protocol_factory_t.events.stream.inStreamAttached is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.inStreamDetached == NULL) {
		FATAL("Event protocol_factory_t.events.stream.inStreamDetached is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamCreated == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamCreated is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamCreateFailed == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamCreateFailed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamClosed == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamClosed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamData == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamData is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamAttached == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamAttached is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.stream.outStreamDetached == NULL) {
		FATAL("Event protocol_factory_t.events.stream.outStreamDetached is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.job.timerCreated == NULL) {
		FATAL("Event protocol_factory_t.events.job.timerCreated is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.job.timerCreateFailed == NULL) {
		FATAL("Event protocol_factory_t::events.events.job.timerCreateFailed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.job.timerClosed == NULL) {
		FATAL("Event protocol_factory_t.events.job.timerClosed is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.job.timerPulse == NULL) {
		FATAL("Event protocol_factory_t.events.job.timerPulse is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.rtmp.clientConnected == NULL) {
		FATAL("Event protocol_factory_t.events.rtmp.clientConnected is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.rtmp.clientDisconnected == NULL) {
		FATAL("Event protocol_factory_t.events.rtmp.clientDisconnected is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.rtmp.clientOutBufferFull == NULL) {
		FATAL("Event protocol_factory_t.events.rtmp.clientOutBufferFull is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.rtmp.requestAvailable == NULL) {
		FATAL("Event protocol_factory_t.events.rtmp.requestAvailable is NULL. Module was %s", STR(_file));
		result &= false;
	}
	if (_pFactory->events.rtmp.responseAvailable == NULL) {
		FATAL("Event protocol_factory_t.events.rtmp.responseAvailable is NULL. Module was %s", STR(_file));
		result &= false;
	}

	return result;
}

bool ExternalProtocolModule::FinishInit(ExternalModuleAPI *pAPI, BaseClientApplication *pApp) {
	BaseIC *pIC = new BaseIC();
	pIC->pAPI = pAPI;
	pIC->pModule = this;
	pIC->pApp = pApp;
	_pFactory->context.pOpaque = pIC;
	pAPI->InitAPI(&_pFactory->api);
	_pFactory->events.lib.initCompleted(_pFactory);
	Variant dummy;
	_pHandler = new ExternalAppProtocolHandler(dummy, _pFactory);
	for (uint32_t i = 0; i < _pFactory->supportedProtocolSize; i++) {
		pApp->RegisterAppProtocolHandler(_pFactory->pSupportedProtocols[i], _pHandler);
	}

	BaseRTMPAppProtocolHandler *pRTMPHandler =
			(BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(PT_INBOUND_RTMP);
	if (pRTMPHandler == NULL) {
		pRTMPHandler = (BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(PT_OUTBOUND_RTMP);
		if (pRTMPHandler == NULL) {
			FATAL("Unable to get RTMP protocol handler");
			return false;
		}
	}

	pRTMPHandler->RegisterExternalModule(_pFactory);
	return true;
}

string ExternalProtocolModule::GetFile() {
	return _file;
}

vector<uint64_t> ExternalProtocolModule::GetHandledProtocols() {
	vector<uint64_t> result;
	for (uint32_t i = 0; i < _pFactory->supportedProtocolSize; i++) {
		ADD_VECTOR_END(result, _pFactory->pSupportedProtocols[i]);
	}
	return result;
}

map<string, vector<uint64_t> > ExternalProtocolModule::GetHandledProtocolChains() {
	map<string, vector<uint64_t> > result;
	for (uint32_t i = 0; i < _pFactory->supportedPorotocolChainSize; i++) {
		protocol_chain_t *pChain = &_pFactory->pSupportedPorotocolChains[i];
		if (MAP_HAS1(result, pChain->pName)) {
			ASSERT("Duplicate protocol chain name %s in module %s",
					pChain->pName, STR(_file));
		}
		for (uint32_t j = 0; j < pChain->chainSize; j++) {
			ADD_VECTOR_END(result[pChain->pName], pChain->pChain[j]);
		}
	}
	return result;
}

BaseProtocol *ExternalProtocolModule::SpawnProtocol(uint64_t type) {
	BaseProtocol *pResult = new ExternalProtocol(type, _pFactory);
	return pResult;
}

BaseAppProtocolHandler *ExternalProtocolModule::GetProtocolHandler(string &scheme) {
	for (uint32_t i = 0; i < _pFactory->supportedPorotocolChainSize; i++) {
		if (_pFactory->pSupportedPorotocolChains[i].pSchema == scheme)
			return _pHandler;
	}
	return NULL;
}

bool ExternalProtocolModule::LoadFunction(string &fileName,
		string &functionName, void **ppDest) {
	*ppDest = GET_PROC_ADDRESS(_libHandler, STR(functionName));
	if (*ppDest == NULL) {
		string strError = OPEN_LIBRARY_ERROR;
		FATAL("Unable to load function %s from library %s. Error was %s",
				STR(functionName), STR(fileName), STR(strError));
		return false;
	}
	return true;
}

#endif /* HAS_PROTOCOL_EXTERNAL */

