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
/**
 * outvmfprotocol - transport the Video Metafile psuedo-stream
 * 
 * ----------------------------------------------------
 **/
#include "protocols/vmf/outvmfprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/netio.h"
#include "application/originapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/vmf/outvmfstream.h"
#include "streaming/baseinstream.h"
#include "streaming/streamstypes.h"


OutVmfProtocol::OutVmfProtocol(uint64_t type)
: BaseProtocol(type) {

}

bool OutVmfProtocol::Initialize(Variant &parameters) {
	_streamName = (string) parameters["streamName"];
	INFO("OutVmfProtocol Init, pushstream: %s", STR(_streamName));
	return true;
}

bool OutVmfProtocol::SendMetadata(string jsonMetaData) {
	//INFO("$b2$: OutVmfProtocol::SendMetadata(len: %d) ", jsonMetaData.length());
	_outBuffer.ReadFromString(jsonMetaData);	// skip opening "{"
	// from outboundrtmpprotocol
	if (_pFarProtocol != NULL) {
		if (!_pFarProtocol->EnqueueForOutbound()) {
			FATAL("Unable to signal output data");
			return false;
		} else {
			//INFO("$b2$: OutVmfProtocol::SendMetadata() EnqueueForOutbound success");
		}
	}else{
		//FATAL("$b2$: OutVmfProtocol::SendMetadata: no Far Protocol!");
	}
	return true;
}

/**
 * static SignalProtocolCreated
 */
bool OutVmfProtocol::SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters) {
	//INFO("$b2$: OutVmfProtocol::SignalProtocolCreated called: %s", STR(*pProtocol));
	//INFO("$b2$: OVP:SPC parms are: %s", STR(customParameters.ToString()));
	if (!pProtocol) {
		FATAL("SignalProtocolCreated called with NULL Protocol!");
		return false;
	}
	pProtocol->Initialize(customParameters);
	OutVmfProtocol * that = (OutVmfProtocol *)pProtocol;	// when we need our object

	string streamName = customParameters["streamName"];
	//* uint32_t inId = customParameters["inStreamId"];
	string appName = customParameters[CONF_APPLICATION_NAME];
	// locate our Application
	app_rdkcrouter::OriginApplication * pApp = (app_rdkcrouter::OriginApplication * )
			ClientApplicationManager::FindAppByName(appName);
	if (!pApp) {
		FATAL("Unable to find App by name: %s", STR(appName));
	}
	pProtocol->SetApplication(pApp);
	// create an outstream
	that->_pOutStream = (BaseOutStream *)new OutVmfStream(pProtocol, ST_OUT_VMF, streamName);

	// NEW BEHAVIOR - now subscribe to MetadataManager (AddPushStream())
	that->_pMM = pApp->GetMetadataManager();
	if (!that->_pMM) {
		FATAL("App returned NULL pointer to MetadataManager");
		return false;
	}
	INFO("*b2*:OVP:SPC: call AddPushStream, streamname: %s", STR(streamName));
	that->_pMM->AddPushStream(streamName, that->_pOutStream);

	return true;
}

void OutVmfProtocol::EnqueueForDelete() {
	_pMM->RemovePushStream(_streamName, _pOutStream);
	BaseProtocol::EnqueueForDelete();
}
