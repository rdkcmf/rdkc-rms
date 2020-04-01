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



#ifndef _FILELOGLOCATION_H
#define	_FILELOGLOCATION_H

#include "utils/logging/baseloglocation.h"

class File;

class DLLEXP FileLogLocation
: public BaseLogLocation {
private:
	bool _canLog;
	uint32_t _counter;
	string _fileName;
	uint32_t _fileHistorySize;
	vector<string> _history;
protected:
	File *_fileStream;
	bool _fileIsClosed;
	string _newLineCharacters;
	uint32_t _fileLength;
	uint32_t _currentLength;
public:
	FileLogLocation(Variant &configuration);
	virtual ~FileLogLocation();

	virtual bool Init();
	virtual bool EvalLogLevel(int32_t level, const char *pFileName,
			uint32_t lineNumber, const char *pFunctionName);
	virtual void Log(int32_t level, const char *pFileName, uint32_t lineNumber,
			const char *pFunctionName, string &message);
	virtual void SignalFork();
protected:
	/* bugfix 1874:
	 * inserts all log files into the vector during initialization so that the
	 * fileHistorySize rule can be applied during start up
	*/
	void InitLogFiles();
	bool OpenFile();
	void CloseFile();
};


#endif	/* _FILELOGLOCATION_H */

