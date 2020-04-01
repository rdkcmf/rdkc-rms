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
*/

#include "netio/netio.h"
#include "configuration/configfile.h"
#include "protocols/protocolmanager.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/defaultprotocolfactory.h"
#include "protocols/rtmp/rtmpmessagefactory.h"
#include "eventlogger/eventlogger.h"
#include "application/webserver.h"
 #ifdef ANDROID
#include "jnibridgeews.h"
#endif	/* ANDROID */

#ifdef LINUX
#include <sys/prctl.h>
#endif

//This is a structure that holds the state
//between config re-reads/re-runs

struct RunningStatus {
	// startup parameters
	Variant commandLine;

	//Configuration file
	ConfigFile *pConfigFile;

	//default protocol factory
	DefaultProtocolFactory * pProtocolFactory;

	//should we run again?
	bool run;

	//is this a daemon already?
	bool daemon;

	//do we have LmMInterface
	bool hasLMInterface;

	//only web ui license works
	bool webUiLicenseOnly;

	RunningStatus() {
		pConfigFile = NULL;
		pProtocolFactory = NULL;
		run = false;
		daemon = false;
		hasLMInterface = false;
		webUiLicenseOnly = false;
	}
};

void QuitSignalHandler(void);
bool Initialize();
void Run();
void Cleanup();
void PrintHelp();
void PrintVersion();
void NormalizeCommandLine(string configFile);
bool ApplyUIDGID();
void WritePidFile(pid_t pid);

RunningStatus gRs;

#ifdef COMPILE_STATIC
BaseClientApplication *SpawnApplication(Variant configuration);
BaseProtocolFactory *SpawnFactory(Variant configuration);
#ifdef ANDROID
#define PRINTOUT(...) __android_log_print(ANDROID_LOG_INFO, "rdkcms", __VA_ARGS__);
#define PRINTERR(...) __android_log_print(ANDROID_LOG_ERROR, "rdkcms", __VA_ARGS__);
#else
#define PRINTOUT(...) fprintf(stdout, __VA_ARGS__);
#define PRINTERR(...) fprintf(stderr, __VA_ARGS__);
#endif
#endif /* COMPILE_STATIC */

#ifndef ANDROID
int main(int argc, const char **argv) {
	//the very first thing we do, we install the crashdump handler
	InstallCrashDumpHandler(NULL);
#else
bool FromRDKCMS() {
	return true;
}
int startEWS(int argc, const char **argv) {
#endif
	//1. Pick up the startup parameters and hold them inside the running status
	//PRINTOUT("Starting Web Server");
	if (argc < 2) {
		fprintf(stdout, "Invalid command line. Use --help\n");
		return -1;
	}

	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, gRs.commandLine)) {
		PrintHelp();
		return -1;
	}
	string configFile = argv[argc - 1];
	configFile = normalizePath(configFile, "");
	NormalizeCommandLine(configFile);

	if ((bool)gRs.commandLine["arguments"]["--help"]) {
		PrintHelp();
		return 0;
	}

	if ((bool)gRs.commandLine["arguments"]["--version"]) {
		PrintVersion();
		return 0;
	}

	if ((bool)gRs.commandLine["arguments"]["--webuilicenseonly"]) {
		gRs.webUiLicenseOnly = true;
	} else {
		//can only use the entire web server if rdkcms is running

	}
	if (configFile == "") {
		fprintf(stderr, "Configuration file not found: `%s`\n", argv[argc - 1]);
		return -1;
	}

#ifdef WIN32
	string mutexName = "webserver";
	size_t separatorPosition = configFile.find_last_of('\\');
	if (separatorPosition == string::npos) {
		mutexName += "-" + configFile;
	} else {
		mutexName += "-" + configFile.substr(separatorPosition + 1);
	}
	CreateMutex(NULL, TRUE, STR(mutexName));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		printf("\nAn instance of Web Server (%s) is already running.\n", STR(mutexName));
		return -1;
	}
#endif	/* WIN32 */

	SRAND();
	InitNetworking();

	do {
		//2. Reset the run flag
		gRs.run = false;

		//3. Initialize the running status
		if (Initialize()) {
			Run();
		} else {
			gRs.run = false;
		}

		//5. Cleanup
		Cleanup();
	} while (gRs.run);

	//6. We are done
	return 0;
}

void LogServerStarted() {
	EventLogger::GetDefaultLogger()->LogServerStarted();
	map<uint32_t, BaseClientApplication *> apps = ClientApplicationManager::GetAllApplications();

	FOR_MAP(apps, uint32_t, BaseClientApplication *, i) {
		if (MAP_VAL(i) == NULL)
			continue;
		EventLogger *pEvtLog = MAP_VAL(i)->GetEventLogger();
		if (pEvtLog == NULL)
			continue;
		pEvtLog->LogServerStarted();
	}
}

bool Initialize() {
	//PRINTOUT("Initializing Web Server");
	Logger::Init();

	if ((bool)gRs.commandLine["arguments"]["--use-implicit-console-appender"]) {
		Variant dummy;
		dummy[CONF_LOG_APPENDER_NAME] = "implicit console appender";
		dummy[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_CONSOLE;
		dummy[CONF_LOG_APPENDER_COLORED] = (bool)true;
		dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
		ConsoleLogLocation * pLogLocation = new ConsoleLogLocation(dummy);
		pLogLocation->SetLevel(_FINEST_);
		Logger::AddLogLocation(pLogLocation);
	}

	//TODO: This is a hack, but I have no choice now
	RTMPMessageFactory::Init();

	INFO("Reading configuration from %s", STR(gRs.commandLine["arguments"]["configFile"]));
#ifdef COMPILE_STATIC
	gRs.pConfigFile = new ConfigFile(SpawnApplication, SpawnFactory);
#else
	gRs.pConfigFile = new ConfigFile(NULL, NULL);
#endif
	string configFilePath = gRs.commandLine["arguments"]["configFile"];
	string fileName;
	string extension;
	splitFileName(configFilePath, fileName, extension);

	if (lowerCase(extension) == "xml") {
		if (!gRs.pConfigFile->LoadXmlFile(configFilePath,
				(bool)gRs.commandLine["arguments"]["--daemon"])) {
			FATAL("Unable to load file %s", STR(configFilePath));
			return false;
		}
	} else if (lowerCase(extension) == "lua") {
#ifdef HAS_LUA
		if (!gRs.pConfigFile->LoadLuaFile(configFilePath, false)) {
			FATAL("Unable to load file %s", STR(configFilePath));
			return false;
		}
#else /* HAS_LUA */
		fprintf(stdout, "Lua is not supported by the current build of the server\n");
		ASSERT("Lua is not supported by the current build of the server");
		return false;
#endif /* HAS_LUA */
	} else {
		FATAL("Invalid file format: %s", STR(configFilePath));
		return false;
	}

	uint32_t currentFdCount = 0;
	uint32_t maxFdCount = 0;
	if ((!setMaxFdCount(currentFdCount, maxFdCount))
			|| (!enableCoreDumps())) {
		WARN("Unable to apply file descriptors count limits and activate core dumps");
	}

	//The master unique lifeId
	Version::_lifeId = format("%"PRIu64"_%"PRIu64"_%s",
			(uint64_t) GetPid(),
			(uint64_t) time(NULL),
			STR(generateRandomString(8))
			);
	Version::_instance = rand() % 10000;
	INFO("Configure Default Event Logger");
	if (!gRs.pConfigFile->ConfigDefaultEventLogger()) {
		WARN("Unable to configure default event logger");
	}

	INFO("Configure logger");
	if (!gRs.pConfigFile->ConfigLogAppenders()) {
		FATAL("Unable to configure log appenders");
		return false;
	}

	INFO("%s", STR(Version::GetBanner()));

	INFO("OS files descriptors count limits: %"PRIu32"/%"PRIu32,
			currentFdCount, maxFdCount);

	INFO("Initialize I/O handlers manager: %s", NETWORK_REACTOR);
	IOHandlerManager::Initialize();

	INFO("Configure modules");
	if (!gRs.pConfigFile->ConfigModules()) {
		FATAL("Unable to configure modules");
		return false;
	}

	INFO("Plug in the default protocol factory");
	gRs.pProtocolFactory = new DefaultProtocolFactory();
	if (!ProtocolFactoryManager::RegisterProtocolFactory(gRs.pProtocolFactory)) {
		FATAL("Unable to register default protocols factory");
		return false;
	}

	INFO("Start I/O handlers manager: %s", NETWORK_REACTOR);
	IOHandlerManager::Start();

	INFO("Configure applications");
	if (!gRs.pConfigFile->ConfigApplications()) {
		FATAL("Unable to configure applications");
		return false;
	}

	INFO("Install the quit signal");
	installQuitSignal(QuitSignalHandler);

	return true;
}

void Run() {
	if (!ApplyUIDGID()) {
		FATAL("Unable to apply user id");
		exit(-1);
	}
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif	
	// At this point do anything else (e.g. launch the processes)
	ClientApplicationManager::SignalServerBootstrapped();

	INFO("\n%s", STR(gRs.pConfigFile->GetServicesInfo()));
	INFO("LIFE: %s", STR(Version::_lifeId));
	INFO("GO! GO! GO! (%u)", (uint32_t) GetPid());
	LogServerStarted();
	while (IOHandlerManager::Pulse()) {
		IOHandlerManager::DeleteDeadHandlers();
		ProtocolManager::CleanupDeadProtocols();
	}
}

void Cleanup() {
	WARN("Shutting down protocols manager");
	ProtocolManager::Shutdown();
	ProtocolManager::CleanupDeadProtocols();

	WARN("Shutting down I/O handlers manager");
	IOHandlerManager::ShutdownIOHandlers();
	IOHandlerManager::DeleteDeadHandlers();				//calling this here for some reason aborts cleanup and results to incomplete logs
	IOHandlerManager::Shutdown();

	WARN("Unregister and delete default protocol handler");
	ProtocolFactoryManager::UnRegisterProtocolFactory(gRs.pProtocolFactory);
	delete gRs.pProtocolFactory;
	gRs.pProtocolFactory = NULL;

	WARN("Shutting down applications");
	ClientApplicationManager::Shutdown();

	WARN("Delete the configuration");
	delete gRs.pConfigFile;
	gRs.pConfigFile = NULL;

	WARN("Doing final OpenSSL cleanup");
	CleanupSSL();

	//TODO: This is a hack, but I have no choice now
	RTMPMessageFactory::Cleanup();

	//IOHandlerManager::DeleteDeadHandlers();

	WARN("Shutting down the logger leaving you in the dark. Bye bye... :(");
	Logger::Free(true);
}

#if defined __HAS_SHUTDOWN_SEQUENCE
#define SHUTDOWN_SEQUENCE \
do { \
	if(gRs.hasLMInterface) { \
		Variant message; \
		message["header"] = "shutdown"; \
		message["payload"] = Variant(); \
		ClientApplicationManager::SendMessageToApplication("lminterface", message); \
	} else { \
		EventLogger::SignalShutdown(); \
	} \
} while (0)
#else /* defined HAS_SHUTDOWN_SEQUENCE && (defined HAS_APP_LMINTERFACE || defined HAS_APP_AXISLICENSEINTERFACE) */
#define SHUTDOWN_SEQUENCE EventLogger::SignalShutdown();
#endif /* defined HAS_SHUTDOWN_SEQUENCE && (defined HAS_APP_LMINTERFACE || defined HAS_APP_AXISLICENSEINTERFACE) */

void QuitSignalHandler(void) {
	WARN("Gracefully shutdown");
	SHUTDOWN_SEQUENCE;
}

void PrintHelp() {
	fprintf(stdout, "Usage: \n%s [OPTIONS] [config_file_path]\n\n", STR(gRs.commandLine["program"]));
	fprintf(stdout, "OPTIONS:\n");
	fprintf(stdout, "    --help\n");
	fprintf(stdout, "      Prints this help and exit\n\n");
	fprintf(stdout, "    --version\n");
	fprintf(stdout, "      Prints the version and exit.\n\n");
	fprintf(stdout, "    --use-implicit-console-appender\n");
	fprintf(stdout, "      Adds a console log appender.\n");
	fprintf(stdout, "      Particularly useful when the server starts and then stops immediately.\n");
	fprintf(stdout, "      Allows you to see if something is wrong with the config file\n\n");
	fprintf(stdout, "    --uid=<uid>\n");
	fprintf(stdout, "      Run the process with the specified user id\n\n");
	fprintf(stdout, "    --gid=<gid>\n");
	fprintf(stdout, "      Run the process with the specified group id\n\n");
}

void PrintVersion() {
	fprintf(stdout, "%s\n", STR(Version::GetBanner()));
	if (Version::GetBuilderOSUname() != "")
		fprintf(stdout, "Compiled on machine: `%s`\n", STR(Version::GetBuilderOSUname()));
}

void NormalizeCommandLine(string configFile) {
	gRs.commandLine["arguments"]["configFile"] = configFile;
	gRs.commandLine["arguments"].RemoveKey(configFile);
	bool tmp = (gRs.commandLine["--help"] != V_NULL);
	gRs.commandLine["--help"] = (bool)tmp;
	tmp = (gRs.commandLine["--version"] != V_NULL);
	gRs.commandLine["--version"] = (bool)tmp;
	tmp = (gRs.commandLine["arguments"]["--use-implicit-console-appender"] != V_NULL);
	gRs.commandLine["arguments"]["--use-implicit-console-appender"] = (bool)tmp;
	tmp = (gRs.commandLine["arguments"]["--daemon"] != V_NULL);
	gRs.commandLine["arguments"]["--daemon"] = (bool)tmp;
	if (gRs.commandLine["arguments"].HasKey("--uid")) {
		gRs.commandLine["arguments"]["--uid"] = (uint32_t) atoi(STR(gRs.commandLine["arguments"]["--uid"]));
	} else {
		gRs.commandLine["arguments"]["--uid"] = (uint32_t) 0;
	}
	if (gRs.commandLine["arguments"].HasKey("--gid")) {
		gRs.commandLine["arguments"]["--gid"] = (uint32_t) atoi(STR(gRs.commandLine["arguments"]["--gid"]));
	} else {
		gRs.commandLine["arguments"]["--gid"] = (uint32_t) 0;
	}
	if (gRs.commandLine["arguments"].HasKey("--ppid")) {
		gRs.commandLine["arguments"]["--ppid"] = (uint32_t)atoi(STR(gRs.commandLine["arguments"]["--ppid"]));
	} else {
		gRs.commandLine["arguments"]["--ppid"] = (uint32_t)0;
	}
}

bool ApplyUIDGID() {
#ifndef WIN32
	FATAL("gid: %u uid: %u", (uint32_t) gRs.commandLine["arguments"]["--gid"],
		(uint32_t) gRs.commandLine["arguments"]["--uid"]);
	if ((uint32_t) gRs.commandLine["arguments"]["--gid"] != 0) {
		if (setgid((uid_t) gRs.commandLine["arguments"]["--gid"]) != 0) {
			FATAL("Unable to set GID");
			return false;
		}
	}
	if ((uint32_t) gRs.commandLine["arguments"]["--uid"] != 0) {
		if (setuid((uid_t) gRs.commandLine["arguments"]["--uid"]) != 0) {
			FATAL("Unable to set UID");
			return false;
		}
	}
#ifdef LINUX
	prctl(PR_SET_DUMPABLE, 1);
#endif
#endif
	return true;
}

void WritePidFile(pid_t pid) {
	/*!
	 * rewrite PID file if it al`y exists
	 */
	string pidFile = gRs.commandLine["arguments"]["--pid"];
	struct stat sb;
	if (stat(STR(pidFile), &sb) == 0) {
		WARN("pid file %s already exists\n", STR(pidFile));
	} else {
		int err = errno;
		if (err != ENOENT) {
#pragma warning ( disable : 4996 )	// allow use of strerror
			WARN("stat: (%d) %s", err, strerror(err));
			return;
		}
	}

	File f;
	if (!f.Initialize(STR(pidFile), FILE_OPEN_MODE_TRUNCATE)) {
		WARN("Unable to open PID file %s", STR(pidFile));
		return;
	}

	string content = format("%"PRIz"d", pid);
	if (!f.WriteString(content)) {
		WARN("Unable to write PID to file %s", STR(pidFile));
		return;
	}
	f.Close();
}

#ifdef COMPILE_STATIC

BaseClientApplication *SpawnApplication(Variant configuration) {
	if (false) {

	}
#ifdef HAS_WEBSERVER
	else if (configuration[CONF_APPLICATION_NAME] == "webserver") {
		configuration["webUilicenseOnly"] = gRs.webUiLicenseOnly;
		configuration["program"] = gRs.commandLine["program"];
		configuration["parentPId"] = gRs.commandLine["arguments"]["--ppid"];
		return GetApplication_WebServer(configuration);	
	}
#endif /* HAS_WEBSERVER */
	return NULL;
}

BaseProtocolFactory *SpawnFactory(Variant configuration) {
	if (false) {

	}
	return NULL;
}
#endif

