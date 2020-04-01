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


#include "configuration/configfile.h"
#include "application/clientapplicationmanager.h"
#include "eventlogger/eventlogger.h"

ConfigFile::ConfigFile(GetApplicationFunction_t staticGetApplicationFunction,
		GetFactoryFunction_t staticGetFactoryFunction) {
	_staticGetApplicationFunction = staticGetApplicationFunction;
	_staticGetFactoryFunction = staticGetFactoryFunction;
	if (((_staticGetApplicationFunction == NULL) && (_staticGetFactoryFunction != NULL))
			|| ((_staticGetApplicationFunction != NULL) && (_staticGetFactoryFunction == NULL))) {
		ASSERT("Invalid config file usage");
	}
	_isOrigin = true;
	_logAppenders.IsArray(true);
}

ConfigFile::~ConfigFile() {

	FOR_MAP(_modules, string, Module, i) {
		MAP_VAL(i).Release();
	}
	_modules.clear();
}

bool ConfigFile::IsDaemon() {
	if (_configuration.HasKeyChain(V_BOOL, true, 1, CONF_DAEMON))
		return (bool)_configuration[CONF_DAEMON];
	return false;
}

bool ConfigFile::IsOrigin() {
	return _isOrigin;
}

string ConfigFile::GetServicesInfo() {
	map<uint32_t, BaseClientApplication *> applications = ClientApplicationManager::GetAllApplications();

	stringstream ss;

	ss << "+-----------------------------------------------------------------------------+" << endl;
	ss << "|";
	ss.width(77);
	ss << "Services";
	ss << "|" << endl;
	ss << "+---+---------------+-----+-------------------------+-------------------------+" << endl;
	ss << "| c |      ip       | port|   protocol stack name   |     application name    |" << endl;

	FOR_MAP(applications, uint32_t, BaseClientApplication *, i) {
		ss << MAP_VAL(i)->GetServicesInfo();
	}

	ss << "+---+---------------+-----+-------------------------+-------------------------+";

	return ss.str();
}

Variant &ConfigFile::GetApplicationsConfigurations() {
	return _applications;
}

bool ConfigFile::LoadLuaFile(string path, bool forceDaemon) {
	if (!ReadLuaFile(path, CONF_CONFIGURATION, _configuration)) {
		FATAL("Unable to read configuration file: %s", STR(path));
		return false;
	}
	if (forceDaemon)
		_configuration[CONF_DAEMON] = (bool)true;
	return Normalize();
}

bool ConfigFile::LoadXmlFile(string path, bool forceDaemon) {
	if (!Variant::DeserializeFromXmlFile(path, _configuration)) {
		FATAL("Unable to read configuration file: %s", STR(path));
		return false;
	}
	if (forceDaemon)
		_configuration[CONF_DAEMON] = (bool)true;
	return Normalize();
}

bool ConfigFile::ConfigLogAppenders() {

	FOR_MAP(_logAppenders, string, Variant, i) {
		if (!ConfigLogAppender(MAP_VAL(i))) {
			FATAL("Unable to configure log appender:\n%s", STR(MAP_VAL(i).ToString()));
			return false;
		}
	}

	return true;
}

bool ConfigFile::ConfigModules() {

	FOR_MAP(_applications, string, Variant, i) {
		if (!ConfigModule(MAP_VAL(i))) {
			FATAL("Unable to configure module:\n%s", STR(MAP_VAL(i).ToString()));
			return false;
		}
	}
	return true;
}

bool ConfigFile::ConfigFactories() {

	FOR_MAP(_modules, string, Module, i) {
		if (!MAP_VAL(i).ConfigFactory()) {
			FATAL("Unable to configure factory");
			return false;
		}
	}
	return true;
}

bool ConfigFile::ConfigAcceptors() {

	FOR_MAP(_modules, string, Module, i) {
		if (!MAP_VAL(i).BindAcceptors()) {
			FATAL("Unable to configure acceptors");
			return false;
		}
	}
	return true;
}

bool ConfigFile::ConfigInstances() {
#ifndef WIN32
	int8_t instancesCount = 0;
	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, "instancesCount")) {
		instancesCount = (int8_t) _configuration.GetValue("instancesCount", false);
	}
	if (instancesCount > 64) {
		FATAL("Invalid number of instances count. Max value is 8");
		return false;
	}

	if (instancesCount == 0) {

		FOR_MAP(_modules, string, Module, i) {
			MAP_VAL(i).config["isOrigin"] = (bool)true;
		}
		return true;
	}

	if (instancesCount < 0) {
		instancesCount = getCPUCount(); 
	}

	if ((instancesCount < 0) || (instancesCount > 64)) {
		FATAL("unable to correctly compute the number of instances");
		return false;
	}

	if (!IsDaemon()) {
		WARN("Daemon mode not activated. No additional instances will be spawned");

		FOR_MAP(_modules, string, Module, i) {
			MAP_VAL(i).config["isOrigin"] = (bool)true;
		}
		return true;
	}

	for (int32_t i = 0; i < instancesCount - 1; i++) { // subtract 1, since the origin should always exist
		pid_t pid = fork();
		if (pid < 0) {
			FATAL("Unable to start child instance. fork() failed");
			return false;
		}

		if (pid == 0) {
			_isOrigin = false;
			Logger::SignalFork();
			break;
		}
	}

	FOR_MAP(_modules, string, Module, i) {
		MAP_VAL(i).config["isOrigin"] = (bool)_isOrigin;
	}

	//dirty fix: need to wait the origin to boot up, so all edges
	//are going to wait 5 seconds
	if (!_isOrigin) {
		sleep(5);
	}

	return true;
#else /* WIN32 */
	WARN("Windows doesn't support multiple instances");

	FOR_MAP(_modules, string, Module, i) {
		MAP_VAL(i).config["isOrigin"] = (bool)true;
	}
	return true;
#endif /* WIN32 */
}

bool ConfigFile::ConfigApplications() {

	FOR_MAP(_modules, string, Module, i) {
		if (!MAP_VAL(i).ConfigApplication()) {
			FATAL("Unable to configure acceptors");
			return false;
		}
	}
	return true;
}

bool ConfigFile::ConfigLogAppender(Variant &node) {
	BaseLogLocation *pLogLocation = NULL;
	if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_COLORED_CONSOLE) {
		node[CONF_LOG_APPENDER_COLORED] = (bool)true;
		if (!IsDaemon()) {
			pLogLocation = new ConsoleLogLocation(node);
		}
	} else if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_CONSOLE) {
		if (!IsDaemon()) {
			pLogLocation = new ConsoleLogLocation(node);
		}
	} else if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_FILE) {
		pLogLocation = new FileLogLocation(node);
	} else if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_DELAYED_FILE) {
		pLogLocation = new DelayedFileLogLocation(node);
#ifdef ANDROID
	} else if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_ANDROIDAPP) {
		pLogLocation = new AndroidAppLogLocation(node);
#endif
#ifdef HAS_REMOTE_LATENCY_LOG
	}
	else if (node[CONF_LOG_APPENDER_TYPE] == "latency logger") {
		RemoteLatencyLogger::SetConfig(node);
#endif
	}
#ifdef HAS_PROTOCOL_API
	else if (node[CONF_LOG_APPENDER_TYPE] == CONF_LOG_APPENDER_TYPE_API_RDK) {
		pLogLocation = new ApiLogLocation(node);
	}
#endif /* HAS_PROTOCOL_API */
	else {
		NYIR;
	}
	if (pLogLocation != NULL) {
		pLogLocation->SetLevel((int32_t) node[CONF_LOG_APPENDER_LEVEL]);
		if (!Logger::AddLogLocation(pLogLocation)) {
			delete pLogLocation;
		}
	}
	return true;
}

bool ConfigFile::Normalize() {
	if (!NormalizeLogAppenders()) {
		FATAL("Unable to normalize log appenders");
		return false;
	}
	if (!NormalizeApplications()) {
		FATAL("Unable to normalize applications");
		return false;
	}
	return true;
}

bool ConfigFile::ConfigModule(Variant &node) {
	Module module;
	module.config = node;
	if (_staticGetApplicationFunction != NULL) {
		module.getApplication = _staticGetApplicationFunction;
		module.getFactory = _staticGetFactoryFunction;
	}

	if (!module.Load()) {
		FATAL("Unable to load module");
		return false;
	}

	_modules[(string) node[CONF_APPLICATION_NAME]] = module;

	return true;
}

bool ConfigFile::NormalizeLogAppenders() {
	if (!_configuration.HasKeyChain(V_MAP, false, 1, CONF_LOG_APPENDERS)) {
		WARN("No log appenders specified");
		return true;
	}
	Variant temp = _configuration.GetValue(CONF_LOG_APPENDERS, false);

	FOR_MAP(temp, string, Variant, i) {
		if (MAP_VAL(i) != V_MAP) {
			WARN("Invalid log appender:\n%s", STR(MAP_VAL(i).ToString()));
			continue;
		}
		if (!NormalizeLogAppender(MAP_VAL(i))) {
			WARN("Invalid log appender:\n%s", STR(MAP_VAL(i).ToString()));
			continue;
		}
		_logAppenders.PushToArray(MAP_VAL(i));
	}
	return true;
}

bool ConfigFile::NormalizeLogAppender(Variant &node) {
	if (!node.HasKeyChain(V_STRING, false, 1, CONF_LOG_APPENDER_NAME)) {
		WARN("Invalid log appender name");
		return false;
	}
	string name = node.GetValue(CONF_LOG_APPENDER_NAME, false);

	if (!node.HasKeyChain(V_STRING, false, 1, CONF_LOG_APPENDER_TYPE)) {
		WARN("Invalid log appender type");
		return false;
	}
	string type = node.GetValue(CONF_LOG_APPENDER_TYPE, false);
	if ((type != CONF_LOG_APPENDER_TYPE_COLORED_CONSOLE)
#ifdef ANDROID
			&& (type != CONF_LOG_APPENDER_TYPE_ANDROIDAPP)
			&& (type != CONF_LOG_APPENDER_TYPE_ANDROIDDEBUG)
#endif
#ifdef HAS_PROTOCOL_API
			&& (type != CONF_LOG_APPENDER_TYPE_API_RDK)
#endif /* HAS_PROTOCOL_API */
			&& (type != CONF_LOG_APPENDER_TYPE_CONSOLE)
			&& (type != CONF_LOG_APPENDER_TYPE_FILE)
			&& (type != CONF_LOG_APPENDER_TYPE_DELAYED_FILE)) {
		WARN("Invalid log appender type");
		return false;
	}

	if (!node.HasKeyChain(_V_NUMERIC, false, 1, CONF_LOG_APPENDER_LEVEL)) {
		WARN("Invalid log appender level");
		return false;
	}
	int8_t level = (int8_t) node.GetValue(CONF_LOG_APPENDER_LEVEL, false);
	if (level < 0) {
		WARN("Invalid log appender level");
		return false;
	}

	node[CONF_LOG_APPENDER_NAME] = name;
	node[CONF_LOG_APPENDER_TYPE] = type;
	node[CONF_LOG_APPENDER_LEVEL] = (uint8_t) level;

	return true;
}

bool ConfigFile::NormalizeApplications() {
	if (!_configuration.HasKeyChain(V_MAP, false, 1, CONF_APPLICATIONS)) {
		WARN("No applications specified");
		return true;
	}
	Variant temp = _configuration.GetValue(CONF_APPLICATIONS, false);

	_rootAppFolder = "";
	if (temp.HasKeyChain(V_STRING, false, 1, CONF_APPLICATIONS_ROOTDIRECTORY))
		_rootAppFolder = (string) temp.GetValue(CONF_APPLICATIONS_ROOTDIRECTORY, false);
	trim(_rootAppFolder);
	if (_rootAppFolder == "")
		_rootAppFolder = ".";
	if (_rootAppFolder[_rootAppFolder.size() - 1] != PATH_SEPARATOR)
		_rootAppFolder += PATH_SEPARATOR;

	_applications.IsArray(true);

	FOR_MAP(temp, string, Variant, i) {
		if (MAP_KEY(i) == CONF_APPLICATIONS_ROOTDIRECTORY)
			continue;
		if (MAP_VAL(i) != V_MAP) {
			FATAL("Invalid application:\n%s", STR(MAP_VAL(i).ToString()));
			return false;
		}
		if (!NormalizeApplication(MAP_VAL(i))) {
			FATAL("Invalid application:\n%s", STR(MAP_VAL(i).ToString()));
			return false;
		}
		_applications.PushToArray(MAP_VAL(i));
	}
	return true;
}

bool ConfigFile::NormalizeApplication(Variant &node) {
	string temp = "";

	if (!node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_NAME)) {
		FATAL("Invalid application name");
		return false;
	}
	string name = node.GetValue(CONF_APPLICATION_NAME, false);
	if (name == "") {
		FATAL("Invalid application name");
		return false;
	}
	if (MAP_HAS1(_uniqueNames, name)) {
		FATAL("Application name %s already taken", STR(name));
		return false;
	}
	_uniqueNames[name] = name;
	node[CONF_APPLICATION_NAME] = name;


	string appDir = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_DIRECTORY))
		appDir = (string) node.GetValue(CONF_APPLICATION_DIRECTORY, false);
	if (appDir == "") {
		appDir = _rootAppFolder + name;
	}
	temp = normalizePath(appDir, "");
	if (temp == "") {
		WARN("Path not found: %s", STR(appDir));
	} else {
		appDir = temp;
	}
	if (appDir[appDir.size() - 1] != PATH_SEPARATOR)
		appDir += PATH_SEPARATOR;
	node[CONF_APPLICATION_DIRECTORY] = appDir;

	string mediaFolder = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_MEDIAFOLDER)) {
		mediaFolder = (string) node.GetValue(CONF_APPLICATION_MEDIAFOLDER, false);

		if (mediaFolder == "") {
			mediaFolder = appDir + "media";
		}
		temp = normalizePath(mediaFolder, "");
		if (temp == "") {
			WARN("Path not found: %s", STR(mediaFolder));
		} else {
			mediaFolder = temp;
		}
		if (mediaFolder[mediaFolder.size() - 1] != PATH_SEPARATOR)
			mediaFolder += PATH_SEPARATOR;
		node[CONF_APPLICATION_MEDIAFOLDER] = mediaFolder;
	}


	string libraryPath = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_LIBRARY))
		libraryPath = (string) node.GetValue(CONF_APPLICATION_LIBRARY, false);
	if (libraryPath == "") {
		libraryPath = appDir + format(LIBRARY_NAME_PATTERN, STR(name));
	}
	temp = normalizePath(libraryPath, "");
	if (temp == "") {
		if ((_staticGetApplicationFunction == NULL)
				|| (_staticGetFactoryFunction == NULL)) {
			FATAL("Library %s not found", STR(libraryPath));
			return false;
		} else {
			libraryPath = temp;
		}
	} else {
		libraryPath = temp;
	}
	node[CONF_APPLICATION_LIBRARY] = libraryPath;

	string initApplicationFunction = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_INIT_APPLICATION_FUNCTION))
		initApplicationFunction = (string) node.GetValue(CONF_APPLICATION_INIT_APPLICATION_FUNCTION, false);
	if (initApplicationFunction == "")
		initApplicationFunction = "GetApplication_" + name;
	node[CONF_APPLICATION_INIT_APPLICATION_FUNCTION] = initApplicationFunction;

	string initFactoryFunction = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_INIT_FACTORY_FUNCTION))
		initFactoryFunction = (string) node.GetValue(CONF_APPLICATION_INIT_FACTORY_FUNCTION, false);
	if (initFactoryFunction == "")
		initFactoryFunction = "GetFactory_" + name;
	node[CONF_APPLICATION_INIT_FACTORY_FUNCTION] = initFactoryFunction;

	bool validateHandshake = true;
	if (node.HasKeyChain(V_BOOL, false, 1, CONF_APPLICATION_VALIDATEHANDSHAKE))
		validateHandshake = (bool) node.GetValue(CONF_APPLICATION_VALIDATEHANDSHAKE, false);
	node[CONF_APPLICATION_VALIDATEHANDSHAKE] = validateHandshake;

	bool defaultApp = false;
	if (node.HasKeyChain(V_BOOL, false, 1, CONF_APPLICATION_DEFAULT))
		defaultApp = (bool) node.GetValue(CONF_APPLICATION_DEFAULT, false);
	node[CONF_APPLICATION_DEFAULT] = defaultApp;

	uint8_t rtcpDetectionInterval = 10;
	if (node.HasKeyChain(_V_NUMERIC, false, 1, CONF_APPLICATION_RTCPDETECTIONINTERVAL))
		rtcpDetectionInterval = (uint8_t) node.GetValue(CONF_APPLICATION_RTCPDETECTIONINTERVAL, false);
	if (rtcpDetectionInterval >= 60)
		rtcpDetectionInterval = 60;
	node[CONF_APPLICATION_RTCPDETECTIONINTERVAL] = rtcpDetectionInterval;

	Variant acceptors;
	acceptors.IsArray(true);
	if (node.HasKeyChain(V_MAP, false, 1, CONF_ACCEPTORS)) {

		FOR_MAP(node[CONF_ACCEPTORS], string, Variant, i) {
			if (MAP_VAL(i) != V_MAP) {
				FATAL("Invalid acceptor:\n%s", STR(MAP_VAL(i).ToString()));
				return false;
			}

			if (!NormalizeApplicationAcceptor(MAP_VAL(i), appDir)) {
				FATAL("Invalid acceptor:\n%s", STR(MAP_VAL(i).ToString()));
				return false;
			}
			acceptors.PushToArray(MAP_VAL(i));
		}
	}
	node[CONF_ACCEPTORS] = acceptors;

	Variant aliases;
	aliases.IsArray(true);
	if (node.HasKeyChain(V_MAP, false, 1, CONF_APPLICATION_ALIASES)) {

		FOR_MAP(node[CONF_APPLICATION_ALIASES], string, Variant, i) {
			if (MAP_VAL(i) != V_STRING) {
				FATAL("Invalid alias value:\n%s", STR(MAP_VAL(i).ToString()));
				return false;
			}
			if (MAP_VAL(i) == "") {
				FATAL("Invalid alias value:\n%s", STR(MAP_VAL(i).ToString()));
				return false;
			}
			if (MAP_HAS1(_uniqueNames, (string) MAP_VAL(i))) {
				FATAL("Alias name %s already taken", STR(MAP_VAL(i)));
				return false;
			}
			_uniqueNames[(string) MAP_VAL(i)] = (string) MAP_VAL(i);
			aliases.PushToArray(MAP_VAL(i));
		}
	}
	node[CONF_APPLICATION_ALIASES] = aliases;

	return true;
}

bool ConfigFile::NormalizeApplicationAcceptor(Variant &node, string baseFolder) {
	string ip = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_IP))
		ip = (string) node.GetValue(CONF_IP, false);
	if (node.HasKeyChain(V_STRING, false, 1, CONF_IP)) {
		if (ip == "") {
			FATAL("Invalid ip: %s", STR(ip));
			return false;
		}
		if (getHostByName(ip) == "") {
			FATAL("Invalid ip: %s", STR(ip));
			return false;
		}
		node[CONF_IP] = ip;
	
		int32_t port = 0;
		if (node.HasKeyChain(_V_NUMERIC, false, 1, CONF_PORT))
			port = (int32_t) node.GetValue(CONF_PORT, false);
		if (port <= 0 || port >= 65536) {
			FATAL("Invalid port: %"PRId32, port);
			return false;
		}
		node[CONF_PORT] = (uint16_t) port;
	} else if (!node.HasKeyChain(V_STRING, false, 1, CONF_PATH)) {
		FATAL("No IP or path given");
		return false;
	}
	string protocol = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_PROTOCOL))
		protocol = (string) node.GetValue(CONF_PROTOCOL, false);
	if (protocol == "") {
		FATAL("Invalid protocol: %s", STR(protocol));
		return false;
	}
	node[CONF_PROTOCOL] = protocol;

	string sslKey = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_SSL_KEY))
		sslKey = (string) node.GetValue(CONF_SSL_KEY, false);
	if (sslKey != "") {
		if (!isAbsolutePath(sslKey)) {
			sslKey = baseFolder + sslKey;
		}
		string temp = normalizePath(sslKey, "");
		if (temp == "") {
			FATAL("SSL key not found: %s", STR(sslKey));
			return false;
		}
		sslKey = temp;
	}
	node[CONF_SSL_KEY] = sslKey;

	string sslCert = "";
	if (node.HasKeyChain(V_STRING, false, 1, CONF_SSL_CERT))
		sslCert = (string) node.GetValue(CONF_SSL_CERT, false);
	if (sslCert != "") {
		if (!isAbsolutePath(sslCert)) {
			sslCert = baseFolder + sslCert;
		}
		string temp = normalizePath(sslCert, "");
		if (temp == "") {
			FATAL("SSL key not found: %s", STR(sslCert));
			return false;
		}
		sslCert = temp;
	}
	node[CONF_SSL_CERT] = sslCert;

	if (((sslKey == "") && (sslCert != "")) || ((sslKey != "") && (sslCert == ""))) {
		FATAL("Invalid ssl key/cert");
		return false;
	}

	return true;
}

bool ConfigFile::NormalizeApplicationAliases(Variant &aliases) {
	NYIR;
}

bool ConfigFile::ConfigDefaultEventLogger() {
	if (_configuration.HasKeyChain(V_MAP, false, 1, "eventLogger")) {
		return EventLogger::InitializeDefaultLogger(
				_configuration.GetValue("eventLogger", false));
	}
	return false;
}

Variant ConfigFile::GetLogAppenders() {
	return _logAppenders;
}

string ConfigFile::GetTimeProbeFilePathPrefix()
{
	if (_configuration.HasKeyChain(V_STRING, false, 1, "timeProbeFilePathPrefix")) {
		string val = static_cast<string>(_configuration.GetValue("timeProbeFilePathPrefix", false));
		return val;
	}
	return string();
}

