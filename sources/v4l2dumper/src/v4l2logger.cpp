/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/
#include "v4l2logger.h"

bool V4L2Logger::_initialized = false;
FILE *V4L2Logger::_pFile = NULL;

string V4L2Logger::GetTime() {
	time_t now = time(NULL);
	struct tm tm = *localtime(&now);
	char buff[80];
	memset(buff, 0, 80);
	strftime(buff, sizeof(buff), "%Y%m%d%H%M%S", &tm);
	return string(buff);
}

bool V4L2Logger::Initialize() {
	if (_initialized)
		return true;
	string ts = GetTime();
//	char filename[80];
//	memset(filename, 0, 80);
//	strcpy(filename, "v4l2_dump");
//	strcat(filename, "_");
//	strcat(filename, ts);
	string filename = "v4l2_dump_" + ts + ".txt";
	_pFile = fopen(STR(filename), "w");
	if (_pFile == NULL)
		return false;
	_initialized = true;
	return true;
}

void V4L2Logger::StampTime() {
	if (!_initialized)
		Initialize();
}

void V4L2Logger::Log(int flag, const char *pFormatString, ...) {
	if (!_initialized && !Initialize())
		return;
	string ts = GetTime();
	if ((flag & LOGFILE) == LOGFILE)
		fprintf(_pFile, "[%s]\n", STR(ts));
	if ((flag & SCREEN) == SCREEN)
		printf("[%s]\n", STR(ts));
	va_list arguments;
	va_start(arguments, pFormatString);
	if ((flag & LOGFILE) == LOGFILE) {
		vfprintf(_pFile, pFormatString, arguments);
		fprintf(_pFile, "\n");
	}
	if ((flag & SCREEN) == SCREEN) {
		vprintf(pFormatString, arguments);
		printf("\n");
	}
}
