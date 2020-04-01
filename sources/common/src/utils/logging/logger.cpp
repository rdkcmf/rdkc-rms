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


#include "utils/logging/logger.h"
#include "utils/logging/baseloglocation.h"
#include "version.h"

Logger *Logger::_pLogger = NULL;
string Version::_lifeId;
uint16_t Version::_instance;

bool Version::HasBindId() {
#ifdef RMS_BIND_ID
	return true;
#else
	return false;
#endif /* RMS_BIND_ID */
}

uint32_t Version::GetBindId() {
#ifdef RMS_BIND_ID
	return RMS_BIND_ID;
#else
	return 0;
#endif /* RMS_BIND_ID */
}

string Version::GetBuildNumber() {
#ifdef RMS_VERSION_BUILD_NUMBER
	return RMS_VERSION_BUILD_NUMBER;
#else /* RMS_VERSION_BUILD_NUMBER */
	return "";
#endif /* RMS_VERSION_BUILD_NUMBER */
}

string Version::GetHash() {
#ifdef RMS_VERSION_HASH
	return RMS_VERSION_HASH;
#else /* RMS_VERSION_HASH */
	return "";
#endif /* RMS_VERSION_HASH */
}

string Version::GetBranchName() {
#ifdef RMS_BRANCH_NAME
	return RMS_BRANCH_NAME;
#else /* RMS_BRANCH_NAME */
	return "";
#endif /* RMS_BRANCH_NAME */
}

uint64_t Version::GetBuildDate() {
#ifdef RMS_VERSION_BUILD_DATE
	return RMS_VERSION_BUILD_DATE;
#else /* RMS_VERSION_BUILD_DATE */
	return 0;
#endif /* RMS_VERSION_BUILD_DATE */
}

string Version::GetBuildDateString() {
	time_t buildDate = (time_t) GetBuildDate();
	if (buildDate == 0) {
		return "";
	}
	Timestamp *pTs = gmtime(&buildDate);
	Variant v(*pTs);
	return (string) v;
}

string Version::GetReleaseNumber() {
#ifdef RMS_VERSION_RELEASE_NUMBER
	return RMS_VERSION_RELEASE_NUMBER;
#else /* RMS_VERSION_RELEASE_NUMBER */
	return "";
#endif /* RMS_VERSION_RELEASE_NUMBER */
}

string Version::GetCodeName() {
#ifdef RMS_VERSION_CODE_NAME
	return RMS_VERSION_CODE_NAME;
#else /* RMS_VERSION_CODE_NAME */
	return "";
#endif /* RMS_VERSION_CODE_NAME */
}

string Version::GetBuilderOSName() {
#ifdef RMS_VERSION_BUILDER_OS_NAME
	return RMS_VERSION_BUILDER_OS_NAME;
#else /* RMS_VERSION_BUILDER_OS_NAME */
	return "";
#endif /* RMS_VERSION_BUILDER_OS_NAME */
}

string Version::GetBuilderOSVersion() {
#ifdef RMS_VERSION_BUILDER_OS_VERSION
	return RMS_VERSION_BUILDER_OS_VERSION;
#else /* RMS_VERSION_BUILDER_OS_VERSION */
	return "";
#endif /* RMS_VERSION_BUILDER_OS_VERSION */
}

string Version::GetBuilderOSArch() {
#ifdef RMS_VERSION_BUILDER_OS_ARCH
	return RMS_VERSION_BUILDER_OS_ARCH;
#else /* RMS_VERSION_BUILDER_OS_ARCH */
	return "";
#endif /* RMS_VERSION_BUILDER_OS_ARCH */
}

string Version::GetBuilderOSUname() {
#ifdef RMS_VERSION_BUILDER_OS_UNAME
	return RMS_VERSION_BUILDER_OS_UNAME;
#else /* RMS_VERSION_BUILDER_OS_UNAME */
	return "";
#endif /* RMS_VERSION_BUILDER_OS_UNAME */
}

string Version::GetBuilderOS() {
	if (GetBuilderOSName() == "")
		return "";
	string result = GetBuilderOSName();
	if (GetBuilderOSVersion() != "") {
		result += "-" + GetBuilderOSVersion();
	}
	if (GetBuilderOSArch() != "") {
		result += "-" + GetBuilderOSArch();
	}
	return result;
}

string Version::GetBanner() {
	string result = HTTP_HEADERS_SERVER_US;
	if (GetReleaseNumber() != "")
		result += " version " + GetReleaseNumber();
	if (GetBuildNumber()!= "")
		result += " build " + GetBuildNumber();
	if (GetHash() != "")
		result += " with hash: " + GetHash();
	if (GetBranchName() != "")
		result += " on branch: " + GetBranchName();
	if (GetCodeName() != "")
		result += " - " + GetCodeName();
	if (GetBuilderOS() != "") {
		result += " - (built for " + GetBuilderOS() + " on " + GetBuildDateString() + ")";
	} else {
		result += " - (built on " + GetBuildDateString() + ")";
	}
	return result;
}

Variant Version::GetAll() {
	Variant result;
	result["buildNumber"] = (string) GetBuildNumber();
	result["hash"] = (string) GetHash();
	result["branchName"] = (string) GetBranchName();
	result["buildDate"] = GetBuildDateString();
	result["releaseNumber"] = (string) GetReleaseNumber();
	result["codeName"] = (string) GetCodeName();
	result["banner"] = (string) GetBanner();
	return result;
}

Variant Version::GetBuilder() {
	Variant result;
	result["name"] = (string) GetBuilderOSName();
	result["version"] = (string) GetBuilderOSVersion();
	result["arch"] = (string) GetBuilderOSArch();
	result["uname"] = (string) GetBuilderOSUname();
	return result;
}

#ifdef HAS_SAFE_LOGGER
pthread_mutex_t *Logger::_pMutex = NULL;

class LogLocker {
private:
	pthread_mutex_t *_pMutex;
public:

	LogLocker(pthread_mutex_t *pMutex) {
		if (pMutex == NULL) {
			printf("Logger not initialized\n");
			o_assert(false);
		}
		_pMutex = pMutex;
		if (pthread_mutex_lock(_pMutex) != 0) {
			printf("Unable to lock the logger");
			o_assert(false);
		}
	};

	virtual ~LogLocker() {
		if (pthread_mutex_unlock(_pMutex) != 0) {
			printf("Unable to unlock the logger");
			o_assert(false);
		}
	}
};
#define LOCK LogLocker __LogLocker__(Logger::_pMutex);
#else
#define LOCK
#endif /* HAS_SAFE_LOGGER */

Logger::Logger() {
	LOCK;
	_freeAppenders = false;
}

Logger::~Logger() {
	LOCK;
	if (_freeAppenders) {

		FOR_VECTOR(_logLocations, i) {
			delete _logLocations[i];
		}
		_logLocations.clear();
	}
}

void Logger::Init() {
#ifdef HAS_SAFE_LOGGER
	if (_pMutex != NULL) {
		printf("logger already initialized");
		o_assert(false);
	}
	_pMutex = new pthread_mutex_t;
	if (pthread_mutex_init(_pMutex, NULL)) {
		printf("Unable to init the logger mutex");
		o_assert(false);
	}
#else
	if (_pLogger != NULL)
		return;
#endif /* HAS_SAFE_LOGGER */
	_pLogger = new Logger();
}

void Logger::Free(bool freeAppenders) {
	LOCK;
	if (_pLogger != NULL) {
		_pLogger->_freeAppenders = freeAppenders;
		delete _pLogger;
		_pLogger = NULL;
	}
}

void Logger::Log(int32_t level, const char *pFileName, uint32_t lineNumber,
		const char *pFunctionName, const char *pFormatString, ...) {
	LOCK;
	if (_pLogger == NULL)
		return;

	va_list arguments;
	va_start(arguments, pFormatString);
	string message = vFormat(pFormatString, arguments);
	va_end(arguments);

	FOR_VECTOR(_pLogger->_logLocations, i) {
		if (_pLogger->_logLocations[i]->EvalLogLevel(level, pFileName, lineNumber,
				pFunctionName))
			_pLogger->_logLocations[i]->Log(level, pFileName,
				lineNumber, pFunctionName, message);
	}
}

bool Logger::AddLogLocation(BaseLogLocation *pLogLocation) {
	LOCK;
	if (_pLogger == NULL)
		return false;
	if (!pLogLocation->Init())
		return false;
	ADD_VECTOR_END(_pLogger->_logLocations, pLogLocation);
	return true;
}

void Logger::SignalFork() {
	LOCK;
	if (_pLogger == NULL)
		return;

	FOR_VECTOR(_pLogger->_logLocations, i) {
		_pLogger->_logLocations[i]->SignalFork();
	}
}

void Logger::SetLevel(int32_t level) {
	LOCK;
	if (_pLogger == NULL)
		return;

	FOR_VECTOR(_pLogger->_logLocations, i) {
		_pLogger->_logLocations[i]->SetLevel(level);
	}
}
