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

#include "common.h"
#include "utils/logging/delayedfileloglocation.h"

//#define DEBUG_LINES
#define DEBUG_LINES_SLEEP

DelayedFileLogLocation::DelayedFileLogLocation(Variant &configuration)
: FileLogLocation(configuration), _exitWriter(false) {
#ifdef WIN32
	_mtx = CreateMutex(NULL, FALSE, NULL);
	DWORD threadId;
	_writerThread = CreateThread(NULL, 0, &DelayedFileLogLocation::Writer, 
		this, 0, &threadId);
#else
	pthread_mutex_init(&_mtx, NULL);
	pthread_create(&_writerThread, NULL, &DelayedFileLogLocation::Writer, 
		(void *)this);
#endif	
}

DelayedFileLogLocation::~DelayedFileLogLocation() {
	ExitWriter();
#ifdef WIN32
	WaitForSingleObject(_writerThread, INFINITE); //wait for thread to finish
	CloseHandle(_writerThread);
	CloseHandle(_mtx);
#else
	pthread_exit(NULL);		//wait for thread to finish
	pthread_mutex_destroy(&_mtx);
#endif
	CloseFile();
}

bool DelayedFileLogLocation::Init() {
	if (!FileLogLocation::Init()) {
		return false;
	}
	return true;
}

void DelayedFileLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	string logEntry = format("%"PRIu64":%d:%s:%u:%s:%s",
			(uint64_t) time(NULL), level, pFileName, lineNumber, pFunctionName,
			STR(message));
	if (_singleLine) {
		replace(logEntry, "\r", "\\r");
		replace(logEntry, "\n", "\\n");
	}
	logEntry += _newLineCharacters;

	Log(logEntry);
}

void DelayedFileLogLocation::Log(string const &logEntry) {
#ifdef WIN32	
	WaitForSingleObject(_mtx, INFINITE);
#else
	pthread_mutex_lock(&_mtx);
#endif
	_lines.push_back(logEntry);
#ifdef WIN32	
	ReleaseMutex(_mtx);
#else
	pthread_mutex_unlock(&_mtx);
#endif
}

#ifdef WIN32
DWORD DelayedFileLogLocation::Writer(LPVOID param) {
#else
void *DelayedFileLogLocation::Writer(void *param) {	
#endif
	DelayedFileLogLocation *dfll = (DelayedFileLogLocation *)param;
	while(1) {
#ifdef WIN32			
		WaitForSingleObject(dfll->_mtx, INFINITE);
#else
		pthread_mutex_lock(&dfll->_mtx);
#endif
		if (dfll->_exitWriter) {			//are we about to exit?
			break;
		}
		queue<string> tempLines;
		if (!dfll->_lines.empty()) {
			tempLines = queue<string>(std::deque<string>(dfll->_lines.begin(),
				dfll->_lines.end()));
			dfll->_lines.clear();
			if (dfll->_fileStream == NULL) {
				dfll->OpenFile();
			}
		}
#ifdef WIN32
		ReleaseMutex(dfll->_mtx);				//release ownership asap
#else
		pthread_mutex_unlock(&dfll->_mtx);		//release ownership asap	
#endif
#ifdef DEBUG_LINES
		if (dfll->_fileStream) {
			//uncomment to see the number of lines queued
			string s = format("lines=[%d]", tempLines.size());
			dfll->_fileStream->WriteString(s);
		}
#endif			
		while(dfll->_fileStream && !tempLines.empty()) {
			string logEntry = tempLines.front();
			tempLines.pop();
			dfll->_fileStream->WriteString(logEntry);
			if (dfll->_fileLength > 0) {
				dfll->_currentLength += (uint32_t) logEntry.length();
				if (dfll->_fileLength < dfll->_currentLength) {
					//calls CloseFile(), which in turn flushes the file
					dfll->OpenFile();		
				}
			}
		}
		if (dfll->_fileStream) {
			dfll->_fileStream->Flush();
		}
#ifdef DEBUG_LINES_SLEEP
		//just for testing delays
#ifdef WIN32						
		Sleep(1000);	
#else
		sleep(1);
#endif	//WIN32
#endif	//DEBUG_LINES_SLEEP			
	}
#ifdef WIN32
	return 1;
#else
	pthread_exit(NULL);
	return NULL;
#endif			
}

void DelayedFileLogLocation::ExitWriter() {
	_exitWriter = true;
}
