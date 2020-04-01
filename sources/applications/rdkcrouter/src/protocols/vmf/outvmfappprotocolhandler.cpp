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
 * outvmfappprotocolhandler.h - own the Video Metafile psuedo-stream stack
 * 
 * ----------------------------------------------------
 **/
#include "protocols/vmf/outvmfappprotocolhandler.h"
#include "protocols/vmf/outvmfstream.h"
#include "protocols/vmf/outvmfprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/protocolfactory.h"
#include "protocols/protocoltypes.h"
#include "defines.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"

OutVmfAppProtocolHandler::OutVmfAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}
/*!
 * GetVmfStream handles setting up the VMF Protocol Stack
 * and then allocates and returns the stream.
 */
bool OutVmfAppProtocolHandler
::ConnectToVmf(string streamName, Variant &settings) {
	// looks like spawnParms is not used!
	string ip = settings["ip"];
	uint16_t port = settings["port"];
	//
	settings["applicationName"] = GetApplication()->GetName();
	//parameters["payload"] = settings;
	//parameters[CONF_PROTOCOL] = PT_OUTBOUND_VMF;
	settings[CONF_PROTOCOL] = CONF_PROTOCOL_OUTBOUND_VMF;
	settings["streamName"] = streamName;
	//parameters["pathToFile"] = settings["pathToFile"];
	//parameters["inStreamPtr"] = settings["inStreamPtr"];
	//INFO("$b2$:ConnectToVmf: settings: %s", STR(settings.ToString()));
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_OUTBOUND_VMF);
	if (!TCPConnector<OutVmfProtocol>::Connect(ip, port, chain, settings)) {
		//FATAL("$b2$!!! Unable to connect to %s:%hu", STR(ip), port);
		return false;
	}

	return true;
}


