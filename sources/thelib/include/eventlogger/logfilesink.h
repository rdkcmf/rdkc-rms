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


#ifndef _LOGFILESINK_H
#define _LOGFILESINK_H

#include "eventlogger/baseeventsink.h"
#include "protocols/timer/basetimerprotocol.h"
#include "application/baseappprotocolhandler.h"

class LogFileSink;
class DLLEXP ChunkTimerProtocol 
: public BaseTimerProtocol {
private:
	LogFileSink * _plogSink;
public:
	ChunkTimerProtocol(LogFileSink *sink);
	~ChunkTimerProtocol();
	bool TimePeriodElapsed();

};
class DLLEXP CountDownTimer
: public BaseTimerProtocol {
private:
	LogFileSink * _pLogSink;
public:
	CountDownTimer(LogFileSink *sink);
	~CountDownTimer();
	bool TimePeriodElapsed();
};
class DLLEXP ChunkTimerProtocolHandler
: public BaseAppProtocolHandler {
public:
	ChunkTimerProtocolHandler(Variant &config);
	virtual ~ChunkTimerProtocolHandler();
	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
};

class DLLEXP LogFileSink
: public BaseEventSink {
private:

	enum LogFileFormat {
		LS_UNKNOWN = -1,
		LS_TEXT,
		LS_XML,
		LS_JSON,
		LS_W3C
	};
	enum FileChunkMethod {
		CHUNK_NONE = -1,
		CHUNK_LENGTH,
		CHUNK_TIME
	};
	File _file;
	LogFileFormat _format;
	FileChunkMethod _chunkMethod;
	uint32_t _logEntryNum;
	bool _hasTimestamp;
	bool _appendTimestamp;
	bool _appendInstance;
	vector<string> _w3cFieldNames;
	Variant _fieldContentMap;
	Variant _defaults;
	string _filenameBase;
	string _extension;
	ChunkTimerProtocol * _pTimer;
	CountDownTimer * _pCountDown;
	ChunkTimerProtocolHandler * _pTimerHandler;
	uint32_t _chunkInterval;
	uint32_t _partNum;

	string EventLogEntry(Variant &event);
	string FormatMap(Variant &vmap, int indent);
	LogFileFormat FormatType(string format);
	void FilterEventList(Variant &config);
	bool WriteHeader();
	void InitializeW3CFields();
	string GetTimestamp(string format);
	string CreateW3CEntry(string eventName, Variant &payload);
	void FillW3CContents(Variant &content, Variant &eventPayload, string key="");
	bool CreateCountdown(string timeString);
	bool CreateLogInterval(int32_t interval);
	bool CreateNewFile();
	void CloseLogFile();

public:
	LogFileSink();
	~LogFileSink();
	virtual bool Initialize(Variant &config, Variant &customData);
	virtual void Log(Variant &event);
	bool TimerCallback();
	void ClearTimers();
};

#endif
