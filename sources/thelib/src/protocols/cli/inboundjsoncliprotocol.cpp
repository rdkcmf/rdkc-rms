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
#include "protocols/cli/inboundjsoncliprotocol.h"
#include "protocols/http/inboundhttpprotocol.h"

InboundJSONCLIProtocol::InboundJSONCLIProtocol()
: InboundBaseCLIProtocol(PT_INBOUND_JSONCLI) {
	_useLengthPadding = false;
}

InboundJSONCLIProtocol::~InboundJSONCLIProtocol() {
}

#define MAX_COMMAND_LENGTH 16*1024

bool InboundJSONCLIProtocol::Initialize(Variant &parameters) {
	InboundBaseCLIProtocol::Initialize(parameters);
	if (parameters["useLengthPadding"] == V_BOOL) {
		_useLengthPadding = (bool)parameters["useLengthPadding"];
	}
	return true;
}

bool InboundJSONCLIProtocol::SignalInputData(IOBuffer &buffer) {
	//1. Get the buffer and the length
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
	if (length == 0)
		return true;

	//2. Walk through the buffer and execute the commands
	string command = "";
	for (uint32_t i = 0; i < length; i++) {
		if ((pBuffer[i] == 0x0d) || (pBuffer[i] == 0x0a)) {
			if (command != "") {
				if (!ParseCommand(command)) {
					FATAL("Unable to parse command\n`%s`", STR(command));
					return false;
				}
			}
			command = "";
			buffer.Ignore(i);
			pBuffer = GETIBPOINTER(buffer);
			length = GETAVAILABLEBYTESCOUNT(buffer);
			i = 0;
			continue;
		}
		command += (char) pBuffer[i];
		if (command.length() >= MAX_COMMAND_LENGTH) {
			FATAL("Command too long");
			return false;
		}
	}

	//3. Done
	return true;
}

bool InboundJSONCLIProtocol::SendMessage(Variant &message) {
	string json;
	if (!message.SerializeToJSON(json, true)) {
		FATAL("Unable to serialize to JSON");
		return false;
	}
	json += "\r\n\r\n";
	if (_useLengthPadding) {
		uint32_t size = EHTONL((uint32_t) json.length());
		_outputBuffer.ReadFromBuffer((uint8_t *) & size, 4);
	}
	_outputBuffer.ReadFromString(json);
	return EnqueueForOutbound();
}

bool InboundJSONCLIProtocol::ParseCommand(string &command) {
	//1. Replace the '\\' escape sequence
	replace(command, "\\\\", "_#slash#_");

	//2. Replace the '\ ' escape sequence
	replace(command, "\\ ", "_#space#_");

	//3. Replace the '\=' escape sequence
	replace(command, "\\=", "_#equal#_");

	//4. Replace the '\,' escape sequence
	replace(command, "\\,", "_#coma#_");

	//5. Append "cmd=" in front of the command
	command = "cmd=" + command;
	//INFO("command: `%s`", STR(command));

	//6. create the map
	map<string, string> rawMap = mapping(command, " ", "=", true);

	//7. Create the variant
	Variant message;
	message["command"] = rawMap["cmd"];
	rawMap.erase("cmd");

	string key;
	string value;
	vector<string> list;

	FOR_MAP(rawMap, string, string, i) {
		key = MAP_KEY(i);
		replace(key, "_#space#_", " ");
		replace(key, "_#slash#_", "\\");
		replace(key, "_#equal#_", "=");
		replace(key, "_#coma#_", ",");

		value = MAP_VAL(i);
		replace(value, "_#space#_", " ");
		replace(value, "_#slash#_", "\\");
		replace(value, "_#equal#_", "=");

		list.clear();
		split(value, ",", list);
		if (list.size() != 1) {
			for (uint32_t j = 0; j < list.size(); j++) {
				trim(list[j]);
				// Special case for transcode command
				if ((list[j] == "") && (lowerCase(message["command"]) != "transcode"))
					continue;
				replace(list[j], "_#coma#_", ",");
				message["parameters"][key].PushToArray(list[j]);
			}
		} else {
			replace(value, "_#coma#_", ",");
			message["parameters"][key] = value;
		}
	}
	return ProcessMessage(message);
}

#endif /* HAS_PROTOCOL_CLI */
