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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define LUAFILENAME "installer.lua"
#define LUASECTION "installer"

void PrintHelp() {
	fprintf(stdout, "Help\n");
}

int main(int argc, const char **argv) {

	// Get the commandline parameters
	Variant commandLine;
	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, commandLine)) {
		PrintHelp();
		return -1;
	}
	cout << "--- Command line parameters ---" << endl;
	cout << commandLine.ToString() << endl;

	// Read the installer file
	cout << "--- Process file '" << LUAFILENAME << "' ---" << endl;
	Variant contents;
	if (fileExists(LUAFILENAME)) {
		// Use LUA to parse the License file
		if (!ReadLuaFile(LUAFILENAME, LUASECTION, contents)) {
			cout << "*** Parse failure ***" << endl;
			return -1; //parse failure;
		}
		cout << "--- Data from section '" << LUASECTION << "' of file '" << LUAFILENAME << "' ---" << endl;
		cout << contents.ToString() << endl;
	} else {
		cout << "*** Invalid file ***" << endl;
		return -2; //invalid file;
	}
	cout << "--- Done ---" << endl;
	return 0;
};
