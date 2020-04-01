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


#ifdef HAS_PROTOCOL_CLI
#include "netio/netio.h"
#include "application/clientapplicationmanager.h"
#include "application/originapplication.h"
#include "protocols/cli/cliappprotocolhandler.h"
#include "protocols/cli/inboundbasecliprotocol.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/rtmpappprotocolhandler.h"
#include "protocols/rtmp/streaming/rtmpplaylist.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/basestream.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include "streaming/hls/outfilehlsstream.h"
#include "protocols/variant/originvariantappprotocolhandler.h"
#include "application/filters.h"
#include "eventlogger/eventlogger.h"
#include "protocols/protocolfactory.h"
#include "protocols/drm/drmdefines.h"
#include "metadata/metadatamanager.h"
#include "configuration/module.h"
using namespace app_rdkcrouter;

#define EXTRACT_RESPONSE_ID(parameters) \
string responseId = ""; \
do { \
	if (parameters.HasKeyChain(V_STRING, false, 1, "commandId")) { \
		responseId = (string) parameters.GetValue("commandId", false); \
		parameters.RemoveKey("commandId", false); \
	} \
} while(0)

#define SET_RESPONSE_ID(data, responseId) data["commandId"] = responseId

#define RESPONSE_ID responseId
#define HAS_PARAMETERS \
do { \
	if (parameters != V_MAP) { \
		FATAL("Syntax error. No parameters provided"); \
		return SendFail(pFrom, "Syntax error. No parameters provided", RESPONSE_ID); \
	} \
} while(0)


#define GATHER_MANDATORY_ARRAY(settings,parameters,name) \
Variant name = ""; \
if (parameters.HasKeyChain(V_MAP, false, 1, #name)) { \
	name = parameters.GetValue(#name, false); \
} else { \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
if ((name.MapSize() == 0) || (!name.IsArray())){ \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
settings[#name]=name;

#define GATHER_MANDATORY_STRING(settings,parameters,name) \
string name = ""; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	name = (string) parameters.GetValue(#name, false); \
	trim(name); \
} \
if (name == "") { \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
settings[#name]=name;
//FINEST("+ %s: %s",#name,STR(name));

#define GATHER_OPTIONAL_STRING(settings,parameters,name,defaultValue) \
string name = ""; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	name = (string) parameters.GetValue(#name, false); \
	trim(name); \
} else { \
	name = defaultValue; \
} \
settings[#name]=name;
//FINEST("- %s: %s",#name,STR(name));

#define GATHER_MANDATORY_NUMBER(settings,parameters,name,min,max) \
uint64_t name = 0; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (uint64_t) atoll(STR(rawVal)); \
	if (format("%"PRIu64, name) != rawVal) { \
		FATAL("Syntax error. Parameter %s is invalid", #name); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
	} \
	if (((min != 0) && (name < min)) || (name > max)) { \
		FATAL("Syntax error. Parameter %s is invalid. Range: %"PRIu64" - %"PRIu64, #name, (uint64_t)min, (uint64_t)max); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid. Range: %"PRIu64" - %"PRIu64, #name, (uint64_t)min, (uint64_t)max), RESPONSE_ID); \
	} \
} else { \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
settings[#name]=(uint64_t) name;
//FINEST("+ %s: %u",#name,name);

#define GATHER_OPTIONAL_NUMBER(settings,parameters,name,min,max,defaultValue) \
uint64_t name = defaultValue; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (uint64_t) atoll(STR(rawVal)); \
	if (format("%"PRIu64, name) != rawVal) { \
		FATAL("Syntax error. Parameter %s is invalid", #name); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
	} \
	if (((min != 0) && (name < min)) || (name > max)) { \
		FATAL("Syntax error. Parameter %s is invalid. Range: %"PRIu64" - %"PRIu64, #name, (uint64_t)min, (uint64_t)max); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid. Range: %"PRIu64" - %"PRIu64, #name, (uint64_t)min, (uint64_t)max), RESPONSE_ID); \
	} \
} \
settings[#name]=(uint64_t) name;
//FINEST("- %s: %u",#name,name);

#define GATHER_MANDATORY_SIGNED_NUMBER(settings,parameters,name,min,max) \
int64_t name = 0; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (int64_t) atoll(STR(rawVal)); \
	if (format("%"PRId64, name) != rawVal) { \
		FATAL("Syntax error. Parameter %s is invalid", #name); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
	} \
	if ((name < min) || (name > max)) { \
		FATAL("Syntax error. Parameter %s is invalid. Range: %"PRId64" - %"PRId64, #name, (int64_t)min, (int64_t)max); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid. Range: %"PRId64" - %"PRId64, #name, (int64_t)min, (int64_t)max), RESPONSE_ID); \
	} \
} else { \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
settings[#name]=(int64_t) name;
//FINEST("+ %s: %i",#name,name);

#define GATHER_OPTIONAL_SIGNED_NUMBER(settings,parameters,name,min,max,defaultValue) \
int64_t name = defaultValue; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (int64_t) atoll(STR(rawVal)); \
	if (format("%"PRId64, name) != rawVal) { \
		FATAL("Syntax error. Parameter %s is invalid", #name); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
	} \
	if (((min != 0) && (name < min)) || (name > max)) { \
		FATAL("Syntax error. Parameter %s is invalid. Range: %"PRId64" - %"PRId64, #name, (int64_t)min, (int64_t)max); \
		return SendFail(pFrom, format("Syntax error. Parameter %s is invalid. Range: %"PRId64" - %"PRId64, #name, (int64_t)min, (int64_t)max), RESPONSE_ID); \
	} \
} \
settings[#name]=(int64_t) name;
//FINEST("- %s: %u",#name,name);

#define GATHER_MANDATORY_BOOLEAN(settings,parameters,name) \
bool name = false; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (((uint32_t) atoi(STR(rawVal)))!=0); \
} else { \
	FATAL("Syntax error. Parameter %s is invalid", #name); \
	return SendFail(pFrom, format("Syntax error. Parameter %s is invalid", #name), RESPONSE_ID); \
} \
settings[#name]=(bool) name;
//FINEST("+ %s: %u",#name,name);


#define GATHER_OPTIONAL_BOOLEAN(settings,parameters,name,defaultValue) \
bool name = defaultValue; \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	string rawVal = (string) parameters.GetValue(#name, false); \
	trim(rawVal); \
	name = (((uint32_t) atoi(STR(rawVal)))!=0); \
} \
settings[#name]=(bool) name;
//FINEST("- %s: %u",#name,name);


#define CHECK_IF_ELEMENTS_MATCH(settings,name1,name2) \
if (!InitializeEmptyParams(settings, #name1, #name2)) { \
	FATAL("Syntax error. Parameters %s and %s have different number of elements", #name1, #name2); \
	return SendFail(pFrom, format("Syntax error. Parameters %s and %s have different number of elements", #name1, #name2), RESPONSE_ID); \
}


#define GATHER_TRANSCODE_PARAMS(settings,parameters,name) \
if (parameters.HasKeyChain(V_STRING, false, 1, #name)) { \
	GATHER_MANDATORY_STRING(settings, parameters, name); \
	settings[#name].Reset(); \
	settings[#name].PushToArray(name); \
} else { \
	GATHER_MANDATORY_ARRAY(settings, parameters, name); \
}


#define COMMAND_BEGIN(name,desc,deprecated) \
do { \
	command.Reset(); \
	command["command"] = name; \
	command["description"] = desc; \
	command["deprecated"] = (bool)deprecated; \
	command["parameters"].IsArray(true); \
}while(0)

#define COMMAND_END \
do { \
	COMMAND_ID_PARAM; \
	_help.PushToArray(command); \
} while (0)

#define NO_PARAMS_COMMAND(name,desc,deprecated) do{ COMMAND_BEGIN(name,desc,deprecated); COMMAND_END;}while(0)

#define GENERIC_PARAM(name,desc,mandatory,defaultVal) \
do { \
	param.Reset(); \
	param["name"] = name; \
	param["description"] = desc; \
	param["mandatory"] = (bool)mandatory; \
	param["defaultValue"] = defaultVal; \
	command["parameters"].PushToArray(param); \
} while(0)

#define OPTIONAL_PARAM(name,desc,defaultVal) GENERIC_PARAM(name,desc,false,defaultVal)
#define MANDATORY_PARAM(name,desc) GENERIC_PARAM(name,desc,true,Variant())
#define COMMAND_ID_PARAM OPTIONAL_PARAM("commandId", "If set to any value, the corresponding message will have a parameter responseId set to the same value. This is used to match the responses to the corresponding commands", "-No default value-");

string CreateStreamSignature(string nearIp, int16_t nearPort, 
		string farIp, int16_t farPort, 
		string type, bool reverseEndpoints) {

	// omit direction
	string mainType = type.substr(1);
	string endpoint1 = format("%s:%"PRId16, STR(nearIp), nearPort);
	string endpoint2 = format("%s:%"PRId16, STR(farIp), farPort);
	return reverseEndpoints ? mainType + "_" + endpoint2 + "-" + endpoint1 :
		mainType + "_" + endpoint1 + "-" + endpoint2;
}

string CreateStreamSignature(Variant streamDetails, bool reverseEndpoints) {
	string type = (string)streamDetails["type"];
	string nearIp = (string)streamDetails["nearIp"];
	string farIp = (string)streamDetails["farIp"];
	int16_t nearPort = (int16_t)streamDetails["nearPort"];
	int16_t farPort = (int16_t)streamDetails["farPort"];
	return CreateStreamSignature(nearIp, nearPort, farIp, farPort, type, reverseEndpoints);
}

string VariantToBoolCheck(Variant variantValue) {
	string variantString = STR(variantValue);
	if (variantString == "true") {
		return "1";
	} else if (variantString == "false") {
		return "0";
	}
	return variantString;
}

CLIAppProtocolHandler::CLIAppProtocolHandler(Variant &configuration)
: BaseCLIAppProtocolHandler(configuration) {
	_pApp = NULL;
	_pSM = NULL;
}

CLIAppProtocolHandler::~CLIAppProtocolHandler() {
}

void CLIAppProtocolHandler::SetApplication(BaseClientApplication *pApplication) {
	if (pApplication == NULL) {
		_pApp = NULL;
		_pSM = NULL;
	}
	_pApp = (OriginApplication *) pApplication;
	_pSM = _pApp->GetStreamsManager();
	BaseCLIAppProtocolHandler::SetApplication(pApplication);
}

bool CLIAppProtocolHandler::ListStreamsIdsResult(Variant &callbackInfo) {
	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t)callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the streams
	map<uint32_t, BaseStream *> allStreams = _pSM->GetAllStreams();
	// For each stream get the info
	Variant data;
	Variant temp;

	// Check the 'disableInternalStreams' flag
	bool disableInternalStreams = (bool)callbackInfo["disableInternalStreams"];
	string type;
	string name;

	vector<string> streams;
	string oRtmp = tagToString(ST_OUT_NET_RTMP);
	string iRtmp = tagToString(ST_IN_NET_RTMP);
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["allStreams"]).MapSize();
		if (disableInternalStreams && (edgeSize > 0)) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["allStreams"], string, Variant, i) {
			if (disableInternalStreams) {
				// Pick out all tagged as internal
				if (MAP_VAL(i).HasKey("_isInternal")) {
					ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
					if ((bool)MAP_VAL(i)["_isInternal"])
						continue;
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
			data.PushToArray((uint64_t) MAP_VAL(i)["uniqueId"]);
		}
	}
	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		if (disableInternalStreams) {
			type = (string)temp["type"];
			if (type.find(iRtmp) != string::npos || type.find(oRtmp) != string::npos) {
				// Filter out inbound rtmp with existing stream name
				bool match = false;
				string streamSig = CreateStreamSignature(temp, false);
				FOR_VECTOR_ITERATOR(string, streams, i) {
					if (VECTOR_VAL(i) == streamSig) {
						match = true;
						streams.erase(i);
						break;
					}
				}
				if (match) {
					continue;
				}
			}
		}
		data.PushToArray((uint64_t) temp["uniqueId"]);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "name uniqueId type";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Available stream IDs", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetStreamInfoResult(Variant &callbackInfo) {
	//1. Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	if (callbackInfo["streamInfo"] == V_MAP) {

#ifdef HAS_PROTOCOL_ASCIICLI
		// for edge response; see CLIAppProtocolHandler::ProcessMessageGetStreamInfo for origin response
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
			callbackInfo["streamInfo"]["asciiList"]["names"] = "name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec";
		}
#endif /* HAS_PROTOCOL_ASCIICLI */

		return SendSuccess(pFrom, "Stream information", callbackInfo["streamInfo"], RESPONSE_ID);
	} else {
		return SendFail(pFrom, format("Stream %"PRIu64":%"PRIu32" not found",
				(uint64_t) callbackInfo["edgeId"],
				(uint32_t) callbackInfo["streamId"]),
				RESPONSE_ID);
	}
}

bool CLIAppProtocolHandler::ListStreamsResult(Variant &callbackInfo) {
	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the streams
	map<uint32_t, BaseStream *> allStreams = _pSM->GetAllStreams();
	// For each stream get the info
	Variant data;
	Variant temp;

	// Check the 'disableInternalStreams' flag
	bool disableInternalStreams = (bool) callbackInfo["disableInternalStreams"];
	string type;

	vector<string> streams;
	string oRtmp = tagToString(ST_OUT_NET_RTMP);
	string iRtmp = tagToString(ST_IN_NET_RTMP);
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["allStreams"]).MapSize();
		if (disableInternalStreams && (edgeSize > 0)) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["allStreams"], string, Variant, i) {
			MAP_VAL(i)["edgePid"] = MAP_VAL(edge)["pid"];

			if (disableInternalStreams) {

				// Pick out all tagged as internal
				if (MAP_VAL(i).HasKey("_isInternal")) {
					
					ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
					if ((bool)MAP_VAL(i)["_isInternal"])
						continue;
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
			data.PushToArray(MAP_VAL(i));
		}
	}
	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		temp["edgePid"] = (uint32_t) 0;
		if (disableInternalStreams) {
			type = (string) temp["type"];
			if (type.find(iRtmp) != string::npos || type.find(oRtmp) != string::npos) {
				// Filter out inbound rtmp with existing stream name
				bool match = false;
				string streamSig = CreateStreamSignature(temp, false);
				FOR_VECTOR_ITERATOR(string, streams, i) {
					if (VECTOR_VAL(i) == streamSig) {
						match = true;
						streams.erase(i);
						break;
					}
				}

				if (match) {
					continue;
				}
			}
		}

		data.PushToArray(temp);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Available streams", data, RESPONSE_ID);
}

/**
 * Used by listInboundStreamNames API command
 * Provides a list of all the current inbound localstreamnames.
 */
bool CLIAppProtocolHandler::ListInboundStreamNamesResult(Variant &callbackInfo) {

	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t)callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the inbound streams
	map<uint32_t, BaseStream *> inboundStreams = _pSM->FindByType(ST_IN, true);
	// For each stream get the info
	Variant data;
	Variant temp;

	// Check the 'disableInternalStreams' flag
	bool disableInternalStreams = (bool)callbackInfo["disableInternalStreams"];
	string type;

	vector<string> streams;
	string iRtmp = tagToString(ST_IN_NET_RTMP);
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["inboundStreams"]).MapSize();
		if (disableInternalStreams && (edgeSize > 0)) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["inboundStreams"], string, Variant, i) {
			if (disableInternalStreams) {
				// Pick out all tagged as internal
				if (MAP_VAL(i).HasKey("_isInternal")) {
					ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
					if ((bool)MAP_VAL(i)["_isInternal"])
						continue;
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
			string name = (string) MAP_VAL(i)["name"];
			temp.Reset();
			temp["name"] = name;
			data.PushToArray(temp);
		}
	}
	FOR_MAP(inboundStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		if (disableInternalStreams) {
			type = (string) temp["type"];
			if (type.find(iRtmp) != string::npos) {
				// Filter out inbound rtmp with existing stream name
				bool match = false;
				string streamSig = CreateStreamSignature(temp, false);
				FOR_VECTOR_ITERATOR(string, streams, i) {
					if (VECTOR_VAL(i) == streamSig) {
						match = true;
						streams.erase(i);
						break;
					}
				}
				if (match) {
					continue;
				}
			}
		}
		temp.Reset();
		temp["name"] = MAP_VAL(i)->GetName();
		data.PushToArray(temp);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "name";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Inbound local stream names", data, RESPONSE_ID);
}

/**
 * Used by listInboundStreams API command
 * Provides a list of inbound streams that are not intermediate.
 */
bool CLIAppProtocolHandler::ListInboundStreamsResult(Variant &callbackInfo) {

	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t)callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the inbound streams
	map<uint32_t, BaseStream *> inboundStreams = _pSM->FindByType(ST_IN, true);
	// For each stream get the info
	Variant data;
	Variant temp;

	// Check the 'disableInternalStreams' flag
	bool disableInternalStreams = (bool)callbackInfo["disableInternalStreams"];
	string type;

	vector<string> streams;
	string iRtmp = tagToString(ST_IN_NET_RTMP);
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["inboundStreams"]).MapSize();
		if (disableInternalStreams && (edgeSize > 0)) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["inboundStreams"], string, Variant, i) {
			MAP_VAL(i)["edgePid"] = MAP_VAL(edge)["pid"];
			if (disableInternalStreams) {
				// Pick out all tagged as internal
				if (MAP_VAL(i).HasKey("_isInternal")) {
					ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
					if ((bool)MAP_VAL(i)["_isInternal"])
						continue;
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
			data.PushToArray(MAP_VAL(i));
		}
	}
	FOR_MAP(inboundStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		temp["edgePid"] = (uint32_t)0;
		if (disableInternalStreams) {
			type = (string)temp["type"];
			if (type.find(iRtmp) != string::npos) {
				// Filter out inbound rtmp with existing stream name
				bool match = false;
				string streamSig = CreateStreamSignature(temp, false);
				FOR_VECTOR_ITERATOR(string, streams, i) {
					if (VECTOR_VAL(i) == streamSig) {
						match = true;
						streams.erase(i);
						break;
					}
				}
				if (match) {
					continue;
				}
			}
		}
		data.PushToArray(temp);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "name uniqueId type outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Available inbound streams", data, RESPONSE_ID);

}

bool CLIAppProtocolHandler::ListConnectionsIdsResult(Variant &callbackInfo) {

	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	// Get all protocols
	map<uint32_t, BaseProtocol *> endpointProtocols;
	ProtocolManager::GetActiveProtocols(endpointProtocols, protocolManagerProtocolTypeFilter);

	// for each stream get the info
	Variant data;
	FOR_MAP(endpointProtocols, uint32_t, BaseProtocol *, i) {
		data.PushToArray(MAP_KEY(i));
	}

	// cycle over the edges
	FOR_MAP(callbackInfo["edges"], string, Variant, i) {
		uint64_t edgePid = MAP_VAL(i)["pid"];

		FOR_MAP(MAP_VAL(i)["connectionsIds"], string, Variant, j) {
			data.PushToArray((uint64_t) ((edgePid << 32) | ((uint64_t) MAP_VAL(j))));
		}
	}

	return SendSuccess(pFrom, "Available connection IDs", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetConnectionInfoResult(Variant &callbackInfo) {
	//1. Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	if (callbackInfo["connectionInfo"] == V_MAP) {

#ifdef HAS_PROTOCOL_ASCIICLI
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
			callbackInfo["connectionInfo"]["asciiList"]["names"] = "carrier id farIP farPort nearIp nearPort rx tx type stack applicationId streams appName audio video codec ip name outStreamsUniqueIds inStreamUniqueId port processId processType uniqueId dashSettings hlsSettings hdsSettings mssSettings pullSettings pushSettings recordSettings localStreamName mp4BinPath pathToFile configId targetFolder";
		}
#endif /* HAS_PROTOCOL_ASCIICLI */

		return SendSuccess(pFrom, "Connection information", callbackInfo["connectionInfo"], RESPONSE_ID);
	} else {
		return SendFail(pFrom, format("Connection %"PRIu64":%"PRIu32" not found",
				(uint64_t) callbackInfo["edgeId"],
				(uint32_t) callbackInfo["connectionId"]),
				RESPONSE_ID);
	}
}

bool CLIAppProtocolHandler::ListConnectionsResult(Variant &callbackInfo) {
	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	// Get all protocols
	map<uint32_t, BaseProtocol *> endpointProtocols;
	ProtocolManager::GetActiveProtocols(endpointProtocols, protocolManagerProtocolTypeFilter);

	// for each stream get the info
	Variant data;
	Variant temp;

	FOR_MAP(endpointProtocols, uint32_t, BaseProtocol *, i) {
		temp.Reset();
		GetLocalConnectionInfo(MAP_KEY(i), temp);
		data.PushToArray(temp);
	}

	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		FOR_MAP(MAP_VAL(edge)["allConnections"], string, Variant, i) {
			data.PushToArray(MAP_VAL(i));
		}
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "carrier id stack streams name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec"; //removed: streamAlias userAgent
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Available connections", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ShutdownStreamResult(Variant &callbackInfo) {
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	// Check if we're able to shutdown a stream
	if (callbackInfo["result"]["streamInfo"] == V_MAP) {

#ifdef HAS_PROTOCOL_ASCIICLI
		// for edge response; see CLIAppProtocolHandler::ProcessMessageShutdownStream for origin response
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
			callbackInfo["result"]["asciiList"]["names"] = "streamInfo name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec"; //removed: serverAgent
		}
#endif /* HAS_PROTOCOL_ASCIICLI */

		return SendSuccess(pFrom, "Stream closed", callbackInfo["result"], RESPONSE_ID);
	} else {
		return SendFail(pFrom, format("Stream %"PRIu64":%"PRIu32" not found",
				(uint64_t) callbackInfo["edgeId"],
				(uint32_t) callbackInfo["streamId"]),
				RESPONSE_ID);
	}
}

bool CLIAppProtocolHandler::GetClientsConnectedResult(Variant &callbackInfo) {
	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t)callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the streams
	map<uint32_t, BaseStream *> allStreams = _pSM->GetAllStreams();
	// For each stream get the info
	Variant data;
	Variant temp;

	string streamName = callbackInfo["localStreamName"];
	vector<string> streams;
	uint32_t outboundCount = 0;
	
	vector<string> outboundNetTags;

	outboundNetTags.push_back(tagToString(ST_OUT_NET));
	outboundNetTags.push_back(tagToString(ST_OUT_NET_RTMP));
	outboundNetTags.push_back(tagToString(ST_OUT_NET_RTP));
	outboundNetTags.push_back(tagToString(ST_OUT_NET_TS));
	outboundNetTags.push_back(tagToString(ST_OUT_NET_EXT));
	outboundNetTags.push_back(tagToString(ST_OUT_NET_PASSTHROUGH));
	outboundNetTags.push_back(tagToString(ST_OUT_VMF));

	//count for edges
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["allStreams"]).MapSize();
		if (edgeSize > 0) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["allStreams"], string, Variant, i) {
			MAP_VAL(i)["edgePid"] = MAP_VAL(edge)["pid"];
			// Pick out all tagged as internal
			if (MAP_VAL(i).HasKey("_isInternal")) {
				//check streamtype
				string streamType = (string) MAP_VAL(i)["type"];
				if (find(outboundNetTags.begin(), outboundNetTags.end(), streamType) != outboundNetTags.end()) {
					string outboundStreamName = MAP_VAL(i)["name"];
					if ((streamName == "") || (streamName == outboundStreamName)) {
						ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
						if ((bool)MAP_VAL(i)["_isInternal"])
							continue;
						outboundCount++;
					}
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
		}
	}
	//count for origin
	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		string type = (string) temp["type"];
		string outboundStreamName = MAP_VAL(i)->GetName();
		if (find(outboundNetTags.begin(), outboundNetTags.end(), type) != outboundNetTags.end()) {
			// Filter out outbound rtmp with endpoints
			bool match = false;
			string streamSig = CreateStreamSignature(temp, false);
			FOR_VECTOR_ITERATOR(string, streams, i) {
				if (VECTOR_VAL(i) == streamSig) {
					match = true;
					streams.erase(i);
					break;
				}
			}

			if (match) {
				continue;
			}
			if ((streamName == "") || (streamName == outboundStreamName)) {
				outboundCount++;
			}
		}
	}

	data["outboundCount"] = outboundCount;


#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "outboundCount";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	string descMessage = (streamName == "") ? ("RMS outbound client connections count") :
		("RMS outbound client connections count for the corresponding localstreamname: " + streamName);

	//5. Done
	return SendSuccess(pFrom, descMessage, data, RESPONSE_ID); 
}

bool CLIAppProtocolHandler::GetStreamsCountResult(Variant &callbackInfo) {
	// Get the protocol
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t)callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);
	// Get all the streams
	map<uint32_t, BaseStream *> allStreams = _pSM->GetAllStreams();
	// For each stream get the info
	Variant data;
	Variant temp;

	// Check the 'disableInternalStreams' flag
	bool disableInternalStreams = (bool)callbackInfo["disableInternalStreams"];
	string type;

	vector<string> streams;
	string oRtmp = tagToString(ST_OUT_NET_RTMP);
	string iRtmp = tagToString(ST_IN_NET_RTMP);

	uint32_t streamCount = 0;

	//count for edges
	FOR_MAP(callbackInfo["edges"], string, Variant, edge) {

		// Check the capacity of the vector and reserve space so that no need to
		// reallocate everytime
		uint64_t edgeSize = (MAP_VAL(edge)["allStreams"]).MapSize();
		if (disableInternalStreams && (edgeSize > 0)) {
			uint64_t sSize = streams.size();
			if ((streams.capacity() - sSize) < edgeSize) {
				streams.reserve((std::size_t) (sSize + edgeSize));
			}
		}

		FOR_MAP(MAP_VAL(edge)["allStreams"], string, Variant, i) {
			if (disableInternalStreams) {
				// Pick out all tagged as internal
				if (MAP_VAL(i).HasKey("_isInternal")) {
					//check streamtype
					ADD_VECTOR_END(streams, CreateStreamSignature(MAP_VAL(i), true));
					if ((bool)MAP_VAL(i)["_isInternal"])
						continue;
				}
			}
			if (MAP_VAL(i).HasKey("_isInternal")) {
				MAP_VAL(i).RemoveKey("_isInternal");
			}
			streamCount++;
		}
	}
	//count for origin
	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		temp.Reset();
		GetLocalStreamInfo(MAP_KEY(i), temp);
		if (disableInternalStreams) {
			type = (string)temp["type"];
			if (type.find(iRtmp) != string::npos || type.find(oRtmp) != string::npos) {
				// Filter out inbound rtmp with existing stream name
				bool match = false;
				string streamSig = CreateStreamSignature(temp, false);
				FOR_VECTOR_ITERATOR(string, streams, i) {
					if (VECTOR_VAL(i) == streamSig) {
						match = true;
						streams.erase(i);
						break;
					}
				}

				if (match) {
					continue;
				}
			}
		}
		streamCount++;
	}

	data["streamCount"] = streamCount;


#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "streamCount";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//5. Done
	return SendSuccess(pFrom, "Active streams count", data, RESPONSE_ID);
}

#ifdef HAS_PROTOCOL_HLS

bool CLIAppProtocolHandler::NormalizeHLSParameters(BaseProtocol *pFrom,
		Variant &parameters, Variant &settings, bool &result) {
	result = false;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	//ASSERT("parameters:\n%s",STR(parameters.ToString()));
	//1. Gather parameters: localStreamNames
	if (parameters.HasKeyChain(V_STRING, false, 1, "localStreamNames")) {
		GATHER_MANDATORY_STRING(settings, parameters, localStreamNames);
		settings["localStreamNames"].Reset();
		settings["localStreamNames"].PushToArray(localStreamNames);
	} else {
		GATHER_MANDATORY_ARRAY(settings, parameters, localStreamNames);
	}

	//2. Gather parameters: bandwidths
	if (parameters.HasKey("bandwidths")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "bandwidths")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, bandwidths, 0, 0xffffffff);
			settings["bandwidths"].Reset();
			settings["bandwidths"].PushToArray(bandwidths);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, bandwidths);
			Variant temp;
			for (uint32_t i = 0; i < settings["bandwidths"].MapSize(); i++) {
				string rawVal = (string) settings["bandwidths"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter bandwidths is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter bandwidths is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["bandwidths"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["localStreamNames"].MapSize(); i++) {
			settings["bandwidths"].PushToArray((uint32_t) 0);
		}
	}

	if (settings["localStreamNames"].MapSize() != settings["bandwidths"].MapSize()) {
		FATAL("Syntax error. Parameters bandwidths and localStreamNames have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters bandwidths and localStreamNames have different number of elements"), RESPONSE_ID);
	}

	//3. Gather parameters: targetFolder
	GATHER_MANDATORY_STRING(settings, parameters, targetFolder);

	//4. Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("hls_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	//5. Gather parameters: playlistType
	GATHER_OPTIONAL_STRING(settings, parameters, playlistType, "appending");
	if ((playlistType != "appending") && (playlistType != "rolling")) {
		return SendFail(pFrom, "Syntax error for appending parameter. Type help for details", RESPONSE_ID);
	}

	//6. Gather parameters: playlistLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, playlistLength, 5, 999999, 10);

	//7. Gather parameters: playlistName
	GATHER_OPTIONAL_STRING(settings, parameters, playlistName, "playlist.m3u8");

	//8. Gather parameters: chunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, chunkLength, 1, 3600, 10);

	//9. Gather parameters: chunkOnIDR
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, chunkOnIDR, true);

	//10. Gather parameters: chunkBaseName
	GATHER_OPTIONAL_STRING(settings, parameters, chunkBaseName, "segment");

	//11. Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	//12. Gather parameters: overwriteDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, overwriteDestination, true);

	//13. Gather parameters: staleRetentionCount
	GATHER_OPTIONAL_NUMBER(settings, parameters, staleRetentionCount, 0, 999999, playlistLength);

	//14. Gather parameters: createMasterPlaylist
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, createMasterPlaylist, true);

	//15. Gather parameters: cleanupDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, cleanupDestination, false);

	//16. Gather parameters: AESKeyCount
	GATHER_OPTIONAL_NUMBER(settings, parameters, AESKeyCount, 1, 20, 5); //max value 20 ?

#ifdef HAS_PROTOCOL_DRM
	//17. Gather parameters: encryptStream (deprecated) / drmType
	if (parameters.HasKey("encryptStream", false)) {
		GATHER_OPTIONAL_BOOLEAN(settings, parameters, encryptStream, false);
		settings["drmType"] = (string) (encryptStream ? DRM_TYPE_RMS : DRM_TYPE_NONE);
		//INFO("encryptStream=%d drmType=%s", encryptStream, ((string) settings["drmType"]).c_str());
	} else {
		GATHER_OPTIONAL_STRING(settings, parameters, drmType, DRM_TYPE_NONE);
		//INFO("drmType=%s", ((string) settings["drmType"]).c_str());
	}
#else /* HAS_PROTOCOL_DRM */
	//17. Gather parameters: encryptStream
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, encryptStream, false);
#endif /* HAS_PROTOCOL_DRM */

    uint8_t hlsVersion = (uint8_t) settings["hlsVersion"];
    string drmType = (string) settings["drmType"];

    if (hlsVersion <= 4 && drmType == DRM_TYPE_SAMPLE_AES) {
		return SendFail(pFrom, "DRM_TYPE_SAMPLE_AES parameter is only applicable in HLS v.5 and up. Please change hlsVersion to higher value in config.lua", RESPONSE_ID);
    }

	//18. Gather parameters: maxChunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, maxChunkLength, 0, 3600, 0);

	//19. Gather parameters: useSystemTime
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, useSystemTime, false);

	//20. Gather parameters: offsetTime (in seconds)
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, offsetTime, -3600, 3600, 0); //max value 3600 ?

	//21. Gather parameters: audioOnly
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, audioOnly, false);

	//22. Gather parameters: hlsResume
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, hlsResume, false);

	//23. Gather parameters: cleanupOnResume
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, cleanupOnClose, false);

    //24. Gather parameters: useByteRange
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, useByteRange, false);

	//25. Gather parameters: fileLength
	uint32_t defValue = 0;
	if ((bool) settings["useByteRange"]) {
		defValue = 120;
	}

    if (hlsVersion <= 3 && useByteRange) {
		return SendFail(pFrom, "useByteRange parameter is only applicable in HLS v.4 and up. Please change hlsVersion to higher value in config.lua", RESPONSE_ID);
    }

	GATHER_OPTIONAL_NUMBER(settings, parameters, fileLength, 1, 3600, defValue);
	
	//26. Gather parameters: startOffset (version 6 only)
	GATHER_OPTIONAL_NUMBER(settings, parameters, startOffset, 1, 3600, 0);

	if (startOffset > 0 && hlsVersion < 6) {
		return SendFail(pFrom, "startOffset parameter is only applicable for HLS v.6 onwards", RESPONSE_ID);
	}

	//if useByteRange is 0, but fileLength was given a value, then this is an input error
	if (!useByteRange && fileLength > 0) {
		return SendFail(pFrom, "fileLength cannot be given a value if useByteRange boolean parameter is set to false.", RESPONSE_ID);
	}

	if (cleanupOnClose && hlsResume) {
		return SendFail(pFrom, "hlsResume won't work when cleanupOnClose is also true. Only one or neither of the two must be set to true.", RESPONSE_ID);
	}

    if (cleanupDestination && hlsResume) {
        return SendFail(pFrom, "hlsResume won't work when cleanupDestination is also true. Only one or neither of the two must be set to true.", RESPONSE_ID);
    }

	result = true;
	return true;
}
#endif /* HAS_PROTOCOL_HLS */

bool CLIAppProtocolHandler::ProcessMessage(BaseProtocol *pFrom,
		Variant &message) {
	GetEventLogger()->LogCLIRequest(message);
	string command = lowerCase(message["command"]);
	INFO("command: %s", STR(command));
	if (command == "help") {
		return ProcessMessageHelp(pFrom, message["parameters"]);
	} else if (command == "customrtspheaders") {
		return ProcessMessageCustomRTSPHeaders(pFrom, message["parameters"]);
	} else if (command == "quit") {
		return ProcessMessageQuit(pFrom, message["parameters"]);
	} else if (command == "setauthentication") {
		return ProcessMessageSetAuthentication(pFrom, message["parameters"]);
	} else if (command == "version") {
		return ProcessMessageVersion(pFrom, message["parameters"]);
	} else if (command == "pullstream") {
		return ProcessMessagePullStream(pFrom, message["parameters"]);
	} else if (command == "pushstream") {
		return ProcessMessagePushStream(pFrom, message["parameters"]);
	} else if (command == "liststreamsids") {
		return ProcessMessageListStreamsIds(pFrom, message["parameters"]);
	} else if (command == "generatelazypullfile") {
		return ProcessMessageGenerateLazyPullFile(pFrom, message["parameters"]);
	} else if (command == "getconfiginfo") {
		return ProcessMessageGetConfigInfo(pFrom, message["parameters"]);
	} else if (command == "getstreamscount") {
		return ProcessMessageGetStreamsCount(pFrom, message["parameters"]);
	} else if (command == "getstreaminfo") {
		return ProcessMessageGetStreamInfo(pFrom, message["parameters"]);
	} else if (command == "clientsconnected") {
		return ProcessMessageClientsConnected(pFrom, message["parameters"]);
	} else if (command == "liststreams") {
		return ProcessMessageListStreams(pFrom, message["parameters"]);
	} else if (command == "listinboundstreamnames") {
		return ProcessMessageListInboundStreamNames(pFrom, message["parameters"]);
	} else if (command == "listinboundstreams") {
		return ProcessMessageListInboundStreams(pFrom, message["parameters"]);
	} else if (command == "isstreamrunning") {
		return ProcessMessageIsStreamRunning(pFrom, message["parameters"]);
	} else if (command == "getserverinfo") {
		return ProcessMessageGetServerInfo(pFrom, message["parameters"]);
	} else if (command == "shutdownstream") {
		return ProcessMessageShutdownStream(pFrom, message["parameters"]);
	} else if ((command == "listpullpushconfig") || (command == "listconfig")) {
		return ProcessMessageListConfig(pFrom, message["parameters"]);
	} else if ((command == "removepullpushconfig") || (command == "removeconfig")) {
		return ProcessMessageRemoveConfig(pFrom, message["parameters"]);
	} else if (command == "listservices") {
		return ProcessMessageListServices(pFrom, message["parameters"]);
	} else if (command == "enableservice") {
		return ProcessMessageEnableService(pFrom, message["parameters"]);
	} else if (command == "shutdownservice") {
		return ProcessMessageShutdownService(pFrom, message["parameters"]);
	} else if (command == "createservice") {
		return ProcessMessageCreateService(pFrom, message["parameters"]);
	} else if (command == "listconnectionsids") {
		return ProcessMessageListConnectionsIds(pFrom, message["parameters"]);
	} else if (command == "getextendedconnectioncounters") {
		return ProcessMessageGetExtendedConnectionCounters(pFrom, message["parameters"]);
	} else if (command == "getconnectionscount") {
		return ProcessMessageGetConnectionsCount(pFrom, message["parameters"]);
	} else if (command == "getconnectioninfo") {
		return ProcessMessageGetConnectionInfo(pFrom, message["parameters"]);
	} else if (command == "listconnections") {
		return ProcessMessageListConnections(pFrom, message["parameters"]);
	} else if (command == "sendwebrtccommand") {
		return ProcessMessageSendWebRTCCommand(pFrom, message["parameters"]);
	}
#ifdef HAS_PROTOCOL_HLS
	else if (command == "createhlsstream") {
		return ProcessMessageCreateHLSStream(pFrom, message["parameters"]);
	}
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
	else if (command == "createhdsstream") {
		return ProcessMessageCreateHDSStream(pFrom, message["parameters"]);
	}
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
	else if (command == "createmssstream") {
		return ProcessMessageCreateMSSStream(pFrom, message["parameters"]);
	}
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
	else if (command == "createdashstream") {
		return ProcessMessageCreateDASHStream(pFrom, message["parameters"]);
	}
#endif /* HAS_PROTOCOL_DASH */
#ifdef HAS_WEBSERVER
	else if (command == "addgroupnamealias") {
		return ProcessMessageAddGroupNameAlias(pFrom, message["parameters"]);
	} else if (command == "getgroupnamebyalias") {
		return ProcessMessageGetGroupNameByAlias(pFrom, message["parameters"]);
	} else if (command == "removegroupnamealias") {
		return ProcessMessageRemoveGroupNameAlias(pFrom, message["parameters"]);
	} else if (command == "listgroupnamealiases") {
		return ProcessMessageListGroupNameAliases(pFrom, message["parameters"]);
	} else if (command == "flushgroupnamealiases") {
		return ProcessMessageFlushGroupNameAliases(pFrom, message["parameters"]);
	} else if (command == "listhttpstreamingsessions") {
		return ProcessMessageListHTTPStreamingSessions(pFrom, message["parameters"]);
	} else if (command == "httpclientsconnected") {
		return ProcessMessageHttpClientsConnected(pFrom, message["parameters"]);
	}
#endif /* HAS_WEBSERVER */
	else if (command == "record") {
		return ProcessMessageRecordStream(pFrom, message["parameters"]);
	} else if (command == "getconnectionscountlimit") {
		return ProcessMessageGetConnectionsCountLimit(pFrom, message["parameters"]);
	} else if (command == "setconnectionscountlimit") {
		return ProcessMessageSetConnectionsCountLimit(pFrom, message["parameters"]);
	} else if (command == "getbandwidth") {
		return ProcessMessageGetBandwidth(pFrom, message["parameters"]);
	} else if (command == "setbandwidthlimit") {
		return ProcessMessageSetBandwidthLimit(pFrom, message["parameters"]);
	} else if (command == "listedges") {
		return ProcessMessageListEdges(pFrom, message["parameters"]);
	} else if (command == "setloglevel") {
		return ProcessMessageSetLogLevel(pFrom, message["parameters"]);
	} else if (command == "resetmaxfdcounters") {
		return ProcessMessageResetMaxFdCounters(pFrom, message["parameters"]);
	} else if (command == "resettotalfdcounters") {
		return ProcessMessageResetTotalFdCounters(pFrom, message["parameters"]);
	} else if (command == "addstreamalias") {
		return ProcessMessageAddStreamAlias(pFrom, message["parameters"]);
	} else if (command == "removestreamalias") {
		return ProcessMessageRemoveStreamAlias(pFrom, message["parameters"]);
	} else if (command == "liststreamaliases") {
		return ProcessMessageListStreamAliases(pFrom, message["parameters"]);
	} else if (command == "flushstreamaliases") {
		return ProcessMessageFlushStreamAliases(pFrom, message["parameters"]);
	} else if (command == "shutdownserver") {
		return ProcessMessageShutdownServer(pFrom, message["parameters"]);
	} else if (command == "launchprocess") {
		return ProcessMessageLaunchProcess(pFrom, message["parameters"]);
	} else if (command == "liststorage") {
		return ProcessMessageListStorage(pFrom, message["parameters"]);
	} else if (command == "addstorage") {
		return ProcessMessageAddStorage(pFrom, message["parameters"]);
	} else if (command == "removestorage") {
		return ProcessMessageRemoveStorage(pFrom, message["parameters"]);
	} else if (command == "settimer") {
		return ProcessMessageSetTimer(pFrom, message["parameters"]);
	} else if (command == "listtimers") {
		return ProcessMessageListTimers(pFrom, message["parameters"]);
	} else if (command == "removetimer") {
		return ProcessMessageRemoveTimer(pFrom, message["parameters"]);
	} else if (command == "createingestpoint") {
		return ProcessMessageCreateIngestPoint(pFrom, message["parameters"]);
	} else if (command == "removeingestpoint") {
		return ProcessMessageRemoveIngestPoint(pFrom, message["parameters"]);
	} else if (command == "listingestpoints") {
		return ProcessMessageListIngestPoints(pFrom, message["parameters"]);
	} else if (command == "insertplaylistitem") {
		return ProcessMessageInsertPlaylistItem(pFrom, message["parameters"]);
	} else if (command == "transcode") {
		return ProcessMessageTranscode(pFrom, message["parameters"]);
	} else if(command == "listmediafiles") {
		return ProcessMessageListMediaFiles(pFrom, message["parameters"]);
	} else if(command == "removemediafile") {
		return ProcessMessageRemoveMediaFile(pFrom, message["parameters"]);
	} else if (command == "getmetadata") {
		return ProcessMessageGetMetadata(pFrom, message["parameters"]);
	} else if (command == "pushmetadata") {
		return ProcessMessagePushMetadata(pFrom, message["parameters"]);
	} else if (command == "shutdownmetadata") {
		return ProcessMessageShutdownMetadata(pFrom, message["parameters"]);
	} else if (command == "uploadmedia") {
		return ProcessMessageUploadMedia(pFrom, message["parameters"]);
	} else if (command == "generateserverplaylist") {
		return ProcessMessageGenerateServerPlaylist(pFrom, message["parameters"]);
	}
#ifdef HAS_PROTOCOL_WEBRTC
	else if (command == "startwebrtc") {
		return ProcessMessageStartWebRTC(pFrom, message["parameters"]);
	} else if (command == "stopwebrtc") {
		return ProcessMessageStopWebRTC(pFrom, message["parameters"]);
	}
#endif // HAS_PROTOCOL_WEBRTC
#ifdef HAS_PROTOCOL_METADATA
	else if (command == "addmetadatalistener") {
		return ProcessMessageAddMetadataListener(pFrom, message["parameters"]);
	} else if (command == "listmetadatalisteners") {
		return ProcessMessageListMetadataListeners(pFrom, message["parameters"]);
	}
#endif // HAS_PROTOCOL_METADATA
	else {
		EXTRACT_RESPONSE_ID(message["parameters"]);
		return SendFail(pFrom,
				format("command %s not known. Type help for list of commands",
				STR(command)), RESPONSE_ID);
	}
}

void CLIAppProtocolHandler::GatherCustomParameters(Variant &src, Variant &dst) {
	if (src != V_MAP)
		return;

	FOR_MAP(src, string, Variant, i) {
		if ((MAP_KEY(i).length() > 0) && (MAP_KEY(i)[0] == '_'))
			dst[MAP_KEY(i)] = MAP_VAL(i);
	}
}

bool CLIAppProtocolHandler::ProcessMessageHelp(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	if (_help == V_NULL) {
		Variant command;
		Variant param;

		NO_PARAMS_COMMAND("help", "prints this help", false);

		NO_PARAMS_COMMAND("quit", "quits CLI", false);

		COMMAND_BEGIN("setAuthentication", "Will enable/disable RTMP authentication", false);
		MANDATORY_PARAM("enabled", "1 to enable, 0 to disable authentication");
		COMMAND_END;

		NO_PARAMS_COMMAND("version", "returns the versions for framework and this application", false);

		COMMAND_BEGIN("pullStream", "Will try to pull in a stream from an external source.  Once a stream has been successfully pulled it is assigned a local stream name which can be used to access the stream from the "BRANDING_PRODUCT_NAME, false);
		MANDATORY_PARAM("uri", "The URI of the external stream. Can be RTMP, RTSP, RTP or unicast/multicast (d)mpegts");
		OPTIONAL_PARAM("keepAlive", "If keepAlive is set to 1, the server will attempt to reestablish connection with a stream source after a connection has been lost. The reconnect will be attempted once every second.", (uint8_t) 1);
		OPTIONAL_PARAM("localStreamName", "If provided, the stream will be given this name. Otherwise, a fall-back technique is used to determine the stream name (based on the URI)", "random stream name");
		OPTIONAL_PARAM("forceTcp", "If 1 and if the stream is RTSP, a TCP connection will be forced.  Otherwise the transport mechanism will be negotiated (UDP or TCP).", (uint8_t) 1);
		OPTIONAL_PARAM("emulateUserAgent", "When specified, that value will be used as the user agent string. It is meaningful only for RTMP", HTTP_HEADERS_SERVER_US);
		OPTIONAL_PARAM("swfUrl", "When specified, this value will be used to set the originating swf URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("pageUrl", "When specified, this value will be used to set the originating web page address in the initial RTMP connect invoke.", "");
		OPTIONAL_PARAM("tcUrl", "When specified, this value will be used to set the TC URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("ttl", "Sets the IP_TTL (time to live) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("tos", "Sets the IP_TOS (Type of Service) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("rtcpDetectionInterval", "How much time (in seconds) should the server wait for RTCP packets before declaring the RTSP stream as a RTCP-less stream", (uint8_t) 10);
		OPTIONAL_PARAM("isAudio", "If 1 and if the stream is RTP, it indicates that the currently pulled stream is an audio source. Otherwise the pulled source is assumed as a video source", (uint8_t) 1);
		OPTIONAL_PARAM("audioCodecBytes", "The audio codec setup of this RTP stream if it is audio.", "hex format without '0x' or 'h'.");
		OPTIONAL_PARAM("spsBytes", "The video SPS bytes of this RTP stream if it is video.", "base 64 encoded bytes");
		OPTIONAL_PARAM("ppsBytes", "The video PPS bytes of this RTP stream if it is video.", "base 64 encoded bytes");
		OPTIONAL_PARAM("ssmIp", "The source IP from source-specific-multicast. Only usable when doing UDP based pull", "");
		OPTIONAL_PARAM("httpProxy", "The ip:port for the http proxy to be used. Only used for rtsp:// pulls. The format is in the form of ip[:port]. The special value of \"self\" (case insensitive) can be used to force the RTSP connection to be HTTP tunneled but without using any proxy", "");
		OPTIONAL_PARAM("rangeStart", "A value from which the playback should start expressed in seconds. There are 2 special values: -2 and -1. For more information, please read about start/len parameters here: http://livedocs.adobe.com/flashmediaserver/3.0/hpdocs/help.html?content=00000185.html", (int8_t) - 2);
		OPTIONAL_PARAM("rangeEnd", "The length in seconds for the playback. -1 is a special value. For more information, please read about start/len parameters here: http://livedocs.adobe.com/flashmediaserver/3.0/hpdocs/help.html?content=00000185.html", (int8_t) - 1);
		OPTIONAL_PARAM("rangeTimeStart", "The start timestamp to be sent in the Play Request message of an RTSP pullstream. This may be left blank if user wants to utilize the current time (now).", "");
		OPTIONAL_PARAM("rangeTimeEnd", "The end timestamp to be sent in the Play Request message on an RTSP pullstream.", "");
		OPTIONAL_PARAM("sendRenewStream", "If sendRenewStream is 1, the server will send RenewStream via SET_PARAMETER when a new client connects. Only valid for RTSP URIs", (uint8_t) 0);
		OPTIONAL_PARAM("sourceType", "This parameter will specify if the source pulled is a livestream or a vod. Values can only be live or vod. Default value is live.", "live");
		//Left out on purpose since this is a private param
		//OPTIONAL_PARAM("sdpCustParams", "Send custom avigilon RTSP headers which can be use for something like FastStart or etc.", "");
		COMMAND_END;

		COMMAND_BEGIN("pushStream", "Will try to push a local stream to an external destination. The pushed stream can use the RTMP, RTSP and mpegts unicast/multicast protocols.", false);
		MANDATORY_PARAM("uri", "The URI of the destination point (without stream name).");
		MANDATORY_PARAM("localStreamName", "The name of the local stream which needs to be pushed on the target URI");
		OPTIONAL_PARAM("keepAlive", "If keepAlive is set to 1, the server will attempt to reestablish connection with a stream source after a connection has been lost. The reconnect will be attempted once every second.", (uint8_t) 1);
		OPTIONAL_PARAM("targetStreamName", "The name of the stream at destination", "If missing, the stream name will be the same as the localStreamName");
		OPTIONAL_PARAM("targetStreamType", "It can be one of following: live, record, append. It is meaningful only for RTMP", "live");
		OPTIONAL_PARAM("emulateUserAgent", "When specified, that value will be used as the user agent string. It is meaningful only for RTMP", HTTP_HEADERS_SERVER_US);
		OPTIONAL_PARAM("swfUrl", "When specified, this value will be used to set the originating swf URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("pageUrl", "When specified, this value will be used to set the originating web page address in the initial RTMP connect invoke.", "");
		OPTIONAL_PARAM("tcUrl", "When specified, this value will be used to set the TC URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("ttl", "Sets the IP_TTL (Time To Live) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("tos", "Sets the IP_TOS (Type of Service) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("rtmpAbsoluteTimestamps", "Forces the timestamps to be absolute when doing RTMP push", (uint8_t) 0);
		OPTIONAL_PARAM("httpProxy", "The ip:port for the http proxy to be used. Only used for rtsp:// pushes. The format is in the form of ip[:port]. The special value of \"self\" (case insensitive) can be used to force the RTSP connection to be HTTP tunneled but without using any proxy", "");
		OPTIONAL_PARAM("useSourcePts", "If useSourcePts set to 1, the outbound stream will use the PTS of the source stream. Otherwise, the pushed stream will start at 0", (uint8_t)0);
		COMMAND_END;

		NO_PARAMS_COMMAND("listStreamsIds", "Get a list of IDs from every active stream.", false);

		NO_PARAMS_COMMAND("getStreamsCount", "Returns the number of active streams", false);

		COMMAND_BEGIN("getStreamInfo", "Returns a detailed set of information about a stream", false);
		OPTIONAL_PARAM("id", "The uniqueId of the stream. Usually a value returned by listStreamsIds", 0);
		OPTIONAL_PARAM("localStreamName", "The name of the stream that we want to have its information", "");
		COMMAND_END;

		COMMAND_BEGIN("clientsConnected", "Returns the number of REAL outbound streams, not counting the outbound streams generated internally", false);
		OPTIONAL_PARAM("localStreamName", "The name of the stream that we want to have its information. If this parameter is present, it will return the count of the corresponding outbound streams associated with this stream name", "");
		COMMAND_END;

		COMMAND_BEGIN("httpClientsConnected", "Returns the number of all the HTTP streaming sessions active within the RDKC Web Server", false);
		OPTIONAL_PARAM("groupName", "If this parameter is present, it will return the count of the http streams associated with the group name", "");
		COMMAND_END;

		COMMAND_BEGIN("listStreams", "Provides a detailed description of every active stream.", true);
		OPTIONAL_PARAM("disableInternalStreams", "If this is 1 (true), internal streams (origin-edge related) are filtered out from the list (default).", (uint8_t) 1);
		COMMAND_END;

		COMMAND_BEGIN("shutdownStream", "Terminates a specific stream. One of the id or localStreamName parameters are mandatory.", false);
		OPTIONAL_PARAM("id", "The uniqueId of the stream that needs to be terminated", 0);
		OPTIONAL_PARAM("localStreamName", "The name of an inbound stream which needs to be terminated.", "");
		OPTIONAL_PARAM("permanently", "If this is 1, the corresponding push/pull configuration will also be terminated. Therefore, the stream will NOT be reconnected when the server restarts.", (uint8_t) 1);
		COMMAND_END;

		COMMAND_BEGIN("getConfigInfo", "Returns the configuration details of a specific configID. Returns success when configID is found", false);
		MANDATORY_PARAM("id", "The uniqueId of the config. Usually a value returned by listConfig");
		COMMAND_END;

		NO_PARAMS_COMMAND("listConfig", "Returns a list with all push/pull configurations. Whenever the pullStream or pushStream interfaces are called, a record containing the details of the pull or push is created in the pullpushconfig.xml file.  Then, the next time the RMS is started, the pullpushconfig.xml file is read, and the RMS attempts to reconnect all of the previous pulled or pushed streams.", false);


		COMMAND_BEGIN("removeConfig", "Removes a pull/push configuration. It also stops the corresponding stream if active", false);
		MANDATORY_PARAM("id", "The configId of the configuration that needs to be removed. Mandatory only if groupName not specified");
#ifdef HAS_PROTOCOL_HLS
		MANDATORY_PARAM("groupName", "The name of the group that needs to be removed (applicable to HLS, HDS and external processes). Mandatory only if configId not specified");
		OPTIONAL_PARAM("removeHlsHdsFiles", "If true and the stream is HLS/HDS, the folder associated with it will be removed", (uint8_t) 0);
#else
#ifdef HAS_PROTOCOL_HDS
		MANDATORY_PARAM("groupName", "The name of the group that needs to be removed (applicable to HLS, HDS and external processes). Mandatory only if configId not specified");
		OPTIONAL_PARAM("removeHlsHdsFiles", "If true and the stream is HLS/HDS, the folder associated with it will be removed", (uint8_t) 0);
#endif /* HAS_PROTOCOL_HDS */
#endif /* HAS_PROTOCOL_HLS */
		COMMAND_END;

		NO_PARAMS_COMMAND("listServices", "returns a list of available services", false);

		COMMAND_BEGIN("enableService", "enable/disable a service", false);
		MANDATORY_PARAM("id", "The id of the service");
		MANDATORY_PARAM("enable", "If true, the service will be enabled. Else, it will be disabled");
		COMMAND_END;

		COMMAND_BEGIN("shutdownService", "terminates a service", false);
		MANDATORY_PARAM("id", "The id of the service");
		COMMAND_END;

		COMMAND_BEGIN("createService", "Creates a new service", false);
		MANDATORY_PARAM("ip", "IP address to bind on");
		MANDATORY_PARAM("port", "port value to bind on");
		MANDATORY_PARAM("protocol", "Protocol stack name to bind on");
		OPTIONAL_PARAM("sslCert", "The SSL certificate path to be used", "");
		OPTIONAL_PARAM("sslKey", "The SSL certificate key to be used", "");
		COMMAND_END;

		NO_PARAMS_COMMAND("listConnectionsIds", "Returns a list containing the IDs of every active connection", false);

		NO_PARAMS_COMMAND("getExtendedConnectionCounters", "Returns a detailed description of the network descriptors counters.  This includes historical high-water-marks for different connection types and cumulative totals.", false);

		NO_PARAMS_COMMAND("getConnectionsCount", "Returns the number of active connections", false);

		COMMAND_BEGIN("getConnectionInfo", "Returns a detailed set of information about a connection", false);
		MANDATORY_PARAM("id", "The uniqueId of the connection. Usually a value returned by listConnectionsIds");
		COMMAND_END;

		COMMAND_BEGIN("listConnections", "Returns details about every active connection", true);
		OPTIONAL_PARAM("excludeNonNetworkProtocols", "If true (non-zero), all the non-networking protocols will be excluded", (uint8_t) 1);
		COMMAND_END;

		COMMAND_BEGIN("generateLazyPullFile", "Will try to generate a lazy pull file. Parameters are similar to pullStream command. Optional parameters that are not stated will not be written", false);
		MANDATORY_PARAM("uri", "The URI of the external stream. Can be RTMP, RTSP, RTP or unicast/multicast (d)mpegts");
		MANDATORY_PARAM("pathToFile", "The path where the file will be saved, with a \".vod\" extension");
		OPTIONAL_PARAM("forceTcp", "If 1 and if the stream is RTSP, a TCP connection will be forced.  Otherwise the transport mechanism will be negotiated (UDP or TCP).", (uint8_t) 1);
		OPTIONAL_PARAM("emulateUserAgent", "When specified, that value will be used as the user agent string. It is meaningful only for RTMP", HTTP_HEADERS_SERVER_US);
		OPTIONAL_PARAM("swfUrl", "When specified, this value will be used to set the originating swf URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("pageUrl", "When specified, this value will be used to set the originating web page address in the initial RTMP connect invoke.", "");
		OPTIONAL_PARAM("tcUrl", "When specified, this value will be used to set the TC URL in the initial RTMP connect invoke", "");
		OPTIONAL_PARAM("ttl", "Sets the IP_TTL (time to live) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("tos", "Sets the IP_TOS (Type of Service) option on the socket", "operating system supplied");
		OPTIONAL_PARAM("rtcpDetectionInterval", "How much time (in seconds) should the server wait for RTCP packets before declaring the RTSP stream as a RTCP-less stream", (uint8_t) 10);
		OPTIONAL_PARAM("isAudio", "If 1 and if the stream is RTP, it indicates that the currently pulled stream is an audio source. Otherwise the pulled source is assumed as a video source", (uint8_t) 1);
		OPTIONAL_PARAM("audioCodecBytes", "The audio codec setup of this RTP stream if it is audio.", "hex format without '0x' or 'h'.");
		OPTIONAL_PARAM("spsBytes", "The video SPS bytes of this RTP stream if it is video.", "base 64 encoded bytes");
		OPTIONAL_PARAM("ppsBytes", "The video PPS bytes of this RTP stream if it is video.", "base 64 encoded bytes");
		OPTIONAL_PARAM("ssmIp", "The source IP from source-specific-multicast. Only usable when doing UDP based pull", "");
		OPTIONAL_PARAM("httpProxy", "The ip:port for the http proxy to be used. Only used for rtsp:// pulls. The format is in the form of ip[:port]. The special value of \"self\" (case insensitive) can be used to force the RTSP connection to be HTTP tunneled but without using any proxy", "");
		OPTIONAL_PARAM("rangeStart", "A value from which the playback should start expressed in seconds. There are 2 special values: -2 and -1. For more information, please read about start/len parameters here: http://livedocs.adobe.com/flashmediaserver/3.0/hpdocs/help.html?content=00000185.html", (int8_t) - 2);
		OPTIONAL_PARAM("rangeEnd", "The length in seconds for the playback. -1 is a special value. For more information, please read about start/len parameters here: http://livedocs.adobe.com/flashmediaserver/3.0/hpdocs/help.html?content=00000185.html", (int8_t) - 1);
		OPTIONAL_PARAM("sendRenewStream", "If sendRenewStream is 1, the server will send RenewStream via SET_PARAMETER when a new client connects. Only valid for RTSP URIs", (uint8_t) 0);
		OPTIONAL_PARAM("keepAlive", "KeepAlive parameter in lazy pull files is a special case compared to pullstream's. KeepAlive=1 will retain the stream in the config even if the playback stopped.", (uint8_t) 0);
		COMMAND_END;
#ifdef HAS_PROTOCOL_HLS
		COMMAND_BEGIN("createHLSStream", "Create an HTTP Live Stream (HLS) out of an existing H.264/AAC stream.  HLS is used to stream live feeds to iOS devices such as iPhones and iPads.  ", false);
		MANDATORY_PARAM("localStreamNames", "The stream(s) that will be used as the input. This is a comma-delimited list of active stream names (local stream names).");
		MANDATORY_PARAM("targetFolder", "The folder where all the *.ts/*.m3u8 files will be stored.  This folder must be accessible by the HLS clients.  It is usually in the web-root of the server.");
		OPTIONAL_PARAM("bandwidths", "The corresponding bandwidths for each stream listed in localStreamNames. Again, this can be a comma-delimited list", "");
		OPTIONAL_PARAM("groupName", "The name assigned to the HLS stream or group. If the localStreamNames parameter contains only one entry and groupName is not specified, groupName will have a random value", "hls_group_xxxx (random)");
		OPTIONAL_PARAM("playlistType", "Either `appending` or `rolling`", "appending");
		OPTIONAL_PARAM("playlistLength", "The length (number of elements) of the playlist. Used only when playlistType == `rolling`. Ignored otherwise", (uint8_t) 10);
		OPTIONAL_PARAM("playlistName", "The *.m3u8 file name", "playlist.m3u8");
		OPTIONAL_PARAM("chunkLength", "The length (in seconds) of each playlist element (*.ts file)", (uint8_t) 10);
		OPTIONAL_PARAM("chunkOnIDR", "If true, chunking is performed ONLY on IDR. Otherwise, chunking is performed whenever chunk length is achieved", (uint8_t) 1);
		OPTIONAL_PARAM("chunkBaseName", "The base name used to generate the *.ts chunks", "segment");
		OPTIONAL_PARAM("keepAlive", "If true, the RMS will attempt to reconnect to the stream source if the connection is severed.", (uint8_t) 1);
		OPTIONAL_PARAM("overwriteDestination", "If true, it will force overwrite of destination files.", (uint8_t) 1);
		OPTIONAL_PARAM("staleRetentionCount", "How many old files are kept besides the ones present in the current version of the playlist. Only applicable for rolling playlists", "If not specified, it will have the value of playlistLength");
		OPTIONAL_PARAM("createMasterPlaylist", "If true, a master playlist will be created", (uint8_t) 1);
		OPTIONAL_PARAM("cleanupDestination", "If 1 (true), all *.ts and *.m3u8 files in the target folder will be removed before HLS creation is started.", (uint8_t) 0);
		OPTIONAL_PARAM("AESKeyCount", "The maximum number of files encrypted using the same key", (uint8_t) 5);
		OPTIONAL_PARAM("maxChunkLength", "The maximum chunk length (in seconds) of each playlist element if chunkOnIDR is set to true", 0);
		OPTIONAL_PARAM("audioOnly", "If true, the RMS will only stream audio.", (uint8_t) 0);
		OPTIONAL_PARAM("hlsResume", "If 1 (true), hls will resume in appending segments to previously created child playlist even in cases of RMS shutdown or cut off stream source.", (uint8_t) 0);
		OPTIONAL_PARAM("cleanupOnClose", "If 1 (true), corresponding hls files to a stream will be deleted if the said stream is removed/shut down/disconnected.", (uint8_t) 0);
		OPTIONAL_PARAM("useByteRange", "Set this parameter to 1 to use the EXT-X-BYTERANGE feature of HLS (version 4 and up)", (uint8_t)0);
		OPTIONAL_PARAM("fileLength", "When using useByteRange=1, this parameter needs to be set too. This will be the size of file before chunking it to another file, this replace the chunkLength in case of EXT-X-BYTERANGE, since chunkLength will be the byterange chunk", "usage is in combination with useByteRange=1");
		OPTIONAL_PARAM("startOffset", "A parameter valid only for HLS v.6 onwards. This will indicate the start offset time (in seconds) for the playback of the playlist.", (uint32_t) 0);
#ifdef HAS_PROTOCOL_DRM
		OPTIONAL_PARAM("encryptStream", "Deprecated. If 1, the output HLS is encrypted w/ local keys (or drmType=rms). If 0 (default), the output HLS is not encrypted (or drmType=none).", (uint8_t) 0);
		OPTIONAL_PARAM("drmType", "The DRM type used for encryption: none (default), rms, verimatrix, sample-aes(HLS version 5 and up). Replaces encryptStream parameter.", "none");
#else /* HAS_PROTOCOL_DRM */
		OPTIONAL_PARAM("encryptStream", "If 1 (true), the output HLS stream is encrypted.", (uint8_t) 0);
#endif /* HAS_PROTOCOL_DRM */
		COMMAND_END;
#endif /* HAS_PROTOCOL_HLS */

#ifdef HAS_PROTOCOL_HDS
		COMMAND_BEGIN("createHDSStream", "Create an HTTP Dynamic Stream (HDS) out of an existing H.264/AAC stream. HDS is a new technology developed by Adobe in response to HLS from Apple", false);
		MANDATORY_PARAM("localStreamNames", "The stream(s) that will be used as the input. This is a comma-delimited list of active stream names (local stream names).");
		MANDATORY_PARAM("targetFolder", "The folder where all the manifest (*.f4m) and fragment (f4v*) files will be stored. This folder must be accessible by the HDS clients. It is usually in the web-root of the server.");
		OPTIONAL_PARAM("bandwidths", "The corresponding bandwidths for each stream listed in localStreamNames. Again, this can be a comma-delimited list", "");
		OPTIONAL_PARAM("groupName", "The name assigned to the HDS stream or group. If the localStreamNames parameter contains only one entry and groupName is not specified, groupName will have a random value", "hds_group_xxxx (random)");
		OPTIONAL_PARAM("playlistType", "Either `appending` or `rolling`", "appending");
		OPTIONAL_PARAM("playlistLength", "The number of fragments before the server starts to overwrite the older fragments. Used only when playlistType is 'rolling'. Ignored otherwise.", (uint8_t) 10);
		OPTIONAL_PARAM("manifestName", "The manifest file name", "defaults to stream name");
		OPTIONAL_PARAM("chunkLength", "The length (in seconds) of fragments to be made.", (uint8_t) 10);
		OPTIONAL_PARAM("chunkBaseName", "The base name used to generate the fragments. The default value follows this format: f4vSeg1-FragXXX.", "f4v");
		OPTIONAL_PARAM("chunkOnIDR", "If true, chunking is performed ONLY on IDR. Otherwise, chunking is performed whenever chunk length is achieved", (uint8_t) 1);
		OPTIONAL_PARAM("keepAlive", "If true, the RMS will attempt to reconnect to the stream source if the connection is severed.", (uint8_t) 1);
		OPTIONAL_PARAM("overwriteDestination", "If true, it will allow overwrite of destination files.", (uint8_t) 1);
		OPTIONAL_PARAM("staleRetentionCount", "How many old files are kept besides the ones present in the current version of the playlist. Only applicable for rolling playlists", "If not specified, it will have the value of playlistLength");
		OPTIONAL_PARAM("createMasterPlaylist", "If true, a master playlist will be created", (uint8_t) 1);
		OPTIONAL_PARAM("cleanupDestination", "If 1 (true), all manifest and fragment files in the target folder will be removed before HDS creation is started.", (uint8_t) 0);
		COMMAND_END;
#endif /* HAS_PROTOCOL_HDS */

#ifdef HAS_PROTOCOL_MSS
		COMMAND_BEGIN("createMSSStream", "Create a Microsoft Smooth Stream (MSS) out of an existing H.264/AAC stream. Smooth Streaming was developed by Microsoft to compete with other adaptive streaming technologies", false);
		MANDATORY_PARAM("localStreamNames", "The stream(s) that will be used as the input. This is a comma-delimited list of active stream names (local stream names).");
		MANDATORY_PARAM("targetFolder", "The folder where the manifest and other MSS subfolders/files will be stored. This folder must be accessible by the MSS clients. It is usually in the web-root of the server.");
		OPTIONAL_PARAM("bandwidths", "The corresponding bandwidths for each stream listed in localStreamNames. Again, this can be a comma-delimited list", "");
		OPTIONAL_PARAM("groupName", "The name assigned to the MSS stream or group. If the localStreamNames parameter contains only one entry and groupName is not specified, groupName will have a random value", "mss_group_xxxx (random)");
		OPTIONAL_PARAM("playlistType", "Either `appending` or `rolling`", "appending");
		OPTIONAL_PARAM("playlistLength", "The number of fragments before the server starts to overwrite the older fragments. Used only when playlistType is 'rolling'. Ignored otherwise.", (uint8_t) 10);
		OPTIONAL_PARAM("manifestName", "The manifest file name", "defaults to 'manifest.ismc'");
		OPTIONAL_PARAM("chunkLength", "The length (in seconds) of fragments to be made.", (uint8_t) 10);
		OPTIONAL_PARAM("chunkOnIDR", "If true, chunking is performed ONLY on IDR. Otherwise, chunking is performed whenever chunk length is achieved", (uint8_t) 1);
		OPTIONAL_PARAM("keepAlive", "If true, the RMS will attempt to reconnect to the stream source if the connection is severed.", (uint8_t) 1);
		OPTIONAL_PARAM("overwriteDestination", "If true, it will allow overwrite of destination files.", (uint8_t) 1);
		OPTIONAL_PARAM("staleRetentionCount", "How many old files are kept besides the ones present in the current version of the playlist. Only applicable for rolling playlists", "If not specified, it will have the value of playlistLength");
		OPTIONAL_PARAM("cleanupDestination", "If 1 (true), all manifest and fragment files in the target folder will be removed before MSS creation is started.", (uint8_t) 0);
		OPTIONAL_PARAM("ismType", "Either `ismc` for serving content to client or `isml` for serving content to smooth server", "ismc");
		OPTIONAL_PARAM("publishingPoint", "This parameter is needed when ismType=`isml`, it is the REST URI where the mss contents will be ingested", "");
		OPTIONAL_PARAM("ingestMode", "Either `single` for a non looping ingest or `loop` for looping an ingest", "single");
		OPTIONAL_PARAM("audioBitrates", "The bitrates of the audio in bytes/second. When this parameter is supplied it will override the values in the bandwidths parameter for audio tracks", (uint8_t) 0);
		OPTIONAL_PARAM("videoBitrates", "The bitrates of the video in bytes/second. When this parameter is supplied it will override the values in the bandwidths parameter for video tracks", (uint8_t) 0);
		OPTIONAL_PARAM("isLive", "Set this parameter to 1(default value) to create a live MSS stream, otherwise set to 0 for VOD", (uint8_t)1);
		COMMAND_END;
#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH
		COMMAND_BEGIN("createDASHStream", "Create an MPEG-DASH Stream (DASH) out of an existing H.264/AAC stream. DASH was developed by the Moving Picture Experts Group (MPEG) to standardize HTTP adaptive bitrate streaming", false);
		MANDATORY_PARAM("localStreamNames", "The stream(s) that will be used as the input. This is a comma-delimited list of active stream names (local stream names).");
		MANDATORY_PARAM("targetFolder", "The folder where the manifest and other DASH subfolders/files will be stored. This folder must be accessible by the DASH clients. It is usually in the web-root of the server.");
		OPTIONAL_PARAM("bandwidths", "The corresponding bandwidths for each stream listed in localStreamNames. Again, this can be a comma-delimited list", "");
		OPTIONAL_PARAM("groupName", "The name assigned to the DASH stream or group. If the localStreamNames parameter contains only one entry and groupName is not specified, groupName will have a random value", "dash_group_xxxx (random)");
		OPTIONAL_PARAM("playlistType", "Either `appending` or `rolling`", "appending");
		OPTIONAL_PARAM("playlistLength", "The number of fragments before the server starts to overwrite the older fragments. Used only when playlistType is 'rolling'. Ignored otherwise.", (uint8_t) 10);
		OPTIONAL_PARAM("manifestName", "The manifest file name", "defaults to 'manifest.mpd'");
		OPTIONAL_PARAM("chunkLength", "The length (in seconds) of fragments to be made.", (uint8_t) 10);
		OPTIONAL_PARAM("chunkOnIDR", "If true, chunking is performed ONLY on IDR. Otherwise, chunking is performed whenever chunk length is achieved", (uint8_t) 1);
		OPTIONAL_PARAM("keepAlive", "If true, the RMS will attempt to reconnect to the stream source if the connection is severed.", (uint8_t) 1);
		OPTIONAL_PARAM("overwriteDestination", "If true, it will allow overwrite of destination files.", (uint8_t) 1);
		OPTIONAL_PARAM("staleRetentionCount", "How many old files are kept besides the ones present in the current version of the playlist. Only applicable for rolling playlists", "If not specified, it will have the value of playlistLength");
		OPTIONAL_PARAM("cleanupDestination", "If 1 (true), all manifest and fragment files in the target folder will be removed before DASH creation is started.", (uint8_t) 0);
		OPTIONAL_PARAM("dynamicProfile", "Set this parameter to 1 (default) for a live dash, otherwise set it to 0 for a VOD", (uint8_t) 1);
		COMMAND_END;
#endif /* HAS_PROTOCOL_DASH */

		NO_PARAMS_COMMAND("getConnectionsCountLimit", "Returns the limit of concurrent connections. This is the maximum number of connections this RMS instance will allow at one time.", false);

		COMMAND_BEGIN("setConnectionsCountLimit", "This interface sets a limit on the number of concurrent connections the RMS will allow", false);
		MANDATORY_PARAM("count", "The maximum number of connections allowed on this instance. 0 means disabled. CLI connections are not affected");
		COMMAND_END;

		COMMAND_BEGIN("setBandwidthLimit", "This interface enforces a limit on the input and output bandwidth. ", false);
		MANDATORY_PARAM("in", "maximum input bandwidth. 0 means disabled. CLI connections are not affected");
		MANDATORY_PARAM("out", "maximum output bandwidth. 0 means disabled. CLI connections are not affected");
		COMMAND_END;

		NO_PARAMS_COMMAND("getBandwidth", "Returns bandwidth information: limits and current values.", false);

		COMMAND_BEGIN("setLogLevel", "Change the log level for all log appenders", false);
		MANDATORY_PARAM("level", "a value between 0 and 7. 0 - no logging; 7 - detailed logs");
		COMMAND_END;

		NO_PARAMS_COMMAND("resetMaxFdCounters", "Reset the maximum, or high-water-mark, from the Connection Counters", false);

		NO_PARAMS_COMMAND("resetTotalFdCounters", "Reset the cumulative totals from the Connection Counters", false);

		COMMAND_BEGIN("record", "The record command shall allow users to record a stream that does or does not yet exist.  When a new stream is brought into the server, it shall be checked against the streams to record and will be recorded if it is on that list.  Records an RTMP/RTSP/TS streams.", false);
		MANDATORY_PARAM("localStreamName", "The stream that will be used as the input for recording.");
		MANDATORY_PARAM("pathToFile", "Specify path for file to write");
		OPTIONAL_PARAM("type", "Optional. The type of file that the recorded stream will have.", "mp4");
		OPTIONAL_PARAM("overwrite", "If false, when a file already exists for the stream name, a new file will be created with the next appropriate number appended. If 1 (true), files with the same name will be overwritten.", (uint8_t) 0);
		OPTIONAL_PARAM("keepAlive", "If keepAlive is set to 1, the server will restart recording every time the stream becomes available again", (uint8_t) 1);
		OPTIONAL_PARAM("chunkLength", "The length (in seconds) of record file chunks. If 0, only one file of any length is recorded.", (uint8_t) 0);
		OPTIONAL_PARAM("winQtCompat", "If true (1), recording is made compatible with Windows Quicktime.", (uint8_t) 1);
		OPTIONAL_PARAM("waitForIDR", "If true (1), recording starts on IDR frames. Otherwise, recording starts on any frame.", (uint8_t) 0);
		OPTIONAL_PARAM("dateFolderStructure", "If true (1), this will make the recording have a special naming scheme for the files(chunks) and folders that will be generated. See documentation for the specifics of this naming scheme.", (uint8_t) 0);
		COMMAND_END;

		COMMAND_BEGIN("addStreamAlias", "Allows you to create secondary name(s) for internal streams. Once an alias is created the localstreamname cannot be used to request playback of that stream.  Once an alias is used (requested by a client) the alias is removed. Aliases are designed to be used to protect/hide your source streams.", false);
		MANDATORY_PARAM("localStreamName", "The original stream name");
		MANDATORY_PARAM("aliasName", "The alias alternative to the localStreamName");
		OPTIONAL_PARAM("expirePeriod", "The expiration period for this alias. Negative values will be treated as one-shot but no longer than the absolute positive value in seconds, 0 means it will not expire, positive values mean the alias can be used multiple times but expires after this many seconds. Defaulted to -600 (one-shot, 10 mins)", -600);
		COMMAND_END;

		COMMAND_BEGIN("removeStreamAlias", "Removes an alias of a stream", false);
		MANDATORY_PARAM("aliasName", "The alias alternative to be removed from the localStreamName");
		COMMAND_END;

		NO_PARAMS_COMMAND("listStreamAliases", "Returns a complete list of aliases", false);

		NO_PARAMS_COMMAND("flushStreamAliases", "Invalidates all streams aliases", false);

		COMMAND_BEGIN("shutdownServer", "Shuts down the server", false);
		MANDATORY_PARAM("key", "The key to shutdown the server. shutdownServer must be called without the key to obtain the key and once again with the returned key to shutdown the server");
		COMMAND_END;

		COMMAND_BEGIN("launchProcess", "Launch an external process", false);
		MANDATORY_PARAM("fullBinaryPath", "Full path to the binary that needs to be launched");
		OPTIONAL_PARAM("arguments", "Complete list of arguments that needs to be passed to the process", "");
		OPTIONAL_PARAM("$<ENV>=<VALUE>", "Any number of environment variables that needs to be set just before launching the process", "");
		OPTIONAL_PARAM("keepAlive", "If keepAlive is set to 1, the server will restart the process if it exits", (uint8_t) 1);
		OPTIONAL_PARAM("groupName", "The group name assigned to this process. If not specified, groupName will have a random value.", "process_group_xxxx (random)");
		COMMAND_END;

		NO_PARAMS_COMMAND("listStorage", "Lists currently available media storage locations", false);

		COMMAND_BEGIN("addStorage", "Adds a new storage location", false);
		MANDATORY_PARAM("mediaFolder", "The path to the media folder");
		OPTIONAL_PARAM("name", "Name given to this storage. Used to better identify the storage", "");
		OPTIONAL_PARAM("description", "Description given to this storage. Used to better identify the storage", "");
		OPTIONAL_PARAM("metaFolder", "Path to the folder which is going to contain all the seek/meta files. If missing, the seek/meta files are going to be generated inside the media folder", "");
		OPTIONAL_PARAM("enableStats", "If true, *.stats files are going to be generated once the media files are used", (bool)false);
		OPTIONAL_PARAM("keyframeSeek", "If true, the seek/meta files are going to be generated having only keyframe seek points", (bool)false);
		OPTIONAL_PARAM("clientSideBuffer", "How much data should be maintained on the client side when a file is played from this storage", (uint32_t) 15);
		OPTIONAL_PARAM("seekGranularity", "Voodoo stuff! You didn't see this one!", (double) 1.0);
		OPTIONAL_PARAM("externalSeekGenerator", "If true, *.seek and *.meta files are going to be generated by another external tool", (bool)false);
		COMMAND_END;

		COMMAND_BEGIN("removeStorage", "Removes a storage location", false);
		MANDATORY_PARAM("mediaFolder", "The path to the media folder for which the storage needs to be removed");
		COMMAND_END;

		COMMAND_BEGIN("setTimer", "Adds a timer. When triggered, it will send an event to the event logger", false);
		MANDATORY_PARAM("value", "The time value for the timer. It can be either the absolute time at which the trigger will be fired (YYYY-MM-DDTHH:MM:SS or HH:MM:SS) or period of time between pulses expressed in seconds between 1 and 86399 (1 sec up to a day)");
		COMMAND_END;

		NO_PARAMS_COMMAND("listTimers", "Lists currently active timers", false);

		COMMAND_BEGIN("getMetadata", "Returns the specified Metadata block", false);
		OPTIONAL_PARAM("streamName","Metadata associated with this incoming streamName(default most recent)", 0);
		OPTIONAL_PARAM("streamId","Metadata associated with this streamId (default most recent)", 0);
		COMMAND_END;

		COMMAND_BEGIN("pushMetadata", "Starts a metadata pseudo-stream", false);
		MANDATORY_PARAM("localStreamName","Metadata associated with this incoming stream Name");
		OPTIONAL_PARAM("type","type of push stream, just a hook for now", "vmf");
		OPTIONAL_PARAM("ip","ip address to push to", "127.0.0.1");
		OPTIONAL_PARAM("port","port address to push to", 8110);
		COMMAND_END;

		COMMAND_BEGIN("shutdownMetadata", "Stops a metadata pseudo-stream", false);
		MANDATORY_PARAM("localStreamName","Metadata associated with this incoming stream Name");
		COMMAND_END;
#ifdef HAS_PROTOCOL_WEBRTC
		COMMAND_BEGIN("startWebRTC", "Starts a WebRTC Signalling client to an RRS (RDKC Rendezvous Server)", false);
		MANDATORY_PARAM("rrs", "RRS (RDKC Rendezvous Server) address, either in IP or domain name format.");
		MANDATORY_PARAM("rrsport", "Port (pppp) of RRS (RDKC Rendezvous Server)");
		MANDATORY_PARAM("roomId", "Room Identifier (string) within RRS (RDKC Rendezvous Server)");
		OPTIONAL_PARAM("token","Token to authorize this RMS instance to connect to the RRS", "");
		COMMAND_END;

		COMMAND_BEGIN("stopWebRTC", "Stops the WebRTC Signalling client to an RRS (RDKC Rendezvous Server)", false);
		COMMAND_END;
#endif // HAS_PROTOCOL_WEBRTC
		
		COMMAND_BEGIN("removeTimer", "Removes a previously armed timer", false);
		MANDATORY_PARAM("id", "The id of the timer");
		COMMAND_END;

		COMMAND_BEGIN("createIngestPoint", "Creates a protected ingest point.", false);
		MANDATORY_PARAM("privateStreamName", "The secret ingest point name");
		MANDATORY_PARAM("publicStreamName", "The public ingest point name.");
		COMMAND_END;

		COMMAND_BEGIN("removeIngestPoint", "Removes a protected ingest point.", false);
		MANDATORY_PARAM("privateStreamName", "The secret ingest point name");
		COMMAND_END;

		NO_PARAMS_COMMAND("listIngestPoints", "Lists currently active ingest points", false);

		COMMAND_BEGIN("insertPlaylistItem", "Inserts a new item into an RTMP playlist", false);
		MANDATORY_PARAM("playlistName", "The name of the *.lst file into which the stream will be inserted.");
		MANDATORY_PARAM("localStreamName", "The name of the live stream or file that needs to be inserted. If a file is specified, the path must be relative to any of the mediaStorage locations");
		OPTIONAL_PARAM("insertPoint", "The absolute time in milliseconds on the playlist timeline where the insertion will occur. Any negative value will be considered as \"immediate\" (starts the very next frame)", -1000);
		OPTIONAL_PARAM("sourceOffset", "The absolute time in milliseconds on the localStreamName's timeline from which the content will be drawn. View the RMS API Definition document for special values.", -2000);
		OPTIONAL_PARAM("duration", "The duration in milliseconds of the inserted clip. View the RMS API Definition document for special values.", -1000);
		COMMAND_END;

		COMMAND_BEGIN("transcode", "Transcodes a source to one or more destinations.", false);
		MANDATORY_PARAM("source", "Can be a URI or a local stream name from RMS.");
		MANDATORY_PARAM("destinations", "The target URI(s) or stream name(s) of the transcoded stream. If only a name is given, it will be pushed back to RMS. Comma-delimited if multiple destinations.");
		OPTIONAL_PARAM("targetStreamNames", "The name of the stream(s) at destination(s). If not specified, name will have a time stamped value.", "transcoded_xxxx (timestamp)");
		OPTIONAL_PARAM("groupName", "The group name assigned to this process. If not specified, groupName will have a random value.", "transcoded_group_xxxx (random)");
		OPTIONAL_PARAM("videoBitrates", "Target output video bitrate(s) (in bits/s, append 'k' to value for kbits/s). Comma-delimited if multiple bitrates. Should match the sequence and number of elements of destinations.", "");
		OPTIONAL_PARAM("videoSizes", "Target output video size(s). Should match the sequence and number of elements of videoBitrates. Ignored if videoBitrates parameter not given.", "");
		OPTIONAL_PARAM("videoAdvancedParamsProfiles", "Name of video profile template that will be used. Should match the sequence and number of elements of videoBitrates. Ignored if videoBitrates parameter not given.", "");
		OPTIONAL_PARAM("audioBitrates", "Target output audio bitrate(s) (in bits/s, append 'k' to value for kbits/s). Comma-delimited if multiple bitrates. Should match the sequence and number of elements of destinations.", "");
		OPTIONAL_PARAM("audioChannelsCounts", "Target output audio channel(s) count(s). Should match the sequence and number of elements of audioBitrates. Ignored if audioBitrates parameter not given.", "");
		OPTIONAL_PARAM("audioFrequencies", "Target output audio frequency(ies) (in Hz). Should match the sequence and number of elements of audioBitrates. Ignored if audioBitrates parameter not given.", "");
		OPTIONAL_PARAM("audioAdvancedParamsProfiles", "Name of audio profile template that will be used. Should match the sequence and number of elements of audioBitrates. Ignored if audioBitrates parameter not given.", "");
		OPTIONAL_PARAM("overlays", "Location of the overlay source(s) to be used. Should match the sequence and number of elements of videoBitrates.", "");
		OPTIONAL_PARAM("croppings", "Target video cropping position(s) and size(s) in 'left : top : width : height' format (e.g. 0:0:200:100. Positions are optional (200:100 for a centered cropping of size 200x100 [wxh]). Should match the sequence and number of elements of videoBitrates.", "");
		OPTIONAL_PARAM("keepAlive", "If keepAlive is set to 1, the server will restart transcoding if it was previously activated.", (uint8_t) 1);
		OPTIONAL_PARAM("commandFlags", "Other commands to the transcode process that are not supported by the baseline transcode command.", "");
		COMMAND_END;
		
		NO_PARAMS_COMMAND("listMediaFiles", "List media files relative to the media folder.", false);
		COMMAND_BEGIN("removeMediaFile", "Remove media file relative to the media folder.", false);
		MANDATORY_PARAM("fileName", "File relative to the media folder. Sub-directories must be specified along with the filename.");
		COMMAND_END;

		NO_PARAMS_COMMAND("listInboundStreamNames", "Provides a list of all the current inbound localstreamnames.", false);
		COMMAND_BEGIN("isStreamRunning", "Checks a specific stream if it is running or not.", false);
		OPTIONAL_PARAM("id", "The unique id of the stream to check", 0);
		OPTIONAL_PARAM("localStreamName", "The name of the stream to check", "");
		COMMAND_END;
		NO_PARAMS_COMMAND("getServerInfo", "Returns a bunch of information regarding the configuration of the running RMS.", false);
#ifdef HAS_WEBSERVER
		COMMAND_BEGIN("addGroupNameAlias", "Allows you to create secondary name(s) for group names. Once an alias is created the group name cannot be used to request HTTP playback of that stream.  Once an alias is used (requested by a client) the alias is removed. Aliases are designed to be used to protect/hide your source streams.", false);
		MANDATORY_PARAM("groupName", "The original group name");
		MANDATORY_PARAM("aliasName", "The alias alternative to the groupName");
		COMMAND_END;

		COMMAND_BEGIN("getGroupNameByAlias", "Returns the group name associated with an alias.", false);
		MANDATORY_PARAM("aliasName", "The alias alternative to the groupName");
		COMMAND_END;

		COMMAND_BEGIN("removeGroupNameAlias", "Removes an alias of a group name", false);
		MANDATORY_PARAM("aliasName", "The alias alternative to be removed from the groupName");
		COMMAND_END;

		NO_PARAMS_COMMAND("listGroupNameAliases", "Returns a complete list of group name aliases", false);

		NO_PARAMS_COMMAND("flushGroupNameAliases", "Invalidates all group name aliases", false);

		NO_PARAMS_COMMAND("listHTTPStreamingSessions", "Lists currently active HTTP streaming sessions", false);
#endif
		COMMAND_BEGIN("uploadMedia", "Creates an acceptor which receives an HTTP POST binary upload", false);
		MANDATORY_PARAM("port", "port value to bind on");
		MANDATORY_PARAM("targetFolder", "the folder where the binary upload will be serialized");
		COMMAND_END;

		COMMAND_BEGIN("generateServerPlaylist", "Generates a server playlist. Parameters are similar to createHlsStream command. Optional parameters that omitted will not be written", false);
		MANDATORY_PARAM("sources", "The stream or media file source(s) to be used as input. This is a comma-delimited list of active stream names or media files.");
		MANDATORY_PARAM("pathToFile", "The path to the output server playlist file.");
		OPTIONAL_PARAM("sourceOffsets", "The corresponding offsets for the source streams/files listed in sources. This can be a comma-delimited list", "");
		OPTIONAL_PARAM("durations", "The corresponding durations for the source streams/files listed in sources. This can be a comma-delimited list", "");
		COMMAND_END;

#ifdef HAS_PROTOCOL_METADATA
		COMMAND_BEGIN("addMetadataListener", "Adds another acceptor for the inboundJsonMeta.", false);
		MANDATORY_PARAM("port", "The stream or media file source(s) to be used as input. This is a comma-delimited list of active stream names or media files.");
		OPTIONAL_PARAM("ip", "IP address of the new acceptor for inboundJsonMeta.", "0.0.0.0");
		OPTIONAL_PARAM("localStreamName", "Corresponding localstreamname for the inboundJsonMeta using this new acceptor", "0~0~0");
		COMMAND_END;

		COMMAND_BEGIN("listMetadataListeners", "Lists all the metadata listeners added through createService/addMetadataListener API or config.lua.", false);
		COMMAND_END;
#endif /* HAS_PROTOCOL_METADATA */

	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		_help["asciiList"]["names"] = "command description parameters defaultValue mandatory name";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Available commands", _help, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageQuit(BaseProtocol *pFrom,
		Variant &parameters) {
	IOHandler *pIOHandler = pFrom->GetIOHandler();
	if ((pIOHandler != NULL)
			&& (pIOHandler->GetType() != IOHT_TCP_CARRIER))
		return true;
	Variant dummy;

	EXTRACT_RESPONSE_ID(parameters);

	if (!SendSuccess(pFrom, "Bye!", dummy, RESPONSE_ID)) {
		FATAL("Unable to send message");
		return false;
	}
	pFrom->GracefullyEnqueueForDelete();
	return true;
}

bool CLIAppProtocolHandler::ProcessMessageSetAuthentication(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	//1. Gather parameters: enabled
	GATHER_MANDATORY_BOOLEAN(settings, parameters, enabled);

	RTMPAppProtocolHandler *pHandler =
			(RTMPAppProtocolHandler *) GetApplication()->GetProtocolHandler(
			PT_INBOUND_RTMP);
	if (pHandler == NULL) {
		return SendFail(pFrom, "Unable to get RTMP protocol handler", RESPONSE_ID);
	}
	pHandler->SetAuthentication(enabled);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "enabled";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Authentication mode updated", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageVersion(BaseProtocol *pFrom, Variant &parameters) {
	Variant data = Version::GetAll();
	
	EXTRACT_RESPONSE_ID(parameters);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "releaseNumber banner buildDate buildNumber codeName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */
	return SendSuccess(pFrom, "Version", data, RESPONSE_ID);
}

void FixEmulateUserAgent(Variant &settings, string &emulateUserAgent, bool player) {
	if (!settings.HasKeyChain(V_STRING, true, 1, "emulateUserAgent"))
		return;
	string value = lowerCase((string) settings["emulateUserAgent"]);
	if (value == "flash") {
		emulateUserAgent = "MAC 11,3,300,265";
	} else if (value == "fmle") {
		emulateUserAgent = "FMLE/3.0 (compatible; FMSc/1.0)";
	} else if (value == "wirecast") {
		emulateUserAgent = "Wirecast/FM 1.0 (compatible; FMSc/1.0)";
	} else if (value == "rms") {
		if (player)
			emulateUserAgent = HTTP_HEADERS_SERVER_US" player";
		else
			emulateUserAgent = HTTP_HEADERS_SERVER_US;
	}
	settings["emulateUserAgent"] = emulateUserAgent;
}

bool CLIAppProtocolHandler::ProcessMessagePullStream(BaseProtocol *pFrom,
		Variant &parameters) {

	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: uri. Do not resolve the host name yet because
	//we may have a httpProxy entry
	GATHER_MANDATORY_STRING(settings, parameters, uri);
	URI uriTester;
	if (!URI::FromString(uri, false, uriTester)) {
		string result = format("Invalid URI: %s", STR(uri));
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	parameters["uri"] = uriTester;

	//2. Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	//3. Gather parameters: localStreamName
	GATHER_OPTIONAL_STRING(settings, parameters, localStreamName, "stream_" + generateRandomString(8));
	if (!GetApplication()->StreamNameAvailable(localStreamName, NULL, true)) {
		string message = format("Stream name %s already taken", STR(localStreamName));
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

	//4. Gather parameters: forceTcp
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, forceTcp, false);

	//5. Gather parameters: swfUrl
	GATHER_OPTIONAL_STRING(settings, parameters, swfUrl, "");

	//6. Gather parameters: pageUrl
	GATHER_OPTIONAL_STRING(settings, parameters, pageUrl, "");

	//7. Gather parameters: tcUrl
	GATHER_OPTIONAL_STRING(settings, parameters, tcUrl, "");

	//8. Gather parameters: ttl
	GATHER_OPTIONAL_NUMBER(settings, parameters, ttl, 0, 0x100, 0x100);

	//9. Gather parameters: tos
	GATHER_OPTIONAL_NUMBER(settings, parameters, tos, 0, 0x100, 0x100);

	//10. Gather parameters: ssmIp
	GATHER_OPTIONAL_STRING(settings, parameters, ssmIp, "");

	//11. Gather parameters: rtcpDetectionInterval
	GATHER_OPTIONAL_NUMBER(settings, parameters, rtcpDetectionInterval, 0, 60, 10);

	//12. Gather parameters: emulateUserAgent
	GATHER_OPTIONAL_STRING(settings, parameters, emulateUserAgent, HTTP_HEADERS_SERVER_US" player");
	FixEmulateUserAgent(settings, emulateUserAgent, true);

	//13. Gather parameters: isAudio
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, isAudio, true);

	//14. Gather parameters: audioCodecBytes (fixed two bytes?)
	GATHER_OPTIONAL_STRING(settings, parameters, audioCodecBytes, "");

	//15. Gather parameters: spsBytes
	GATHER_OPTIONAL_STRING(settings, parameters, spsBytes, "");

	//16. Gather parameters: ppsBytes
	GATHER_OPTIONAL_STRING(settings, parameters, ppsBytes, "");

	//17. Gather parameters: httpProxy
	GATHER_OPTIONAL_STRING(settings, parameters, httpProxy, "");

	//18. Gather parameters: rangeStart
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, rangeStart, -2, 999999999, -2);

	//19. Gather parameters: rangeEnd
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, rangeEnd, -1, 999999999, -1);

	//20. Gather parameters: rangeTimeStart
	GATHER_OPTIONAL_STRING(settings, parameters, rangeTimeStart, "");

	//21. Gather parameters: rangeTimeEnd
	GATHER_OPTIONAL_STRING(settings, parameters, rangeTimeEnd, "");

	if ((rangeStart != -2 || rangeEnd != -1) && (rangeTimeStart != "" || rangeTimeEnd != "")) {
		string result = "Must only fill either rangeStart/End or rangeTimeStart/End, not both";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//22. Gather parameters: sendRenewStream
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, sendRenewStream, false);

	GATHER_OPTIONAL_STRING(settings, parameters, sourceType, "live");

	// This is specific for Avigilon and is used to trigger special PLAY headers
	// to have the Avigilon ACC send a keyframe immediately
	GATHER_OPTIONAL_STRING(settings, parameters, sdpCustParams, "");

#ifdef HAS_MULTIPROGRAM_TS
	//23. Gather parameters: programPID
	GATHER_OPTIONAL_NUMBER(settings, parameters, programPID, 0, 999999999, 0);

	//24. Gather parameters: audioLanguage
	GATHER_OPTIONAL_STRING(settings, parameters, audioLanguage, "");

	//25. Gather parameters: videoPID
	GATHER_OPTIONAL_NUMBER(settings, parameters, videoPID, 0, 999999999, 0);

	//26. Gather parameters: audioPID
	GATHER_OPTIONAL_NUMBER(settings, parameters, audioPID, 0, 999999999, 0);

#endif	/* HAS_MULTIPROGRAM_TS */
	if (((rangeStart < 0) && (rangeStart != -2) && (rangeStart != -1))
			|| ((rangeEnd < 0) && (rangeEnd != -1))) {
		string result = "rangeStart and rangeEnd can only have positive values, -2 or -1";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	if (sourceType != "live" && sourceType != "vod") {
		string result = "sourceType can only have values of live or vod";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//27. Make sure that we have the necessary parameters if we are pulling RTP
	string scheme = uriTester.scheme();
	if (scheme == "rtp") {
		if (isAudio) {
			// Check if we have the audio codec setup
			if (audioCodecBytes == "") {
				string result = "Unable to pull RTP AUDIO stream without codec setup. Please include 'audioCodecBytes' parameter.";
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		} else {
			// Check if we have the PPS and SPS bytes for video
			if ((spsBytes == "") || (ppsBytes == "")) {
				string result = "Unable to pull RTP VIDEO stream without SPS/PPS bytes. Please include 'spsBytes' AND 'ppsBytes' parameter.";
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		}
	}
#ifdef HAS_PROTOCOL_API
	if (scheme != "file" && scheme != "sercom") {
#else
	if (scheme != "file") {
#endif /* HAS_PROTOCOL_API */
		//28. Validate the httpProxy
		trim(httpProxy);
		httpProxy = lowerCase(httpProxy);
		if ((httpProxy != "") && (httpProxy != "self")) {
			//28.a. We have a proxy candidate. Parse and resolve the proxy entry
			uriTester.Reset();
			if (!URI::FromString("http://" + httpProxy, true, uriTester)) {
				string result = format("Unable to parse httpProxy: %s", STR(httpProxy));
				return SendFail(pFrom, result, RESPONSE_ID);
			}
			httpProxy = format("%s:%"PRIu16, STR(uriTester.ip()), uriTester.port());
		} else {
			//28.b. We don't have a proxy entry or is "self". That means that we
			//have to resolve the host name from the URI
			if (!URI::FromString(uri, true, uriTester)) {
				string result = format("Invalid URI: %s", STR(uri));
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		}
		settings["httpProxy"] = httpProxy;
	} else {
		settings["httpProxy"] = "Don'tResolveHost"; // dummy value to bypass resolving of host in baseclientapplication.cpp for file scheme
	}

	GATHER_OPTIONAL_STRING(settings, parameters, videoSourceIndex, "high");
	videoSourceIndex = lowerCase(videoSourceIndex);
	if ("med" != videoSourceIndex && "low" != videoSourceIndex && 
			"high" != videoSourceIndex) {
		settings["videoSourceIndex"] = "high";
	} else {
		settings["videoSourceIndex"] = videoSourceIndex;
	}
	
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, serializeToConfig, true);

	//29. Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	//30. Do the request
	if (!GetApplication()->PullExternalStream(settings)) {
		string result = "Unable to pull requested stream";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//31. Done
	settings["uri"] = uriTester;
	string result = format("Stream %s enqueued for pulling", STR(uri));
	parameters["ok"] = (bool)true;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamName configId keepAlive forceTcp uri scheme port fullUri";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, result, settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessagePushStream(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: uri. Do not resolve the host name yet because
	//we may have a httpProxy entry
	GATHER_MANDATORY_STRING(settings, parameters, uri);
	URI uriTester;
	if (!URI::FromString(uri, false, uriTester)) {
		string result = format("Invalid URI: %s", STR(uri));
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//2. Gather parameters: localStreamName
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);

	//3. Gather parameters: emulateUserAgent
	GATHER_OPTIONAL_STRING(settings, parameters, emulateUserAgent, HTTP_HEADERS_SERVER_US);
	FixEmulateUserAgent(settings, emulateUserAgent, false);

	//4. Gather parameters: targetStreamName
	GATHER_OPTIONAL_STRING(settings, parameters, targetStreamName, localStreamName);

	//5. Gather parameters: targetStreamType
	GATHER_OPTIONAL_STRING(settings, parameters, targetStreamType, "live");
	if ((targetStreamType != "record")
			&& (targetStreamType != "append")
			&& (targetStreamType != "live")) {
		string result = "Syntax error for targetStreamType parameter. Type help for details";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//6. Gather parameters: swfUrl
	GATHER_OPTIONAL_STRING(settings, parameters, swfUrl, "");

	//7. Gather parameters: pageUrl
	GATHER_OPTIONAL_STRING(settings, parameters, pageUrl, "");

	//8. Gather parameters: tcUrl
	GATHER_OPTIONAL_STRING(settings, parameters, tcUrl, "");

	//9. Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	//10. Gather parameters: ttl
	GATHER_OPTIONAL_NUMBER(settings, parameters, ttl, 0, 0x100, 0x100);

	//11. Gather parameters: tos
	GATHER_OPTIONAL_NUMBER(settings, parameters, tos, 0, 0x100, 0x100);

	//12. Gather parameters: forceTcp
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, forceTcp, false);

	//13. Gather parameters: rtmpAbsoluteTimestamps
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, rtmpAbsoluteTimestamps, false);

	//14. Gather parameters: httpProxy
	GATHER_OPTIONAL_STRING(settings, parameters, httpProxy, "");

	//15. Gather parameters: useSourcePts
	bool useSourcePtsDefault = false;
	if (GetApplication() != NULL && GetApplication()->GetConfiguration().HasKeyChain(V_BOOL, false, 1, "useSourcePts")) {
		useSourcePtsDefault = (bool) GetApplication()->GetConfiguration()["useSourcePts"];
	}
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, useSourcePts, useSourcePtsDefault);

	//16. Prepare the push request (minor adjustment: settings expects targetUri instead of uri)
	settings["targetUri"] = uri;
	settings.RemoveKey("uri");

	//16. Validate the httpProxy
	trim(httpProxy);
	httpProxy = lowerCase(httpProxy);
	if ((httpProxy != "") && (httpProxy != "self")) {
		//16.a. We have a proxy candidate. Parse and resolve the proxy entry
		uriTester.Reset();
		if (!URI::FromString("http://" + httpProxy, true, uriTester)) {
			string result = format("Unable to parse httpProxy: %s", STR(httpProxy));
			return SendFail(pFrom, result, RESPONSE_ID);
		}
		httpProxy = format("%s:%"PRIu16, STR(uriTester.ip()), uriTester.port());
	} else {
		//16.b. We don't have a proxy entry or is "self". That means that we
		//have to resolve the host name from the URI
		if (!URI::FromString(uri, true, uriTester)) {
			string result = format("Invalid URI: %s", STR(uri));
			return SendFail(pFrom, result, RESPONSE_ID);
		}
	}
	settings["httpProxy"] = httpProxy;

	//17. Gather RTMP SetChunkSizeRequestOption
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, sendChunkSizeRequest, true);

	//17. Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	//18. Do the request
	if (!GetApplication()->PushLocalStream(settings)) {
		string result = "Unable to push requested stream";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//19. Done
	settings["targetUri"] = uriTester;
	string result = format("Local stream %s enqueued for pushing to %s as %s",
			STR(localStreamName),
			STR(uri), STR(targetStreamName));

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamName configId forceTcp keepAlive targetStreamName targetStreamType targetUri fullUri port scheme";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, result, settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageGenerateLazyPullFile(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: uri. Do not resolve the host name yet because
	//we may have a httpProxy entry
	GATHER_MANDATORY_STRING(settings, parameters, uri);
	URI uriTester;
	if (!URI::FromString(uri, false, uriTester)) {
		string result = format("Invalid URI: %s", STR(uri));
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	parameters["uri"] = uriTester;

	//2. Gather parameters: pathToFile
	GATHER_MANDATORY_STRING(settings, parameters, pathToFile);

	//3. Check pathToFile format
	if (pathToFile.find(".vod") == string::npos) {
		pathToFile = pathToFile + ".vod";
		WARN("Filename format corrected to %s", STR(pathToFile));
	}
	parameters["pathToFile"] = pathToFile;

	// Just for sanity, eliminate localstreamname and keepalive parameters
	if (parameters.HasKey("localStreamName", false)) parameters.RemoveKey("localStreamName", false);

	//4. Gather parameters: forceTcp
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, forceTcp, true);

	//5. Gather parameters: swfUrl
	GATHER_OPTIONAL_STRING(settings, parameters, swfUrl, "");

	//6. Gather parameters: pageUrl
	GATHER_OPTIONAL_STRING(settings, parameters, pageUrl, "");

	//7. Gather parameters: tcUrl
	GATHER_OPTIONAL_STRING(settings, parameters, tcUrl, "");

	//8. Gather parameters: ttl
	GATHER_OPTIONAL_NUMBER(settings, parameters, ttl, 0, 0x100, 0x100);

	//9. Gather parameters: tos
	GATHER_OPTIONAL_NUMBER(settings, parameters, tos, 0, 0x100, 0x100);

	//10. Gather parameters: ssmIp
	GATHER_OPTIONAL_STRING(settings, parameters, ssmIp, "");

	//11. Gather parameters: rtcpDetectionInterval
	GATHER_OPTIONAL_NUMBER(settings, parameters, rtcpDetectionInterval, 0, 60, 10);

	//12. Gather parameters: emulateUserAgent
	GATHER_OPTIONAL_STRING(settings, parameters, emulateUserAgent, HTTP_HEADERS_SERVER_US" player");
	FixEmulateUserAgent(settings, emulateUserAgent, true);

	//13. Gather parameters: isAudio
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, isAudio, true);

	//14. Gather parameters: audioCodecBytes (fixed two bytes?)so
	GATHER_OPTIONAL_STRING(settings, parameters, audioCodecBytes, "");

	//15. Gather parameters: spsBytes
	GATHER_OPTIONAL_STRING(settings, parameters, spsBytes, "");

	//16. Gather parameters: ppsBytes
	GATHER_OPTIONAL_STRING(settings, parameters, ppsBytes, "");

	//17. Gather parameters: ghttpProxy
	GATHER_OPTIONAL_STRING(settings, parameters, httpProxy, "");

	//18. Gather parameters: rangeStart
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, rangeStart, -2, 999999999, -2);

	//19. Gather parameters: rangeEnd
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, rangeEnd, -1, 999999999, -1);

	//20. Gather parameters: rangeTimeStart
	GATHER_OPTIONAL_STRING(settings, parameters, rangeTimeStart, "");

	//21. Gather parameters: rangeTimeEnd
	GATHER_OPTIONAL_STRING(settings, parameters, rangeTimeEnd, "");

	if ((rangeStart != -2 || rangeEnd != -1) && (rangeTimeStart != "" || rangeTimeEnd != "")) {
		string result = "Must only fill either rangeStart/End or rangeTimeStart/End, not both";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//22. Gather parameters: sendRenewStream
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, sendRenewStream, false);

	//23. Gather parameters: sourceType
	GATHER_OPTIONAL_STRING(settings, parameters, sourceType, "live");
	if (sourceType != "live" && sourceType != "vod") {
		string result = "sourceType can only have values of live or vod";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	// This is specific for Avigilon and is used to trigger special PLAY headers
	// to have the Avigilon ACC send a keyframe immediately
	GATHER_OPTIONAL_STRING(settings, parameters, sdpCustParams, "");

	//24. Gather parameters: keepAlive (new parameter via Feature #633 - no stream shutdown in lazypull)
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, false);

	if (((rangeStart < 0) && (rangeStart != -2) && (rangeStart != -1))
			|| ((rangeEnd < 0) && (rangeEnd != -1))) {
		string result = "rangeStart and rangeEnd can only have positive values, -2 or -1";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	//25. Make sure that we have the necessary parameters if we are pulling RTP
	string scheme = uriTester.scheme();
	if (scheme == "rtp") {
		if (isAudio) {
			// Check if we have the audio codec setup
			if (audioCodecBytes == "") {
				string result = "Unable to pull RTP AUDIO stream without codec setup. Please include 'audioCodecBytes' parameter.";
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		} else {
			// Check if we have the PPS and SPS bytes for video
			if ((spsBytes == "") || (ppsBytes == "")) {
				string result = "Unable to pull RTP VIDEO stream without SPS/PPS bytes. Please include 'spsBytes' AND 'ppsBytes' parameter.";
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		}
	}

	//26. Validate the httpProxy
	trim(httpProxy);
	httpProxy = lowerCase(httpProxy);
	if ((httpProxy != "") && (httpProxy != "self")) {
		//26.a. We have a proxy candidate. Parse and resolve the proxy entry
		uriTester.Reset();
		if (!URI::FromString("http://" + httpProxy, true, uriTester)) {
			string result = format("Unable to parse httpProxy: %s", STR(httpProxy));
			return SendFail(pFrom, result, RESPONSE_ID);
		}
		httpProxy = format("%s:%"PRIu16, STR(uriTester.ip()), uriTester.port());
	} else {
		//26.b. We don't have a proxy entry or is "self". That means that we
		//have to resolve the host name from the URI
		if (!URI::FromString(uri, true, uriTester)) {
			string result = format("Invalid URI: %s", STR(uri));
			return SendFail(pFrom, result, RESPONSE_ID);
		}
	}
	settings["httpProxy"] = httpProxy;

	//27. Gather video source index
	GATHER_OPTIONAL_STRING(settings, parameters, videoSourceIndex, "high");
	videoSourceIndex = lowerCase(videoSourceIndex);
	if ("med" != videoSourceIndex && "low" != videoSourceIndex &&
		"high" != videoSourceIndex) {
		settings["videoSourceIndex"] = "high";
	} else {
		settings["videoSourceIndex"] = videoSourceIndex;
	}

	//28. Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	//29. Do the request
	File lazyFile;
	if (!lazyFile.Initialize(pathToFile, FILE_OPEN_MODE_TRUNCATE)) {
		string result = format("Cannot write to %s", STR(pathToFile));
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	FOR_MAP(settings, string, Variant, i) {
		// exclude pathFile and parameters that are not set
		if (MAP_KEY(i) == "pathToFile" || !parameters.HasKey(MAP_KEY(i), false))
			continue;

		string fileEntry = format("%s=%s\n", STR(MAP_KEY(i)), STR(VariantToBoolCheck(MAP_VAL(i))));
		lazyFile.WriteString(fileEntry);
	}
	lazyFile.Close();

	//26. Done
	settings["uri"] = uriTester;
	string result = format("Stream parameters written to %s", STR(pathToFile));
	parameters["ok"] = (bool)true;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "forceTcp keepAlive pathToFile uri fullUri";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, result, settings, RESPONSE_ID);
}
bool CLIAppProtocolHandler::ProcessMessageListStreamsIds(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listStreamsIds";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);
	// Gather parameters: disableInternalStreams
	GATHER_OPTIONAL_BOOLEAN(callbackInfo, parameters, disableInternalStreams, true);

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *)GetApplication()->GetProtocolHandler((uint64_t)PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageGetStreamsCount(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "getStreamsCount";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);
	// Gather parameters: disableInternalStreams
	GATHER_OPTIONAL_BOOLEAN(callbackInfo, parameters, disableInternalStreams, true);

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *)GetApplication()->GetProtocolHandler((uint64_t)PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageClientsConnected(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	Variant callbackInfo;

	//1.  Prepare the callback for the edge/origin dialog
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "clientsConnected";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//2. Gather optional parameter: localStreamName
	GATHER_OPTIONAL_STRING(callbackInfo, parameters, localStreamName, "");

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *)GetApplication()->GetProtocolHandler((uint64_t)PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;

}

bool CLIAppProtocolHandler::ProcessMessageHttpClientsConnected(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	//1. Gather optional parameter: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");

	string grpName = (string) settings["groupName"];

	//2. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["groupName"] = grpName;
	callbackInfo["operation"] = "getHttpClientsConnected";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//2. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "httpStreamingSessionsCount groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

/**
 * Used by getStreamInfo API command
 * Used the streamId to find a specific stream.
 * Returns all the information of the specified stream
 */
bool CLIAppProtocolHandler::GetLocalStreamInfo(uint32_t streamId, Variant &result) {
	result.Reset();

	//1. Get the stream stats
	BaseStream *pStream = _pSM->FindByUniqueId(streamId);
	if (pStream == NULL) {
		FATAL("Stream not found");
		return false;
	}
	pStream->GetStats(result, 0);

	//2. Try to get the push/pull config if necessary
	BaseProtocol *pProtocol = pStream->GetProtocol();
	if (pProtocol != NULL) {
		Variant streamConfig;
		OperationType operationType = _pApp->GetOperationType(pProtocol, streamConfig);
		if (operationType != OPERATION_TYPE_STANDARD) {
			string key = "";
			switch (operationType) {
				case OPERATION_TYPE_PULL:
				{
					result["pullSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_PUSH:
				{
					result["pushSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_HLS:
				{
					result["hlsSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_HDS:
				{
					result["hdsSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_MSS:
				{
					result["mssSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_DASH:
				{
					result["dashSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_RECORD:
				{
					result["recordSettings"] = streamConfig;
					break;
				}
				default:
				{
					WARN("streamConfig %s can't be persisted", STR(streamConfig.ToString()));
				}
			}
		}
	}
	return true;
}

/**
 * Used by getStreamInfo API command
 * Used the localStreamName to find a specific stream.
 * Returns all the information of the specified stream
 */
bool CLIAppProtocolHandler::GetLocalStreamInfo(const string& streamName, Variant &result) {
	result.Reset();

	//1. Get the stream stats
	BaseStream *pStream = NULL;
	map<uint32_t, BaseStream *> pStreamMap = _pSM->FindByName(streamName);
	FOR_MAP(pStreamMap, uint32_t, BaseStream*, i){
		pStream = MAP_VAL(i);
		if(pStream) break;
	}
	if (pStream == NULL) {
		FATAL("Stream not found");
		return false;
	}
	pStream->GetStats(result, 0);

	//2. Try to get the push/pull config if necessary
	BaseProtocol *pProtocol = pStream->GetProtocol();
	if (pProtocol != NULL) {
		Variant streamConfig;
		OperationType operationType = _pApp->GetOperationType(pProtocol, streamConfig);
		if (operationType != OPERATION_TYPE_STANDARD) {
			string key = "";
			switch (operationType) {
				case OPERATION_TYPE_PULL:
				{
					result["pullSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_PUSH:
				{
					result["pushSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_HLS:
				{
					result["hlsSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_HDS:
				{
					result["hdsSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_MSS:
				{
					result["mssSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_DASH:
				{
					result["dashSettings"] = streamConfig;
					break;
				}
				case OPERATION_TYPE_RECORD:
				{
					result["recordSettings"] = streamConfig;
					break;
				}
				default:
				{
					WARN("streamConfig %s can't be persisted", STR(streamConfig.ToString()));
				}
			}
		}
	}
	return true;
}

/**
 * Used by isStreamRunning API command
 * Checks a specific stream using streamId if it is running or not.
 * Return the status in boolean form i.e. true or false
 */
bool CLIAppProtocolHandler::GetStreamStatus(Variant& callbackInfo){
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	bool status = false;
	Variant data;

	Variant callbackInfoLocal = callbackInfo;

	if (callbackInfo.HasKey("edges", true)) {
		FOR_MAP(callbackInfo["edges"], string, Variant, edge) {
			if (MAP_VAL(edge).HasKey("streamStatus", true)) {
				callbackInfoLocal = MAP_VAL(edge);
				break;
			}
		}
	}

	if (!callbackInfoLocal.HasKey("streamStatus", true)) {
		BaseStream *pStream = _pSM->FindByUniqueId(callbackInfoLocal["streamId"]);
		if (pStream) {
			status = true;
		}
	} else { // Status came from one of the edges
		status = callbackInfo["streamStatus"];
	}
	
	data["Running"] = status;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "Running";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Stream status:", data, RESPONSE_ID);
}

/**
 * Used by isStreamRunning API command
 * Checks a specific stream using localStreamName if it is running or not.
 * Return the status in boolean form i.e. true or false
 */
bool CLIAppProtocolHandler::GetStreamStatus(const string& localStreamName, Variant& callbackInfo){
	BaseStream *pStream = NULL;
	bool status = false;
	map<uint32_t, BaseStream *> pStreamMap = _pSM->FindByName(localStreamName);
	FOR_MAP(pStreamMap, uint32_t, BaseStream*, i){
		pStream = MAP_VAL(i);
		if(pStream) {
			status = true;
			break;
		}
	}

	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	Variant data;

	data["Running"] = status;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "Running";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Stream status:", data, RESPONSE_ID);
}

/**
 * Used by getServerInfo API command
 * Returns a bunch of information regarding the configuration of the running RMS.
 */
bool CLIAppProtocolHandler::GetServerInfo(Variant& serverInfo){
	//version information
	serverInfo = Version::GetAll();
	Variant appConfiguration = ClientApplicationManager::GetDefaultApplication()->GetConfiguration();
	//media folder
	serverInfo["mediafolder"] = appConfiguration["mediaStorage"][1]["mediaFolder"];
	//connections count
	serverInfo["connectionsCount"] = _pApp->GetConnectionsCount();
	//current streams count
	serverInfo["streamsCount"] = (uint32_t)_pSM->GetAllStreams().size();
	//acceptors information (port, ip ...)
	serverInfo["acceptors"] = appConfiguration["acceptors"];
#ifdef HAS_LICENSE
	License *pLicense = License::GetLicenseInstance();

	//get license expiration time(epoch), convert it to human readable format
	time_t expirationTime = 0;
	struct tm* expTimeHuman = NULL;
	char buffer[30];
	expirationTime = (time_t) pLicense->GetExpirationTime();
	expTimeHuman = localtime(&expirationTime);
	strftime (buffer, sizeof(buffer), "%B %d, %Y %I:%M%p.", expTimeHuman);

	//license expiration time with format (e.g. January 01, 2014 1:00 PM)
	serverInfo["licenseExpiration"] = buffer;
#endif /* HAS_LICENSE */
	char hostBuffer[255];
	struct hostent *hostInformation;
	char *hostLocalIP;

	gethostname(hostBuffer, sizeof(hostBuffer));
	hostInformation = gethostbyname(hostBuffer);
	hostLocalIP = inet_ntoa (*(struct in_addr *)*hostInformation->h_addr_list);
	serverInfo["Server IP"] = hostLocalIP;

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageGetStreamInfo(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	if ((!parameters.HasKeyChain(V_STRING, false, 1, "id"))
			&& (!parameters.HasKeyChain(V_STRING, false, 1, "localStreamName"))) {
		return SendFail(pFrom, "id or localStreamName parameters needs to be supplied", RESPONSE_ID);
	}

	if ((parameters.HasKeyChain(V_STRING, false, 1, "id"))
			&& (parameters.HasKeyChain(V_STRING, false, 1, "localStreamName"))) {
		return SendFail(pFrom, "id and localStreamName must not go together, choose only 1", RESPONSE_ID);
	}

	Variant data;

	if (parameters.HasKey("id")) {
		//1. Gather parameters: id
		GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffffffffffffLL);

		//2. split the id into the edge id and stream id
		uint32_t edgeId = (uint32_t) (id >> 32);
		uint32_t streamId = (uint32_t) (id & 0xffffffff);

		if (edgeId == 0) {
			if (!GetLocalStreamInfo(streamId, data)) {
				return SendFail(pFrom, format("Stream 0:%"PRIu32" not found", streamId), RESPONSE_ID);
			}
		} else {
			//1. Prepare the callback for the edge/origin dialog
			Variant callbackInfo;
			callbackInfo["cliProtocolId"] = pFrom->GetId();
			callbackInfo["operation"] = "getStreamInfo";
			callbackInfo["streamId"] = streamId;
			callbackInfo["edgeId"] = edgeId;
			SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

		//2. Get the origin handler
		OriginVariantAppProtocolHandler *pOrigin =
				(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
		if (pOrigin == NULL) {
			FATAL("Unable to get the handler");
			return false;
		}

		//3. Do the call
		if (!pOrigin->EnqueueCall(callbackInfo)) {
			return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
		}

			return true;
		}
	} else {
		/**
		 * Get the stream information of the stream indicated by localStreamName
		 */
		//Gather parameters: name
		GATHER_MANDATORY_STRING(settings, parameters, localStreamName);
		if (!GetLocalStreamInfo(localStreamName, data)) {
			return SendFail(pFrom, format("Stream %s not found", STR(localStreamName)), RESPONSE_ID);
		}
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// for origin response; see CLIAppProtocolHandler::GetStreamInfoResult for edge response
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Stream information", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListStreams(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listStreams";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);
	// Gather parameters: disableInternalStreams
	GATHER_OPTIONAL_BOOLEAN(callbackInfo, parameters, disableInternalStreams, true);

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

/**
 * Process the listInboundStreamNames API command
 */
bool CLIAppProtocolHandler::ProcessMessageListInboundStreamNames(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listInboundStreamNames";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);
	// Gather parameters: disableInternalStreams
	GATHER_OPTIONAL_BOOLEAN(callbackInfo, parameters, disableInternalStreams, true);

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *)GetApplication()->GetProtocolHandler((uint64_t)PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

/**
 * Process the listInboundStreamNames API command
 */
bool CLIAppProtocolHandler::ProcessMessageListInboundStreams(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listInboundStreams";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);
	// Gather parameters: disableInternalStreams
	GATHER_OPTIONAL_BOOLEAN(callbackInfo, parameters, disableInternalStreams, true);

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *)GetApplication()->GetProtocolHandler((uint64_t)PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

/**
 * Process the isStreamRunning API command
 */
bool CLIAppProtocolHandler::ProcessMessageIsStreamRunning(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	if ((!parameters.HasKeyChain(V_STRING, false, 1, "id"))
			&& (!parameters.HasKeyChain(V_STRING, false, 1, "localStreamName"))) {
		return SendFail(pFrom, "id or localStreamName parameters needs to be supplied", RESPONSE_ID);
	}

	if ((parameters.HasKeyChain(V_STRING, false, 1, "id"))
			&& (parameters.HasKeyChain(V_STRING, false, 1, "localStreamName"))) {
		return SendFail(pFrom, "id and localStreamName must not go together, choose only 1", RESPONSE_ID);
	}

	// Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "Running";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	if (parameters.HasKey("id")) {
		//1. Gather parameters: id
		GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffffffffffffLL);

		//2. split the id into the edge id and stream id
		uint32_t edgeId = (uint32_t) (id >> 32);
		uint32_t streamId = (uint32_t) (id & 0xffffffff);
		callbackInfo["streamId"] = streamId;

		if (edgeId == 0) {
			if (!GetStreamStatus(callbackInfo)) {
				return SendFail(pFrom, format("Stream 0:%"PRIu32" not found", streamId), RESPONSE_ID);
			}
		} else {
			callbackInfo["operation"] = "isStreamRunning";
			callbackInfo["edgeId"] = edgeId;
			// Get the origin handler
			OriginVariantAppProtocolHandler *pOrigin =
					(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
			if (pOrigin == NULL) {
				FATAL("Unable to get the handler");
				return false;
			}

			// Do the call
			if (!pOrigin->EnqueueCall(callbackInfo)) {
				return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
			}

			return true;
		}
	} else {
		// Gather parameters: localstreamname
		GATHER_MANDATORY_STRING(settings, parameters, localStreamName);
		if (!GetStreamStatus(localStreamName, callbackInfo)) {
			return SendFail(pFrom, format("Stream %s not found", STR(localStreamName)), RESPONSE_ID);
		}
	}

	return true;
}

/**
 * Process the getServerInfo API command
 */
bool CLIAppProtocolHandler::ProcessMessageGetServerInfo(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant serverInfo;

	if (!GetServerInfo(serverInfo)) {
		return SendFail(pFrom, format("Server information not available"), RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		serverInfo["asciiList"]["names"] = "licenseExpiration mediafolder releaseNumber buildNumber acceptors ip port protocol";
		//serverInfo["asciiList"]["debug"] = 1;
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Server information:", serverInfo, RESPONSE_ID);
}

/**
* Process the UploadMedia API command
*/
bool CLIAppProtocolHandler::ProcessMessageUploadMedia(BaseProtocol *pFrom,
	Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	Variant settings;

	GATHER_MANDATORY_NUMBER(settings, parameters, port, 1, 65535);
	GATHER_MANDATORY_STRING(settings, parameters, targetFolder);

	vector<uint64_t> protocolChain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_INBOUND_HTTP_MEDIA_RCV);
	if (protocolChain.empty()) {
		return SendFail(pFrom, "Invalid protocol", RESPONSE_ID);
	}

	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();
	FOR_MAP(services, uint32_t, IOHandler *, i) {
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pAcceptor = (TCPAcceptor *)MAP_VAL(i);
		if ((pAcceptor->GetParameters()[CONF_IP] == "0.0.0.0")
			&& ((uint16_t)pAcceptor->GetParameters()[CONF_PORT] == port)) {
			return SendFail(pFrom, format("0.0.0.0:%u is taken", port), RESPONSE_ID);
		}
	}

	Variant config;
	config[CONF_IP] = "0.0.0.0";
	config[CONF_PORT] = port;
	config["targetFolder"] = targetFolder;

	TCPAcceptor *pAcceptor = new TCPAcceptor("0.0.0.0", (uint16_t)port, config, protocolChain);
	if (!pAcceptor->Bind()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		return SendFail(pFrom, "Unable to bind acceptor", RESPONSE_ID);
	}
	pAcceptor->SetApplication(GetApplication());
	if (!pAcceptor->StartAccept()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		return SendFail(pFrom, "Unable to start acceptor", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		config["asciiList"]["names"] = "ip port targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Media upload acceptor configuration. ", config, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageShutdownStream(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	BaseStream *pStream = NULL;

	if ((!parameters.HasKeyChain(V_STRING, false, 1, "id"))
			&& (!parameters.HasKeyChain(V_STRING, false, 1, "localStreamName"))) {
		return SendFail(pFrom, "id or localStreamName parameters needs to be supplied", RESPONSE_ID);
	}

	if (parameters.HasKey("id")) {
		// Gather parameters: id
		GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffffffffffffLL);

		// Check if we need to target the streams on the edge servers
		if ((id & 0xffffffff00000000LL) != 0) {
			// Split the id into the edge id and stream id
			uint32_t edgeId = (uint32_t) (id >> 32);
			uint32_t streamId = (uint32_t) (id & 0xffffffff);

			Variant callbackInfo;
			callbackInfo["cliProtocolId"] = pFrom->GetId();
			callbackInfo["operation"] = "shutdownStream";
			callbackInfo["edgeId"] = edgeId;
			callbackInfo["streamId"] = streamId;
			SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

			// Get the origin handler
			OriginVariantAppProtocolHandler *pOrigin =
					(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
			if (pOrigin == NULL) {
				FATAL("Unable to get the handler");
				return false;
			}

			// Do the call
			if (!pOrigin->EnqueueCall(callbackInfo)) {
				return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
			}

			// Exit
			return true;
		}

		pStream = _pSM->FindByUniqueId((uint32_t) id);
	} else {
		// ID not given, get the localStreamName instead
		GATHER_MANDATORY_STRING(settings, parameters, localStreamName);
		map<uint32_t, BaseStream *> streams = _pSM->FindByTypeByName(ST_IN, localStreamName, true, false);
		if (streams.size() > 0)
			pStream = MAP_VAL(streams.begin());
	}

	// Gather parameters: permanently
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, permanently, true);

	// Get the stream in question
	if (pStream == NULL) {
		return SendFail(pFrom, format("Stream no longer available"), RESPONSE_ID);
	}

	// Get the protocol associated with the stream
	BaseProtocol *pProtocol = pStream->GetProtocol();
	if (pProtocol == NULL) {
		return SendFail(pFrom, format("Stream no longer available"), RESPONSE_ID);
	}

	// If this needs to be permanently shut down, also remove the pull/push configuration
	if (permanently && pProtocol != NULL) {
		Variant streamConfig;
		OperationType operationType = _pApp->GetOperationType(pProtocol, streamConfig);
		pProtocol->GetCustomParameters().RemoveKey("customParameters");
		if (operationType != OPERATION_TYPE_STANDARD) {
			_pApp->RemoveConfig(streamConfig, operationType, false);
		}
	}

	// Get the last stats about the protocol and the stream
	Variant data;
	pProtocol->GetStackStats(data["protocolStackInfo"], 0);
	pStream->GetStats(data["streamInfo"], 0);

	// Enqueue for delete
	pProtocol->EnqueueForDelete();

#ifdef HAS_PROTOCOL_ASCIICLI
	// for origin response; see CLIAppProtocolHandler::ShutdownStreamResult for edge response
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "streamInfo name uniqueId type inStreamUniqueId outStreamsUniqueIds farIp nearIp processId processType audio video bytesCount codec"; //removed: serverAgent, protocolStackInfo...
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Stream closed", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListConfig(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	//2. Get the list and return
	Variant data = _pApp->GetStreamsConfig();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "dash hds hls mss process pull push record webrtc metalistener configId localStreamName port targetFolder streamNames groupName status current description uniqueStreamId uri targetUri targetStreamName pathToFile fullBinaryPath rrsip rrsport roomid";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Run-time configuration", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageGetConfigInfo(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffffffffffffLL);

	Variant data;
	if (_pApp->GetConfig( data, (uint32_t) id)) {
#ifdef HAS_PROTOCOL_ASCIICLI
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		        data["asciiList"]["names"] = "configId localStreamName targetFolder streamNames groupName operationType status current previous description uniqueStreamId keepAlive swfUrl tcUrl uri targetUri targetStreamName pathToFile fullBinaryPath chunkLength playlistType";
		}
#endif /* HAS_PROTOCOL_ASCIICLI */
		return SendSuccess(pFrom, "Configuration Info", data, RESPONSE_ID);
	} else {
		return SendFail(pFrom, "Config ID not found", RESPONSE_ID);
	}
}
bool CLIAppProtocolHandler::ProcessMessageRemoveConfig(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: id
	uint32_t configId = 0;
	string group = "";
	if (parameters.HasKey("id", false)) {
		GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffff);
		configId = (uint32_t) id;
	} else {
		if (parameters.HasKey("groupName", false)) {
			GATHER_MANDATORY_STRING(settings, parameters, groupName);
			group = groupName;
		} else if (parameters.HasKey("hlsHdsMssGroup", false)) {
			GATHER_MANDATORY_STRING(settings, parameters, hlsHdsMssGroup);
			group = hlsHdsMssGroup;
		}
		trim(group);
	}
	if ((configId == 0)&&(group == "")) {
		FATAL("removeConfig failed. No id nor group name provided");
		return SendFail(pFrom, "No id nor group name provided", RESPONSE_ID);
	}

	//6. Gather parameters: removeHlsHdsFiles
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, removeHlsHdsFiles, false);

	//4. Get all the streams
	map<uint32_t, BaseProtocol *> allProtocols = ProtocolManager::GetActiveProtocols();

	//5. for each stream get the info
	Variant streamConfigSample;
	OperationType operationType = OPERATION_TYPE_STANDARD;

	FOR_MAP(allProtocols, uint32_t, BaseProtocol *, i) {
		//7. Get the stream configuration
		Variant streamConfig;
		operationType = _pApp->GetOperationType(MAP_VAL(i), streamConfig);
		if (operationType == OPERATION_TYPE_STANDARD)
			continue;

		if (configId != 0) {
			//8. inspect the config id
			if ((uint32_t) streamConfig["configId"] != configId)
				continue;
		} else {
			if ((operationType != OPERATION_TYPE_HLS)
					&& (operationType != OPERATION_TYPE_HDS)
					&& (operationType != OPERATION_TYPE_MSS)
					&& (operationType != OPERATION_TYPE_DASH))
				continue;
			if (streamConfig["groupName"] != group)
				continue;
		}
		streamConfigSample = streamConfig;

		//9. Terminate the connection
		MAP_VAL(i)->GetCustomParameters().RemoveKey("customParameters");
		// Set the files to be deleted if needed
		if (removeHlsHdsFiles
				&& ((operationType == OPERATION_TYPE_HLS)
				|| (operationType == OPERATION_TYPE_HDS)
				|| (operationType == OPERATION_TYPE_MSS)
				|| (operationType == OPERATION_TYPE_DASH))) {
			MAP_VAL(i)->GetCustomParameters()["IsTaggedForDeletion"] = (bool)true;
		}

		if (operationType == OPERATION_TYPE_WEBRTC) {
			MAP_VAL(i)->GetCustomParameters()["keepAlive"] = bool(false);
		}

		MAP_VAL(i)->EnqueueForDelete();


#ifdef HAS_PROTOCOL_DASH
		if (operationType == OPERATION_TYPE_DASH) {
			Variant config;
			if (_pApp->GetConfig(config, configId)) {
				group = (string)config["groupName"];
				_pApp->RemoveDASHGroup(group, false);
			}
		}
#endif
#ifdef HAS_PROTOCOL_MSS
		if (operationType == OPERATION_TYPE_MSS) {
			Variant config;
			if (_pApp->GetConfig(config, configId)) {
				group = (string)config["groupName"];
				_pApp->RemoveMSSGroup(group, false);
			}
		}
#endif
		//10. Remove the pullPush
		// Default to false for removal of hls/hds/mss files since this is handled above
		_pApp->RemoveConfig(streamConfig, operationType, false);

		break;
	}

	//11. Remove the config by id if no streams were found
	if (configId != 0) {
#ifdef HAS_PROTOCOL_DASH
		if (operationType == OPERATION_TYPE_DASH) {
			Variant config;
			if (_pApp->GetConfig(config, configId)) {
				group = (string)config["groupName"];
				_pApp->RemoveDASHGroup(group, false);
			}
		}
#endif
#ifdef HAS_PROTOCOL_MSS
		if (operationType == OPERATION_TYPE_MSS) {
			Variant config;
			if (_pApp->GetConfig(config, configId)) {
				group = (string)config["groupName"];
				_pApp->RemoveMSSGroup(group, false);
			}
		}
#endif
		_pApp->RemoveConfigId(configId, removeHlsHdsFiles);
	} else {
#ifdef HAS_PROTOCOL_HLS
		_pApp->RemoveHLSGroup(group, removeHlsHdsFiles);
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
		_pApp->RemoveHDSGroup(group, removeHlsHdsFiles);
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
		_pApp->RemoveMSSGroup(group, removeHlsHdsFiles);
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
		_pApp->RemoveDASHGroup(group, removeHlsHdsFiles);
#endif /* HAS_PROTOCOL_DASH */
		_pApp->RemoveProcessGroup(group);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		streamConfigSample["asciiList"]["names"] = "configId localStreamName targetFolder uri";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//12. Done
	return SendSuccess(pFrom, "Configuration terminated", streamConfigSample, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListServices(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();

	Variant data;

	FOR_MAP(handlers, uint32_t, IOHandler *, i) {
		Variant item;

		IOHandler *pHandler = MAP_VAL(i);
		if ((pHandler->GetType() != IOHT_ACCEPTOR)
				&& (pHandler->GetType() != IOHT_UDP_CARRIER))
			continue;
		item.Reset();
		pHandler->GetStats(item, 0);
		data.PushToArray(item);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "id appName appId protocol port acceptedConnectionsCount enabled nearPort type";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Available services", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageEnableService(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: id
	GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffff);

	//2. Gather parameters: enable
	GATHER_MANDATORY_BOOLEAN(settings, parameters, enable);

	//3. Get the handler
	map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();
	if (!MAP_HAS1(handlers, (uint32_t) id)) {
		return SendFail(pFrom, format("Service with id %u not found", id), RESPONSE_ID);
	}
	if (handlers[(uint32_t) id]->GetType() != IOHT_ACCEPTOR) {
		return SendFail(pFrom, format("Service with id %u not found", id), RESPONSE_ID);
	}
	TCPAcceptor *pAcceptor = (TCPAcceptor *) handlers[(uint32_t) id];

	//4. Activate/deactivate it
	pAcceptor->Enable(enable);

	//5. Done
	Variant stats;
	pAcceptor->GetStats(stats, 0);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		stats["asciiList"]["names"] = "id appName appId protocol port acceptedConnectionsCount enabled";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Status changed", stats, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageShutdownService(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	//1. Gather parameters: id
	GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffff);

	//2. Get the service
	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();

	if (!MAP_HAS1(services, (uint32_t) id)) {
		return SendFail(pFrom, format("Service with id %u not found", id), RESPONSE_ID);
	}
	if (services[(uint32_t) id]->GetType() != IOHT_ACCEPTOR) {
		return SendFail(pFrom, format("Service with id %u not found", id), RESPONSE_ID);
	}

	//3. Gather some last minute info
	Variant info;
	services[(uint32_t) id]->GetStats(info, 0);

	Variant data = _pApp->GetStreamsConfig();
	if (data.HasKey("metalistener")) {
		Variant streamConfig = data["metalistener"];
		FOR_MAP(streamConfig, string, Variant, i) {
			if ((uint32_t)(MAP_VAL(i)["acceptor"]["id"]) == (uint32_t)id) {
				((TCPAcceptor*)services[(uint32_t)id])->Enable(false);
				_pApp->RemoveConfigId(MAP_VAL(i)["configId"], false);
			}
		}
	}

	//4. Shut it down
	IOHandlerManager::EnqueueForDelete(services[(uint32_t) id]);

	// Select items to list for ASCII CLI
	//if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
	//	info["asciiList"]["names"] = "id appName appId protocol port acceptedConnectionsCount enabled";
	//}

	return SendSuccess(pFrom, format("Service %u removed", id), info, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageCreateService(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	
	//1. Gather parameters: ip
	GATHER_MANDATORY_STRING(settings, parameters, ip);
	if (getHostByName(ip) != ip) {
		return SendFail(pFrom, "Invalid ip", RESPONSE_ID);
	}

	//2. Gather parameters: port
	GATHER_MANDATORY_NUMBER(settings, parameters, port, 1, 65535);

	//3. Gather parameters: proto
	GATHER_MANDATORY_STRING(settings, parameters, protocol);
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(protocol);
	if (chain.size() == 0) {
		return SendFail(pFrom, "Invalid protocol", RESPONSE_ID);
	}
	if (chain[0] != PT_TCP) {
		return SendFail(pFrom, "Invalid protocol", RESPONSE_ID);
	}

	//4. Do we already have a service bound on ip:port?
	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();

	FOR_MAP(services, uint32_t, IOHandler *, i) {
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pAcceptor = (TCPAcceptor *) MAP_VAL(i);
		if ((pAcceptor->GetParameters()[CONF_IP] == ip)
				&& ((uint16_t) pAcceptor->GetParameters()[CONF_PORT] == port)) {
			return SendFail(pFrom, format("%s:%u is taken", STR(ip), port), RESPONSE_ID);
		}
	}

	//5. create the configuration
	Variant config;
	config[CONF_IP] = ip;
	config[CONF_PORT] = (uint16_t) port;
	config[CONF_PROTOCOL] = protocol;

	//6. Spawn the service
	TCPAcceptor *pAcceptor = new TCPAcceptor(ip, (uint16_t) port, config, chain);
	if (!pAcceptor->Bind()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		return SendFail(pFrom, "Unable to bind acceptor", RESPONSE_ID);
	}
	pAcceptor->SetApplication(GetApplication());
	if (!pAcceptor->StartAccept()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		return SendFail(pFrom, "Unable to start acceptor", RESPONSE_ID);
	}

	//7. Get some info
	Variant info;
	pAcceptor->GetStats(info, 0);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		info["asciiList"]["names"] = "id appName appId protocol port acceptedConnectionsCount enabled";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//8. Done
	return SendSuccess(pFrom, "Service created", info, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListConnectionsIds(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	//1. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listConnectionsIds";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//2. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//3. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageGetExtendedConnectionCounters(BaseProtocol* pFrom, Variant& parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Get the local stats
	FdStats &stats = IOHandlerManager::GetStats(true);
	Variant data;
	data["origin"] = stats.ToVariant();

	// Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	// Get the list of edges
	map<uint64_t, EdgeServer> &edges = pOrigin->GetEdges();

	int64_t total = stats.Current();

	// Get the edges stats
	FOR_MAP(edges, uint64_t, EdgeServer, e) {
#ifdef HAS_WEBSERVER
		if (MAP_VAL(e).IsWebServer())
			continue;
#endif
		total += (int64_t) MAP_VAL(e).IOStats()["grandTotal"]["current"];
		data["edges"].PushToArray(MAP_VAL(e).IOStats());
	}
	data["total"] = (int64_t) total;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "origin grandTotal managedNonTcpUdp managedTcp managedTcpAcceptors managedTcpConnectors managedUdp rawUdp current";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Connection counters", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageGetConnectionsCount(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// for each stream get the info
	Variant data;
	data["count"] = (int64_t) _pApp->GetConnectionsCount();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "count";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Active connections count", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetLocalConnectionInfo(uint32_t connectionId, Variant &result) {
	result.Reset();
	URI u;

	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(connectionId);
	if (pProtocol == NULL) {
		FATAL("Connection not found");
		return false;
	}

	//5. Get the stats
	pProtocol->GetStackStats(result, 0);

	//6. Try to get the push/pull config if necessary
	if (pProtocol != NULL) {
		Variant streamConfig;
		OperationType operationType = _pApp->GetOperationType(pProtocol, streamConfig);
		if (operationType != OPERATION_TYPE_STANDARD) {
			string key = "";
			switch (operationType) {
				case OPERATION_TYPE_PULL:
				{
					key = "pullSettings";
					result[key] = streamConfig;
					URI::FromString(result[key]["uri"], false, u);
					result[key]["uri"] = u;
					break;
				}
				case OPERATION_TYPE_PUSH:
				{
					key = "pushSettings";
					result[key] = streamConfig;
					URI::FromString(result[key]["targetUri"], false, u);
					result[key]["targetUri"] = u;
					break;
				}
				case OPERATION_TYPE_HLS:
				{
					key = "hlsSettings";
					result[key] = streamConfig;
					break;
				}
				case OPERATION_TYPE_HDS:
				{
					key = "hdsSettings";
					result[key] = streamConfig;
					break;
				}
				case OPERATION_TYPE_MSS:
				{
					key = "mssSettings";
					result[key] = streamConfig;
					break;
				}
				case OPERATION_TYPE_DASH:
				{
					key = "dashSettings";
					result[key] = streamConfig;
					break;
				}
				case OPERATION_TYPE_RECORD:
				{
					key = "recordSettings";
					result[key] = streamConfig;
					break;
				}
				default:
				{
					WARN("streamConfig %s can't be persisted", STR(streamConfig.ToString()));
				}
			}
		}
	}
	return true;
}

bool CLIAppProtocolHandler::ProcessMessageGetConnectionInfo(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: id
	GATHER_MANDATORY_NUMBER(settings, parameters, id, 0, 0xffffffffffffffffLL);

	//2. split the id into the edge id and stream id
	uint32_t edgeId = (uint32_t) (id >> 32);
	uint32_t connectionId = (uint32_t) (id & 0xffffffff);

	if (edgeId == 0) {
		Variant data;
		if (!GetLocalConnectionInfo(connectionId, data)) {
			return SendFail(pFrom, format("Connection 0:%"PRIu32" not found", connectionId), RESPONSE_ID);
		}

#ifdef HAS_PROTOCOL_ASCIICLI
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
			data["asciiList"]["names"] = "carrier id type stack applicationId type";
		}
#endif /* HAS_PROTOCOL_ASCIICLI */

		return SendSuccess(pFrom, "Connection information", data, RESPONSE_ID);
	} else {
		//1. Prepare the callback for the edge/origin dialog
		Variant callbackInfo;
		callbackInfo["cliProtocolId"] = pFrom->GetId();
		callbackInfo["operation"] = "getConnectionInfo";
		callbackInfo["connectionId"] = connectionId;
		callbackInfo["edgeId"] = edgeId;
		SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

		//2. Get the origin handler
		OriginVariantAppProtocolHandler *pOrigin =
				(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
		if (pOrigin == NULL) {
			FATAL("Unable to get the handler");
			return false;
		}

		//3. Do the call
		if (!pOrigin->EnqueueCall(callbackInfo)) {
			return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
		}

		return true;
	}
}

bool CLIAppProtocolHandler::ProcessMessageListConnections(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	//1. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listConnections";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//2. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//3. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

//
// COMMAND: sendwebrtccommand <command=command_name> 
//                            <localstreamname=target_stream> 
//                            <argument=payload>
//
bool CLIAppProtocolHandler::ProcessMessageSendWebRTCCommand(BaseProtocol * pFrom, Variant & parameters) {
	EXTRACT_RESPONSE_ID(parameters);
	Variant settings;
	string failureMessage;

	//
	// Validate parameters
	//
	if (parameters.HasKeyChain(V_STRING, false, 1, "command")) {
		GATHER_MANDATORY_STRING(settings, parameters, command);
		settings["command"] = command;
	} else {
		failureMessage = "missing command ";
	}
	if (parameters.HasKeyChain(V_STRING, false, 1, "localstreamname")) {
		GATHER_MANDATORY_STRING(settings, parameters, localstreamname);
		settings["localstreamname"] = localstreamname;
	} else {
		failureMessage += "missing targetstream ";
	}
	if (parameters.HasKeyChain(V_STRING, false, 1, "argument")) {
		GATHER_MANDATORY_STRING(settings, parameters, argument);
		settings["argument"] = argument;
	} else {
		failureMessage += "missing argument ";
	}

	GATHER_OPTIONAL_BOOLEAN(settings, parameters, base64, false);
	settings["base64"] = base64?"true":"false";

	if (!failureMessage.empty()) {
		return SendFail(pFrom, failureMessage, RESPONSE_ID);
	}

	if (!_pApp->SendWebRTCCommand(settings)) {
		failureMessage = "Command not sent.";
		return SendFail(pFrom, failureMessage, RESPONSE_ID);
	} else {
		return SendSuccess(pFrom, "Command sent.", settings, RESPONSE_ID);
	}
}
#ifdef HAS_PROTOCOL_HLS

bool CLIAppProtocolHandler::ProcessMessageCreateHLSStream(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;
	bool result = false;
	settings.Reset();
	settings["hlsVersion"] = _pApp->GetHLSVersion();
	EXTRACT_RESPONSE_ID(parameters);
	HAS_PARAMETERS;
	SET_RESPONSE_ID(parameters, RESPONSE_ID);
	if ((!NormalizeHLSParameters(pFrom, parameters, settings, result)) || (!result)) {
		return true;
	}

	// Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	// Create the HLS stream
	if (!_pApp->CreateHLSStream(settings)) {
		string message = "Unable to create HLS stream";
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamNames groupName playlistName playlistType targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "HLS stream created", settings, RESPONSE_ID);
}

#endif /* HAS_PROTOCOL_HLS */

#ifdef HAS_PROTOCOL_HDS

bool CLIAppProtocolHandler::ProcessMessageCreateHDSStream(BaseProtocol *pFrom,
		Variant &parameters) {
	
	EXTRACT_RESPONSE_ID(parameters);
	
	Variant settings;

	HAS_PARAMETERS;
	//	//ASSERT("parameters:\n%s",STR(parameters.ToString()));
	// Gather parameters: localStreamNames
	if (parameters.HasKeyChain(V_STRING, false, 1, "localStreamNames")) {
		GATHER_MANDATORY_STRING(settings, parameters, localStreamNames);
		settings["localStreamNames"].Reset();
		settings["localStreamNames"].PushToArray(localStreamNames);
	} else {
		GATHER_MANDATORY_ARRAY(settings, parameters, localStreamNames);
	}

	// Gather parameters: bandwidths
	if (parameters.HasKey("bandwidths")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "bandwidths")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, bandwidths, 0, 0xffffffff);
			settings["bandwidths"].Reset();
			settings["bandwidths"].PushToArray(bandwidths);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, bandwidths);
			Variant temp;
			for (uint32_t i = 0; i < settings["bandwidths"].MapSize(); i++) {
				string rawVal = (string) settings["bandwidths"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter bandwidths is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter bandwidths is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["bandwidths"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["localStreamNames"].MapSize(); i++) {
			settings["bandwidths"].PushToArray((uint32_t) 0);
		}
	}

	if (settings["localStreamNames"].MapSize() != settings["bandwidths"].MapSize()) {
		FATAL("Syntax error. Parameters bandwidths and localStreamNames have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters bandwidths and localStreamNames have different number of elements"), RESPONSE_ID);
	}

	// Gather parameters: targetFolder
	GATHER_MANDATORY_STRING(settings, parameters, targetFolder);

	// Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("hds_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	/*
	 * No Longer needed since the following will be the default behaviour:
	 * Live stream type while inbound stream is currently active.
	 * Once inbound stream is complete, replace 'live' stream type with
	 * 'recorded'.
	 */
	// Gather parameters: streamType
	//GATHER_OPTIONAL_STRING(settings, parameters, streamType, "live");
	//if ((streamType != "live") && (streamType != "recorded")) {
	//	return SendFail(pFrom, "Syntax error for appending parameter. Type help for details");
	//}

	// Gather parameters: manifestName, will be replaced with stream name if empty
	GATHER_OPTIONAL_STRING(settings, parameters, manifestName, "");

	// Gather parameters: chunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, chunkLength, 1, 3600, 10);

	// Gather parameters: chunkOnIDR
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, chunkOnIDR, true);

	// Gather parameters: chunkBaseName use as id
	GATHER_OPTIONAL_STRING(settings, parameters, chunkBaseName, "f4v");

	// Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	// Gather parameters: overwriteDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, overwriteDestination, true);

	// Gather parameters: playlistType
	GATHER_OPTIONAL_STRING(settings, parameters, playlistType, "appending");
	if ((playlistType != "appending") && (playlistType != "rolling")) {
		return SendFail(pFrom, "Syntax error for appending parameter. Type help for details", RESPONSE_ID);
	}

	// Gather parameters: playlistLength (default: 10), applicable if rolling = true
	GATHER_OPTIONAL_NUMBER(settings, parameters, playlistLength, 5, 4000, 10);

	// Gather parameters: staleRetentionCount
	GATHER_OPTIONAL_NUMBER(settings, parameters, staleRetentionCount, 0, 999999, playlistLength);

	// Gather parameters: createMasterPlaylist
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, createMasterPlaylist, true);

	// Gather parameters: cleanupDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, cleanupDestination, false);

	// Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	// Create the HDS stream
	if (!_pApp->CreateHDSStream(settings)) {
		string message = "Unable to create HDS stream";
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamNames groupName manifestName playlistType targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "HDS stream created", settings, RESPONSE_ID);
}

#endif /* HAS_PROTOCOL_HDS */

#ifdef HAS_PROTOCOL_MSS

bool CLIAppProtocolHandler::ProcessMessageCreateMSSStream(BaseProtocol *pFrom,
		Variant &parameters) {
	
	EXTRACT_RESPONSE_ID(parameters);
	
	Variant settings;

	HAS_PARAMETERS;
	//	//ASSERT("parameters:\n%s",STR(parameters.ToString()));
	// Gather parameters: localStreamNames
	if (parameters.HasKeyChain(V_STRING, false, 1, "localStreamNames")) {
		GATHER_MANDATORY_STRING(settings, parameters, localStreamNames);
		settings["localStreamNames"].Reset();
		settings["localStreamNames"].PushToArray(localStreamNames);
	} else {
		GATHER_MANDATORY_ARRAY(settings, parameters, localStreamNames);
	}

	// Gather parameters: bandwidths
	if (parameters.HasKey("bandwidths")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "bandwidths")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, bandwidths, 0, 0xffffffff);
			settings["bandwidths"].Reset();
			settings["bandwidths"].PushToArray(bandwidths);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, bandwidths);
			Variant temp;
			for (uint32_t i = 0; i < settings["bandwidths"].MapSize(); i++) {
				string rawVal = (string) settings["bandwidths"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter bandwidths is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter bandwidths is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["bandwidths"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["localStreamNames"].MapSize(); i++) {
			settings["bandwidths"].PushToArray((uint32_t) 0);
		}
	}

	bool audiorateExists = parameters.HasKey("audioBitrates", false);
	bool videorateExists = parameters.HasKey("videoBitrates", false);

	// Gather parameters: ismType
	GATHER_OPTIONAL_STRING(settings, parameters, ismType, "ismc"); // default value is ISMC
	if ((ismType != "ismc") && (ismType != "isml")) {
		FATAL("Syntax error for ismType parameter. Type help for details");
		return SendFail(pFrom, "Syntax error for ismType parameter. Values can only be `ismc` or `isml`", RESPONSE_ID);
	}

	if (audiorateExists) {
		if (parameters.HasKey("bandwidths")) {
			FATAL("audioBitrates parameter cannot be used in conjunction with bandwidths. Use either but not both");
			return SendFail(pFrom, "audioBitrates parameter cannot be used in conjunction with bandwidths. Use either but not both", RESPONSE_ID);
		}
		if (ismType == "ismc") {
			FATAL("audioBitrates parameter cannot be used for ismType=`ismc`, it is only used in ismType=`isml`. Use bandwidths instead");
			return SendFail(pFrom, "audioBitrates parameter cannot be used for ismType=`ismc`, it is only used in ismType=`isml`. Use bandwidths instead", RESPONSE_ID);
		}

		if (parameters.HasKeyChain(V_STRING, false, 1, "audioBitrates")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, audioBitrates, 0, 0xffffffff);
			settings["audioBitrates"].Reset();
			settings["audioBitrates"].PushToArray(audioBitrates);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, audioBitrates);
			Variant temp;
			for (uint32_t i = 0; i < settings["audioBitrates"].MapSize(); i++) {
				string rawVal = (string) settings["audioBitrates"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter audioBitrates is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter audioBitrates is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["audioBitrates"] = temp;
		}
	}

	if (videorateExists) {
		if (parameters.HasKey("bandwidths")) {
			FATAL("videoBitrates parameter cannot be used in conjunction with bandwidths. Use either but not both");
			return SendFail(pFrom, "videoBitrates parameter cannot be used in conjunction with bandwidths. Use either but not both", RESPONSE_ID);
		}
		if (ismType == "ismc") {
			FATAL("videoBitrates parameter cannot be used for ismType=`ismc`, it is only used in ismType=`isml`. Use bandwidths instead");
			return SendFail(pFrom, "videoBitrates parameter cannot be used for ismType=`ismc`, it is only used in ismType=`isml`. Use bandwidths instead", RESPONSE_ID);
		}

		if (parameters.HasKeyChain(V_STRING, false, 1, "videoBitrates")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, videoBitrates, 0, 0xffffffff);
			settings["videoBitrates"].Reset();
			settings["videoBitrates"].PushToArray(videoBitrates);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, videoBitrates);
			Variant temp;
			for (uint32_t i = 0; i < settings["videoBitrates"].MapSize(); i++) {
				string rawVal = (string) settings["videoBitrates"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter videoBitrates is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter videoBitrates is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["videoBitrates"] = temp;
		}
	}

	if ((!audiorateExists || !videorateExists) 
		&& settings["localStreamNames"].MapSize() != settings["bandwidths"].MapSize()) {
		FATAL("Syntax error. Parameters bandwidths and localStreamNames have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters bandwidths and localStreamNames have different number of elements"), RESPONSE_ID);
	}

	if (audiorateExists && settings["localStreamNames"].MapSize() != settings["audioBitrates"].MapSize()) {
		FATAL("Syntax error. audioBitrates must have equal number of elements with localStreamNames");
		return SendFail(pFrom, format("Syntax error. audioBitrates must have equal number of elements with localStreamNames"), RESPONSE_ID);
	}

	if (videorateExists && settings["localStreamNames"].MapSize() != settings["videoBitrates"].MapSize()) {
		FATAL("Syntax error. videoBitrates must have equal number of elements with localStreamNames");
		return SendFail(pFrom, format("Syntax error. videoBitrates must have equal number of elements with localStreamNames"), RESPONSE_ID);
	}

	// Gather parameters: targetFolder
	GATHER_MANDATORY_STRING(settings, parameters, targetFolder);

	// Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("mss_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	// Gather parameters: playlistType
	GATHER_OPTIONAL_STRING(settings, parameters, playlistType, "appending");
	if ((playlistType != "appending") && (playlistType != "rolling")) {
		return SendFail(pFrom, "Syntax error for appending parameter. Type help for details", RESPONSE_ID);
	}

	// Gather parameters: playlistLength (default: 10), applicable if rolling = true
	GATHER_OPTIONAL_NUMBER(settings, parameters, playlistLength, 5, 4000, 10);

	// Gather parameters: manifestName
	GATHER_OPTIONAL_STRING(settings, parameters, manifestName, "manifest.ismc");

	// Gather parameters: chunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, chunkLength, 1, 3600, 10);

	// Gather parameters: chunkOnIDR
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, chunkOnIDR, true);

	// Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	// Gather parameters: overwriteDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, overwriteDestination, true);

	// Gather parameters: staleRetentionCount
	GATHER_OPTIONAL_NUMBER(settings, parameters, staleRetentionCount, 0, 999999, playlistLength);

	// Gather parameters: cleanupDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, cleanupDestination, false);

	// Gather parameters: isLive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, isLive, true);

	if (ismType == "isml") {
		// Gather parameters: publishingPoint
		GATHER_OPTIONAL_STRING(settings, parameters, publishingPoint, "");
		if (publishingPoint == "") {
			FATAL("Syntax error for publishingPoint parameter. Type help for details");
			return SendFail(pFrom, "Syntax error for publishingPoint parameter. This must not be empty for ismType=`isml`", RESPONSE_ID);
		}
		// Gather parameters: ingestMode
		GATHER_OPTIONAL_STRING(settings, parameters, ingestMode, "single");
		if ((ingestMode != "single") && (ingestMode != "loop")) {
			FATAL("Syntax error for ingestMode parameter. Type help for details");
			return SendFail(pFrom, "Syntax error for ingestMode parameter. Values can only be `single` or `loop`", RESPONSE_ID);
		}
	}
	// Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	// Create the MSS stream
	if (!_pApp->CreateMSSStream(settings)) {
		string message = "Unable to create MSS stream";
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamNames groupName manifestName playlistType targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "MSS stream created", settings, RESPONSE_ID);
}

#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH

bool CLIAppProtocolHandler::ProcessMessageCreateDASHStream(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;

	HAS_PARAMETERS;
	//	//ASSERT("parameters:\n%s",STR(parameters.ToString()));
	// Gather parameters: localStreamNames
	if (parameters.HasKeyChain(V_STRING, false, 1, "localStreamNames")) {
		GATHER_MANDATORY_STRING(settings, parameters, localStreamNames);
		settings["localStreamNames"].Reset();
		settings["localStreamNames"].PushToArray(localStreamNames);
	} else {
		GATHER_MANDATORY_ARRAY(settings, parameters, localStreamNames);
	}

	// Gather parameters: bandwidths
	if (parameters.HasKey("bandwidths")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "bandwidths")) {
			GATHER_MANDATORY_NUMBER(settings, parameters, bandwidths, 0, 0xffffffff);
			settings["bandwidths"].Reset();
			settings["bandwidths"].PushToArray(bandwidths);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, bandwidths);
			Variant temp;
			for (uint32_t i = 0; i < settings["bandwidths"].MapSize(); i++) {
				string rawVal = (string) settings["bandwidths"][(uint32_t) i];
				trim(rawVal);
				uint32_t val = (uint32_t) atoi(STR(rawVal));
				if (format("%"PRIu32, val) != rawVal) {
					FATAL("Syntax error. Parameter bandwidths is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter bandwidths is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["bandwidths"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["localStreamNames"].MapSize(); i++) {
			settings["bandwidths"].PushToArray((uint32_t) 0);
		}
	}

	if (settings["localStreamNames"].MapSize() != settings["bandwidths"].MapSize()) {
		FATAL("Syntax error. Parameters bandwidths and localStreamNames have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters bandwidths and localStreamNames have different number of elements"), RESPONSE_ID);
	}

	// Gather parameters: targetFolder
	GATHER_MANDATORY_STRING(settings, parameters, targetFolder);

	// Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("dash_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	// Gather parameters: playlistType
	GATHER_OPTIONAL_STRING(settings, parameters, playlistType, "appending");
	if ((playlistType != "appending") && (playlistType != "rolling")) {
		return SendFail(pFrom, "Syntax error for appending parameter. Type help for details", RESPONSE_ID);
	}

	// Gather parameters: playlistLength (default: 10), applicable if rolling = true
	GATHER_OPTIONAL_NUMBER(settings, parameters, playlistLength, 5, 4000, 10);

	// Gather parameters: manifestName
	GATHER_OPTIONAL_STRING(settings, parameters, manifestName, "manifest.mpd");

	// Gather parameters: chunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, chunkLength, 1, 3600, 10);

	// Gather parameters: chunkOnIDR
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, chunkOnIDR, true);

	// Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	// Gather parameters: overwriteDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, overwriteDestination, true);

	// Gather parameters: staleRetentionCount
	GATHER_OPTIONAL_NUMBER(settings, parameters, staleRetentionCount, 0, 999999, playlistLength);

	// Gather parameters: cleanupDestination
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, cleanupDestination, false);

	// Gather parameters: dynamicProfile
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, dynamicProfile, true);

	// Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	// Create the DASH stream
	if (!_pApp->CreateDASHStream(settings)) {
		string message = "Unable to create DASH stream";
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamNames groupName manifestName playlistType targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "DASH stream created", settings, RESPONSE_ID);
}

#endif /* HAS_PROTOCOL_DASH */

#ifdef HAS_WEBSERVER
bool CLIAppProtocolHandler::GetListHttpStreamingSessionsResult(Variant &callbackInfo) {
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}
	EXTRACT_RESPONSE_ID(callbackInfo);

	if (!callbackInfo["result"]) { 
		FATAL("Unable to get all open HTTP streaming sessions");
		return SendFail(pFrom, "Unable to get all open HTTP streaming sessions", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["httpStreamingSessions"]["asciiList"]["names"] = "sessionId clientIP elapsedTime targetFolder";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Currently open HTTP streaming sessions", callbackInfo["httpStreamingSessions"], RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetHttpClientsConnectedResult(Variant &callbackInfo) {
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}
	EXTRACT_RESPONSE_ID(callbackInfo);

	string groupName = callbackInfo["groupName"];

	if (!callbackInfo["result"]) { 
		FATAL("Unable to get count of all open HTTP streaming sessions");
		return SendFail(pFrom, "Unable to get count of all open HTTP streaming sessions", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "httpStreamingSessionsCount groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	string descMessage = (groupName == "") ? "Number of clients connected to RDKC Web Server" :
											("Number of clients connected to RDKC Web Server under groupname: " + groupName);

	return SendSuccess(pFrom, descMessage, callbackInfo["httpClientsConnected"], RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetAddGroupNameAliasResult(Variant &callbackInfo) {

	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	if (!callbackInfo["result"]) { 
		FATAL("Unable to set group name alias. Check if group name aliasing is enabled.");
		return SendFail(pFrom, "Unable to set group name alias. Check if group name aliasing is enabled.", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "aliasName groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Alias applied to group name", callbackInfo, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetRemoveGroupNameAliasResult(Variant &callbackInfo) {
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	if (!callbackInfo["result"]) { 
		FATAL("Unable to remove group name alias. Check if group name aliasing is enabled.");
		return SendFail(pFrom, "Unable to remove group name alias. Check if group name aliasing is enabled.", RESPONSE_ID);
	}


#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "aliasName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Group name alias removed", callbackInfo, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetListGroupNameAliasesResult(Variant &callbackInfo) {

	EXTRACT_RESPONSE_ID(callbackInfo);

	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	if (!callbackInfo["result"]) { 
		FATAL("Unable to get all group name aliases. Check if group name aliasing is enabled.");
		return SendFail(pFrom, "Unable to get all group name aliases. Check if group name aliasing is enabled.", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["groupNameAliases"]["asciiList"]["names"] = "aliasName groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Currently available group name aliases", callbackInfo["groupNameAliases"], RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetGroupNameByAliasResult(Variant &callbackInfo) {

	EXTRACT_RESPONSE_ID(callbackInfo);

	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	if (!callbackInfo["result"]) { 
		FATAL("Unable to get group name. Check if group name aliasing is enabled.");
		return SendFail(pFrom, "Unable to get group name. Check if group name aliasing is enabled.", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "aliasName groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Group name", callbackInfo, RESPONSE_ID);
}

bool CLIAppProtocolHandler::GetFlushGroupNameAliasesResult(Variant &callbackInfo) {
	BaseProtocol *pFrom = ProtocolManager::GetProtocol((uint32_t) callbackInfo["cliProtocolId"]);
	if (pFrom == NULL) {
		WARN("Dead protocol");
		return false;
	}

	EXTRACT_RESPONSE_ID(callbackInfo);

	if (!callbackInfo["result"]) { 
		FATAL("Unable to flush all group name aliases. Check if group name aliasing is enabled.");
		return SendFail(pFrom, "Unable to flush all group name aliases. Check if group name aliasing is enabled.",RESPONSE_ID);
	}
	Variant dummy;

	// Select items to list for ASCII CLI
	//if (pFrom != NULL) {
	//	if (pFrom->GetType() == PT_INBOUND_ASCIICLI) {
	//		callbackInfo["asciiList"]["names"] = "";
	//	}
	//}

	return SendSuccess(pFrom, "All group name aliases are flushed", dummy, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListHTTPStreamingSessions(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	//1. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "getHttpStreamingSessions";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//2. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		callbackInfo["asciiList"]["names"] = "sessionId targetFolder clientIP elapsedTime";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageAddGroupNameAlias(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	Variant settings;

	//1. Gather parameters: groupName
	GATHER_MANDATORY_STRING(settings, parameters, groupName);

	//2. Gather parameters: aliasName
	GATHER_MANDATORY_STRING(settings, parameters, aliasName);

	//1. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "addGroupNameAlias";
	callbackInfo["groupName"] = groupName;
	callbackInfo["aliasName"] = aliasName;

	//2. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//3. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageRemoveGroupNameAlias(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	Variant settings;

	//1. Gather parameters: alias
	GATHER_MANDATORY_STRING(settings, parameters, aliasName);

	//2. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "removeGroupNameAlias";
	callbackInfo["aliasName"] = aliasName;
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//3. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//4. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageListGroupNameAliases(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	//2. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "listGroupNameAliases";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//3. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//4. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageGetGroupNameByAlias(BaseProtocol *pFrom,
		Variant &parameters) {
	
	EXTRACT_RESPONSE_ID(parameters);
	
	HAS_PARAMETERS;
	Variant settings;

	//1. Gather parameters: alias
	GATHER_MANDATORY_STRING(settings, parameters, aliasName);

	//2. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "getGroupNameByAlias";
	callbackInfo["aliasName"] = aliasName;
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//3. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//4. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

bool CLIAppProtocolHandler::ProcessMessageFlushGroupNameAliases(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	//2. Prepare the callback for the edge/origin dialog
	Variant callbackInfo;
	callbackInfo["cliProtocolId"] = pFrom->GetId();
	callbackInfo["operation"] = "flushGroupNameAliases";
	SET_RESPONSE_ID(callbackInfo, RESPONSE_ID);

	//3. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
		(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//4. Do the call
	if (!pOrigin->EnqueueCall(callbackInfo)) {
		return SendFail(pFrom, "Unable to enqueue call", RESPONSE_ID);
	}

	return true;
}

#endif /* HAS_WEBSERVER */

bool CLIAppProtocolHandler::ProcessMessageGetConnectionsCountLimit(
		BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the data
	Variant data;
	data["limit"] = _pApp->GetConnectionsCountLimit();
	data["current"] = _pApp->GetConnectionsCount();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "current limit";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Connection counters", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageSetConnectionsCountLimit(
		BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	//1. Gather parameters: count
	GATHER_MANDATORY_NUMBER(settings, parameters, count, 0, 0xffffffff);

	//3. Set up the new limit
	_pApp->SetConnectionsCountLimit((uint32_t) count);

	//4. Prepare the data
	Variant data;
	data["limit"] = _pApp->GetConnectionsCountLimit();
	data["current"] = _pApp->GetConnectionsCount();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "limit";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Connection counters", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageGetBandwidth(
		BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	// Prepare the data
	Variant data;
	double in = 0, out = 0, maxIn = 0, maxOut = 0;
	_pApp->GetBandwidth(in, out, maxIn, maxOut);
	data["current"]["in"] = in;
	data["current"]["out"] = out;
	data["max"]["in"] = maxIn;
	data["max"]["out"] = maxOut;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "current max in out";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Bandwidth in B/s", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageSetBandwidthLimit(
		BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	// Gather parameters: in
	GATHER_MANDATORY_NUMBER(settings, parameters, in, 0, 0xffffffff);

	// Gather parameters: out
	GATHER_MANDATORY_NUMBER(settings, parameters, out, 0, 0xffffffff);

	// Set up the new limit
	_pApp->SetBandwidthLimit((double) in, (double) out);

	// Prepare the data
	Variant data;
	double tmpIn = 0, tmpOut = 0, maxIn = 0, maxOut = 0;
	_pApp->GetBandwidth(tmpIn, tmpOut, maxIn, maxOut);
	data["current"]["in"] = tmpIn;
	data["current"]["out"] = tmpOut;
	data["max"]["in"] = maxIn;
	data["max"]["out"] = maxOut;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "current max in out";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	// Done
	return SendSuccess(pFrom, "Bandwidth in B/s", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListEdges(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	//1. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	//2. Get the list of edges
	map<uint64_t, EdgeServer> &temp = pOrigin->GetEdges();
	Variant data;
	data.IsArray(true);

	FOR_MAP(temp, uint64_t, EdgeServer, i) {
		Variant temp;
		temp["ip"] = MAP_VAL(i).Ip();
		temp["port"] = MAP_VAL(i).Port();
		temp["pid"] = MAP_VAL(i).Pid();
		temp["load"] = MAP_VAL(i).Load();
		temp["loadIncrement"] = MAP_VAL(i).LoadIncrement();
		data.PushToArray(temp);
	}

	//4. Send the response
	return SendSuccess(pFrom, "Edges list", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageSetLogLevel(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	//1. Gather parameters: level
	GATHER_MANDATORY_SIGNED_NUMBER(settings, parameters, level, -7, 7);

	Logger::SetLevel((int32_t) level);

	Variant response;
	return SendSuccess(pFrom, "Log level is set", response, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageResetMaxFdCounters(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	IOHandlerManager::GetStats(false).ResetMax();

	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	pOrigin->ResetMaxFdCounters();

	Variant dummy;
	return SendSuccess(pFrom, "Max counters are cleared", dummy, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageResetTotalFdCounters(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	IOHandlerManager::GetStats(false).ResetTotal();

	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return false;
	}

	pOrigin->ResetTotalFdCounters();

	Variant dummy;
	return SendSuccess(pFrom, "Total counters are cleared", dummy, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageRecordStream(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	//1. Gather parameters: localStreamName
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);

	//2. Gather parameters: pathToFile
	GATHER_MANDATORY_STRING(settings, parameters, pathToFile);

	GATHER_OPTIONAL_NUMBER(settings, parameters, preset, 0, 99999, 0);
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, dateFolderStructure, false);

	//3. Gather parameters: targetStreamType
	GATHER_OPTIONAL_STRING(settings, parameters, type, "mp4");
	type = lowerCase(type);
	settings["type"] = type;
	if ((type != "ts") && (type != "flv") && (type != "mp4") && (type != "vmf")) {
		return SendFail(pFrom, "Syntax error for type parameter. Type help for details", RESPONSE_ID);
	}

	//4. Gather parameters: overwrite
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, overwrite, false);

	//5. Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	//6. Gather parameters: chunkLength
	GATHER_OPTIONAL_NUMBER(settings, parameters, chunkLength, 0, 999999, 0);

	//7. Gather parameters: winQtCompat
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, winQtCompat, true);

	//8. Gather parameters: waitForIDR
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, waitForIDR, false);

	//9. Gather parameters: hasAudio
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, hasAudio, true);

	//10. Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	//11. Override settings for presets
	switch(preset) {
		case 1:
			settings["type"] = "mp4";
			settings["chunkLength"] = 1800;
			settings["waitForIDR"] = true;
			dateFolderStructure = false;
			break;
	}

	//12. Start the record
	if (!_pApp->RecordStream(settings)) {
		FATAL("Unable to record stream");
		return SendFail(pFrom, "Unable to record stream", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamName configId type pathToFile chunkLength";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//13. Done
	return SendSuccess(pFrom, "Recording Stream", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageAddStreamAlias(BaseProtocol *pFrom,
		Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	Variant settings;

	//1. Gather parameters: localStreamName
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);

	//2. Gather parameters: aliasName
	GATHER_MANDATORY_STRING(settings, parameters, aliasName);

	//3. gather parameters: expirePeriod
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, expirePeriod, -86401, 86401, -600);

	//4. Add the alias
	StreamAlias *value = NULL;
	if (!GetApplication()->SetStreamAlias(localStreamName, aliasName,
			expirePeriod, value)) {
		FATAL("Unable to set stream alias");
		return SendFail(pFrom, "Unable to set stream alias", RESPONSE_ID);
	}

	//5. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return SendFail(pFrom, "Unable to set stream alias", RESPONSE_ID);
	}

	pOrigin->SetStreamAlias(localStreamName, aliasName, expirePeriod);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "aliasName localStreamName expirePeriod";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Alias applied", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageRemoveStreamAlias(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;
	Variant settings;

	//1. Gather parameters: aliasName
	GATHER_MANDATORY_STRING(settings, parameters, aliasName);

	//2. Remove the alias
	if (!GetApplication()->RemoveStreamAlias(aliasName)) {
		FATAL("Unable to remove stream alias");
		return SendFail(pFrom, "Unable to remove stream alias", RESPONSE_ID);
	}

	//3. Get the origin handler
	OriginVariantAppProtocolHandler *pOrigin =
			(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
	if (pOrigin == NULL) {
		FATAL("Unable to get the handler");
		return SendFail(pFrom, "Unable to set stream alias", RESPONSE_ID);
	}

	//4. remove it from all edges
	pOrigin->RemoveStreamAlias(aliasName);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "aliasName localStreamName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//5. Done
	return SendSuccess(pFrom, "Alias removed", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListStreamAliases(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	map<string, StreamAlias> &aliases = GetApplication()->GetAllStreamAliases();
	Variant res;
	res.IsArray(true);

	FOR_MAP(aliases, string, StreamAlias, i) {
		res.PushToArray(MAP_VAL(i));
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		res["asciiList"]["names"] = "aliasName localStreamName expirePeriod oneShot permanent";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Currently available aliases", res, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageFlushStreamAliases(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	GetApplication()->GetAllStreamAliases().clear();
	Variant dummy;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		dummy["asciiList"]["names"] = "aliasName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "All aliases are flushed", dummy, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageShutdownServer(BaseProtocol *pFrom,
		Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	if (parameters != V_NULL) {
		HAS_PARAMETERS;
		GATHER_MANDATORY_STRING(settings, parameters, key);
		if (key == _shutdownKey) {
			//Is this needed here since origin will call ShutdownEdges() anyway?
			//			OriginVariantAppProtocolHandler *pOrigin =
			//					(OriginVariantAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_BIN_VAR);
			//			if (pOrigin == NULL) {
			//				FATAL("Unable to get the handler");
			//				return false;
			//			}
			//
			//			pOrigin->ShutdownEdges();
			Variant message;
			message["header"] = "shutdown";
			if (!ClientApplicationManager::SendMessageToApplication("lminterface", message)) {
				EventLogger::SignalShutdown();
			}
			return true;
		} else {
			return SendFail(pFrom, "Shutdown key provided is invalid", RESPONSE_ID);
		}
	} else {
		_shutdownKey = generateRandomString(16);
		Variant data;
		data["key"] = _shutdownKey;
        
#ifdef HAS_PROTOCOL_ASCIICLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
			data["asciiList"]["names"] = "key";
		}
#endif /* HAS_PROTOCOL_ASCIICLI */
        
		return SendSuccess(pFrom, "Call shutdownserver again with the provided key", data, RESPONSE_ID);
	}
}

bool CLIAppProtocolHandler::ProcessMessageLaunchProcess(BaseProtocol *pFrom,
		Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: fullBinaryPath
	GATHER_MANDATORY_STRING(settings, parameters, fullBinaryPath);
	string normalizedFullBinaryPath = normalizePath(fullBinaryPath, "");
	if (normalizedFullBinaryPath == "") {
		string result = format("binary path not found: `%s`", STR(fullBinaryPath));
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	settings["fullBinaryPath"] = normalizedFullBinaryPath;

	//2. Gather parameters: arguments
	GATHER_OPTIONAL_STRING(settings, parameters, arguments, "");

	//3. Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	//4. Gather the environment variables

	FOR_MAP(parameters, string, Variant, i) {
		if ((MAP_KEY(i).length() > 0) && (MAP_KEY(i)[0] == '$'))
			settings[MAP_KEY(i)] = MAP_VAL(i);
	}

	// Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("process_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	//5. Gather the custom parameters
	GatherCustomParameters(parameters, settings);

	//6. Do the request
	if (!_pApp->LaunchProcess(settings)) {
		string result = "Unable to launch process";
		return SendFail(pFrom, result, RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "configId fullBinaryPath arguments groupName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//7. Done
	return SendSuccess(pFrom, "Process enqueued for start", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListStorage(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	StreamMetadataResolver *pSMR = _pApp->GetStreamMetadataResolver();
	if (pSMR == NULL) {
		return SendFail(pFrom, "Unable to get the streams metadata resolver", RESPONSE_ID);
	}

	Variant storage = pSMR->GetAllStorages();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		storage["asciiList"]["names"] = "name mediaFoldermetaFolder description";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Available storages", storage, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageAddStorage(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	StreamMetadataResolver *pSMR = _pApp->GetStreamMetadataResolver();
	if (pSMR == NULL) {
		return SendFail(pFrom, "Unable to get the streams metadata resolver", RESPONSE_ID);
	}

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: mediaFolder
	GATHER_MANDATORY_STRING(settings, parameters, mediaFolder);

	//2. Gather parameters: name
	GATHER_OPTIONAL_STRING(settings, parameters, name, "");

	//3. Gather parameters: description
	GATHER_OPTIONAL_STRING(settings, parameters, description, "");

	//4. Gather parameters: metaFolder
	GATHER_OPTIONAL_STRING(settings, parameters, metaFolder, "");

	//5. Gather parameters: enableStats
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, enableStats, false);

	//6. Gather parameters: keyframeSeek
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keyframeSeek, false);

	//7. Gather parameters: clientSideBuffer
	GATHER_OPTIONAL_NUMBER(settings, parameters, clientSideBuffer, 3, 2600, 15);

	//8. Gather parameters: seekGranularity
	GATHER_OPTIONAL_NUMBER(settings, parameters, seekGranularity, 100, 300000, 1000);
	settings["seekGranularity"] = (double) seekGranularity / 1000.0;

	//9. Gather parameters: externalSeekGenerator
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, externalSeekGenerator, false);


	Storage storage;
	if (!pSMR->InitializeStorage(name, settings, storage)) {
		return SendFail(pFrom, "Failed to initialize storage. See the logs for more details", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		storage["asciiList"]["names"] = "name mediaFoldermetaFolder description";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Storage created", storage, RESPONSE_ID);

}

bool CLIAppProtocolHandler::ProcessMessageRemoveStorage(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	StreamMetadataResolver *pSMR = _pApp->GetStreamMetadataResolver();
	if (pSMR == NULL) {
		return SendFail(pFrom, "Unable to get the streams metadata resolver", RESPONSE_ID);
	}

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: mediaFolder
	GATHER_MANDATORY_STRING(settings, parameters, mediaFolder);

	Storage storage;
	if (!pSMR->RemoveStorage(mediaFolder, storage)) {
		return SendFail(pFrom, "Unable to remove storage. Check storage path or storage permission.", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		storage["asciiList"]["names"] = "name mediaFoldermetaFolder description";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Storage removed", storage, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageSetTimer(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: value
	GATHER_MANDATORY_STRING(settings, parameters, value);
	trim(value);
	switch (value.size()) {
		case 19: //YYYY-MM-DDTHH:MM:SS
		{
			if (!Variant::ParseTime(value.c_str(), "%Y-%m-%dT%T", settings["value"])) {
				return SendFail(pFrom, "Unable to parse the string as a date/time", RESPONSE_ID);
			}
			break;
		}
		case 8: //HH:MM:SS
		{
			if (!Variant::ParseTime(value.c_str(), "%T", settings["value"])) {
				return SendFail(pFrom, "Unable to parse the string as a date/time", RESPONSE_ID);
			}
			break;
		}
		default:
		{
			settings.RemoveKey("value");
			GATHER_MANDATORY_NUMBER(settings, parameters, value, 1, 86399);
			break;
		}
	}

	GatherCustomParameters(parameters, settings);

	//2. Do the damage
	if (!_pApp->SetCustomTimer(settings)) {
		return SendFail(pFrom, "Unable to start custom timer", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "timerId triggerCount value";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Custom timer enqueued", settings, RESPONSE_ID);
}

bool customTimersFilter(BaseProtocol *pProtocol) {
	return (pProtocol != NULL)
			&& (pProtocol->GetType() == PT_TIMER)
			&& (pProtocol->GetCustomParameters().HasKeyChain(_V_NUMERIC, true, 2, "settings", "timerId"));
}

bool CLIAppProtocolHandler::ProcessMessageListTimers(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	map<uint32_t, BaseProtocol *> timers;
	ProtocolManager::GetActiveProtocols(timers, customTimersFilter);
	Variant result;

	FOR_MAP(timers, uint32_t, BaseProtocol *, i) {
		result.PushToArray(MAP_VAL(i)->GetCustomParameters()["settings"]);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		result["asciiList"]["names"] = "timerId triggerCount value";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "List of armed timers", result, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageRemoveTimer(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	//1. Gather parameters: id
	GATHER_MANDATORY_NUMBER(settings, parameters, id, 1, 0xffffffff);

	BaseProtocol *pProtocol = ProtocolManager::GetProtocol((uint32_t) id);
	if ((pProtocol == NULL)
			|| (pProtocol->GetType() != PT_TIMER)
			|| (pProtocol->GetId() != (uint32_t) id)
			|| (!pProtocol->GetCustomParameters().HasKeyChain(_V_NUMERIC, true, 2, "settings", "timerId"))
			|| ((uint32_t) pProtocol->GetCustomParameters()["settings"]["timerId"] != (uint32_t) id)
			) {
		return SendFail(pFrom, "Timer not found", RESPONSE_ID);
	}

	Variant data = pProtocol->GetCustomParameters()["settings"];
	pProtocol->EnqueueForDelete();

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "timerId triggerCount value";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Timer removed", data, RESPONSE_ID);
}

/*
 * ProcessMessageGetMetadata - fetch matching metadata and return it
 */
bool CLIAppProtocolHandler::ProcessMessageGetMetadata(BaseProtocol *pFrom,
		Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);
	// get the parms, use Name first
	string name;
	uint32_t id = 0;
	bool noWrap = false;

	if (parameters.MapSize()) {
		if (parameters.HasKey("streamname", false)) {
			name = (string)(parameters.GetValue("streamname", false));
		} else if (parameters.HasKey("localstreamname", false)) {
			name = (string)(parameters.GetValue("localstreamname", false));
		} else if (parameters.HasKey("streamid", false)){
			string temp = (string)(parameters.GetValue("streamid", false));
			id = atoi(temp.c_str());
		//} else {
		//	return SendFail(pFrom, "Need streamname or streamid", RESPONSE_ID);
		}
		if (parameters.HasKey("nowrap", false)
			&& ((string)"1" == (string)parameters.GetValue("nowrap", false)) ) {
			noWrap = true;
		}
	}
	// now get the data from the manager
	MetadataManager * pMM = _pApp->GetMetadataManager();
	string mdStr;
	if (name.length()) {
		//INFO("ProcGetMetadata: calling GetMetadata(name)");
		mdStr = pMM->GetMetadata(name);
	}else { // return default if no id (0)
		//INFO("ProcGetMetadata: calling GetMetadata(%d)", id);
		mdStr = pMM->GetMetadata(id);
	}
	// got data?
	Variant result;
	result = mdStr;
	if (mdStr.length()) {
		if (!noWrap) { // normal wrapped response
			return SendSuccess(pFrom, "Metadata", result, RESPONSE_ID);
		} else { // send raw unwrapped response
			InboundBaseCLIProtocol * pTo = (InboundBaseCLIProtocol *)pFrom;
			return pTo->SendRaw(mdStr);
		}
	}
	return SendSuccess(pFrom, "No Metadata", result, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessagePushMetadata(BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	// Gather Mandatory parameters
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);
	settings["streamName"] = settings["localStreamName"];
	// Gather Optional parameters
	GATHER_OPTIONAL_STRING(settings, parameters, type, "vmf");
	type = lowerCase(type);
	settings["type"] = type;
	//if (type != "vmf") {
	//	return SendFail(pFrom, "type must be 'vmf' (only option at this time)");
	//}

	GATHER_OPTIONAL_STRING(settings, parameters, ip, "127.0.0.1");

	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	GATHER_OPTIONAL_NUMBER(settings, parameters, port, 100, 65535, 8110);

	GatherCustomParameters(parameters, settings);

	if (!_pApp->PushMetadata(settings)) {
		FATAL("Unable to push metadata");
		return SendFail(pFrom, "Unable to push metadata", RESPONSE_ID);
	}

	return SendSuccess(pFrom, "Launched Metadata Push Stream", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageShutdownMetadata(BaseProtocol *pFrom, Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	// Gather Mandatory parameters
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);
	settings["streamName"] = settings["localStreamName"];

	// Gather Optional parameters
	GATHER_OPTIONAL_STRING(settings, parameters, type, "vmf");
	type = lowerCase(type);
	settings["type"] = type;
	if (type != "vmf") {
		return SendFail(pFrom, "type must be 'vmf' (only option at this time)", RESPONSE_ID);
	}

	GatherCustomParameters(parameters, settings);

	if (!_pApp->ShutdownMetadata(settings)) {
		FATAL("Unable to shutdown metadata");
		return SendFail(pFrom, "Unable to shutdown metadata", RESPONSE_ID);
	}

	return SendSuccess(pFrom, "Shutdown Metadata Push Stream", settings, RESPONSE_ID);
}
#ifdef HAS_PROTOCOL_WEBRTC
bool CLIAppProtocolHandler::ProcessMessageStartWebRTC(BaseProtocol *pFrom, Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);

	HAS_PARAMETERS;

	Variant settings;

	// Gather Mandatory parameters
	//GATHER_MANDATORY_STRING(settings, parameters, rrs);
	GATHER_MANDATORY_NUMBER(settings, parameters, rrsport, 1, 65535);
	GATHER_MANDATORY_STRING(settings, parameters, roomid);

	// Gather optional
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);
	GATHER_OPTIONAL_STRING(settings, parameters, rrsip, "");
	GATHER_OPTIONAL_STRING(settings, parameters, rrs, "");
	GATHER_OPTIONAL_STRING(settings, parameters, token, "");
	if (parameters.HasKey("rrsOverSsl", false)) {
		GATHER_OPTIONAL_BOOLEAN(settings, parameters, rrsOverSsl, false);
	}

	// Custom??
	GatherCustomParameters(parameters, settings);

	// Get the IP address
	string ip = "";
	if (rrs != "") {
		ip = getHostByName(rrs);
		if (ip == "") {
			FATAL("Unable to resolve RRS: %s", STR(rrs));
			return SendFail(pFrom, "Unable to resolve RRS", RESPONSE_ID);
		}
	} else if (rrsip != "") {
		ip = rrsip;
		settings["rrs"] = rrsip;
	}
	else {
		// No rrs or rrsip parameters included
		FATAL("No rrs parameter passed!");
		return SendFail(pFrom, "No rrs parameter passed", RESPONSE_ID);
	}

	settings["rrsip"] = ip;
	if (!_pApp->StartWebRTC(settings)) {
		FATAL("Unable to start WebRTC");
		return SendFail(pFrom, "Unable to start WebRTC", RESPONSE_ID);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "configId rrsip rrsport name roomid sslCert sslKey";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Started WebRTC Negotiation Service", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageStopWebRTC(BaseProtocol *pFrom, Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);
	Variant settings;
	// Gather Mandatory parameters
	GATHER_OPTIONAL_STRING(settings, parameters, rrsip, "");
	GATHER_OPTIONAL_NUMBER(settings, parameters, rrsport, 0, 65535, 0);
	GATHER_OPTIONAL_STRING(settings, parameters, roomid, "");

	if (!_pApp->StopWebRTC(settings)) {
		FATAL("Unable to stop WebRTC");
		return SendFail(pFrom, "Unable to stop WebRTC", RESPONSE_ID);
	}

	return SendSuccess(pFrom, "Stopped WebRTC Negotiation Service", settings, RESPONSE_ID);
}
#endif // HAS_PROTOCOL_WEBRTC

bool CLIAppProtocolHandler::ProcessMessageCreateIngestPoint(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	GATHER_MANDATORY_STRING(settings, parameters, privateStreamName);

	GATHER_MANDATORY_STRING(settings, parameters, publicStreamName);

	if (!_pApp->CreateIngestPoint(privateStreamName, publicStreamName)) {
		string msg = format("Unable to create ingest point `%s` -> `%s`",
				STR(privateStreamName), STR(publicStreamName));
		FATAL("%s", STR(msg));
		return SendFail(pFrom, msg, RESPONSE_ID);
	}

	Variant data;
	data["privateStreamName"] = privateStreamName;
	data["publicStreamName"] = publicStreamName;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "privateStreamName publicStreamName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Ingest point created", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageRemoveIngestPoint(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	
	Variant settings;
	HAS_PARAMETERS;

	GATHER_MANDATORY_STRING(settings, parameters, privateStreamName);

	string publicStreamName;
	_pApp->RemoveIngestPoint(privateStreamName, publicStreamName);

	Variant data;
	data["privateStreamName"] = privateStreamName;
	data["publicStreamName"] = publicStreamName;

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "privateStreamName publicStreamName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Ingest point removed", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListIngestPoints(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	map<string, string> &ingestPoints = _pApp->ListIngestPoints();

	Variant temp;
	Variant data;
	data.IsArray(true);

	FOR_MAP(ingestPoints, string, string, i) {
		temp["privateStreamName"] = MAP_KEY(i);
		temp["publicStreamName"] = MAP_VAL(i);
		data.PushToArray(temp);
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		data["asciiList"]["names"] = "privateStreamName publicStreamName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Available ingest points", data, RESPONSE_ID);
}

bool CLIAppProtocolHandler::IsValidURI(string uri, string &name) {
	if (uri == "") {
		// We should not be getting empty strings for URIs or stream names
		return false;
	}

	if (uri.find("://") != string::npos) {
		// Seems to be a URI, check for validity
		URI uriTester;
		if (!URI::FromString(uri, false, uriTester)) {
			return false;
		}
	} else {
		name = uri;
	}

	return true;
}

bool CLIAppProtocolHandler::InitializeEmptyParams(Variant &settings, string paramRef,
		string paramName) {
	uint32_t sizeRef = 0;
	uint32_t sizeName = 0;

	// Check if reference parameter is an array
	if (settings[paramRef].IsArray()) {
		sizeRef = settings[paramRef].MapSize();
	}

	// Check if target parameter is an array
	if (settings[paramName].IsArray()) {
		sizeName = settings[paramName].MapSize();
	}

	// Sanity check
	if ((sizeRef == 0) && (sizeName != 0)) {
		// Reference is not an array but target parameter is!
		return false;
	}

	if ((sizeRef == 0) && (sizeName == 0)) {
		// Not an array, focus on that element
		if (settings[paramName] == "") {
			settings[paramName] = "0";
		}
	} else if (sizeName == 0) {
		// Loop and then set the fields to 'na' since it was not passed by user
		for (uint32_t i = 0; i < sizeRef; i++) {
			settings[paramName].PushToArray("na");
		}
	} else {
		// Loop with condition and then set to '0' all empty values
		for (uint32_t i = 0; i < sizeName; i++) {
			if ((string) settings[paramName][i] == "") {
				settings[paramName][i] = "0";
			}
		}
	}

	// Sanity check
	if ((sizeRef != 0) && (sizeRef != settings[paramName].MapSize())) {
		// For some odd reason, the elements did not match!
		return false;
	}

	return true;
}

string CLIAppProtocolHandler::FormTranscodeArgs(Variant &settings, uint32_t destIndex, bool &forceRE) {
	string args = "";
	string tmp = "";

	forceRE = false;

	// Add the source, check if localStreamName is set
	tmp = (string) settings["localStreamName"];
	if (tmp != "") {
		// Adjust since this is a stream name coming from RMS
		string prefix = (string) settings["srcUriPrefix"];
		if (prefix[prefix.size() - 1] != '/') {
			prefix += '/';
		}

		args = prefix + tmp;
	} else {
		string source = (string) settings["source"];

		// Check if source is a file scheme
		if (source.find("file://") != string::npos) {
			source = source.substr(7); // Get whatever is after 'file://'
			forceRE = true;
		}

		// Add as-is
		args = source;
	}

	// Add the videoBitrates
	args += " " + (string) settings["videoBitrates"][destIndex];

	// Add the videoSizes
	args += " " + lowerCase((string) settings["videoSizes"][destIndex]);

	// Add the videoAdvancedParamsProfiles
	args += " " + (string) settings["videoAdvancedParamsProfiles"][destIndex];

	// Add the audioBitrates
	args += " " + (string) settings["audioBitrates"][destIndex];

	// Add the audioChannelsCounts
	args += " " + lowerCase((string) settings["audioChannelsCounts"][destIndex]);

	// Add the audioFrequencies
	args += " " + (string) settings["audioFrequencies"][destIndex];

	// Add the audioAdvancedParamsProfiles
	args += " " + (string) settings["audioAdvancedParamsProfiles"][destIndex];

	// Add the overlays
	args += " " + (string) settings["overlays"][destIndex];

	// Parse the croppings
	vector<string> cropVals;
	split((string) settings["croppings"][destIndex], ":", cropVals);

	uint32_t cropSizes = (uint32_t) cropVals.size();
	if ((cropSizes != 2) && (cropSizes != 4)) {
		// Invalid crop inputs, set as 'na'
		args += " na na na na";
	} else {
		if (cropSizes == 2) {
			// The x and y parameters are optional
			args += " na na";
		}

		for (uint32_t i = 0; i < cropSizes; i++) {
			trim(cropVals[i]);
			if (cropVals[i] == "") {
				// Invalid crop inputs, set as na
				if (cropSizes == 2) {
					args += " na na";
				} else {
					args += " na na na na";
				}

				break;
			}
			args += " " + cropVals[i];
		}
	}

	// Add the destinations, check if rms target stream name is set
	tmp = (string) settings["rmsTargetStreamName"];
	if (tmp != "") {
		// Adjust since this is a stream name going to RMS
		// Since we have destination URI that has spaces on it, do some mambo-jambo
		vector<string> dstUriFields;
		split((string) settings["dstUriPrefix"], " ", dstUriFields);

		string dstUri = "";

		uint32_t dstUriSize = (uint32_t) dstUriFields.size() - 1;
		for (uint32_t i = 0; i < dstUriSize; i++) {
			dstUri += dstUriFields[i] + "\\ ";
		}

		dstUri += dstUriFields[dstUriSize];

		// Add the dstUriPrefix as parameter
		args += " " + dstUri;

		// target stream name would now be the rms stream name
		settings["targetStreamNames"][destIndex] = tmp;
	} else {
		string destination = (string) settings["destinations"][destIndex];

		// Check if source is a file scheme
		if (destination.find("file://") != string::npos) {
			destination = destination.substr(7); // Get whatever is after 'file://'
			forceRE = false; // toggle the flag since this is outbound file
		}

		// Add as-is
		args += " " + destination;
	}

	// Add the target stream name
	tmp = (string) settings["targetStreamNames"][destIndex];
	if (tmp == "na") {
		uint64_t time;
		GETCLOCKS(time, uint64_t);
		args += " transcoded" + format("_%"PRIu64, time);
	} else {
		// Add as-is
		args += " " + tmp;
	}

	if (settings.HasKey("commandFlags")) {
		tmp = (string) settings["commandFlags"];
		if (tmp != "") {
			args += " " + tmp;
		}
	}

	return args;
}

bool CLIAppProtocolHandler::ProcessMessageInsertPlaylistItem(BaseProtocol *pFrom,
		Variant &parameters) {
	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;

	// Gather parameters: playlistInstanceName
	GATHER_OPTIONAL_STRING(settings, parameters, playlistInstanceName, "");

	// Gather parameters: playlistName
	GATHER_OPTIONAL_STRING(settings, parameters, playlistName, "");

	if (playlistInstanceName.empty() && playlistName.empty()) {
		string result = "The playlistName and playlistInstanceName cannot "
			"be empty.";
		FATAL("%s", STR(result));
		return SendFail(pFrom, result, RESPONSE_ID);
	}

	// Gather parameters: localStreamName
	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);

	// Gather parameters: insertPoint
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, insertPoint, -3000, 3600 * 24 * 1000, -1000);

	// Gather parameters: sourceOffset
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, sourceOffset, -3000, 3600 * 24 * 1000, -2000);

	// Gather parameters: duration
	GATHER_OPTIONAL_SIGNED_NUMBER(settings, parameters, duration, -3000, 3600 * 24 * 1000, -1000);

	// User input sanitize
	if ((insertPoint < 0)&&(insertPoint != -1000)) {
		insertPoint = -1000;
		settings["insertPoint"] = insertPoint;
	}

	if ((sourceOffset < 0)&&(sourceOffset != -1000)&&(sourceOffset != -2000)) {
		sourceOffset = -2000;
		settings["sourceOffset"] = sourceOffset;
	}

	if ((duration < 0)&&(duration != -1000)) {
		duration = -1000;
		settings["duration"] = duration;
	}

	bool usePlaylistIntanceName = !playlistInstanceName.empty();
	if (usePlaylistIntanceName) {
		//find the last ;* 
		playlistName = playlistInstanceName;
		string delim = ";*";
		size_t i = playlistInstanceName.find_last_of(delim);
		if (i != string::npos) {
			playlistName = playlistInstanceName.substr(i + delim.size() - 1);
		}
	}
	
	//Get all the playlists with that name
	const map<uint32_t, RTMPPlaylist *> _playlists = RTMPPlaylist::GetPlaylists(playlistName);
	if (_playlists.size() == 0) {
		OriginApplication *pApp = (OriginApplication *) GetApplication();
		pApp->InsertPlaylistItem(playlistName, localStreamName, (double) insertPoint, (double) sourceOffset, (double) duration);
	} else {
		//Insert the item in all of them
		map<uint32_t, RTMPPlaylist *>::const_iterator i = _playlists.begin();
		for (; i != _playlists.end(); i++) {
			RTMPPlaylist *playlist = MAP_VAL(i);
			if (playlist) {
				if (usePlaylistIntanceName) {
					string originalRequestedStreamName = 
						playlist->GetOriginalRequestedStreamName();
					if (originalRequestedStreamName.compare(
							playlistInstanceName) == 0) {
						playlist->InsertItem(localStreamName, (double)insertPoint,
							(double)sourceOffset, (double)duration);
						//there should only be one
						break;
					}
				} else {
					playlist->InsertItem(localStreamName, (double)insertPoint,
						(double)sourceOffset, (double)duration);
				}
			}
		}
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "localStreamName playlistName insertPoint sourceOffset duration";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	//Done
	return SendSuccess(pFrom, "Playlist item inserted", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageTranscode(BaseProtocol *pFrom,
		Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);

	Variant settings;
	HAS_PARAMETERS;
	string localStreamName = "";
	string rmsTargetStreamName = "";

	// Gather parameters: source, check if this is a stream name
	GATHER_MANDATORY_STRING(settings, parameters, source);
	if (!IsValidURI(source, localStreamName)) {
		string result = format("Invalid source URI: %s", STR(source));
		FATAL("%s", STR(result));
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	settings["localStreamName"] = localStreamName;

	// Gather parameters: groupName
	GATHER_OPTIONAL_STRING(settings, parameters, groupName, "");
	if (groupName == "") {
		groupName = format("transcoded_group_%s", STR(generateRandomString(8)));
		settings["groupName"] = groupName;
	}

	// Gather parameters: destinations
	GATHER_TRANSCODE_PARAMS(settings, parameters, destinations);

	// Gather parameters: targetStreamNames
	if (parameters.HasKey("targetStreamNames")) {
		GATHER_TRANSCODE_PARAMS(settings, parameters, targetStreamNames);
	}

	// Check if we have the same number of elements with destinations and targetStreamNames
	CHECK_IF_ELEMENTS_MATCH(settings, destinations, targetStreamNames);

	// Gather parameters: videoBitrates
	if (parameters.HasKey("videoBitrates", false)) {
		GATHER_TRANSCODE_PARAMS(settings, parameters, videoBitrates);

		// Gather parameters: videoSizes
		if (parameters.HasKey("videoSizes", false)) {
			GATHER_TRANSCODE_PARAMS(settings, parameters, videoSizes);
		}

		// Gather parameters: videoAdvancedParamsProfiles
		if (parameters.HasKey("videoAdvancedParamsProfiles", false)) {
			GATHER_TRANSCODE_PARAMS(settings, parameters, videoAdvancedParamsProfiles);
		}
	} else {
		settings["videoAdvancedParamsProfiles"] = "veryfast";
		settings["videoAdvancedParamsProfiles"].Reset();
		settings["videoAdvancedParamsProfiles"].PushToArray("veryfast");
	}

	// Check if we have the same number of elements with destinations and videoBitrates
	CHECK_IF_ELEMENTS_MATCH(settings, destinations, videoBitrates);

	// Check if we have the same number of elements with videoBitrates and videoSizes
	CHECK_IF_ELEMENTS_MATCH(settings, videoBitrates, videoSizes);

	// Check if we have the same number of elements with videoBitrates and videoAdvancedParamsProfiles
	CHECK_IF_ELEMENTS_MATCH(settings, videoBitrates, videoAdvancedParamsProfiles);

	// Gather parameters: audioBitrates
	if (parameters.HasKey("audioBitrates", false)) {
		GATHER_TRANSCODE_PARAMS(settings, parameters, audioBitrates);

		// Gather parameters: audioChannelsCounts
		if (parameters.HasKey("audioChannelsCounts", false)) {
			GATHER_TRANSCODE_PARAMS(settings, parameters, audioChannelsCounts);
		}

		// Gather parameters: audioFrequencies
		if (parameters.HasKey("audioFrequencies", false)) {
			GATHER_TRANSCODE_PARAMS(settings, parameters, audioFrequencies);
		}

		// Gather parameters: audioAdvancedParamsProfiles
		if (parameters.HasKey("audioAdvancedParamsProfiles", false)) {
			GATHER_TRANSCODE_PARAMS(settings, parameters, audioAdvancedParamsProfiles);
		}
	}

	// Check if we have the same number of elements with destinations and audioBitrates
	CHECK_IF_ELEMENTS_MATCH(settings, destinations, audioBitrates);

	// Check if we have the same number of elements with audioBitrates and audioChannelsCounts
	CHECK_IF_ELEMENTS_MATCH(settings, audioBitrates, audioChannelsCounts);

	// Check if we have the same number of elements with audioBitrates and audioFrequencies
	CHECK_IF_ELEMENTS_MATCH(settings, audioBitrates, audioFrequencies);

	// Check if we have the same number of elements with audioBitrates and audioAdvancedParamsProfiles
	CHECK_IF_ELEMENTS_MATCH(settings, audioBitrates, audioAdvancedParamsProfiles);

	// Gather parameters: overlays
	if (parameters.HasKey("overlays", false)) {
		GATHER_TRANSCODE_PARAMS(settings, parameters, overlays);
	}

	// Check if we have the same number of elements with videoBitrates and overlays
	CHECK_IF_ELEMENTS_MATCH(settings, videoBitrates, overlays);

	// Gather parameters: croppings
	if (parameters.HasKey("croppings", false)) {
		GATHER_TRANSCODE_PARAMS(settings, parameters, croppings);
	}

	// Check if we have the same number of elements with videoBitrates and croppings
	CHECK_IF_ELEMENTS_MATCH(settings, videoBitrates, croppings);

	// Gather parameters: keepAlive
	GATHER_OPTIONAL_BOOLEAN(settings, parameters, keepAlive, true);

	// Get the additional parameters from config lua
	// Assign the binary path of the transcoder script
	string fullBinaryPath = "";
	if (_configuration.HasKeyChain(V_STRING, false, 2, "transcoder", "scriptPath")) {
		fullBinaryPath = (string) (_configuration.GetValue("transcoder", false)
				.GetValue("scriptPath", false));
	} else {
		fullBinaryPath = "./rmsTranscoder.sh";
	}
	string fullBinaryPathNormalized = normalizePath(fullBinaryPath, "");
	if (fullBinaryPathNormalized == "") {
		string error = format("Transcode utility not found: %s", STR(fullBinaryPath));
		FATAL("%s", STR(error));
		return SendFail(pFrom, error, RESPONSE_ID);
	}
	settings["fullBinaryPath"] = fullBinaryPathNormalized;

	// Assign the source URI prefix
	if (_configuration.HasKeyChain(V_STRING, false, 2, "transcoder", "srcUriPrefix")) {
		settings["srcUriPrefix"] = (string) (_configuration.GetValue("transcoder", false)
				.GetValue("srcUriPrefix", false));
	} else {
		settings["srcUriPrefix"] = "rtsp://localhost:5544/";
	}

	// Assign the destination URI prefix
	if (_configuration.HasKeyChain(V_STRING, false, 2, "transcoder", "dstUriPrefix")) {
		settings["dstUriPrefix"] = (string) (_configuration.GetValue("transcoder", false)
				.GetValue("dstUriPrefix", false));
	} else {
		settings["dstUriPrefix"] = "-f flv tcp://localhost:6666/";
	}

	// Compute the number of destinations
	uint32_t size = settings["destinations"].MapSize();

	// Prepare the settings for the launchProcess command
	Variant settingsNew;
	GatherCustomParameters(parameters, settingsNew);

	FOR_MAP(parameters, string, Variant, i) {
		if ((MAP_KEY(i).length() > 0) && (MAP_KEY(i)[0] == '$'))
			settingsNew[MAP_KEY(i)] = MAP_VAL(i);
	}
	settingsNew["fullBinaryPath"] = settings["fullBinaryPath"];
	settingsNew["groupName"] = settings["groupName"];
	settingsNew["keepAlive"] = settings["keepAlive"];

	bool forceReFlag;
	for (uint32_t i = 0; i < size; i++) {
		rmsTargetStreamName = "";

		if (!IsValidURI((string) settings["destinations"][i], rmsTargetStreamName)) {
			string result = format("Invalid destination URI: %s", STR((string) settings["destinations"][i]));
			FATAL("%s", STR(result));
			return SendFail(pFrom, result, RESPONSE_ID);
		}

		settings["rmsTargetStreamName"] = rmsTargetStreamName;
		// Check here first if the target destination stream name exists
		if (rmsTargetStreamName != "") {
			if (_pApp->IsStreamAlreadyCreated(OPERATION_TYPE_LAUNCHPROCESS, settings)) {
				string result = format("Destination stream (%s) to RMS already exists!", STR(rmsTargetStreamName));
				FATAL("%s", STR(result));
				return SendFail(pFrom, result, RESPONSE_ID);
			}
		}

		// Set the new 'arguments'
		string commandFlags;
		if (parameters.HasKey("commandFlags")) {
			settings["commandFlags"] = parameters["commandFlags"];
		}
		settingsNew["arguments"] = FormTranscodeArgs(settings, i, forceReFlag);
		// Remove the configId
		settingsNew.RemoveKey("configId");
		// Check if inbound file source and outbound non-file source
		if (forceReFlag) {
			settingsNew["$RMS_FORCE_RE"] = "true";
		} else {
			settingsNew.RemoveKey("$RMS_FORCE_RE");
		}

		// Launch the process
		if (!_pApp->LaunchProcess(settingsNew)) {
			string result = format("Unable to transcode with the following parameters: %s", STR(settingsNew["arguments"]));
			return SendFail(pFrom, result, RESPONSE_ID);
		}
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "audioBitrates croppings destinations dstUriPrefix rmsTargetStreamName fullBinaryPath groupName keepAlive localStreamName source srcUriPrefix targetStreamNames videoBitrates videoSizes";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, "Transcoding successfully started.", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListMediaFiles(BaseProtocol *pFrom,
	Variant &parameters)
{
	Variant settings;
	Variant appConfiguration = ClientApplicationManager::GetDefaultApplication()->GetConfiguration();
	string mediaFolder = appConfiguration["mediaStorage"][1]["mediaFolder"];
	vector<string> mediaFiles;

	EXTRACT_RESPONSE_ID(parameters);

	if(!listFolder(mediaFolder, mediaFiles, false, false, true)){
		return SendFail(pFrom, "Unable to list files", RESPONSE_ID);
	}

	settings["mediaFiles"].Reset();

	for(vector<string>::const_iterator i=mediaFiles.begin();i!=mediaFiles.end();++i){
		settings["mediaFiles"].PushToArray((*i).substr(mediaFolder.size() + 1));
	}	

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "mediaFiles";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */
	
	return SendSuccess(pFrom, "Successfully listed media files", settings, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageRemoveMediaFile(BaseProtocol *pFrom,
	Variant &parameters)
{
	Variant settings;
	Variant appConfiguration = ClientApplicationManager::GetDefaultApplication()->GetConfiguration();
	string mediaFolder = appConfiguration["mediaStorage"][1]["mediaFolder"];
	string fullMediaFilePath;

	EXTRACT_RESPONSE_ID(parameters);
	HAS_PARAMETERS;

	// Gather parameters: fileName
	if(parameters.HasKey("fileName", false)){
		GATHER_MANDATORY_STRING(settings, parameters, fileName);
		settings["fileName"] = fileName;
		fullMediaFilePath = mediaFolder + PATH_SEPARATOR + fileName;
	}

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	if(fileExists(fullMediaFilePath)){
		if(deleteFile(fullMediaFilePath)){
			return SendSuccess(pFrom, format("Successfully removed media file %s", STR(settings["fileName"])), settings, RESPONSE_ID);
		}

		return SendFail(pFrom, format("Cannot delete file %s", STR(settings["fileName"])), RESPONSE_ID);		
	}
	
	return SendFail(pFrom, format("File %s does not exist", STR(settings["fileName"])), RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageGenerateServerPlaylist(BaseProtocol *pFrom,
		Variant &parameters) {
	Variant settings;
	EXTRACT_RESPONSE_ID(parameters);
	SET_RESPONSE_ID(parameters, RESPONSE_ID);

	HAS_PARAMETERS;
	//FINEST("parameters:\n%s", STR(parameters.ToString()));

	// Gather parameters: sources
	if (parameters.HasKeyChain(V_STRING, false, 1, "sources")) {
		GATHER_MANDATORY_STRING(settings, parameters, sources);
		settings["sources"].Reset();
		settings["sources"].PushToArray(sources);
	} else {
		GATHER_MANDATORY_ARRAY(settings, parameters, sources);
	}

	// Gather parameters: sourceOffsets
	if (parameters.HasKey("sourceOffsets")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "sourceOffsets")) {
			GATHER_MANDATORY_SIGNED_NUMBER(settings, parameters, sourceOffsets, -2, 0xffffffff);
			settings["sourceOffsets"].Reset();
			settings["sourceOffsets"].PushToArray(sourceOffsets);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, sourceOffsets);
			Variant temp;
			for (uint32_t i = 0; i < settings["sourceOffsets"].MapSize(); i++) {
				string rawVal = (string) settings["sourceOffsets"][(uint32_t) i];
				trim(rawVal);
				int32_t val = (int32_t) atoi(STR(rawVal));
				if (format("%"PRId32, val) != rawVal) {
					FATAL("Syntax error. Parameter sourceOffsets is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter sourceOffsets is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["sourceOffsets"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["sources"].MapSize(); i++) {
			settings["sourceOffsets"].PushToArray((uint32_t) 0);
		}
	}

	if (settings["sources"].MapSize() != settings["sourceOffsets"].MapSize()) {
		FATAL("Syntax error. Parameters sources and sourceOffsets have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters sources and sourceOffsets have different number of elements"), RESPONSE_ID);
	}

	// Gather parameters: durations
	if (parameters.HasKey("durations")) {
		if (parameters.HasKeyChain(V_STRING, false, 1, "durations")) {
			GATHER_MANDATORY_SIGNED_NUMBER(settings, parameters, durations, -2, 0xffffffff);
			settings["durations"].Reset();
			settings["durations"].PushToArray(durations);
		} else {
			GATHER_MANDATORY_ARRAY(settings, parameters, durations);
			Variant temp;
			for (uint32_t i = 0; i < settings["durations"].MapSize(); i++) {
				string rawVal = (string) settings["durations"][(uint32_t) i];
				trim(rawVal);
				int32_t val = (int32_t) atoi(STR(rawVal));
				if (format("%"PRId32, val) != rawVal) {
					FATAL("Syntax error. Parameter durations is invalid");
					return SendFail(pFrom, format("Syntax error. Parameter durations is invalid"), RESPONSE_ID);
				}
				temp.PushToArray(val);
			}
			settings["durations"] = temp;
		}
	} else {
		for (uint32_t i = 0; i < settings["sources"].MapSize(); i++) {
			settings["durations"].PushToArray((uint32_t) 0);
		}
	}

	if (settings["sources"].MapSize() != settings["durations"].MapSize()) {
		FATAL("Syntax error. Parameters sources and durations have different number of elements");
		return SendFail(pFrom, format("Syntax error. Parameters sources and durations have different number of elements"), RESPONSE_ID);
	}

	// Gather parameters: pathToFile
	GATHER_MANDATORY_STRING(settings, parameters, pathToFile);

	// Check pathToFile format
	if (pathToFile.find(".lst") == string::npos) {
		pathToFile = pathToFile + ".lst";
		WARN("Filename format corrected to %s", STR(pathToFile));
	}
	parameters["pathToFile"] = pathToFile;
	FINEST("pathToFile: %s", STR(pathToFile));

	File serverPlaylist;
	if (!serverPlaylist.Initialize(pathToFile, FILE_OPEN_MODE_TRUNCATE)) {
		string message = format("Unable to create server playlist '%s'", STR(pathToFile));
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}

	string header = "# sourceOfset,duration,localStreamName\r\n";
	serverPlaylist.WriteString(header);
	for (uint32_t i = 0; i < settings["sourceOffsets"].MapSize(); i++) {
		string sourceOffset = (string) settings["sourceOffsets"][(uint32_t) i];
		string sourceDuration = (string) settings["durations"][(uint32_t) i];
		string streamName = (string) settings["sources"][(uint32_t) i];
		string playlistEntry = format("%s,%s,%s\r\n", STR(sourceOffset), STR(sourceDuration), STR(streamName));
		serverPlaylist.WriteString(playlistEntry);
	}
	serverPlaylist.Close();

	// Done
	string message = format("Server playlist written to %s", STR(pathToFile));

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		settings["asciiList"]["names"] = "sources pathToFile sourceOffsets durations";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */

	return SendSuccess(pFrom, message, settings, RESPONSE_ID);
}

#ifdef HAS_PROTOCOL_METADATA

bool CLIAppProtocolHandler::ProcessMessageAddMetadataListener(BaseProtocol *pFrom,
		Variant &parameters) {
	
	EXTRACT_RESPONSE_ID(parameters);
	
	StreamMetadataResolver *pSMR = _pApp->GetStreamMetadataResolver();
	if (pSMR == NULL) {
		return SendFail(pFrom, "Unable to get the streams metadata resolver", RESPONSE_ID);
	}
	
	HAS_PARAMETERS;
	Variant settings;
	Module module;
	
	//1. Gather parameters: port
	GATHER_MANDATORY_NUMBER(settings, parameters, port, 1, 65535);;
	
	//2. Gather parameters: ip
	GATHER_OPTIONAL_STRING(settings, parameters, ip, "0.0.0.0");
	if (getHostByName(ip) != ip) {
		return SendFail(pFrom, "Invalid ip", RESPONSE_ID);
	}
	
	//3. Gather parameters: localstreamname
	GATHER_OPTIONAL_STRING(settings, parameters, localStreamName, "0~0~0");

	TCPAcceptor *pAcceptor = NULL;

	//4. Add metadata listener
	if (!_pApp->AddMetadataListener(settings, pAcceptor)) {
		string result = "Unable to add new metadata listener";
		return SendFail(pFrom, result, RESPONSE_ID);
	}
	
	//5. Get some info
	Variant info;
	pAcceptor->GetStats(info, 0);

#ifdef HAS_PROTOCOL_ASCIICLI
	// Select items to list for ASCII CLI
	if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		info["asciiList"]["names"] = "id appName appId protocol port acceptedConnectionsCount enabled";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */
	
	//6. Done
	return SendSuccess(pFrom, "Metadata listener created", info, RESPONSE_ID);
}

bool CLIAppProtocolHandler::ProcessMessageListMetadataListeners(BaseProtocol *pFrom,
	Variant &parameters) {

	EXTRACT_RESPONSE_ID(parameters);
	
	map<uint32_t, IOHandler *> services = IOHandlerManager::GetActiveHandlers();
	
	Variant listeners;
	Variant match;
	Variant data = _pApp->GetStreamsConfig();
	listeners.IsArray(true);

	FOR_MAP(data["metalistener"], string, Variant, i) {
		match = MAP_VAL(i);
		match["id"] = match["acceptor"]["id"];
		listeners.PushToArray(match);
		match.Reset();
	}
	
	FOR_MAP(services, uint32_t, IOHandler *, i) {
		match.Reset();
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pAcceptor = (TCPAcceptor *)MAP_VAL(i);
		if (pAcceptor->GetParameters()[CONF_PROTOCOL] == CONF_PROTOCOL_INBOUND_JSONMETADATA) {
			MAP_VAL(i)->GetStats(match);
			bool foundInConfig = false;
			FOR_MAP(listeners, string, Variant, j) {
				if ((uint32_t)MAP_VAL(j)["id"] == (uint32_t)match["id"]) {
					foundInConfig = true;
					break;
				}
			}
			if (!foundInConfig) {
				listeners.PushToArray(match);
			}
			foundInConfig = false;
		}
	}
	
	// Get some info
	Variant info;
	info["listeners"] = listeners;

#ifdef HAS_PROTOCOL_ASCIICLI
		// Select items to list for ASCII CLI
		if ((pFrom != NULL) && (pFrom->GetType() == PT_INBOUND_ASCIICLI)) {
		info["asciiList"]["names"] = "listeners id ip port enabled localStreamName";
	}
#endif /* HAS_PROTOCOL_ASCIICLI */
		
		// Done
		return SendSuccess(pFrom, "Listing all the metadata listeners", info, RESPONSE_ID);
}

#endif  /*HAS_PROTOCOL_METADATA*/

bool CLIAppProtocolHandler::ProcessMessageCustomRTSPHeaders(BaseProtocol* pFrom,
	Variant &parameters) {
	Variant settings;
	EXTRACT_RESPONSE_ID(parameters);
	SET_RESPONSE_ID(parameters, RESPONSE_ID);

	HAS_PARAMETERS;

	// Gather parameters: pathToFile
	GATHER_MANDATORY_STRING(settings, parameters, scale);

	GATHER_MANDATORY_STRING(settings, parameters, localStreamName);

	string message;

	double numericScale = atof(scale.c_str());
	if (numericScale == 0) {
		message = "Invalid parameter scale. Valid values are numeric, decimal, positive and negative";
		FATAL("%s", STR(message));
		return SendFail(pFrom, message, RESPONSE_ID);
	}
	message = "RTSP server adds `scale` in the header in PLAY message for streamname: " + localStreamName;

	_pApp->InsertRtspStreamWithScale(localStreamName, (double)numericScale);

	return SendSuccess(pFrom, message, settings, RESPONSE_ID);
}

#endif	/* HAS_PROTOCOL_CLI */
