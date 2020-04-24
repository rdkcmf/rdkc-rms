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
#include "utils/misc/timeprobemanager.h"
#ifdef ANDROID
//#include "jni.h"
#include "jnibridge.h"
#elif defined IOS
#include "rdkcms.h"
#endif	/* ANDROID/IOS */
#ifdef HAS_PROTOCOL_API
#include "api3rdparty/apiprotocolhandler.h"
#endif	/* HAS_PROTOCOL_API */

#ifdef BREAKPAD
#include "breakpadwrap.h"
#endif

#ifdef HAS_LICENSE
#if defined HAS_APP_LMINTERFACE && defined HAS_APP_AXISLICENSEINTERFACE
#error "Only one licensing app can be defined"
#endif /* defined HAS_APP_LMINTERFACE && defined HAS_APP_AXISLICENSEINTERFACE */
#if (!defined HAS_APP_LMINTERFACE) && (!defined HAS_APP_AXISLICENSEINTERFACE)
#error "A licensing application is required when HAS_LICENSE is defined"
#endif /* (!defined HAS_APP_LMINTERFACE) && (!defined HAS_APP_AXISLICENSEINTERFACE) */
#include "utils/misc/license.h"
#endif /* HAS_LICENSE */

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

#ifdef HAS_WEBSERVER
	pid_t webServerPid;
#endif /*HAS_WEBSERVER*/

	RunningStatus() {
		pConfigFile = NULL;
		pProtocolFactory = NULL;
		run = false;
		daemon = false;
		hasLMInterface = false;
#ifdef HAS_WEBSERVER
		webServerPid = 0;
#endif /*HAS_WEBSERVER*/
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
#ifdef HAS_WEBSERVER
void RunWebServer(bool licenseMode);
#endif /*HAS_WEBSERVER*/

static RunningStatus gRs;

#ifdef COMPILE_STATIC
BaseClientApplication *SpawnApplication(Variant configuration);
BaseProtocolFactory *SpawnFactory(Variant configuration);

#ifdef HAS_LICENSE
#ifdef HAS_APP_AXISLICENSEINTERFACE
#define AXIS_LICENSE_INTERFACE_APP_NAME "licenseDetails"
extern "C" BaseClientApplication *GetApplication_axisinterface(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_axisinterface(Variant configuration);
#endif /* HAS_APP_AXISLICENSEINTERFACE */
#ifdef HAS_APP_LMINTERFACE
#define LM_INTERFACE_APP_NAME "lminterface"
extern "C" BaseClientApplication *GetApplication_lminterface(Variant configuration);
#endif /* HAS_APP_LMINTERFACE */
#else /* HAS_LICENSE */
#undef HAS_APP_AXISLICENSEINTERFACE
#undef HAS_APP_LMINTERFACE
#endif /* HAS_LICENSE */

#ifdef ANDROID
#define PRINTOUT(...) __android_log_print(ANDROID_LOG_INFO, "rdkcms", __VA_ARGS__);
#define PRINTERR(...) __android_log_print(ANDROID_LOG_ERROR, "rdkcms", __VA_ARGS__);
#else
#define PRINTOUT(...) fprintf(stdout, __VA_ARGS__);
#define PRINTERR(...) fprintf(stderr, __VA_ARGS__);
#endif
#endif /* COMPILE_STATIC */

#if (!defined ANDROID) && (!defined IOS)
int main(int argc, const char **argv) {
	//the very first thing we do, we install the crashdump handler
	//InstallCrashDumpHandler(NULL);
#ifdef BREAKPAD
	sleep(1);
	BreakPadWrapExceptionHandler eh;
	eh = newBreakPadWrapExceptionHandler();
	if(NULL != eh) {
		INFO("Breakpad Initialized\n");
	}
#endif
#else
bool FromRDKCMS() {
	return true;
}
int startRDKCMS(int argc, const char **argv) {
#endif
	//1. Pick up the startup parameters and hold them inside the running status
	if (argc < 2) {
		PRINTOUT("Invalid command line. Use --help\n");
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

	if (configFile == "") {
		PRINTERR("Configuration file not found: `%s`\n", argv[argc - 1]);
		return -1;
	}

#ifdef WIN32
	string mutexName = "rmsmediaserver";
	size_t separatorPosition = configFile.find_last_of('\\');
	if (separatorPosition == string::npos) {
		mutexName += "-" + configFile;
	} else {
		mutexName += "-" + configFile.substr(separatorPosition + 1);
	}
	CreateMutex(NULL, TRUE, STR(mutexName));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		printf("\nAn instance of RMS (%s) is already running.\n", STR(mutexName));
		return -1;
	}
#endif	/* WIN32 */

	SRAND();
	InitNetworking();

#ifdef HAS_LICENSE
#ifdef HAS_APP_LMINTERFACE
	vector<string> licenseFilePathCandidates;
#ifdef ANDROID
	ADD_VECTOR_END(licenseFilePathCandidates, AndroidPlatform::GetCacheDirectory() + "rms/config/License.lic");
#endif /* ANDROID */
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
	if (licenseFilePath == "" || !fileExists(licenseFilePath)) {
		fprintf(stderr,
				"No license file found. A license file is required and it can be in one of the \
following locations:\n\n");
		FOR_VECTOR(licenseFilePathCandidates, i) {
			string tempString = licenseFilePathCandidates[i];
			trim(tempString);
			if (tempString != "") {
				PRINTERR("\t%s\n", STR(tempString));
			}
		}
		PRINTERR("\nor it can be manually specified by setting RMS_LICENSE_FILE_PATH environment variable\n\n");
#ifdef HAS_WEBSERVER
		RunWebServer(true);
#endif /*HAS_WEBSERVER*/
		exit(-1);
	}

	License::SetLicenseFile(licenseFilePath);
	License *pLicense = License::GetLicenseInstance();
	LICENSE_VALIDATION retval = pLicense->ValidateLicense();

	if ((retval != VALID) && (retval != FOR_LM_VERIFICATION)) {
		PRINTERR("LICENSE IS INVALID!\n");
		PRINTERR("Fail code: %"PRIu32" : %s\n", (uint32_t) retval,
				STR(pLicense->InterpretValidationCode(retval)));
#ifdef HAS_WEBSERVER		
		RunWebServer(true);
#endif /*HAS_WEBSERVER*/
		exit(-1);
	}

	if (retval == FOR_LM_VERIFICATION)
		gRs.hasLMInterface = true;
#endif /* HAS_APP_LMINTERFACE */
#endif /* HAS_LICENSE */

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

#ifdef ANDROID
		Variant dummy;
		dummy[CONF_LOG_APPENDER_NAME] = "android log appender";
		dummy[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_ANDROIDDEBUG;
		dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
		AndroidLogLocation * pLogLocation = new AndroidLogLocation(dummy);
		pLogLocation->SetLevel(_FINEST_);
		Logger::AddLogLocation(pLogLocation);
#endif
		
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
			WARN("Unable to load file %s", STR(configFilePath));
			return false;
		}
	} else if (lowerCase(extension) == "lua") {
#ifdef HAS_LUA
		if (!gRs.pConfigFile->LoadLuaFile(configFilePath,
				(bool)gRs.commandLine["arguments"]["--daemon"])) {
			WARN("Unable to load file %s", STR(configFilePath));
			return false;
		}
#else /* HAS_LUA */
		PRINTOUT("Lua is not supported by the current build of the server\n");
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

#ifndef WIN32
	if (gRs.pConfigFile->IsDaemon()) {
		if (!gRs.daemon) {
			INFO("Daemonize...");
			setFdCloseOnExec(STDIN_FILENO);
			setFdCloseOnExec(STDOUT_FILENO);
			setFdCloseOnExec(STDERR_FILENO);
			pid_t pid = fork();
			if (pid < 0) {
				FATAL("Unable to start as daemon. fork() failed");
				return false;
			}

			if (pid > 0) {
				if (gRs.commandLine["arguments"].HasKey("--pid"))
					WritePidFile(pid);
				return false;
			}

			FINEST("Create a new SID for the daemon");
			pid_t sid = setsid();
			if (sid < 0) {
				FATAL("Unable to start as daemon. setsid() failed");
				return false;
			}

			gRs.daemon = true;

			Logger::SignalFork();
		}
	}

#ifdef HAS_WEBSERVER
#ifndef ANDROID
	INFO("Instantiating the web server");
	RunWebServer(false);
#endif /*ANDROID*/
#endif /*HAS_WEBSERVER*/
#else /*ifndef WIN32*/
#ifdef HAS_WEBSERVER
	INFO("Instantiating the web server");
	RunWebServer(false);
#endif /*HAS_WEBSERVER*/
#endif /* WIN32 */
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

	PROBE_INIT(gRs.pConfigFile->GetTimeProbeFilePathPrefix());

	//extract the folder name of the logs folder
	Variant logAppenders = gRs.pConfigFile->GetLogAppenders();
	if( logAppenders[1].IsArray() && logAppenders[1].HasKey("fileName", false ) ) {
		string logFileName= (string) logAppenders[1]["fileName"];

		//use / and \ as delimiters
		size_t index1 = logFileName.rfind("/");
		index1 = (index1 == std::string::npos) ? 0 : index1;
		size_t index2 = logFileName.rfind("\\");
		index2 = (index2 == std::string::npos) ? 0 : index2;
		uint32_t index = (uint32_t) (index1 | index2);

		//create log folder if it is missing
		string logFolder = logFileName.substr(0,index);
		if (!fileExists(logFolder)) {
			createFolder(logFolder, true);
		}
	}
			

	INFO("%s", STR(Version::GetBanner()));

	INFO("Reading configuration from %s", STR(gRs.commandLine["arguments"]["configFile"]));

	INFO("OS files descriptors count limits: %"PRIu32"/%"PRIu32,
			currentFdCount, maxFdCount);

	INFO("Initialize I/O handlers manager: %s", NETWORK_REACTOR);
	IOHandlerManager::Initialize();

#ifdef HAS_LICENSE
	Variant &allApps = gRs.pConfigFile->GetApplicationsConfigurations();
#ifdef HAS_APP_AXISLICENSEINTERFACE
	bool hasAxis = false;

	FOR_MAP(allApps, string, Variant, i) {
		Variant app = MAP_VAL(i);
		if (app["name"] == AXIS_LICENSE_INTERFACE_APP_NAME) {
			if (app.HasKeyChain(V_STRING, false, 1, "licenseUser")
					&& app.HasKeyChain(V_STRING, false, 1, "licensePassword")) {
				hasAxis = true;
				break;
			}
		}
	}

	if (!hasAxis) {
		FATAL("License Details missing or incomplete");
		return false;
	}
#endif /* HAS_APP_AXISLICENSEINTERFACE */
#ifdef HAS_APP_LMINTERFACE
	if (gRs.hasLMInterface) {
		License *pLicense = License::GetLicenseInstance();

		//1.  Set starting UID and GID
		pLicense->SetStartingUID();
		pLicense->SetStartingGID();

		//3.  Set running UID and GID.
		pLicense->SetRunningGID();
		pLicense->SetRunningUID();

		Variant lmConfig;
		lmConfig[CONF_APPLICATION_NAME] = LM_INTERFACE_APP_NAME;
		lmConfig[CONF_ACCEPTORS].IsArray(true);
		lmConfig[CONF_APPLICATION_LIBRARY] = "";
		lmConfig["licenseID"] = pLicense->GetLicenseId();
		lmConfig["machineID"] = pLicense->GenerateMachineId();
		//		lmConfig["details"] = pLicense->GenerateExtendedMachineDetails();
		lmConfig["key"] = pLicense->GetRMSKey();
		lmConfig["cert"] = pLicense->GetRMSCertificate();
		lmConfig["lmCert"] = pLicense->GetLMCertificate();
		lmConfig["tolerant"] = pLicense->IsTolerant();
		if (pLicense->GetRuntime() > 0) {
			lmConfig["runtime"] = pLicense->GetRuntime();
		}
		allApps.PushToArray(lmConfig);
	}
#endif /* HAS_APP_LMINTERFACE */
#endif /* HAS_LICENSE */

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

	INFO("Configure factories");
	if (!gRs.pConfigFile->ConfigFactories()) {
		FATAL("Unable to configure factories");
		return false;
	}

	INFO("Configure acceptors");
	if (!gRs.pConfigFile->ConfigAcceptors()) {
		FATAL("Unable to configure acceptors");
		return false;
	}

	INFO("Configure instances");
	if (!gRs.pConfigFile->ConfigInstances()) {
		FATAL("Unable to configure instances");
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

	// At this point do anything else (e.g. launch the processes)
	ClientApplicationManager::SignalServerBootstrapped();

	INFO("\n%s", STR(gRs.pConfigFile->GetServicesInfo()));
	INFO("LIFE: %s", STR(Version::_lifeId));
	INFO("GO! GO! GO! (%u)", (uint32_t) GetPid());
	INFO("RMS started");
	LogServerStarted();

#ifdef ANDROID
	AndroidPlatform::SendInitializedFeedback();
	while (IOHandlerManager::Pulse() && AndroidPlatform::CheckRuntimeStatus()) {
#else
	while (IOHandlerManager::Pulse()) {
#endif	/* ANDROID */
		IOHandlerManager::DeleteDeadHandlers();
		ProtocolManager::CleanupDeadProtocols();
	}
}

void Cleanup() {
#ifdef HAS_LICENSE
#ifdef HAS_APP_LMINTERFACE
	License::ResetLicense();
#endif /* HAS_APP_LMINTERFACE */
#endif /* HAS_LICENSE */
	WARN("Shutting down protocols manager");
	ProtocolManager::Shutdown();
	ProtocolManager::CleanupDeadProtocols();

	WARN("Shutting down I/O handlers manager");
	IOHandlerManager::ShutdownIOHandlers();
	IOHandlerManager::DeleteDeadHandlers();
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

#ifdef ANDROID
	AndroidPlatform::SendShutdownFeedback();
#endif	/* ANDROID */

#ifdef HAS_WEBSERVER
	if (gRs.webServerPid != 0) {
		WARN("Killing the web server");
#ifdef WIN32
		killProcess(gRs.webServerPid);
#else
		kill(gRs.webServerPid, SIGTERM);
#endif
	}
#endif /*HAS_WEBSERVER*/

	WARN("Shutting down the logger leaving you in the dark. Bye bye... :(");
	Logger::Free(true);
}

#ifdef HAS_LICENSE
#define __HAS_SHUTDOWN_SEQUENCE
#endif /* HAS_LICENSE */

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
	PRINTOUT("Usage: \n%s [OPTIONS] [config_file_path]\n\n", STR(gRs.commandLine["program"]));
	PRINTOUT("OPTIONS:\n");
	PRINTOUT("    --help\n");
	PRINTOUT("      Prints this help and exit\n\n");
	PRINTOUT("    --version\n");
	PRINTOUT("      Prints the version and exit.\n\n");
	PRINTOUT("    --use-implicit-console-appender\n");
	PRINTOUT("      Adds a console log appender.\n");
	PRINTOUT("      Particularly useful when the server starts and then stops immediately.\n");
	PRINTOUT("      Allows you to see if something is wrong with the config file\n\n");
	PRINTOUT("    --daemon\n");
	PRINTOUT("      Overrides the daemon setting inside the config file and forces\n");
	PRINTOUT("      the server to start in daemon mode.\n\n");
	PRINTOUT("    --uid=<uid>\n");
	PRINTOUT("      Run the process with the specified user id\n\n");
	PRINTOUT("    --gid=<gid>\n");
	PRINTOUT("      Run the process with the specified group id\n\n");
	PRINTOUT("    --pid=<pid_file>\n");
	PRINTOUT("      Create PID file.\n");
	PRINTOUT("      Works only if --daemon option is specified\n\n");
}

void PrintVersion() {
	PRINTOUT("%s\n", STR(Version::GetBanner()));
	if (Version::GetBuilderOSUname() != "")
		PRINTOUT("Compiled on machine: `%s`\n", STR(Version::GetBuilderOSUname()));
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
}

bool ApplyUIDGID() {
#ifndef WIN32
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

#ifdef HAS_WEBSERVER
void RunWebServer(bool licenseMode) {
	Variant settings;
	string fullBinaryPath = normalizePath(gRs.commandLine["program"], "");
	size_t pos = fullBinaryPath.find_last_of("\\/");
	if (pos == string::npos) {
		FATAL("Unable to launch process: (Invalid binary file path): %s\n", STR(fullBinaryPath));
		return;
	}
#ifdef WIN32
	fullBinaryPath = fullBinaryPath.substr(0, pos + 1) + "rms-webserver.exe"; 
#else
	fullBinaryPath = fullBinaryPath.substr(0, pos + 1) + "rms-webserver"; 
#endif /* WIN32 */
	settings["fullBinaryPath"] = fullBinaryPath;
	settings["arguments"].IsArray(true);
	string configFilePath = gRs.commandLine["arguments"]["configFile"];
	string fileName, extension;
	splitFileName(configFilePath, fileName, extension);
	pos = configFilePath.find_last_of("\\/");
	if (pos == string::npos) {
		FATAL("Unable to launch process: (Invalid config file path): %s\n", STR(configFilePath));
		return;
	}
#ifndef WIN32	
	string webConfig = configFilePath.substr(0, pos + 1) + "webconfig.lua";
	// uid and gid are passed to, and is used by, the web server so it can be killed by rdkcms
	string arguments = format("--uid=%u --gid=%u --ppid=%u %s", 
			(uint32_t)gRs.commandLine["arguments"]["--uid"],
			(uint32_t)gRs.commandLine["arguments"]["--gid"],
			getpid(),
			STR(licenseMode ? "--webuilicenseonly " + webConfig : webConfig));
#else
	string webConfig = "\"" + configFilePath.substr(0, pos + 1) + "webconfig.lua\"";
	string arguments = format("--ppid=%u %s",
		GetCurrentProcessId(),
		STR(licenseMode ? "--webuilicenseonly " + webConfig : webConfig));
#endif  /* WIN32 */
	settings["arguments"] = arguments;
	if (!Process::Launch(settings, gRs.webServerPid))
		FATAL("Unable to launch process:\n%s", STR(settings.ToString()));
}
#endif /*HAS_WEBSERVER*/

#ifdef COMPILE_STATIC

#ifdef HAS_APP_RDKCROUTER
extern "C" BaseClientApplication *GetApplication_rdkcrouter(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_rdkcrouter(Variant configuration);
#endif /* HAS_APP_RDKCROUTER */

#ifdef HAS_APP_HTTPTESTS
extern "C" BaseClientApplication *GetApplication_httptests(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_httptests(Variant configuration);
#endif /* HAS_APP_HTTPTESTS */

#ifdef HAS_APP_LIVERTMPDISSECTOR
extern "C" BaseClientApplication *GetApplication_livertmpdissector(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_livertmpdissector(Variant configuration);
#endif /* HAS_APP_LIVERTMPDISSECTOR */

#ifdef HAS_APP_VMF
extern "C" BaseClientApplication *GetApplication_vmfapp(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_vmfapp(Variant configuration);
#endif /* HAS_APP_VMF */

BaseClientApplication *SpawnApplication(Variant configuration) {
	if (false) {

	}
#ifdef HAS_APP_RDKCROUTER
	else if (configuration[CONF_APPLICATION_NAME] == "rdkcms") {
		return GetApplication_rdkcrouter(configuration);
	}
#endif /* HAS_APP_RDKCROUTER */
#ifdef HAS_APP_HTTPTESTS
	else if (configuration[CONF_APPLICATION_NAME] == "httptests") {
		return GetApplication_httptests(configuration);
	}
#endif /* HAS_APP_HTTPTESTS */
#ifdef HAS_APP_AXISLICENSEINTERFACE
	else if (configuration[CONF_APPLICATION_NAME] == AXIS_LICENSE_INTERFACE_APP_NAME) {
		return GetApplication_axisinterface(configuration);
	}
#endif /* HAS_APP_AXISLICENSEINTERFACE */
#ifdef HAS_APP_LMINTERFACE
	else if (configuration[CONF_APPLICATION_NAME] == LM_INTERFACE_APP_NAME) {
		return GetApplication_lminterface(configuration);
	}
#endif /* HAS_APP_LMINTERFACE */
#ifdef HAS_APP_LIVERTMPDISSECTOR
	else if (configuration[CONF_APPLICATION_NAME] == "livertmpdissector") {
		return GetApplication_livertmpdissector(configuration);
	}
#endif /* HAS_APP_LIVERTMPDISSECTOR */
#ifdef HAS_APP_VMF
	else if (configuration[CONF_APPLICATION_NAME] == "rms_vfm_app") {
		return GetApplication_vmfapp(configuration);
	}
#endif /* HAS_APP_VMF */
	return NULL;
}

BaseProtocolFactory *SpawnFactory(Variant configuration) {
	if (false) {

	}
#ifdef HAS_APP_RDKCROUTER
	else if (configuration[CONF_APPLICATION_NAME] == "rdkcms") {
		return GetFactory_rdkcrouter(configuration);
	}
#endif /* HAS_APP_RDKCROUTER */
#ifdef HAS_APP_HTTPTESTS
	else if (configuration[CONF_APPLICATION_NAME] == "httptests") {
		return GetFactory_httptests(configuration);
	}
#endif /* HAS_APP_HTTPTESTS */
#ifdef HAS_APP_LIVERTMPDISSECTOR
	else if (configuration[CONF_APPLICATION_NAME] == "livertmpdissector") {
		return GetFactory_livertmpdissector(configuration);
	}
#endif /* HAS_APP_LIVERTMPDISSECTOR */
#ifdef HAS_APP_AXISLICENSEINTERFACE
	else if (configuration[CONF_APPLICATION_NAME] == AXIS_LICENSE_INTERFACE_APP_NAME) {
		return GetFactory_axisinterface(configuration);
	}
#endif /* HAS_APP_AXISLICENSEINTERFACE */
#ifdef HAS_APP_VMF
	else if (configuration[CONF_APPLICATION_NAME] == "rms_vfm_app") {
		return GetFactory_vmfapp(configuration);
	}
#endif /* HAS_APP_VMF */
	return NULL;
}
#endif

