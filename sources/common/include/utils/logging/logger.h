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



#ifndef _LOGGER_H
#define _LOGGER_H

#include "common.h"
#ifdef HAS_SAFE_LOGGER
#include <pthread.h>
#endif /* HAS_SAFE_LOGGER */

class BaseLogLocation;

class DLLEXP Version {
public:
	static string _lifeId;
	static uint16_t _instance;
	static uint32_t GetBindId();
	static bool HasBindId();
	static string GetBuildNumber();
	static string GetHash();
	static string GetBranchName();
	static uint64_t GetBuildDate();
	static string GetBuildDateString();
	static string GetReleaseNumber();
	static string GetCodeName();
	static string GetBuilderOSName();
	static string GetBuilderOSVersion();
	static string GetBuilderOSArch();
	static string GetBuilderOSUname();
	static string GetBuilderOS();
	static string GetBanner();
	static Variant GetAll();
	static Variant GetBuilder();
};

class DLLEXP Logger {
private:
	static Logger *_pLogger; //! Pointer to the Logger class.
	vector<BaseLogLocation *> _logLocations; //! Vector that stores the location of the log file.
	bool _freeAppenders; //! Boolean that releases the logger.
#ifdef HAS_SAFE_LOGGER
public:
	static pthread_mutex_t *_pMutex;
#endif
public:
	Logger();
	virtual ~Logger();

	static void Init();
	static void Free(bool freeAppenders);
	static void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, const char *pFormatString, ...);
	static bool AddLogLocation(BaseLogLocation *pLogLocation);
	static void SignalFork();
	static void SetLevel(int32_t level);
};

#endif /* _LOGGER_H */

