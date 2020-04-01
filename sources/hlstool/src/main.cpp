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
#include "protocols/cli/cliappprotocolhandler.h"
#include "application/originapplication.h"
#include "protocols/rtmp/streaming/infilertmpstream.h"
#include "protocols/rtmp/inboundrtmpprotocol.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "protocols/protocolmanager.h"
#include "protocols/passthrough/passthroughappprotocolhandler.h"
#include "streaming/hls/outfilehlsstream.h"
#ifdef HAS_LICENSE
#include "utils/misc/license.h"
#endif /* HAS_LICENSE */
using namespace app_rdkcrouter;

bool Work(int argc, const char **argv);
bool StartHLS(string &fileName, Variant &settings, string &tempFolder,
		bool deleteSeekAndMeta);

int main(int argc, const char **argv) {

	// Check first the license!
#ifdef HAS_LICENSE
	vector<string> licenseFilePathCandidates;
	ADD_VECTOR_END(licenseFilePathCandidates, GetEnvVariable("RMS_LICENSE_FILE_PATH"));
	ADD_VECTOR_END(licenseFilePathCandidates, "/etc/rdkcms/License.lic");
	ADD_VECTOR_END(licenseFilePathCandidates, "/usr/local/etc/rdkcms/License.lic");
	ADD_VECTOR_END(licenseFilePathCandidates, "./config/License.lic");
	ADD_VECTOR_END(licenseFilePathCandidates, "../config/License.lic");
	ADD_VECTOR_END(licenseFilePathCandidates, "./License.lic");

	string licenseFilePath = "";

	FOR_VECTOR(licenseFilePathCandidates, i) {
		string tempString = licenseFilePathCandidates[i];
		trim(tempString);
		if (tempString == "")
			continue;
		licenseFilePath = normalizePath(tempString, "");
		if ((licenseFilePath != "")&&(fileExists(licenseFilePath)))
			break;
	}
	if (licenseFilePath == "") {
		fprintf(stderr,
				"No license file found. A license file is required and it can be in one of the \
following locations:\n\n");

		FOR_VECTOR(licenseFilePathCandidates, i) {
			string tempString = licenseFilePathCandidates[i];
			trim(tempString);
			if (tempString != "") {
				fprintf(stderr, "\t%s\n", STR(tempString));
			}
		}
		fprintf(stderr, "\nor it can be manually specified by setting RMS_LICENSE_FILE_PATH environment variable\n\n");
		exit(-1);
	}

	License::SetLicenseFile(licenseFilePath);
	License *pLicense = License::GetLicenseInstance();
	LICENSE_VALIDATION retval = pLicense->ValidateLicense();
	if (retval != VALID) {
		fprintf(stderr, "LICENSE IS INVALID!\n");
		fprintf(stderr, "Fail code: %"PRIu32" : %s\n", (uint32_t) retval,
				STR(pLicense->InterpretValidationCode(retval)));
		if (retval == INVALID_FILE) {
			fprintf(stderr, "File 'License.lic' should be located within same folder.\n");
		}
		exit(-1);
	}
#endif /* HAS_LICENSE */

	//1. Initialize the random number generator
	SRAND();

	//2. Init the logger
	Logger::Init();
	Variant dummy;
	dummy[CONF_LOG_APPENDER_NAME] = "implicit console appender";
	dummy[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_CONSOLE;
	dummy[CONF_LOG_APPENDER_COLORED] = (bool)true;
	dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
	ConsoleLogLocation * pLogLocation = new ConsoleLogLocation(dummy);
	pLogLocation->SetLevel(_FINEST_);
	Logger::AddLogLocation(pLogLocation);

	//3. Do the damage
	bool result = Work(argc, argv);

#ifdef HAS_LICENSE
	License::ResetLicense();
#endif /* HAS_LICENSE */

	//4. Shutdown the logger
	WARN("Shutting down the logger leaving you in the dark. Bye bye... :(");
	Logger::Free(true);

	//5. Done
	return result ? 0 : -1;
}

bool Work(int argc, const char **argv) {

	Variant dummy;
	dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;

	//1. read the parameters
	Variant fullParameters;
	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, fullParameters)) {
		FATAL("Unable to read the parameters from command line");
		return false;
	}

	//2. Get the arguments
	Variant arguments = fullParameters["arguments"];

	//3. Do we have a valid file parameters?
	if (!arguments.HasKeyChain(V_STRING, false, 1, "file")) {
		FATAL("file parameter is mandatory");
		return false;
	}
	string file = (string) arguments.GetValue("file", false);
	if (!fileExists(file)) {
		FATAL("File `%s` not found", STR(file));
		return false;
	}

	//4. Get the temp folder
	string tempString = "/tmp";
	if (arguments.HasKeyChain(V_STRING, false, 1, "tempFolder")) {
		tempString = (string) arguments.GetValue("tempFolder", false);
	}
	trim(tempString);
	string tempFolder = normalizePath(tempString, "");
	if (tempFolder == "") {
		FATAL("incorrect tempFolder specified: `%s`", STR(tempString));
		return false;
	}

	//5. Get the deleteSeekAndMeta flag
	bool deleteSeekAndMeta = true;
	if (arguments.HasKeyChain(V_STRING, false, 1, "deleteSeekAndMeta")) {
		tempString = lowerCase((string) arguments.GetValue("deleteSeekAndMeta", false));
		trim(tempString);
		deleteSeekAndMeta = (tempString == "1")
				|| (tempString == "true")
				|| (tempString == "on")
				|| (tempString == "yes");
	}

	//5. This is going to always be an appending playlist
	arguments["playlistType"] = "rolling";

	//6. normalize the settings
	Variant settings;
	bool result = false;
	if ((!CLIAppProtocolHandler::NormalizeHLSParameters(NULL, arguments,
			settings, result)) || (!result)) {
		FATAL("Unable to normalize the settings");
		return false;
	}

	//7. Compute the hls paths
	Variant hlsStreams;
	if (!OriginApplication::ComputeHLSPaths(settings, hlsStreams)) {
		FATAL("Unable to compute HLS paths");
		return false;
	}

	//8. Start the HLS process
	if (hlsStreams.MapSize() < 1) {
		FATAL("Unable to compute HLS paths");
		return false;
	}

	return StartHLS(file, hlsStreams[(uint32_t) 0], tempFolder, deleteSeekAndMeta);
}

bool StartHLS(string &fileName, Variant &settings, string &tempFolder,
		bool deleteSeekAndMeta) {
	//3. Initialize a dummy application
	Variant appConfig;
	appConfig["name"] = "HLSApplication";
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_MEDIAFOLDER] = "/";
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_METAFOLDER] = tempFolder;
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_KEYFRAMESEEK] = (bool)true;
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_CLIENTSIDEBUFFER] = 0xFFFFFFFF;
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_SEEKGRANULARITY] = (int32_t) 1;
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["default"][CONF_APPLICATION_EXTERNALSEEKGENERATOR] = (bool)false;
	appConfig[CONF_APPLICATION_MEDIASTORAGE]["hasTimers"] = (bool)false;
	BaseClientApplication *pApp = new BaseClientApplication(appConfig);
	if (!pApp->Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}
	if (pApp->GetStreamMetadataResolver() == NULL) {
		FATAL("Unable to get stream metadata resolver");
		return false;
	}
	Metadata metadata;
	if (!pApp->GetStreamMetadataResolver()->ResolveMetadata(fileName, metadata, true)) {
		FATAL("Unable to compute metadata for stream %s", STR(fileName));
		return false;
	}

	//4. add the RTMP and pass through handlers to it
	BaseRTMPAppProtocolHandler *pRTMPHandler = new BaseRTMPAppProtocolHandler(appConfig);
	pApp->RegisterAppProtocolHandler(PT_INBOUND_RTMP, pRTMPHandler);
	PassThroughAppProtocolHandler *pPTHandler = new PassThroughAppProtocolHandler(appConfig);
	pApp->RegisterAppProtocolHandler(PT_PASSTHROUGH, pPTHandler);

	//5. create a dummy RTMP protocol handler
	InboundRTMPProtocol *pProtocol = new InboundRTMPProtocol();
	pProtocol->SetApplication(pApp);

	//6. get in stream
	//FINEST("metadata:\n%s", STR(metadata.ToString()));
	uint32_t rtmpSreamId = 0;
	if (!pProtocol->CreateNeutralStream(rtmpSreamId)) {
		FATAL("Unable to create neutral stream");
		return false;
	}
	InFileRTMPStream *pInStream = pProtocol->CreateIFS(rtmpSreamId, metadata);
	pInStream->SingleGop(false);
	//FINEST("pRTMPInFileStream: %p", pRTMPInFileStream);

	//7. Create the out stream
	settings["playlistType"] = "appending";
	//ASSERT("settings: %s", STR(settings.ToString()));
	OutFileHLSStream *pOutStream = OutFileHLSStream::GetInstance(
			pApp,
			settings["localStreamName"],
			settings);

	bool result = true;
	if (pOutStream != NULL) {
		//8. Link them together
		if (pInStream->Link(pOutStream)) {
			if (!pInStream->FeedAllFile()) {
				FATAL("Unable to feed the file");
				result = false;
			}
		} else {
			FATAL("Source stream %s not compatible with HLS streaming", STR(settings["localStreamName"]));
			result = false;
		}
	} else {
		FATAL("Unable to create HLS stream");
		result = false;
	}

	//9. Cleanup
	pOutStream->EnqueueForDelete();
	pProtocol->EnqueueForDelete();
	ProtocolManager::CleanupDeadProtocols();
	pApp->UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
	pApp->UnRegisterAppProtocolHandler(PT_PASSTHROUGH);
	delete pRTMPHandler;
	delete pPTHandler;
	delete pApp;

	if (deleteSeekAndMeta) {
		deleteFile(metadata.metaFileFullPath());
		deleteFile(metadata.seekFileFullPath());
	}

	if (!result) {
		FATAL("Feed process failed");
	}
	return result;
}
