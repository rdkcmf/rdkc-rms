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

#include "eventlogger/logfilesink.h"
#include "eventlogger/sinktypes.h"
#include "eventlogger/eventlogger.h"

ChunkTimerProtocol::ChunkTimerProtocol(LogFileSink *sink) : BaseTimerProtocol() {
	_plogSink = sink;
}

ChunkTimerProtocol::~ChunkTimerProtocol() {
	if (_plogSink != NULL) {
		_plogSink->ClearTimers();
	}
}
bool ChunkTimerProtocol::TimePeriodElapsed() {
	return _plogSink->TimerCallback();
}

CountDownTimer::CountDownTimer(LogFileSink *sink) : BaseTimerProtocol() {
	_pLogSink = sink;
}

CountDownTimer::~CountDownTimer() {
	if (_pLogSink != NULL) {
		_pLogSink->ClearTimers();
	}
}

bool CountDownTimer::TimePeriodElapsed() {
	return _pLogSink->TimerCallback();
}

ChunkTimerProtocolHandler::ChunkTimerProtocolHandler(Variant &config) 
		: BaseAppProtocolHandler(config) {
}
ChunkTimerProtocolHandler::~ChunkTimerProtocolHandler(){
}
void ChunkTimerProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
}
void ChunkTimerProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}

//LogFileSink *ChunkTimerProtocol::GetLogSink() {
//	return _plogSink;
//}

LogFileSink::LogFileSink() : BaseEventSink(SINK_FILE) {
	_logEntryNum = 0;
	_hasTimestamp = true;
	_appendTimestamp = true;
	_appendInstance = false;
	_pTimer = NULL;
	_pTimerHandler = NULL;
	_pCountDown = NULL;
	_chunkInterval = 0;
	_partNum = 0;
	_extension = "";
}

LogFileSink::~LogFileSink() {
	if (_pTimer != NULL)
		_pTimer->EnqueueForDelete();
	if (_pCountDown != NULL)
		_pCountDown->EnqueueForDelete();
}

bool LogFileSink::Initialize(Variant &config, Variant &customData) {
	//1. Validate all mandatory fields
	if (!config.HasKeyChain(V_STRING, false, 1, "filename")) {
		WARN("Cannot create log file. Filename not specified");
		return false;
	}

	if (!config.HasKeyChain(V_STRING, false, 1, "format")) {
		WARN("Cannot create log file. Format not specified");
		return false;
	}

	_format = FormatType(config.GetValue("format", false));

	if (config.HasKeyChain(V_BOOL, false, 1, "appendTimestamp"))
		_appendTimestamp = config.GetValue("appendTimestamp", false);

	_hasTimestamp = (config.HasKeyChain(V_BOOL, false, 1, "timestamp") && 
			config.GetValue("timestamp", false));
	_appendInstance = (config.HasKeyChain(V_BOOL, false, 1, "appendInstance") && 
			(bool) config.GetValue("appendInstance", false));
	if (_format == LS_UNKNOWN) {
		WARN("Cannot create log file. Unknown format specified");
		return false;
	} else if (_format == LS_W3C) {
		FilterEventList(config);
		InitializeW3CFields();
	}

	if (!BaseEventSink::Initialize(config, customData)) {
		FATAL("Unable to initialize event sink");
		return false;
	}

	_chunkMethod = CHUNK_NONE;

	if (config.HasKeyChain(_V_NUMERIC, false, 1, "fileChunkLength") &&
			!config.HasKeyChain(V_STRING, false, 1, "fileChunkTime")) {
		_chunkMethod = CHUNK_LENGTH;
		_chunkInterval = (uint32_t) config["fileChunkLength"];
		CreateLogInterval(_chunkInterval);
	} else if (!config.HasKeyChain(_V_NUMERIC, false, 1, "fileChunkLength") &&
			config.HasKeyChain(V_STRING, false, 1, "fileChunkTime")) {
		_chunkMethod = CHUNK_TIME;
		string timeString = (string) config["fileChunkTime"];
		if (!CreateCountdown(timeString)) {
			FATAL("Cannot create time-based file chunking");
		}
	} else if (config.HasKeyChain(_V_NUMERIC, false, 1, "fileChunkLength") &&
			config.HasKeyChain(V_STRING, false, 1, "fileChunkTime")) {
		WARN("Only one chunking method is allowed. Disabling file chunking");
	}

	_filenameBase = (string) config.GetValue("filename", false);

	//check for extension
	// separate dir from filename
	size_t filenamePosition = _filenameBase.rfind(PATH_SEPARATOR);
	string filename;
	string dir = "";
	if (filenamePosition == string::npos) {
		filename = _filenameBase;
	} else if (filenamePosition == _filenameBase.length() - 1) {// ends on separator
		filename = "events.log"; //give default name
		dir = _filenameBase;
	} else {
		filename = _filenameBase.substr(filenamePosition + 1);
		dir = _filenameBase.substr(0, filenamePosition + 1);
	}

	//separate filename from extension
	size_t dotLocation = filename.find('.');
	if (dotLocation != string::npos) {
		// split filename
		_extension = filename.substr(dotLocation); // include "."
		filename = filename.substr(0, dotLocation);
	}

	// join dir and filename
	_filenameBase = dir + filename;
	if (!CreateNewFile()) {
		FATAL("Cannot initialize log file");
		return false;
	}
	return true;
}

bool LogFileSink::CreateNewFile() {
	if (_filenameBase == "")
		return true;
	string append = "";
	if (_appendInstance) {
		append += "_" + format("%04"PRIu16, Version::_instance);
	}
	if (_appendTimestamp)
		append += "_" + GetTimestamp("%Y%m%d_%H%M%S");
	else if (_chunkMethod != CHUNK_NONE)
		append += "_" + format("%04"PRIu32, ++_partNum);
	if (_file.IsOpen())
		CloseLogFile();
	string filename = _filenameBase + append + _extension;
	if (fileExists(filename)) {
//		if (!deleteFile(filename)) {
//			FATAL("Cannot delete old duplicate log file");
//			return false;
//		}
		if (!_file.Initialize(filename, FILE_OPEN_MODE_APPEND)) {
			FATAL("Cannot write log file");
			return false;
		}
	} else {
		if (!_file.Initialize(filename, FILE_OPEN_MODE_TRUNCATE)) {
			FATAL("Cannot write log file");
			return false;
		}
		WriteHeader();
	}
	return true;
}
LogFileSink::LogFileFormat LogFileSink::FormatType(string format) {
	if (format == "xml") {
		return LS_XML;
	} else if (format == "json") {
		return LS_JSON;
	} else if (format == "w3c") {
		return LS_W3C;
	} else {
		return LS_TEXT;
	}
}

void LogFileSink::Log(Variant &event) {
	if (CanLogEvent(event["type"])) {
		string logEntry = "";

		logEntry += EventLogEntry(event) + "\n";
		_file.WriteString(logEntry);
		_file.Flush();
	}
}

string LogFileSink::EventLogEntry(Variant &event) {
	string logEntry = "";
	Variant varLog = event;

	varLog["_logEntry"] = format("%05"PRIu32, _logEntryNum++);

	if (_hasTimestamp) {
		time_t now = time(NULL);
		struct tm * timestamp = localtime(&now);
		varLog["timestamp"] = *timestamp;
	}

	switch (_format) {
		case LS_W3C:
			logEntry = CreateW3CEntry(varLog["type"], varLog["payload"]);
			break;
		case LS_TEXT:
			if (_hasTimestamp) {
				logEntry = format("%05"PRIu32"\t%s\t%s\n", _logEntryNum, STR(varLog["type"]), STR(varLog["timestamp"].ToString()));
			} else {
				logEntry = format("%05"PRIu32"\t%s\n", _logEntryNum, STR(varLog["type"]));
			}
			logEntry += format("%s\n", STR(FormatMap(varLog["payload"], 1)));
			break;
		case LS_XML:
			varLog.SerializeToXml(logEntry, true);
			replace(logEntry, "<?xml version=\"1.0\" ?>", "");
			break;
		case LS_JSON:
			varLog.SerializeToJSON(logEntry, true);
			logEntry += ",";
			break;
		case LS_UNKNOWN:
			break;
	}

	return logEntry;
}

string LogFileSink::FormatMap(Variant &vmap, int indent) {
	string retval = "";
	string leadingTabs = string(indent, '\t');
	if (vmap != V_MAP)
		return "";

	FOR_MAP(vmap, string, Variant, i) {
		string label = MAP_KEY(i);
		if ((label.length() == 10) && (label[0] == '0') && (label[1] == 'x')) {
			label = format("%"PRIu32, (uint32_t) strtol(label.c_str(), NULL, 16));
		}
		if (MAP_VAL(i) != V_NULL) {
			if (MAP_VAL(i) != V_MAP && MAP_VAL(i) != V_TYPED_MAP)
				retval += leadingTabs + label + ": " + STR(MAP_VAL(i)) + "\n";
			else
				retval += leadingTabs + label + ":\n" + FormatMap(MAP_VAL(i), indent + 1);
		}
	}
	return retval;
}
string LogFileSink::CreateW3CEntry(string eventName, Variant &payload) {
	Variant temp(_defaults);
	if (_hasTimestamp)
		temp["timestamp"] = GetTimestamp("%Y-%b-%d %H:%M:%S");
	temp["INFO"] = eventName;
	FillW3CContents(temp, payload);
	string retval = "";
	FOR_VECTOR(_w3cFieldNames, i) {
		string field = _w3cFieldNames[i];
		retval += (retval == "" ? "" : "\t") + (string) temp.GetValue(field, false);
	}
	return retval;
}
void LogFileSink::FillW3CContents(Variant &content, Variant &eventPayload, string key) {
	if (key != "")
		key += "/";
	FOR_MAP(eventPayload, string, Variant, i) {
		string label = key + MAP_KEY(i);
		if (MAP_VAL(i) != V_NULL) {
			if (MAP_VAL(i) != V_MAP && MAP_VAL(i) != V_TYPED_MAP) {
				if (_fieldContentMap.HasKey(label, false)) {
					content[_fieldContentMap[label]] = STR(MAP_VAL(i));
				}
			} else {
				FillW3CContents(content, MAP_VAL(i), label);
			}
		}
	}
}

void LogFileSink::FilterEventList(Variant &config){
	Variant events;
	events.PushToArray("inStreamCreated");
	events.PushToArray("outStreamCreated");
	events.PushToArray("streamCreated");
	events.PushToArray("inStreamCodecsUpdated");
	events.PushToArray("outStreamCodecsUpdated");
	events.PushToArray("streamCodecsUpdated");
	events.PushToArray("inStreamClosed");
	events.PushToArray("outStreamClosed");
	events.PushToArray("streamClosed");
	events.PushToArray("audioFeedStopped");
	events.PushToArray("videoFeedStopped");
	events.PushToArray("mediaFileDownloaded");
	events.PushToArray("streamingSessionStarted");
	events.PushToArray("streamingSessionEnded");
	if (config.HasKeyChain(V_MAP, false, 1, "enabledEvents")
			&& config["enabledEvents"].IsArray()) {
		Variant tmp;

		FOR_MAP(config["enabledEvents"], string, Variant, i) {
			if (events.MapSize() == 0)
				break;
			FOR_MAP(events, string, Variant, j) {
				if (MAP_VAL(i) == MAP_VAL(j)) {
					tmp.PushToArray(MAP_VAL(j));
					events.RemoveKey(MAP_KEY(j));
					break;
				}
			}
		}
		config.RemoveKey("enabledEvents");
		config["enabledEvents"] = tmp;
	} else {
		config["enabledEvents"] = events;
	}
}

bool LogFileSink::WriteHeader() {
	string header = "";
	if (_format == LS_XML) {
		header = "<?xml version=\"1.0\" ?>";
	} else if (_format == LS_W3C) {
		header += "#Version: 1.0\n"; //TODO: replace version
		header += format("#Start-Date: %s\n", STR(GetTimestamp("%Y-%b-%d %H:%M:%S UTC")));
		header += format("#Software: RDKC Media Server %s %s\n",
			STR(Version::GetReleaseNumber()), STR(Version::GetBuildNumber()));
		string fields = "#Fields: ";
		FOR_VECTOR(_w3cFieldNames, i) {
			fields += _w3cFieldNames[i] + (i < _w3cFieldNames.size() - 1 ? "\t" : "\n");
		}
		header += fields;
	}
	if (!_file.WriteString(header) || !_file.Flush()) {
		FATAL("Cannot write to log file.");
		return false;
	}
	
	return true;
}

string LogFileSink::GetTimestamp(string format) {
	char tempBuff[128] = {0};
//	struct tm * tm;
	time_t t = getutctime();
	
//	gmtime_r(&t, &tm);
	strftime(tempBuff, 127, STR(format), localtime(&t));
	return string(tempBuff);
}

void LogFileSink::InitializeW3CFields() {
	if (_hasTimestamp)
		_w3cFieldNames.push_back("timestamp");
	_w3cFieldNames.push_back("processId");
	_w3cFieldNames.push_back("INFO");
	_w3cFieldNames.push_back("name");
	_w3cFieldNames.push_back("applicationName");
	_w3cFieldNames.push_back("creationTimestamp");
	_w3cFieldNames.push_back("type");
	_w3cFieldNames.push_back("typeNumeric");
	_w3cFieldNames.push_back("uniqueId");
	_w3cFieldNames.push_back("upTime");
	_w3cFieldNames.push_back("queryTimestamp");
	_w3cFieldNames.push_back("bandwidth");
	_w3cFieldNames.push_back("connectionType");
	_w3cFieldNames.push_back("farIp");
	_w3cFieldNames.push_back("farPort");
	_w3cFieldNames.push_back("IP");
	_w3cFieldNames.push_back("nearIP");
	_w3cFieldNames.push_back("nearPort");
	_w3cFieldNames.push_back("port");
	_w3cFieldNames.push_back("swf-url");
	_w3cFieldNames.push_back("audio-bytes");
	_w3cFieldNames.push_back("audio-codec");
	_w3cFieldNames.push_back("audio-codecNumeric");
	_w3cFieldNames.push_back("audio-droppedBytes");
	_w3cFieldNames.push_back("audio-droppedPackets");
	_w3cFieldNames.push_back("audio-packets");
	_w3cFieldNames.push_back("video-bytes");
	_w3cFieldNames.push_back("video-codec");
	_w3cFieldNames.push_back("video-codecNumeric");
	_w3cFieldNames.push_back("video-droppedBytes");
	_w3cFieldNames.push_back("video-droppedPackets");
	_w3cFieldNames.push_back("video-packets");
	_w3cFieldNames.push_back("pullSettings-audioCodecBytes");
	_w3cFieldNames.push_back("pullSettings-configId");
	_w3cFieldNames.push_back("pullSettings-emulateUserAgent");
	_w3cFieldNames.push_back("pullSettings-forceTcp");
	_w3cFieldNames.push_back("pullSettings-httpProxy");
	_w3cFieldNames.push_back("pullSettings-isAudio");
	_w3cFieldNames.push_back("pullSettings-keepAlive");
	_w3cFieldNames.push_back("pullSettings-localStreamName");
	_w3cFieldNames.push_back("pullSettings-operationType");
	_w3cFieldNames.push_back("pullSettings-pageUrl");
	_w3cFieldNames.push_back("pullSettings-ppsBytes");
	_w3cFieldNames.push_back("pullSettings-rangeEnd");
	_w3cFieldNames.push_back("pullSettings-rangeStart");
	_w3cFieldNames.push_back("pullSettings-rtcpDetectionInterval");
	_w3cFieldNames.push_back("pullSettings-sendRenewStream");
	_w3cFieldNames.push_back("pullSettings-spsBytes");
	_w3cFieldNames.push_back("pullSettings-ssmIp");
	_w3cFieldNames.push_back("pullSettings-swfUrl");
	_w3cFieldNames.push_back("pullSettings-tcUrl");
	_w3cFieldNames.push_back("pullSettings-tos");
	_w3cFieldNames.push_back("pullSettings-ttl");
	_w3cFieldNames.push_back("pullSettings-uri");
	_w3cFieldNames.push_back("recordSettings-chunkLength");
	_w3cFieldNames.push_back("recordSettings-computedPathToFile");
	_w3cFieldNames.push_back("recordSettings-configId");
	_w3cFieldNames.push_back("recordSettings-keepAlive");
	_w3cFieldNames.push_back("recordSettings-localStreamName");
	_w3cFieldNames.push_back("recordSettings-mp4BinPath");
	_w3cFieldNames.push_back("recordSettings-operationType");
	_w3cFieldNames.push_back("recordSettings-overwrite");
	_w3cFieldNames.push_back("recordSettings-pathToFile");
	_w3cFieldNames.push_back("recordSettings-type");
	_w3cFieldNames.push_back("recordSettings-waitForIDR");
	_w3cFieldNames.push_back("recordSettings-winQtCompat");
#ifdef HAS_W3C_FOR_EWS
	_w3cFieldNames.push_back("ews-startTime");
	_w3cFieldNames.push_back("ews-stopTime");
	_w3cFieldNames.push_back("ews-elapsed");
	_w3cFieldNames.push_back("ews-clientIP");
	_w3cFieldNames.push_back("ews-sessionId");
	_w3cFieldNames.push_back("ews-playlist");
	_w3cFieldNames.push_back("ews-mediaFile");
#endif /* HAS_W3C_FOR_EWS */
	_fieldContentMap["PID"] = "processId";
	_fieldContentMap["name"] = "name";
	_fieldContentMap["appName"] = "applicationName";
	_fieldContentMap["creationTimestamp"] = "creationTimestamp";
	_fieldContentMap["type"] = "type";
	_fieldContentMap["typeNumeric"] = "typeNumeric";
	_fieldContentMap["uniqueId"] = "uniqueId";
	_fieldContentMap["upTime"] = "upTime";
	_fieldContentMap["queryTimestamp"] = "queryTimestamp";
	_fieldContentMap["bandwidth"] = "bandwidth";
	_fieldContentMap["connectionType"] = "connectionType";
	_fieldContentMap["farIp"] = "farIp";
	_fieldContentMap["farPort"] = "farPort";
	_fieldContentMap["ip"] = "IP";
	_fieldContentMap["nearIp"] = "nearIP";
	_fieldContentMap["nearPort"] = "nearPort";
	_fieldContentMap["port"] = "port";
	_fieldContentMap["audio/bytesCount"] = "audio-bytes";
	_fieldContentMap["audio/codec"] = "audio-codec";
	_fieldContentMap["audio/codecNumeric"] = "audio-codecNumeric";
	_fieldContentMap["audio/droppedBytesCount"] = "audio-droppedBytes";
	_fieldContentMap["audio/droppedPacketsCount"] = "audio-droppedPackets";
	_fieldContentMap["audio/packetsCount"] = "audio-packets";
	_fieldContentMap["video/bytesCount"] = "video-bytes";
	_fieldContentMap["video/codec"] = "video-codec";
	_fieldContentMap["video/codecNumeric"] = "video-codecNumeric";
	_fieldContentMap["video/droppedBytesCount"] = "video-droppedBytes";
	_fieldContentMap["video/droppedPacketsCount"] = "video-droppedPackets";
	_fieldContentMap["video/packetsCount"] = "video-packets";
	_fieldContentMap["pullSettings/audioCodecBytes"] = "pullSettings-audioCodecBytes";
	_fieldContentMap["pullSettings/configId"] = "pullSettings-configId";
	_fieldContentMap["pullSettings/emulateUserAgent"] = "pullSettings-emulateUserAgent";
	_fieldContentMap["pullSettings/forceTcp"] = "pullSettings-forceTcp";
	_fieldContentMap["pullSettings/httpProxy"] = "pullSettings-httpProxy";
	_fieldContentMap["pullSettings/isAudio"] = "pullSettings-isAudio";
	_fieldContentMap["pullSettings/keepAlive"] = "pullSettings-keepAlive";
	_fieldContentMap["pullSettings/localStreamName"] = "pullSettings-localStreamName";
	_fieldContentMap["pullSettings/operationType"] = "pullSettings-operationType";
	_fieldContentMap["pullSettings/pageUrl"] = "pullSettings-pageUrl";
	_fieldContentMap["pullSettings/ppsBytes"] = "pullSettings-ppsBytes";
	_fieldContentMap["pullSettings/rangeEnd"] = "pullSettings-rangeEnd";
	_fieldContentMap["pullSettings/rangeStart"] = "pullSettings-rangeStart";
	_fieldContentMap["pullSettings/rtcpDetectionInterval"] = "pullSettings-rtcpDetectionInterval";
	_fieldContentMap["pullSettings/sendRenewStream"] = "pullSettings-sendRenewStream";
	_fieldContentMap["pullSettings/spsBytes"] = "pullSettings-spsBytes";
	_fieldContentMap["pullSettings/ssmIp"] = "pullSettings-ssmIp";
	_fieldContentMap["pullSettings/swfUrl"] = "pullSettings-swfUrl";
	_fieldContentMap["pullSettings/tcUrl"] = "pullSettings-tcUrl";
	_fieldContentMap["pullSettings/tos"] = "pullSettings-tos";
	_fieldContentMap["pullSettings/ttl"] = "pullSettings-ttl";
	_fieldContentMap["pullSettings/uri"] = "pullSettings-uri";
	_fieldContentMap["recordSettings/chunkLength"] = "recordSettings-chunkLength";
	_fieldContentMap["recordSettings/computedPathToFile"] = "recordSettings-computedPathToFile";
	_fieldContentMap["recordSettings/configId"] = "recordSettings-configId";
	_fieldContentMap["recordSettings/keepAlive"] = "recordSettings-keepAlive";
	_fieldContentMap["recordSettings/localStreamName"] = "recordSettings-localStreamName";
	_fieldContentMap["recordSettings/mp4BinPath"] = "recordSettings-mp4BinPath";
	_fieldContentMap["recordSettings/operationType"] = "recordSettings-operationType";
	_fieldContentMap["recordSettings/overwrite"] = "recordSettings-overwrite";
	_fieldContentMap["recordSettings/pathToFile"] = "recordSettings-pathToFile";
	_fieldContentMap["recordSettings/type"] = "recordSettings-type";
	_fieldContentMap["recordSettings/waitForIDR"] = "recordSettings-waitForIDR";
	_fieldContentMap["recordSettings/winQtCompat"] = "recordSettings-winQtCompat";
	_fieldContentMap["swfUrl"] = "swf-url";
#ifdef HAS_WEBSERVER
#ifdef HAS_W3C_FOR_EWS
	_fieldContentMap["startTime"] = "ews-startTime";
	_fieldContentMap["stopTime"] = "ews-stopTime";
	_fieldContentMap["elapsed"] = "ews-elapsed";
	_fieldContentMap["clientIP"] = "ews-clientIP";
	_fieldContentMap["sessionId"] = "ews-sessionId";
	_fieldContentMap["playlist"] = "ews-playlist";
	_fieldContentMap["mediaFile"] = "ews-mediaFile";
 #endif /* HAS_W3C_FOR_EWS */
 #endif /* HAS_WEBSERVER */
	_defaults["name"] = "-";
	_defaults["applicationName"] = "-";
	_defaults["creationTimestamp"] = "-";
	_defaults["type"] = "-";
	_defaults["typeNumeric"] = "-";
	_defaults["uniqueId"] = "-";
	_defaults["upTime"] = "-";
	_defaults["queryTimestamp"] = "-";
	_defaults["bandwidth"] = "-";
	_defaults["connectionType"] = "-";
	_defaults["farIp"] = "-";
	_defaults["farPort"] = "-";
	_defaults["IP"] = "-";
	_defaults["nearIP"] = "-";
	_defaults["nearPort"] = "-";
	_defaults["port"] = "-";
	_defaults["audio-bytes"] = "-";
	_defaults["audio-codec"] = "-";
	_defaults["audio-codecNumeric"] = "-";
	_defaults["audio-droppedBytes"] = "-";
	_defaults["audio-droppedPackets"] = "-";
	_defaults["audio-packets"] = "-";
	_defaults["video-bytes"] = "-";
	_defaults["video-codec"] = "-";
	_defaults["video-codecNumeric"] = "-";
	_defaults["video-droppedBytes"] = "-";
	_defaults["video-droppedPackets"] = "-";
	_defaults["video-packets"] = "-";
	_defaults["pullSettings-audioCodecBytes"] = "-";
	_defaults["pullSettings-configId"] = "-";
	_defaults["pullSettings-emulateUserAgent"] = "-";
	_defaults["pullSettings-forceTcp"] = "-";
	_defaults["pullSettings-httpProxy"] = "-";
	_defaults["pullSettings-isAudio"] = "-";
	_defaults["pullSettings-keepAlive"] = "-";
	_defaults["pullSettings-localStreamName"] = "-";
	_defaults["pullSettings-operationType"] = "-";
	_defaults["pullSettings-pageUrl"] = "-";
	_defaults["pullSettings-ppsBytes"] = "-";
	_defaults["pullSettings-rangeEnd"] = "-";
	_defaults["pullSettings-rangeStart"] = "-";
	_defaults["pullSettings-rtcpDetectionInterval"] = "-";
	_defaults["pullSettings-sendRenewStream"] = "-";
	_defaults["pullSettings-spsBytes"] = "-";
	_defaults["pullSettings-ssmIp"] = "-";
	_defaults["pullSettings-swfUrl"] = "-";
	_defaults["pullSettings-tcUrl"] = "-";
	_defaults["pullSettings-tos"] = "-";
	_defaults["pullSettings-ttl"] = "-";
	_defaults["pullSettings-uri"] = "-";
	_defaults["recordSettings-chunkLength"] = "-";
	_defaults["recordSettings-computedPathToFile"] = "-";
	_defaults["recordSettings-configId"] = "-";
	_defaults["recordSettings-keepAlive"] = "-";
	_defaults["recordSettings-localStreamName"] = "-";
	_defaults["recordSettings-mp4BinPath"] = "-";
	_defaults["recordSettings-operationType"] = "-";
	_defaults["recordSettings-overwrite"] = "-";
	_defaults["recordSettings-pathToFile"] = "-";
	_defaults["recordSettings-type"] = "-";
	_defaults["recordSettings-waitForIDR"] = "-";
	_defaults["recordSettings-winQtCompat"] = "-";
	_defaults["swf-url"] = "-";
#ifdef HAS_W3C_FOR_EWS
	_defaults["ews-startTime"] = "-";
	_defaults["ews-stopTime"] = "-";
	_defaults["ews-elapsed"] = "-";
	_defaults["ews-clientIP"] = "-";
	_defaults["ews-sessionId"] = "-";
	_defaults["ews-playlist"] = "-";
	_defaults["ews-mediaFile"] = "-";
#endif /* HAS_W3C_FOR_EWS */
}

bool LogFileSink::CreateCountdown(string timeString) {
	struct tm target;
	time_t now = time(NULL);
	struct tm *ref = localtime(&now);
	memcpy(&target, ref, sizeof(struct tm));
	vector<string> timeParts;

	split(timeString, ":", timeParts);
	if (timeParts.size() < 2) {
		FATAL("Invalid FileChunkTime format");
		return false;
	}
	
	target.tm_hour = atoi(STR(timeParts[0]));
	target.tm_min = atoi(STR(timeParts[1]));
	target.tm_sec = timeParts.size() > 3 ? atoi(STR(timeParts[0])) : 0;

	time_t test = mktime(&target);

	if (test < now) {
		target.tm_mday++;
		test = mktime(&target);
	}

	now = time(NULL);
	_pCountDown = new CountDownTimer(this);
	_pCountDown->EnqueueForTimeEvent((uint32_t)(test - now));
	return true;
}

bool LogFileSink::CreateLogInterval(int32_t interval) {
	if (_pTimer != NULL)
		delete _pTimer;
	_pTimer = new ChunkTimerProtocol(this);
	_pTimer->EnqueueForTimeEvent(_chunkInterval);
	return true;
}
bool LogFileSink::TimerCallback() {
	if (_chunkMethod == CHUNK_LENGTH) {
		return CreateNewFile();
	} else if (_chunkMethod == CHUNK_TIME){
		delete _pCountDown;
		_pCountDown = NULL;
		CreateLogInterval(24 * 60 * 60);
		_chunkMethod = CHUNK_LENGTH;
		return CreateNewFile();
	} else {
		FATAL("Unknown error. It should not have reached this point");
		return false;
	}
}

void LogFileSink::CloseLogFile() {
	if (_format == LS_W3C) {
		string footer = "\n# ======= End of file ========";
		_file.WriteString(footer);
	}
	_file.Close();
}

void LogFileSink::ClearTimers() {
	_pTimer = NULL;
	_pCountDown = NULL;
}

