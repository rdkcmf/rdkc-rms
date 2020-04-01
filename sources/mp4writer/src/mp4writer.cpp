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
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughappprotocolhandler.h"
#include "recording/mp4/outfilemp4.h"

#include <stdio.h>
#include <string>

using namespace std;

bool Work(int argc, const char **argv);

int main(int argc, const char **argv) {
	// Initialize the random number generator
	SRAND();

	// Init the logger
	Logger::Init();
	Variant dummy;
	dummy[CONF_LOG_APPENDER_NAME] = "implicit console appender";
	dummy[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_CONSOLE;
	dummy[CONF_LOG_APPENDER_COLORED] = (bool)true;
	dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
	ConsoleLogLocation * pLogLocation = new ConsoleLogLocation(dummy);
//	pLogLocation->SetLevel(_FINEST_);
	pLogLocation->SetLevel(_ERROR_);
	Logger::AddLogLocation(pLogLocation);

	// Start the process
	bool result = Work(argc, argv);

	// Shutdown the logger
	WARN("Shutting down the logger leaving you in the dark. Bye bye... :(");
	Logger::Free(true);

	//5. Done
	return result ? 0 : -1;
}

bool Work(int argc, const char **argv) {
		Variant commandLine;
	string filePath;
	bool recoveryMode = false;

	// Get the arguments
	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, commandLine)) {
		FATAL("Could not deserialize parameters!");
		return false;
	}

	if (commandLine["arguments"].HasKey("-path")) {
		filePath = (string) commandLine["arguments"]["-path"];
	}

	if (commandLine["arguments"].HasKey("--recovery")) {
		recoveryMode = true;
	}
	Variant settings;
	string name = "mp4writer";

	// Initialize a dummy application
	Variant appConfig;
	appConfig["name"] = "MP4Application";
	BaseClientApplication *pApp = new BaseClientApplication(appConfig);
	if (!pApp->Initialize()) {
		FATAL("Unable to initialize application!");
		return false;
	}

	// Add a dummy handler
	PassThroughAppProtocolHandler *pPTHandler = new PassThroughAppProtocolHandler(appConfig);
	pApp->RegisterAppProtocolHandler(PT_PASSTHROUGH, pPTHandler);

	// Get instance of MP4 writer
	OutFileMP4 *pMP4 = OutFileMP4::GetInstance(pApp, name, settings);

	if (!pMP4->InitializeFromFile(filePath, recoveryMode)) {
		FATAL("Could not initialize from file (%s)!", STR(filePath));
		return false;
	}

	if (!pMP4->Assemble()) {
		FATAL("Could not assemble the mp4 file!");
		return false;
	}
	
	// Clean-up
	pApp->UnRegisterAppProtocolHandler(PT_PASSTHROUGH);
	delete pMP4;
	delete pPTHandler;
	delete pApp;
	
	return true;
}
