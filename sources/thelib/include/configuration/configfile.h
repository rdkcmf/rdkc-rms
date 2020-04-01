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



#ifndef _CONFIGFILE_H
#define	_CONFIGFILE_H

#include "common.h"
#include "configuration/module.h"

class DLLEXP ConfigFile {
private:
	Variant _configuration;
	Variant _logAppenders;
	string _rootAppFolder;
	Variant _applications;
	map<string, string> _uniqueNames;
	GetApplicationFunction_t _staticGetApplicationFunction;
	GetFactoryFunction_t _staticGetFactoryFunction;
	map<string, Module> _modules;
	bool _isOrigin;
public:
	ConfigFile(GetApplicationFunction_t staticGetApplicationFunction,
			GetFactoryFunction_t staticGetFactoryFunction);
	virtual ~ConfigFile();

	bool IsDaemon();
	bool IsOrigin();
	string GetServicesInfo();
	Variant &GetApplicationsConfigurations();

	bool LoadLuaFile(string path, bool forceDaemon);
	bool LoadXmlFile(string path, bool forceDaemon);

	bool ConfigLogAppenders();
	bool ConfigModules();
	bool ConfigFactories();
	bool ConfigAcceptors();
	bool ConfigInstances();
	bool ConfigApplications();
	bool ConfigDefaultEventLogger();

	Variant GetLogAppenders();
	string GetTimeProbeFilePathPrefix();

private:
	bool ConfigLogAppender(Variant &node);
	bool ConfigModule(Variant &node);

	bool Normalize();
	bool NormalizeLogAppenders();
	bool NormalizeLogAppender(Variant &node);
	bool NormalizeApplications();
	bool NormalizeApplication(Variant &node);
	bool NormalizeApplicationAcceptor(Variant &node, string baseFolder);
	bool NormalizeApplicationAliases(Variant &node);
};


#endif	/* _CONFIGFILE_H */
