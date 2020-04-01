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



#ifndef _DELAYFILELOGLOCATION_H
#define	_DELAYFILELOGLOCATION_H

#include "utils/logging/fileloglocation.h"

class File;

class DLLEXP DelayedFileLogLocation
: public FileLogLocation {
private:
	vector<string> _lines;
#ifdef WIN32
#define MUTEX HANDLE
#define THREAD HANDLE
#else
#define MUTEX pthread_mutex_t
#define THREAD pthread_t
#endif
	MUTEX _mtx;
	THREAD _writerThread;
	bool _exitWriter;
public:
	DelayedFileLogLocation(Variant &configuration);
	virtual ~DelayedFileLogLocation();

	virtual bool Init();
	virtual void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, string &message);
private:
	void Log(string const &logEntry);
#ifdef WIN32
	static DWORD WINAPI Writer(LPVOID param);
#else
	static void *Writer(void *param);
#endif
	void ExitWriter();
};


#endif	/* _FILELOGLOCATION_H */

