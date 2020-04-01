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
#ifdef HAS_PROTOCOL_ASCIICLI
#include "protocols/cli/inboundasciicliprotocol.h"
#include <fstream>

InboundASCIICLIProtocol::InboundASCIICLIProtocol()
: InboundBaseCLIProtocol(PT_INBOUND_ASCIICLI) {
}

InboundASCIICLIProtocol::~InboundASCIICLIProtocol() {
}

#define MAX_COMMAND_LENGTH 16*1024

bool InboundASCIICLIProtocol::Initialize(Variant &parameters) {
    InboundBaseCLIProtocol::Initialize(parameters);
    return true;
}

bool InboundASCIICLIProtocol::SignalInputData(IOBuffer &buffer) {
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

namespace {
    bool full = true;
    bool indented = true;
    bool debug = false;

    string Prettify(string result, Variant data, uint8_t level, string &names) {
        string indent;
        bool fresh = true;
        uint8_t count = 0;

        if (indented) {
            for (int i = 0; i < level; i++) {
                indent += "  ";
            }
        }
        if (data == V_MAP) {
            // is an array or a hash map
            if (data.IsArray()) {
                // is an array
                if (data.MapSize() == 0) {
                    result += "[]\n\r";
                }
                FOR_MAP(data, string, Variant, iData) {
                    count++;
                    if (fresh) {
                        if (debug) result += "--ARRAY--";
                        result += "\n\r";
                        fresh = false;
                    }
                    result += indent;
                    if (debug) result += format("[%"PRIu8"]: ", count);
                    else result += "-- ";
                    // print array entry
                    Variant detail = MAP_VAL(iData);
                    result += Prettify("", detail, level + 1, names);
                }
            } else {
                // is a hash map
                if (data.MapSize() == 0) {
                    result += debug ? "{}\n\r" : "\n\r";
                }
                FOR_MAP(data, string, Variant, iData) {
                    count++;
                    if (full || debug ||
                            (names.find(" " + lowerCase(MAP_KEY(iData) + " ")) != string::npos)) { //add spaces for full match
                        if (fresh) {
                            if (debug) result += "--HASH MAP--";
                            result += "\n\r";
                            fresh = false;
                        }
                        result += indent;
                        if (debug) result += format("{%"PRIu8"}: ", count);
                        // print hash map entry
                        result += format("%s: ", STR(MAP_KEY(iData)));
                        Variant detail = MAP_VAL(iData);
                        result += Prettify("", detail, level + 1, names);
                    }
                }
            }
        } else {
            // is not an array nor a hash map
            // print value if not empty, or dot if empty
            if ((data != V_NULL) && (((string) data).size() > 0))
                result += (string) data;
            else
                result += debug ? "." : "";
            result += "\n\r";
        }
        return result;
    }

    string PrettyPrint(string result, Variant data, uint8_t level, Variant options) {
        string names;
        if (options.HasKey("names", false)) {
            names = " " + lowerCase((string) options["names"]) + " "; //add spaces for full match
            full = false;
        }
        if (options.HasKey("debug", false)) {
            debug = (bool) options["debug"];
            indented = debug;
        }
        result += Prettify("", data, level + 1, names);
        return result;
    }
}

bool InboundASCIICLIProtocol::SendMessage(Variant &message) {
    string result;
    if (message["status"] == "SUCCESS") {
        Variant asciiList;
        if (message.HasKeyChain(V_MAP, false, 2, "data", "asciiList")) {
            asciiList = message["data"].GetValue("asciiList", false);
            message["data"].RemoveKey("asciiList", false);
        }
        uint8_t level = 1;
        result = PrettyPrint(result, message["data"], level, asciiList);
        _outputBuffer.ReadFromString("Command entered successfully!\n\r" +
                (string) message["description"] + "\n\r" + result + "\n\r\n");

	string descr = (string) message["description"];
	string msg = "Run-time configuration";
	if(descr.compare(msg) == 0) {
		INFO("Saving Run-time configuration");
		ofstream fp("/tmp/rmslistconfig.txt");
		if (!fp) {
			FATAL("Error writing to /tmp/rmslistconfig.txt");
		}
		else {
			fp << result;
		}
	}
	else {
		INFO("its not a Run-time configuration");
	}
    } else {
        result = format("%s", STR((string) message["description"]));
        _outputBuffer.ReadFromString("Command failed!\n\r" + result + "\n\r\n\r");
    }
    return EnqueueForOutbound();
}

bool InboundASCIICLIProtocol::ParseCommand(string &command) {
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

#endif /* HAS_PROTOCOL_ASCIICLI */
#endif /* HAS_PROTOCOL_CLI */
