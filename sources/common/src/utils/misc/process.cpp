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

void Process::GetFinished(vector<pid_t> &pids, bool &noMorePids) {
	GetFinishedProcesses(pids, noMorePids);
	//	FOR_VECTOR(pids, i) {
	//		FINEST("Pid terminated: %"PRIu64, (uint64_t) pids[i]);
	//	}
}

bool Process::Launch(Variant &settings, pid_t &pid) {
	string rawArguments = settings["arguments"];
	//FINEST("BEFORE: `%s`", STR(rawArguments));
	replace(rawArguments, "\\\\", "_#slash#_");
	replace(rawArguments, "\\ ", "_#space#_");
	replace(rawArguments, "\\\"", "_#quote#_");
	//FINEST(" AFTER: `%s`", STR(rawArguments));

	bool insideQuotes = false;

	size_t pos = 0;
	size_t start = 0;
	vector<string> quotedStrings;

	while ((pos = rawArguments.find("\"", pos)) != string::npos) {

		if (!insideQuotes) {
			insideQuotes = true;
			start = pos;
		} else {
			insideQuotes = false;
			string quotedString = rawArguments.substr(start, pos - start + 1);
			ADD_VECTOR_END(quotedStrings, quotedString);
		}
		pos++;
	}

	FOR_VECTOR(quotedStrings, i) {
		string guardedString = quotedStrings[i];
		replace(guardedString, " ", "_#space#_");
		replace(guardedString, "\"", "");
		replace(rawArguments, quotedStrings[i], guardedString);
	}

	vector<string> arguments;
	split(rawArguments, " ", arguments);

	FOR_VECTOR(arguments, i) {
		replace(arguments[i], "_#space#_", " ");
		replace(arguments[i], "_#slash#_", "\\");
		replace(arguments[i], "_#quote#_", "\\\"");
	}


	vector<string> envVars;

	FOR_MAP(settings, string, Variant, i) {
		string key = MAP_KEY(i);
		if ((key.size() < 2) || (key[0] != '$'))
			continue;
		ADD_VECTOR_END(envVars, format("%s=%s", STR(key.substr(1)), STR(MAP_VAL(i))));
	}

	string fullBinaryPath = (string) settings["fullBinaryPath"];

	bool result = LaunchProcess(fullBinaryPath, arguments, envVars, pid);
	//	if (result) {
	//		FINEST("Pid for %s created: %"PRIu64, STR(settings["fullBinaryPath"]), (uint64_t) pid);
	//	}
	return result;
}
