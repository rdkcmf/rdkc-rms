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


#ifdef HAS_REMOTE_LATENCY_LOG
#include "utils/misc/remotelatencylogger.h"
#define LOG_PORT 8888
#define MAX_LOG_LIMIT 50

RemoteLatencyLogger *RemoteLatencyLogger::_pInstance = NULL;
Variant RemoteLatencyLogger::_config;

RemoteLatencyLogger::RemoteLatencyLogger() {
	_fd = -1;
	memset(&_dest, 0, sizeof(_dest));
	_enabled = false;
	_counter = 0;
	_maxCounter = MAX_LOG_LIMIT;
}
RemoteLatencyLogger::~RemoteLatencyLogger() {
	if (_fd > 0)
		close(_fd);
}

void RemoteLatencyLogger::SetConfig(Variant &config) {
	_config = config;
}

bool RemoteLatencyLogger::Initialize() {
	_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (_fd < 0) {
		FATAL("Cannot create socket for logging");
		return false;
	}
	string addr = (_config.HasKeyChain(V_STRING, false, 1, "ip")) ? (string)_config["ip"] : "192.168.2.134";
	uint16_t port = (_config.HasKeyChain(_V_NUMERIC, false, 1, "port")) ? (uint32_t)_config["port"] : LOG_PORT;
	_dest.sin_family = AF_INET;
	_dest.sin_port = EHTONS(port);
	_dest.sin_addr.s_addr = inet_addr(addr.c_str());
	if (_config.HasKeyChain(_V_NUMERIC, false, 1, "maxCount")) {
		_maxCounter = (uint32_t)_config["maxCount"];
	}
	INFO("[LATENCY_LOG] Created remote latency logging instance. ip=%s, port=%"PRIu16, STR(addr), port);
	return true;
}

RemoteLatencyLogger * RemoteLatencyLogger::GetInstance() {
	if (_pInstance == NULL) {
		_pInstance = new RemoteLatencyLogger();
		if (!_pInstance->Initialize()) {
			delete _pInstance;
			_pInstance = NULL;
		}
	}
	return _pInstance;
}

void RemoteLatencyLogger::SendLogV(const char *pFormatString, va_list args) {
	string message = vFormat(pFormatString, args);

	// append '\n' if there is none.
	if (message == "" || message[message.size()] != '\n') {
		message += "\n";
	}

	if (sendto(_fd, STR(message), message.size(), 0, (sockaddr *)&_dest, sizeof(_dest)) == -1) {
		FATAL("Cannot send message to remote logger");
	}
	INFO("[LATENCY_LOG] %s", STR(message));
}

void RemoteLatencyLogger::SendLog(const char *pFormatString, ...) {
	if (!_enabled)
		return;

	va_list arguments;
	va_start(arguments, pFormatString);
	SendLogV(pFormatString, arguments);
	va_end(arguments);
}

void RemoteLatencyLogger::SendLog(bool increment, const char *pFormatString, ...) {
	if (!_enabled)
		return;

	INFO("[LATENCY_LOG] Countdown: %"PRIu32"/%"PRIu32, _counter, _maxCounter);
	va_list arguments;
	va_start(arguments, pFormatString);
	SendLogV(pFormatString, arguments);
	va_end(arguments);

	if (_counter++ >= _maxCounter)
		SetEnable(false);
}
void RemoteLatencyLogger::SetEnable(bool state) {
	_enabled = state;
	INFO("[LATENCY_LOG] %s remote latency logging", _enabled ? "Enabled" : "Disabling");
}
#endif /* HAS_REMOTE_LATENCY_LOG */
