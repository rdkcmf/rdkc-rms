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

#ifndef _REMOTELATENCYLOG_H
#define _REMOTELATENCYLOG_H

#ifndef HAS_REMOTE_LATENCY_LOG
#define ENABLE_LATENCY_LOGGING
#define DISABLE_LATENCY_LOGGING
#define LOG_LATENCY(...)
#define LOG_LATENCY_INC(...)
#else
#include "platform/platform.h"

#define SET_LATENCY_LOGGING(state) \
	do { \
		RemoteLatencyLogger *logger = RemoteLatencyLogger::GetInstance(); \
		if (logger != NULL) { \
			logger->SetEnable(state); \
						} \
	} while(0)
#define ENABLE_LATENCY_LOGGING SET_LATENCY_LOGGING(true)
#define DISABLE_LATENCY_LOGGING SET_LATENCY_LOGGING(false)
#define LOG_LATENCY(...) \
	do { \
		RemoteLatencyLogger *logger = RemoteLatencyLogger::GetInstance(); \
		if (logger != NULL) { \
			logger->SendLog(__VA_ARGS__); \
				} \
		} while (0)
#define LOG_LATENCY_INC(...) \
	do { \
		RemoteLatencyLogger *logger = RemoteLatencyLogger::GetInstance(); \
		if (logger != NULL) { \
			logger->SendLog(true, __VA_ARGS__); \
		} \
	} while (0)
class RemoteLatencyLogger {
private:
	int _fd;
	bool _enabled;
	uint32_t _counter;
	uint32_t _maxCounter;
	struct sockaddr_in _dest;
	static RemoteLatencyLogger *_pInstance;
	static Variant _config;
	RemoteLatencyLogger();
	bool Initialize();
	void SendLogV(const char *pFormatString, va_list args);

public:
	~RemoteLatencyLogger();
	static RemoteLatencyLogger *GetInstance();
	static void SetConfig(Variant &config);
	void SendLog(const char *pFormatString, ...);
	void SendLog(bool increment, const char *pFormatString, ...);
	void SetEnable(bool state);
};

#endif /* HAS_REMOTE_LATENCY_LOG */
#endif /* _REMOTELATENCYLOG_H */
