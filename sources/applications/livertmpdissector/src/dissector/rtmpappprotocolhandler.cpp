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


#include "dissector/rtmpappprotocolhandler.h"
#include "dissector/rtmpdissectorprotocol.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"

RTMPAppProtocolHandler::RTMPAppProtocolHandler(Variant &configuration)
: BaseRTMPAppProtocolHandler(configuration) {
	_pOutputFile = NULL;
}

RTMPAppProtocolHandler::~RTMPAppProtocolHandler() {
	CloseFile();
}

bool RTMPAppProtocolHandler::SetOutputFile(string outputFile) {
	CloseFile();
	_pOutputFile = new File();
	if (!_pOutputFile->Initialize(outputFile, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to open output file %s", STR(outputFile));
		return false;
	}
	string header = "<?xml version=\"1.0\" ?>\n<messages>\n";
	if (!_pOutputFile->WriteString(header)) {
		FATAL("Unable to initialize output file %s", STR(outputFile));
		return false;
	}
	return true;
}

bool RTMPAppProtocolHandler::InboundMessageAvailable(BaseRTMPProtocol *pFrom,
		Channel &channel, Header &header) {
	pFrom->GetCustomParameters()["enabled"] = (bool)true;
	Variant request;
	if (!_rtmpProtocolSerializer.Deserialize(header, channel.inputData, request)) {
		FATAL("Unable to deserialize message");
		return false;
	}
	VH_TS(request) = channel.lastInAbsTs;

	if ((uint8_t) VH_MT(request) == RM_HEADER_MESSAGETYPE_CHUNKSIZE) {
		if (!BaseRTMPAppProtocolHandler::InboundMessageAvailable(pFrom, request)) {
			FATAL("Unable to process chunk size message");
			return false;
		}
	}

	return AnalyzeRequest((RTMPDissectorProtocol *) pFrom, request);
}

bool RTMPAppProtocolHandler::FeedAVData(BaseRTMPProtocol *pFrom, uint8_t *pData,
		uint32_t dataLength, uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio, bool isAbsolute) {
	//	if (!(bool)pFrom->GetCustomParameters()["enabled"])
	//		return true;
	if (processedLength != 0)
		return true;
	pFrom->GetCustomParameters()["enabled"] = (bool)false;
	string message = format("%s data: dLen: %5"PRIu32"; procLen: %6"PRIu32"; totLen: %8"PRIu32"; pts: %8.2f; dts: %8.2f; IA: %d",
			isAudio ? "A" : "V",
			dataLength,
			processedLength,
			totalLength,
			pts,
			dts,
			isAbsolute);

	if (dataLength + processedLength == totalLength)
		message += " *";

	string nodeName;
	if (((RTMPDissectorProtocol *) pFrom)->IsClient()) {
		WARN(" C->S: %s", STR(message));
		nodeName = "clientServer";
	} else {
		FINEST(" S->C: %s", STR(message));
		nodeName = "serverClient";
	}

	if (_pOutputFile != NULL) {
		Variant request;
		request["type"] = "AVData";
		request["info"]["dataLength"] = (uint32_t) dataLength;
		request["info"]["processedLength"] = (uint32_t) processedLength;
		request["info"]["totalLength"] = (uint32_t) totalLength;
		request["info"]["pts"] = (double) pts;
		request["info"]["dts"] = (double) dts;
		request["info"]["isAudio"] = (bool) isAudio;
		request["info"]["isAbsolute"] = (bool) isAbsolute;
		request["info"]["completed"] = (bool)((dataLength + processedLength) == totalLength);
		string msg = "    <" + nodeName + " type=\"AVData\" value=\"normal\">"
				+ "\n" + request.ToString("", 2) + "\n    "
				+ "</" + nodeName + ">\n";
		if (!_pOutputFile->WriteString(msg)) {
			FATAL("Unable to write message to file");
			return false;
		}
	}

	return true;
}

bool RTMPAppProtocolHandler::FeedAVDataAggregate(BaseRTMPProtocol *pFrom,
		uint8_t *pData, uint32_t dataLength, uint32_t processedLength,
		uint32_t totalLength, double pts, double dts, bool isAbsolute) {
	//	if (!(bool)pFrom->GetCustomParameters()["enabled"])
	//		return true;
	if (processedLength != 0)
		return true;
	pFrom->GetCustomParameters()["enabled"] = (bool)false;
	string message = format("g data: dLen: %5"PRIu32"; procLen: %6"PRIu32"; totLen: %8"PRIu32"; pts: %8.2f; dts: %8.2f; IA: %d",
			dataLength,
			processedLength,
			totalLength,
			pts,
			dts,
			isAbsolute);
	if (dataLength + processedLength == totalLength)
		message += " *";

	string nodeName;
	if (((RTMPDissectorProtocol *) pFrom)->IsClient()) {
		WARN("C->S: %s", STR(message));
		nodeName = "clientServer";
	} else {
		FINEST("S->C: %s", STR(message));
		nodeName = "serverClient";
	}

	if (_pOutputFile != NULL) {
		Variant request;
		request["type"] = "AVDataAggregate";
		request["info"]["dataLength"] = (uint32_t) dataLength;
		request["info"]["processedLength"] = (uint32_t) processedLength;
		request["info"]["totalLength"] = (uint32_t) totalLength;
		request["info"]["pts"] = (double) pts;
		request["info"]["dts"] = (double) dts;
		request["info"]["isAbsolute"] = (bool) isAbsolute;
		request["info"]["completed"] = (bool)((dataLength + processedLength) == totalLength);
		string msg = "    <" + nodeName + " type=\"AVData\" value=\"aggregate\">"
				+ "\n" + request.ToString("", 2) + "\n    "
				+ "</" + nodeName + ">\n";
		if (!_pOutputFile->WriteString(msg)) {
			FATAL("Unable to write message to file");
			return false;
		}
	}

	return true;
}

void RTMPAppProtocolHandler::CloseFile() {
	if (_pOutputFile != NULL) {
		string end = "</messages>\n";
		if (!_pOutputFile->WriteString(end)) {
			WARN("Unable to write the end of the output file");
		}
		_pOutputFile->Close();
		delete _pOutputFile;
		_pOutputFile = NULL;
	}
}

bool RTMPAppProtocolHandler::AnalyzeRequest(RTMPDissectorProtocol *pFrom, Variant &request) {
	string nodeName;
	switch ((uint8_t) VH_MT(request)) {
		case RM_HEADER_MESSAGETYPE_ACK:
		{
			return true;
		}
		case RM_HEADER_MESSAGETYPE_USRCTRL:
		{
			switch ((uint16_t) M_USRCTRL_TYPE(request)) {
				case RM_USRCTRL_TYPE_UNKNOWN1:
				case RM_USRCTRL_TYPE_UNKNOWN2:
				{
					return true;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}

	if (pFrom->IsClient()) {
		WARN("C->S:\n%s", STR(request.ToString()));
		nodeName = "clientServer";
	} else {
		FINEST("S->C:\n%s", STR(request.ToString()));
		nodeName = "serverClient";
	}

	string type = "";
	string value = "";

	switch ((uint8_t) VH_MT(request)) {
		case RM_HEADER_MESSAGETYPE_WINACKSIZE:
		{
			type = "winAck";
			if (request.HasKeyChain(_V_NUMERIC, true, 1, RM_WINACKSIZE)) {
				value = (string) request[RM_WINACKSIZE];
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_PEERBW:
		{
			type = "peerBw";
			if (request.HasKeyChain(_V_NUMERIC, true, 2, RM_PEERBW, "value")) {
				value = (string) request[RM_PEERBW]["value"];
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_ACK:
		{
			type = "ack";
			if (request.HasKeyChain(_V_NUMERIC, true, 1, RM_ACK)) {
				value = (string) request[RM_ACK];
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_CHUNKSIZE:
		{
			type = "chunkSize";
			if (request.HasKeyChain(_V_NUMERIC, true, 1, RM_CHUNKSIZE)) {
				value = (string) request[RM_CHUNKSIZE];
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_USRCTRL:
		{
			if (request.HasKeyChain(V_STRING, true, 2, RM_USRCTRL, RM_USRCTRL_TYPE_STRING)) {
				type = (string) M_USRCTRL_TYPE_STRING(request);
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_NOTIFY:
		{
			type = "notify";
			if (request.HasKeyChain(V_MAP, true, 2, RM_NOTIFY, RM_NOTIFY_PARAMS)) {
				Variant &params = M_NOTIFY_PARAMS(request);
				if (params.MapSize() > 0) {
					Variant &first = MAP_VAL(params.begin());
					if (first == V_STRING)
						value = (string) first;
				}
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_FLEXSTREAMSEND:
		{
			type = "flexStreamSend";
			break;
		}
		case RM_HEADER_MESSAGETYPE_FLEX:
		case RM_HEADER_MESSAGETYPE_INVOKE:
		{
			type = "invoke";
			if (request.HasKeyChain(V_STRING, true, 2, RM_INVOKE, RM_INVOKE_FUNCTION)) {
				value = (string) M_INVOKE_FUNCTION(request);
			}
			break;
		}
		case RM_HEADER_MESSAGETYPE_SHAREDOBJECT:
		case RM_HEADER_MESSAGETYPE_FLEXSHAREDOBJECT:
		{
			type = "sharedObject";
			break;
		}
		case RM_HEADER_MESSAGETYPE_ABORTMESSAGE:
		{
			type = "abort";
			if (request.HasKeyChain(_V_NUMERIC, true, 1, RM_ABORTMESSAGE)) {
				value = (string) request[RM_ABORTMESSAGE];
			}
			break;
		}
		default:
		{
			break;
		}
	}

	if (_pOutputFile != NULL) {
		string msg = "    <" + nodeName + " type=\"" + type + "\" value=\"" + value + "\">"
				+ "\n" + request.ToString("", 2) + "\n    "
				+ "</" + nodeName + ">\n";
		if (!_pOutputFile->WriteString(msg)) {
			FATAL("Unable to write message to file");
			return false;
		}
	}
	return true;
}
