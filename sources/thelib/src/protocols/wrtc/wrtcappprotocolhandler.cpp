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
* wrtcconnection.cpp - create and funnel data through a WebRTC Connection
*
*  - 6/17/15
**/
#ifdef HAS_PROTOCOL_WEBRTC
#include "protocols/wrtc/wrtcappprotocolhandler.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"

map<uint32_t, BaseProtocol *> WrtcAppProtocolHandler::_instances;

WrtcAppProtocolHandler::WrtcAppProtocolHandler(Variant &configuration)
	: BaseAppProtocolHandler(configuration),
	  _hasUserAgent(false) {

#ifdef RTMSG
	 rtConnection_Create(&connectionSend, "RMS_SEND", "tcp://127.0.0.1:10001");
#endif

}

WrtcAppProtocolHandler::~WrtcAppProtocolHandler() {
	// $ToDo$

#ifdef RTMSG
	rtConnection_Destroy(connectionSend);
#endif

}

bool WrtcAppProtocolHandler::GetUserAgent(struct UserAgent &userAgent) {

	if (_hasUserAgent) {
		userAgent = _userAgent;
		INFO("Retrieving User Agent Details");
		return true;
	}
	else {
		FATAL("User Agent details are not available");
		return false;
	}
}

bool WrtcAppProtocolHandler::SetUserAgent() {
#ifdef RDKC
	char fw_name[FW_NAME_MAX_LENGTH] = {0};
	char device_mac[CAM_MAC_MAX_LENGTH] = {0};
	char version_num[VER_NUM_MAX_LENGTH] = {0};
#ifdef USE_MFRLIB
	char modelName[128] = {0};
#else
	char modelName[128] = SC_MODEL_NAME;
#endif

	//Retrieve the Firmware version
	if (getCameraFirmwareVersion(fw_name) != 0) {
		FATAL("ERROR in reading camera firmware version");
	}

	//Update the Firmware version
	string fwVersion(fw_name);
	_userAgent.fwVersion = fwVersion;
	INFO("The firmware image is %s", STR(_userAgent.fwVersion));

	//Retrieve the Device MAC
	if (getDeviceMacValue(device_mac) != 0) {
		FATAL("ERROR in reading camera MAC");
	}

	//Update the Device MAC
	string deviceMAC(device_mac);
	_userAgent.deviceMAC = deviceMAC;
	INFO("The device MAC is %s", STR(_userAgent.deviceMAC));

	//Retrieve the Version Number
	if (getCameraVersionNum(version_num) != 0) {
		FATAL("ERROR in reading camera image version number");
	}

	//Update the Version Number
	string versionNo(version_num);
	_userAgent.versionNo = versionNo;
	INFO("The device image version number is %s", STR(_userAgent.versionNo));

#ifdef USE_MFRLIB
	//Retrieve the model
        mfrSerializedData_t stdata = {NULL, 0, NULL};
        mfrSerializedType_t stdatatype = mfrSERIALIZED_TYPE_MODELNAME;
        if(mfrGetSerializedData(stdatatype, &stdata) == mfrERR_NONE) {
		strncpy(modelName,stdata.buf,stdata.bufLen);
		modelName[stdata.bufLen] = '\0';
                if (stdata.freeBuf != NULL) {
			stdata.freeBuf(stdata.buf);
			stdata.buf = NULL;
		}
	}
	else {
		FATAL("Error in reading the device model");
	}
#endif

	//Update the Model
	string deviceModel(modelName);
	_userAgent.deviceModel = deviceModel;
	INFO("The device model is %s", STR(_userAgent.deviceModel));

	//Update the Make
	_userAgent.deviceMake = "Sercomm";
	INFO("The device make is %s", STR(_userAgent.deviceMake));

#else //RDKC
	_userAgent.fwVersion = "not set: dev build";
	_userAgent.deviceMAC = "ab:cd:ef:gh:ij:kl";
	_userAgent.versionNo = "not set: dev build";
	_userAgent.deviceModel = "not set: dev build";
	_userAgent.deviceMake = "insert dev name here";
#endif

	return true;
}

bool WrtcAppProtocolHandler::StartWebRTC(Variant & settings, bool resolveRRS) {
	string rrs = settings["rrs"];
	uint16_t port = settings["rrsport"];
	string room = settings["roomid"];

	FINE("WebRTC Joiner Service Starting Up...");
	vector<uint64_t> chain;

	if ((bool)settings["rrsOverSsl"] == true) {
		chain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_WRTC_SIG_SSL);
		// Indicate to outbound ssl that we're not using client side certs
		settings["ignoreCertsForOutbound"] = (bool)true;

		// If we wish to support client side cert similar to RMS - LM, we need to:
		// 1. Read the cert the files as string
		// 2. Refactor outbound DTLS to read strings instead of files
	} else {
		chain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_WRTC_SIG);
	}

	if(!_hasUserAgent) {
		if(SetUserAgent()) {
			_hasUserAgent = true;
			INFO("Successfully set the User Agent details");
		}
		else {
			FATAL("Not able to set the User Agent details");
		}
	}
	else {
		INFO("User Agent details are already available");
	}

	// first resolve rrs and connect
	if(resolveRRS) {
		vector <string> addrList;
		INFO("Resolving RRS and Connecting");
		getAllHostByName(rrs, addrList);
		INFO("List of RRS ips are: ");
		FOR_VECTOR_ITERATOR(string, addrList, i) {
			INFO(" %s", STR(VECTOR_VAL(i)) );
		}

		FOR_VECTOR_ITERATOR(string, addrList, i) {
			INFO("Connecting to %s rrs ip", STR(VECTOR_VAL(i)) );
			settings["rrsip"] = STR(VECTOR_VAL(i));
			settings["host"] = (bool) settings.HasKey("rrs", false) ? (string) settings["rrs"] : STR(VECTOR_VAL(i));
			if (TCPConnector<WrtcAppProtocolHandler>::Connect(STR(VECTOR_VAL(i)), port, chain, settings)) {
				INFO("WebRTC connected to %s:%"PRIu16"!", STR(VECTOR_VAL(i)), port);
				return true;
			}
		}
	}

	// then use the cached rrs ip to connect
	string ip = settings["rrsip"];
	INFO("Not resolving RRS, using the cached rrs ip %s to connect", STR(ip));
	settings["host"] = (bool) settings.HasKey("rrs", false) ? (string) settings["rrs"] : ip;

	if (!TCPConnector<WrtcAppProtocolHandler>::Connect(ip, port, chain, settings)) {
		FATAL("WebRTC Unable to connect to %s:%"PRIu16"!", STR(ip), port);
		return false;
	}
	else {
		INFO("WebRTC connected to %s:%"PRIu16"!", STR(ip), port);
		return true;
	}

	// all tries failed
	FATAL("WebRTC Unable to connect to %s:%"PRIu16"!", STR(rrs), port);

	return false;
}

bool WrtcAppProtocolHandler::StopWebRTC(uint32_t tgtConfigId) {
    FINE("WebRTC shutting down config: %d", (int)tgtConfigId);
	// Enqueue for delete existing instances of wrtcsigprotocol
	BaseProtocol *pProtocol;

	map<uint32_t, BaseProtocol *> tempList = _instances;

	FOR_MAP(tempList, uint32_t, BaseProtocol*, i) {
		pProtocol = MAP_VAL(i);
		// promote this protocol, note below we only store this type
		WrtcSigProtocol * pSig = (WrtcSigProtocol *)pProtocol;
		uint32_t thatId = pSig->GetConfigId();
		if (!tgtConfigId || (tgtConfigId == thatId)) {
			MAP_ERASE1(_instances, pProtocol->GetId());
			//pProtocol->EnqueueForDelete();
			pSig->Shutdown(true, true);
		}
	}
	return true;
}

void WrtcAppProtocolHandler::SetApplication(BaseClientApplication *pApplication) {
	BaseAppProtocolHandler::SetApplication(pApplication);
}

void WrtcAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
//#ifdef WEBRTC_DEBUG
	FINE("RegisterProtocol(%s)", STR(tagToString(pProtocol->GetType())));
//#endif
	if (MAP_HAS1(_instances, pProtocol->GetId())) {
		// Duplicate, nothing to do
		return;
	}

	// We are only interested with wrtcsigprotocol
	if (pProtocol->GetType() == PT_WRTC_SIG) {
		_instances[pProtocol->GetId()] = pProtocol;
	}
}

void WrtcAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
//#ifdef WEBRTC_DEBUG
	FINE("UnRegisterProtocol(%s)", STR(tagToString(pProtocol->GetType())));
//#endif
	
	MAP_ERASE1(_instances, pProtocol->GetId());
}

bool WrtcAppProtocolHandler::SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters) {
    //1. Get the application  designated for the newly created connection
	if (!customParameters.HasKey(CONF_APPLICATION_NAME, false)) {
            FATAL("Connect parameters must have an application name!");
            return false;
    }

    BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
                    customParameters[CONF_APPLICATION_NAME]);
    if (pApplication == NULL) {
            FATAL("Application %s not found", STR(customParameters[CONF_APPLICATION_NAME]));
            return false;
    }

    if (pProtocol == NULL) {
//#ifdef WEBRTC_DEBUG
            DEBUG("Connection failed:\n%s", STR(customParameters.ToString()));
//#endif
            return pApplication->OutboundConnectionFailed(customParameters);
    }

//#ifdef WEBRTC_DEBUG
	FINE("SignalProtocolCreated(%s): %s", STR(tagToString(pProtocol->GetType())), 
			STR(customParameters.ToString()));
//#endif

    //2. Set the application
    pProtocol->SetApplication(pApplication);

    //3. Trigger processing, including handshake
    WrtcSigProtocol *pWrtcSigProtocol = (WrtcSigProtocol *) pProtocol;
    pWrtcSigProtocol->SetOutboundConnectParameters(customParameters);
	
	/***************
    //4 - get the default streamName (false == not case sensitive)
	***************/
    
	//5 do this
    IOBuffer dummy;

    return pWrtcSigProtocol->SignalInputData(dummy);
}

bool WrtcAppProtocolHandler::SendWebRTCCommand(Variant& settings) {
	// THEN get a handle to the list of WRTC sessions
	// [WRTCAppProtocolHandler]<>--[WRTCSigProtocol]<>--[WRTCConnection]
	//                                                   < >
	//                                                    |
	//                                [OutNetFMP4Stream]--+
	// 
	// WRTCConnection::SendCommandData() IF the connected stream == targetstreamname

	// First order of business -- iterate each WRTGSigProtocol instance.
	bool result = true;
	map<uint32_t, BaseProtocol *> mapWRTCSigs = _instances;
	FOR_MAP(mapWRTCSigs, uint32_t, BaseProtocol*, i) {
		BaseProtocol* pProtocol = MAP_VAL(i);
		WrtcSigProtocol * pSig = (WrtcSigProtocol *)pProtocol;
		if (!pSig->SendWebRTCCommand(settings)) {
			result = false;
		}
	}
	return result;
}

#ifdef RTMSG
rtConnection WrtcAppProtocolHandler::GetRtSendMessageTag() {
	return connectionSend;
}
#endif

#endif // HAS_PROTOCOL_WEBRTC

