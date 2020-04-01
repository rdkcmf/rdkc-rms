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

FileLogLocation::FileLogLocation(Variant &configuration)
: BaseLogLocation(configuration) {
	_fileStream = NULL;
	_canLog = false;
	_counter = 0;
	_newLineCharacters = "\n";
	_fileHistorySize = 0;
	_fileLength = 0;
	_currentLength = 0;
	_fileIsClosed = true;
}

FileLogLocation::~FileLogLocation() {
	CloseFile();
}

bool FileLogLocation::Init() {
	if (!BaseLogLocation::Init())
		return false;
	if (!_configuration.HasKeyChain(V_STRING, false, 1,
			CONF_LOG_APPENDER_FILE_NAME))
		return false;
	_fileName = (string) _configuration.GetValue(
			CONF_LOG_APPENDER_FILE_NAME, false);
	if (_configuration.HasKeyChain(V_STRING, false, 1,
			CONF_LOG_APPENDER_NEW_LINE_CHARACTERS))
		_newLineCharacters = (string) _configuration.GetValue(
			CONF_LOG_APPENDER_NEW_LINE_CHARACTERS, false);
	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1,
			CONF_LOG_APPENDER_FILE_HISTORY_SIZE))
		_fileHistorySize = (uint32_t) _configuration.GetValue(
			CONF_LOG_APPENDER_FILE_HISTORY_SIZE, false);
	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1,
			CONF_LOG_APPENDER_FILE_LENGTH))
		_fileLength = (uint32_t) _configuration.GetValue(
			CONF_LOG_APPENDER_FILE_LENGTH, false);

	InitLogFiles();

	if (!OpenFile())
		return false;
	return true;
}

bool FileLogLocation::EvalLogLevel(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName) {
	if (!_canLog)
		return false;
	return BaseLogLocation::EvalLogLevel(level, pFileName, lineNumber, pFunctionName);
}

void FileLogLocation::Log(int32_t level, const char *pFileName,
		uint32_t lineNumber, const char *pFunctionName, string &message) {
	if (_fileIsClosed) {
		OpenFile();
		if (_fileIsClosed)
			return;
	}
	string logEntry = format("%"PRIu64":%d:%s:%u:%s:%s",
			(uint64_t) time(NULL), level, pFileName, lineNumber, pFunctionName,
			STR(message));
	if (_singleLine) {
		replace(logEntry, "\r", "\\r");
		replace(logEntry, "\n", "\\n");
	}
	logEntry += _newLineCharacters;
	_fileStream->WriteString(logEntry);
	_fileStream->Flush();
	if (_fileLength > 0) {
		_currentLength += (uint32_t) logEntry.length();
		if (_fileLength < _currentLength)
			OpenFile();
	}
}

void FileLogLocation::SignalFork() {
	_fileIsClosed = true;
	_history.clear();
}

struct sortbydate_oldestfirst{
	bool operator()(uint64_t a, uint64_t b) const {
		return (a < b);
	}
};

void FileLogLocation::InitLogFiles() {
	string logFolder = _fileName.substr(0, _fileName.find_last_of(PATH_SEPARATOR));
	logFolder += PATH_SEPARATOR;
	vector<string> logFiles;
	listFolder(logFolder, logFiles,true,false,false);
	map<uint64_t, string, sortbydate_oldestfirst> sortedLogFiles;

	FOR_VECTOR(logFiles, i) {
		size_t pos1 = 0, pos2 = 0;
		if ((pos1 = logFiles[i].find(".log")) != string::npos){
			if((pos2 = logFiles[i].rfind(".", pos1-1))){
				string timestr = logFiles[i].substr(pos2 + 1, pos1 - pos2 - 1);
				istringstream conv(timestr);
				uint64_t timeval = 0;
				conv >> timeval;
				sortedLogFiles.insert(pair<uint64_t, string>(timeval, logFiles[i]));
			}
		}
	}

	FOR_MAP(sortedLogFiles, uint64_t, string, i){
		if (_fileHistorySize > 0) {
			ADD_VECTOR_END(_history, MAP_VAL(i));
		}
	}

	while (_history.size() > _fileHistorySize) {
		deleteFile(_history[0]);
		_history.erase(_history.begin());
	}
}

bool FileLogLocation::OpenFile() {
	CloseFile();
#ifdef WIN32
	uint8_t pathSeparator = '\\';
#else
	uint8_t pathSeparator = '/';
#endif
	string logFolder = _fileName.substr(0, _fileName.find_last_of(pathSeparator));
	if (!fileExists(logFolder)) {
		createFolder(logFolder, true);
	}
	double ts;
	GETCLOCKS(ts, double);
	ts = (ts / CLOCKS_PER_SECOND) * 1000;
	string filename = format("%s.%"PRIu64".%"PRIu64".log", STR(_fileName), (uint64_t) GetPid(), (uint64_t) ts);
	_fileStream = new File();
	if (!_fileStream->Initialize(filename, FILE_OPEN_MODE_TRUNCATE)) {
		return false;
	}
	string header = format("PID: %"PRIu64"; TIMESTAMP: %"PRIz"u; LIFE: %s%s%s%s",
			(uint64_t) GetPid(),
			time(NULL),
			STR(Version::_lifeId),
			STR(_newLineCharacters),
			STR(Version::GetBanner()),
			STR(_newLineCharacters));
	if (!_fileStream->WriteString(header)) {
		return false;
	}
	if (_fileHistorySize > 0) {
		ADD_VECTOR_END(_history, filename);
		while (_history.size() > _fileHistorySize) {
			deleteFile(_history[0]);
			_history.erase(_history.begin());
		}
	}
	_currentLength = 0;
	_canLog = true;
	_fileIsClosed = false;
	return true;
}

void FileLogLocation::CloseFile() {
	if (_fileStream != NULL) {
		delete _fileStream;
		_fileStream = NULL;
	}
	_fileIsClosed = true;
	_canLog = false;
}
