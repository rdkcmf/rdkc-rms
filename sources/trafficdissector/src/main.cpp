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


#include "common.h"
#include "pcapfilesource.h"
#include "rtmptrafficdissector.h"

void PrintHelp(Variant &parameters);
void PrintVersion();
bool NormalizeCommandLine(Variant &parameters);
bool NormalizeIpPort(Variant &parameters, string key, bool isServer);

int main(int argc, const char **argv) {
	Logger::Init();

	Variant consoleAppender;
	consoleAppender[CONF_LOG_APPENDER_NAME] = "implicit console appender";
	consoleAppender[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_CONSOLE;
	consoleAppender[CONF_LOG_APPENDER_COLORED] = (bool)true;
	consoleAppender[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
	ConsoleLogLocation * pLogLocation = new ConsoleLogLocation(consoleAppender);
	pLogLocation->SetLevel(_FINEST_);
	Logger::AddLogLocation(pLogLocation);

	//1. Pick up the startup parameters and hold them inside the running status
	if (argc < 2) {
		FATAL("Invalid command line. Use --help for details");
		Logger::Free(true);
		return -1;
	}

	Variant parameters;

	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, parameters)) {
		PrintHelp(parameters);
		Logger::Free(true);
		return -1;
	}

	if (!NormalizeCommandLine(parameters)) {
		FATAL("Invalid command line. Use --help for details");
		Logger::Free(true);
		return -1;
	};

	if ((bool)parameters["arguments"]["help"]) {
		PrintHelp(parameters);
		Logger::Free(true);
		return 0;
	}

	if ((bool)parameters["arguments"]["version"]) {
		PrintVersion();
		Logger::Free(true);
		return 0;
	}

	string trafficFile = argv[argc - 1];
	if (trafficFile.find("--") == 0)
		trafficFile = "";
	trafficFile = normalizePath(trafficFile, "");

	if (trafficFile == "") {
		FATAL("Invalid command line. Use --help for details");
		Logger::Free(true);
		return -1;
	}
	parameters["arguments"].RemoveKey(trafficFile);
	parameters["arguments"]["trafficFile"] = trafficFile;

	INFO("%s", STR(Version::GetBanner()));
	INFO("Starting traffic dissector on file `%s`", STR(trafficFile));

	RTMPTrafficDissector dissector;
	if (!dissector.Init(parameters)) {
		FATAL("Unable to initialize the dissector");
		Logger::Free(true);
		return -1;
	}

	if (!dissector.Run()) {
		FATAL("Unable to run the dissector");
		Logger::Free(true);
		return -1;
	}

	INFO("Dissector operations completed.");

	Logger::Free(true);
	return 0;
}

void PrintHelp(Variant &parameters) {
	fprintf(stdout, "\n%s\n", STR(Version::GetBanner()));
	fprintf(stdout, "\nUsage:\n");
	fprintf(stdout, "%s --help\n", STR(parameters["program"]));
	fprintf(stdout, "%s --version\n", STR(parameters["program"]));
	fprintf(stdout, "%s --client-ip-port=ip:port trafficFile\n\n", STR(parameters["program"]));
	fprintf(stdout, "OPTIONS:\n");
	fprintf(stdout, "    --help\n");
	fprintf(stdout, "      Prints this help and exit\n\n");
	fprintf(stdout, "    --version\n");
	fprintf(stdout, "      Prints the version and exit.\n\n");
	fprintf(stdout, "    --client-ip-port\n");
	fprintf(stdout, "      The ip/port of the client connecting to the server.\n");
	fprintf(stdout, "      Used to filter the relevant traffic and is mandatory\n\n");
	fprintf(stdout, "    --server-ip-port\n");
	fprintf(stdout, "      The ip/port on which the server will accept the connection.\n");
	fprintf(stdout, "      Used to filter the relevant traffic and is mandatory\n\n");
	fprintf(stdout, "    --output-file\n");
	fprintf(stdout, "      Path to the file to spit out all messages. It is optional\n\n");
}

void PrintVersion() {
	fprintf(stdout, "%s\n", STR(Version::GetBanner()));
	if (Version::GetBuilderOSUname() != "")
		fprintf(stdout, "Compiled on machine: `%s`\n", STR(Version::GetBuilderOSUname()));
}

bool NormalizeCommandLine(Variant &parameters) {
	bool exit = false;
	bool tmp = (parameters["arguments"]["--help"] != V_NULL);
	parameters["arguments"]["help"] = (bool)tmp;
	parameters["arguments"].RemoveKey("--help", false);
	exit |= tmp;
	tmp = (parameters["arguments"]["--version"] != V_NULL);
	parameters["arguments"]["version"] = (bool)tmp;
	parameters["arguments"].RemoveKey("--version", false);
	exit |= tmp;
	if (exit)
		return true;

	if (parameters["arguments"].HasKeyChain(V_STRING, false, 1, "--output-file")) {
		string outputFile = parameters["arguments"].GetValue("--output-file", false);
		parameters["arguments"].RemoveKey("--output-file", false);
		if (fileExists(outputFile)) {
			FATAL("Output file exists: %s", STR(outputFile));
			return false;
		}
		parameters["arguments"]["output-file"] = outputFile;
	}

	if (!NormalizeIpPort(parameters, "--client-ip-port", false)) {
		if (!NormalizeIpPort(parameters, "--server-ip-port", true)) {
			FATAL("Invalid --client-ip-port or --server-ip-port");
			return false;
		}
	}

	return true;
}

bool NormalizeIpPort(Variant &parameters, string key, bool isServer) {
	if (!parameters["arguments"].HasKeyChain(V_STRING, false, 1, STR(key)))
		return false;

	string raw = (string) parameters["arguments"].GetValue(key, false);
	parameters["arguments"].RemoveKey(key, false);
	vector<string> parts;
	split(raw, ":", parts);
	if (parts.size() != 2)
		return false;
	trim(parts[0]);
	trim(parts[1]);
	if ((parts[0] == "") || (parts[1] == ""))
		return false;

	in_addr dummy;
	if (inet_aton(STR(parts[0]), &dummy) == 0)
		return false;

	uint16_t port = atoi(STR(parts[1]));
	if (format("%"PRIu16, port) != parts[1])
		return false;

	parameters["arguments"]["filter"]["ip"] = (uint32_t) dummy.s_addr;
	parameters["arguments"]["filter"]["port"] = (uint16_t) EHTONS(port);
	parameters["arguments"]["filter"]["isServer"] = (bool)isServer;
	parameters["arguments"]["filter"]["complete"] = "tcp and host " + parts[0] + " and port " + parts[1];

	return true;
}
