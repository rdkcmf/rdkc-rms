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

#ifdef HAS_PROTOCOL_API
#include "api3rdparty/apiprotocolhandler.h"
#include "api3rdparty/apifdhandler.h"
#include "protocols/protocolmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "application/clientapplicationmanager.h"
#include "threading/threading.h"

ApiProtocolHandler::ApiProtocolHandler(Variant &configuration) : BaseAppProtocolHandler(configuration) {
	
}

ApiProtocolHandler::~ApiProtocolHandler() {
	
}

void ApiProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void ApiProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}


bool ApiProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	// Get the stream profile ID
	uint32_t streamId = atoi(STR(uri.fullDocumentPath()));
	if (streamId > MAX_STREAM_ID) {
		FATAL("Stream profile ID should be within 0 to 16! stream profile ID is %d",streamId);
		return false;
	}
	INFO("Stream profile ID: %" PRIu32, streamId);

	// Normalize the stream name
	string localStreamName = "";
	if (streamConfig["localStreamName"] == V_STRING) {
		localStreamName = (string) streamConfig["localStreamName"];
	}
	trim(localStreamName);
	if (localStreamName == "") {
		FATAL("No target stream name provided!");
		return false;
	}
	FINEST("Stream name: %s", STR(localStreamName));

	streamConfig["apiStreamProfileId"] = streamId;
	streamConfig["localStreamName"] = localStreamName;
	
	// Need to prepare the custom parameters for pull
	Variant parameters;
	parameters["customParameters"]["externalStreamConfig"] = streamConfig;
	parameters[CONF_APPLICATION_NAME] = GetApplication()->GetName();
	parameters[CONF_PROTOCOL] = CONF_PROTOCOL_API_RAW;
	
	// Initialize the API
	return Initialize(parameters);
}

bool ApiProtocolHandler::Initialize(Variant &config) {
	// Get the default application
	BaseClientApplication *pApp = ClientApplicationManager::GetDefaultApplication();
	
	if (pApp == NULL) {
		FATAL("Could not get default application!");
		return false;
	}

	INFO("Initializing SDK integration...");

	// Instantiate the protocol stack
	BaseProtocol *pNearProtocol = ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_API_RAW, config);
	if (pNearProtocol == NULL) {
		FATAL("Unable to create the protocol chain");
		return false;
	}
	
	// Set the application
	pNearProtocol->GetNearEndpoint()->SetApplication(pApp);
	
	// Get reference to the API protocol
	ApiProtocol *pProtocol = (ApiProtocol *) pNearProtocol->GetFarEndpoint();
	config["customParameters"]["externalStreamConfig"]["apiProtocolId"] = pProtocol->GetId();

#ifdef HAS_THREAD	
	// Start async initialization
	InitAsync(config["customParameters"]["externalStreamConfig"]);
#else
	// Initialize the API protocol directly
	if (!pProtocol->InitializeApi(config["customParameters"]["externalStreamConfig"])) {
		FATAL("API initialization failed!");
		return false;
	}

	CompleteInit(config["customParameters"]["externalStreamConfig"]);
	INFO("API initialization completed.");
#endif /* HAS_THREAD */
	return true;
}

#ifdef HAS_THREAD
void InitTask(Variant &params, void *requestingObject) {
    ApiProtocolHandler *pHandler = (ApiProtocolHandler *) requestingObject;
    TASK_STATUS(params) = pHandler->DoInitAsyncTask(params);
}

void InitCompletionTask(Variant &params, void *requestingObject) {
    ApiProtocolHandler *pHandler = (ApiProtocolHandler *) requestingObject;
    pHandler->CompleteInitAsyncTask(params);
}

bool ApiProtocolHandler::InitAsync(Variant &params) {
    return ThreadManager::QueueAsyncTask(&InitTask, &InitCompletionTask, params, (void *)this);
}

bool ApiProtocolHandler::DoInitAsyncTask(Variant &params) {
	ApiProtocol *pProtocol = (ApiProtocol *)ProtocolManager::GetProtocol((uint32_t)params["apiProtocolId"]);
	if (pProtocol == NULL) {
		FATAL("API Protocol is NULL!");
		return false;
	}
	
	if (pProtocol->InitializeApi(params)) {
		CompleteInit(params);
		return true;
	}

	FATAL("API initialization failed!");
	return false;
}

void ApiProtocolHandler::CompleteInitAsyncTask(Variant &params) {
	INFO("API initialization completed.");
}
#endif  /* HAS_THREAD */

void ApiProtocolHandler::CompleteInit(Variant &params) {
	ApiProtocol *pProtocol = (ApiProtocol *)ProtocolManager::GetProtocol((uint32_t)params["apiProtocolId"]);
	if (pProtocol == NULL) {
		FATAL("API Protocol is NULL!");
		return;
	}
	
	// Link the IO handler
	ApiFdHandler *pIoHandler = new ApiFdHandler(pProtocol->GetStreamFd());
	pIoHandler->SetProtocol(pProtocol);
	pProtocol->SetIOHandler(pIoHandler);
}

#endif	/* HAS_PROTOCOL_API */
