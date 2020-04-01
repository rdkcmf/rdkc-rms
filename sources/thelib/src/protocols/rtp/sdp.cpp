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
*/

#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/sdp.h"
#include "streaming/codectypes.h"
#include <functional>

#define STATIC_TYPE(x) ((x) < 96)
#define PAYLOAD_TYPE_PCMU 0
#define PAYLOAD_TYPE_PCMA 8

SDP::SDP()
: Variant() {
}

SDP::~SDP() {
}

bool SDP::ParseSDP(SDP &sdp, string &raw, bool includeAll) {
	//FINEST("%s", STR(raw));
	//1. Reset
	sdp.Reset();

	//2. Prepare the sections
	sdp[SDP_SESSION].IsArray(false);
	sdp[SDP_MEDIATRACKS].IsArray(true);

	//3. Split the raw content into lines
	replace(raw, "\\r\\n", "\\n");
	FINE(" Raw SDP string is %s", STR(raw));
	vector<string> lines;
	split(raw, "\\n", lines);
	FINE("The number of lines on raw SDP string are %d", lines. size());

	//4. Detect the media tracks indexes
	vector<uint32_t> trackIndexes;
	for (uint32_t i = 0; i < lines.size(); i++) {
		trim(lines[i]);
		if (lines[i].find("m=") == 0) {
			ADD_VECTOR_END(trackIndexes, i);
		}
	}
	INFO("The number of tracks available on SDP are %d", trackIndexes.size());
	if (trackIndexes.size() == 0) {
		FATAL("No tracks found");
		return false;
	}

	//5. Parse the header
	if (!ParseSection(sdp[SDP_SESSION], lines, 0, trackIndexes[0])) {
		FATAL("Unable to parse header");
		return false;
	}

	Variant media;
	for (uint32_t i = 0; i < trackIndexes.size() - 1; i++) {
		media.Reset();
		media.IsArray(false);
		if (!ParseSection(media, lines, trackIndexes[i], trackIndexes[i + 1] - trackIndexes[i])) {
			FATAL("Unable to parse header");
			return false;
		}
		NormalizeMedia(sdp[SDP_MEDIATRACKS], media, includeAll);
#if 0
		bool recvonly = (bool)media[SDP_A]["recvonly"];
		bool sendrecv = (bool)media[SDP_A]["sendrecv"];
		string mediaT = media[SDP_M]["media_type"];

		INFO("%d track recvonly: %d sendrecv: %d media: %s", i, recvonly, sendrecv, STR(mediaT));
#endif
	}

	//7. Parse the last media section
	media.Reset();
	media.IsArray(false);
	if (!ParseSection(media, lines,
			trackIndexes[(uint32_t) trackIndexes.size() - 1],
			(uint32_t) trackIndexes.size() - trackIndexes[(uint32_t) trackIndexes.size() - 1])) {
		FATAL("Unable to parse header");
		return false;
	}
	NormalizeMedia(sdp[SDP_MEDIATRACKS], media, includeAll);
#if 0
	bool recvonly = (bool)media[SDP_A]["recvonly"];
	bool sendrecv = (bool)media[SDP_A]["sendrecv"];
	string mediaT = media[SDP_M]["media_type"];

	INFO("Last track, recvonly: %d sendrecv: %d media: %s", recvonly, sendrecv, STR(mediaT));
#endif

	sdp.CalcVideoSourceIndex();

	return true;
}

Variant SDP::GetVideoTrack(string const &videoSourceIndex, string contentBase) {
	//1. Find the track
	Variant track = GetTrack(videoSourceIndex, "video");
	if (track == V_NULL) {
		FATAL("Video track index %s not found", STR(videoSourceIndex));
		return Variant();
	}

	//2. Prepare the info
	Variant result;
	SDP_VIDEO_SERVER_IP(result) = (*this)[SDP_SESSION][SDP_O]["address"];
	string control = track[SDP_A].GetValue("control", false);
	if (control.find("rtsp") == 0) {
		SDP_VIDEO_CONTROL_URI(result) = control;
	} else {
		if ((contentBase != "") && (contentBase[contentBase.size() - 1] != '/')) {
			contentBase += "/";
		}
		SDP_VIDEO_CONTROL_URI(result) = contentBase + control;
	}
	SDP_VIDEO_CODEC(result) = track[SDP_A].GetValue("rtpmap", false)["encodingName"];
	if ((uint64_t)SDP_VIDEO_CODEC(result) != CONTAINER_AUDIO_VIDEO_MP2T) {
		if ((uint64_t)SDP_VIDEO_CODEC(result) != CODEC_VIDEO_H264) {
			FATAL("The only supported video codec is h264");
			return Variant();
		}
		SDP_VIDEO_CODEC_H264_SPS(result) = track[SDP_A]
			.GetValue("fmtp", false)
			.GetValue("sprop-parameter-sets", false)["SPS"];
		SDP_VIDEO_CODEC_H264_PPS(result) = track[SDP_A]
			.GetValue("fmtp", false)
			.GetValue("sprop-parameter-sets", false)["PPS"];
	}
	SDP_TRACK_GLOBAL_INDEX(result) = SDP_TRACK_GLOBAL_INDEX(track);
	SDP_TRACK_IS_AUDIO(result) = (bool)false;
	if (track.HasKeyChain(V_UINT32, false, 1, SDP_B))
		SDP_TRACK_BANDWIDTH(result) = track[SDP_B];
	else
		SDP_TRACK_BANDWIDTH(result) = (uint32_t) 0;

	SDP_TRACK_CLOCKRATE(result) = track[SDP_A].GetValue("rtpmap", false)["clockRate"];
	
	// Get the frame rate
	if (track.HasKeyChain(V_DOUBLE, false, 2, SDP_A, "framerate")) {
		SDP_VIDEO_FRAME_RATE(result) = track[SDP_A].GetValue("framerate", false);
	} else {
		SDP_VIDEO_FRAME_RATE(result) = (double) -1;
	}

	//3. Done
	return result;
}

Variant SDP::GetAudioTrack(string const &videoSourceIndex, string contentBase) {
	//1. Find the track
	Variant track = GetTrack(videoSourceIndex, "audio");
	if (track == V_NULL) {
		FATAL("Audio track index %s not found", STR(videoSourceIndex));
		return Variant();
	}

	//2. Prepare the info
	Variant result;
	SDP_AUDIO_SERVER_IP(result) = (*this)[SDP_SESSION][SDP_O]["address"];
	string control = track[SDP_A].GetValue("control", false);
	if (control.find("rtsp") == 0) {
		SDP_AUDIO_CONTROL_URI(result) = control;
	} else {
		if ((contentBase != "") && (contentBase[contentBase.size() - 1] != '/')) {
			contentBase += "/";
		}
		SDP_AUDIO_CONTROL_URI(result) = contentBase + control;
	}
	SDP_AUDIO_CODEC(result) = track[SDP_A].GetValue("rtpmap", false)["encodingName"];

#ifdef HAS_G711
	uint64_t codecType = (uint64_t) SDP_AUDIO_CODEC(result);
	if (!AllowedCodec(codecType)) {
#else
	if ((uint64_t) SDP_AUDIO_CODEC(result) != CODEC_AUDIO_AAC) {
		FATAL("The only supported audio codec is aac");
#endif	/* HAS_G711 */
		FATAL("The unsupported audio codec");
		return Variant();
	}

#ifdef HAS_G711
	if (codecType == CODEC_AUDIO_AAC)
#endif	/* HAS_G711 */
		SDP_AUDIO_CODEC_SETUP(result) = track[SDP_A]
				.GetValue("fmtp", false)
				.GetValue("config", false);
	SDP_AUDIO_TRANSPORT(result) = track[SDP_A].GetValue("rtpmap", false)["encodingNameString"];
	SDP_TRACK_GLOBAL_INDEX(result) = SDP_TRACK_GLOBAL_INDEX(track);
	SDP_TRACK_IS_AUDIO(result) = (bool)true;
	if (track.HasKeyChain(V_UINT32, false, 1, SDP_B))
		SDP_TRACK_BANDWIDTH(result) = track[SDP_B];
	else
		SDP_TRACK_BANDWIDTH(result) = (uint32_t) 0;

	SDP_TRACK_CLOCKRATE(result) = track[SDP_A].GetValue("rtpmap", false)["clockRate"];
	//3. Done
	return result;
}

uint32_t SDP::GetTotalBandwidth() {
	if (HasKeyChain(V_UINT32, false, 2, SDP_SESSION, SDP_B))
		return (uint32_t) ((*this)[SDP_SESSION][SDP_B]);
	else
		return 0;
}

string SDP::GetStreamName() {
	if (!HasKey(SDP_SESSION))
		return "";
	if (!(*this)[SDP_SESSION].HasKey(SDP_S))
		return "";
	return (string) (*this)[SDP_SESSION][SDP_S];
}

bool SDP::ParseTransportLine(string raw, Variant &result) {
	//raw = "MP2T/H2221/UDP;unicast;destination=192.168.1.150;client_port=11112,MP2T/H2221/UDP;multicast,RAW/RAW/UDP;unicast;destination=192.168.1.150;client_port=11112,RAW/RAW/UDP;multicast";
	//raw = "MP2T/H2221/UDP;unicast;destination=192.168.1.150;client_port=11112," + raw;
	result.Reset();
	result["original"] = raw;
	result["alternatives"].IsArray(true);

	vector<string> parts;
	split(raw, ",", parts);
	for (vector<string>::size_type i = 0; i < parts.size(); i++) {
		Variant temp;
		if (!ParseTransportLinePart(parts[i], temp)) {
			WARN("Invalid transport part: %s", STR(parts[i]));
			continue;
		}
		result["alternatives"].PushToArray(temp);
	}
	//FINEST("%s", STR(result.ToString()));
	return result["alternatives"].MapSize() != 0;
}

bool SDP::ParseTransportLinePart(string raw, Variant &result) {
	result.Reset();
	result["original"] = raw;

	//1. split after ';'
	vector<string> parts;
	split(raw, ";", parts);

	//2. Construct the result
	for (uint32_t i = 0; i < parts.size(); i++) {
		string part = parts[i];
		trim(part);
		if (part == "")
			continue;
		string::size_type pos = part.find('=');
		if (pos == string::npos) {
			result[lowerCase(part)] = (bool)true;
			continue;
		}
		result[lowerCase(part.substr(0, pos))] = part.substr(pos + 1);
	}

	vector<string> keys;
	ADD_VECTOR_END(keys, "client_port");
	ADD_VECTOR_END(keys, "server_port");
	ADD_VECTOR_END(keys, "interleaved");
	ADD_VECTOR_END(keys, "port");

	for (uint32_t i = 0; i < keys.size(); i++) {
		string key = keys[i];
		if (!result.HasKey(key))
			continue;
		parts.clear();
		raw = (string) result[key];
		split(raw, "-", parts);
		if ((parts.size() != 2) && (parts.size() != 1)) {
			FATAL("Invalid transport line: %s", STR(raw));
			return false;
		}
		string all = "";
		uint16_t data = 0;
		uint16_t rtcp = 0;
		if (parts.size() == 2) {
			data = (uint16_t) atoi(STR(parts[0]));
			rtcp = (uint16_t) atoi(STR(parts[1]));
			if (((data % 2) != 0) || ((data + 1) != rtcp))
				WARN("Invalid port numbers. Data should be even, rtcp should be odd: %s", STR(raw));
			all = format("%"PRIu16"-%"PRIu16, data, rtcp);
		} else {
			data = (uint16_t) atoi(STR(parts[0]));
			all = format("%"PRIu16, data);
			rtcp = 0;
		}
		if (all != raw) {
			FATAL("Invalid transport line: %s", STR(raw));
			return false;
		}
		result.RemoveKey(key);
		result[key]["data"] = (uint16_t) data;
		result[key]["rtcp"] = (uint16_t) rtcp;
		result[key]["all"] = all;
	}

	return true;
}

bool SDP::ParseSection(Variant &result, vector<string> &lines,
		uint32_t start, uint32_t length) {
	for (uint32_t i = 0; ((i + start) < lines.size()) && (i < length); i++) {
		if (lines[i + start] == "")
			continue;
		if (!ParseSDPLine(result, lines[i + start])) {
			FATAL("Parsing line %s failed", STR(lines[i + start]));
			return false;
		}
	}
	return true;
}

bool SDP::ParseSDPLine(Variant &result, string &line) {
	//1. test if this is a valid line
	if (line.size() < 2) {
		FATAL("Invalid line: %s", STR(line));
		return false;
	}
	if (!((line[0] >= 'a') && (line[0] <= 'z'))) {
		FATAL("Invalid line: %s", STR(line));
		return false;
	}
	if (line[1] != '=') {
		FATAL("Invalid line: %s", STR(line));
		return false;
	}

#define FORBID_DUPLICATE(x) do{if (result.HasKey(x)) {FATAL("Duplicate value: %s", STR(line));return false;}}while(0);

	switch (line[0]) {
		case 'a':
		{
			string name;
			Variant value;
			if (!ParseSDPLineA(name, value, line.substr(2)))
				return false;
			if (result[SDP_A] == V_NULL)
				result[SDP_A].IsArray(false);
			if (value.HasKeyChain(_V_NUMERIC, true, 1, "payloadType"))
				result[SDP_A][name][(uint32_t) value["payloadType"]] = value;
			else
				result[SDP_A][name] = value;
			return true;
		}
		case 'b':
		{
			//FORBID_DUPLICATE(SDP_B);
			return ParseSDPLineB(result[SDP_B], line.substr(2));
		}
		case 'c':
		{
			//FORBID_DUPLICATE(SDP_C);
			return ParseSDPLineC(result[SDP_C], line.substr(2));
		}
		case 'e':
		{
			Variant node;
			if (!ParseSDPLineE(node, line.substr(2)))
				return false;
			result[SDP_E].PushToArray(node);
			return true;
		}
		case 'i':
		{
			FORBID_DUPLICATE(SDP_I);
			return ParseSDPLineI(result[SDP_I], line.substr(2));
		}
		case 'k':
		{
			FORBID_DUPLICATE(SDP_K);
			return ParseSDPLineK(result[SDP_K], line.substr(2));
		}
		case 'm':
		{
			FORBID_DUPLICATE(SDP_M);
			return ParseSDPLineM(result[SDP_M], line.substr(2));
		}
		case 'o':
		{
			FORBID_DUPLICATE(SDP_O);
			if (!ParseSDPLineO(result[SDP_O], line.substr(2))) {
				WARN("SDP line parsing failed: `%s`", STR(line));
			}
			return true;
		}
		case 'p':
		{
			Variant node;
			if (!ParseSDPLineP(node, line.substr(2)))
				return false;
			result[SDP_P].PushToArray(node);
			return true;
		}
		case 'r':
		{
			FORBID_DUPLICATE(SDP_R);
			return ParseSDPLineR(result[SDP_R], line.substr(2));
		}
		case 's':
		{
			FORBID_DUPLICATE(SDP_S);
			return ParseSDPLineS(result[SDP_S], line.substr(2));
		}
		case 't':
		{
			Variant node;
			if (!ParseSDPLineT(node, line.substr(2)))
				return false;
			result[SDP_T].PushToArray(node);
			return true;
		}
		case 'u':
		{
			FORBID_DUPLICATE(SDP_U);
			return ParseSDPLineU(result[SDP_U], line.substr(2));
		}
		case 'v':
		{
			FORBID_DUPLICATE(SDP_V);
			return ParseSDPLineV(result[SDP_V], line.substr(2));
		}
		case 'z':
		{
			FORBID_DUPLICATE(SDP_Z);
			return ParseSDPLineZ(result[SDP_Z], line.substr(2));
		}
		default:
		{
			FATAL("Invalid line: %s", STR(line));
			return false;
		}
	}
}

bool SDP::ParseSDPLineA(string &attributeName, Variant &value, string line) {
	string::size_type pos = line.find(':');
	if ((pos == string::npos)
			|| (pos == 0)
			|| (pos == (line.size() - 1))) {
		attributeName = line;
		value = (bool)true;
		return true;
	}

	attributeName = line.substr(0, pos);
	string rawValue = line.substr(line.find(':') + 1);
	if (attributeName == "control") {
		value = rawValue;
		return true;
	} else if (attributeName == "maxprate") {
		value = (double) strtod(STR(rawValue), NULL);
		return true;
	} else if (attributeName.find("x-") == 0) {
		value = rawValue;
		return true;
	} else if (attributeName == "rtpmap") {
		//rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parameters>]
		vector<string> parts;
		split(rawValue, " ", parts);
		if (parts.size() != 2)
			return false;
		value["payloadType"] = (uint8_t) atoi(STR(parts[0]));
		split(parts[1], "/", parts);
		if ((parts.size() != 2) && (parts.size() != 3))
			return false;
		value["encodingName"] = parts[0];
		if (lowerCase((string) value["encodingName"]) == "h264") {
			value["encodingName"] = (uint64_t) CODEC_VIDEO_H264;
		} else if (lowerCase((string) value["encodingName"]) == "mpeg4-generic") {
			value["encodingName"] = (uint64_t) CODEC_AUDIO_AAC;
			value["encodingNameString"] = "mpeg4-generic";
		} else if (lowerCase((string) value["encodingName"]) == "mp4a-latm") {
			value["encodingName"] = (uint64_t) CODEC_AUDIO_AAC;
			value["encodingNameString"] = "mp4a-latm";
#ifdef HAS_G711
		} else if ((string) lowerCase(value["encodingName"]) == "pcma") {
			value["encodingName"] = (uint64_t) CODEC_AUDIO_G711A;
			value["encodingNameString"] = "pcma";
		} else if ((string) lowerCase(value["encodingName"]) == "pcmu") {
			value["encodingName"] = (uint64_t) CODEC_AUDIO_G711U;
			value["encodingNameString"] = "pcmu";
#endif	/* HAS_G711 */
		} else if (lowerCase((string)value["encodingName"]) == "mp2t") {
			value["encodingName"] = (uint64_t)CONTAINER_AUDIO_VIDEO_MP2T;
			value["encodingNameString"] = "mp2t";
		} else {
			WARN("Invalid codec: %s", STR(value["encodingName"]));
			return true;
		}
		value["clockRate"] = (uint32_t) atoi(STR(parts[1]));
		if (parts.size() == 3) {
			value["encodingParameters"] = parts[2];
		}

		return true;
	} else if (attributeName == "fmtp") {
		replace(rawValue, "; ", ";");
		vector<string> parts;
		split(rawValue, " ", parts);
		if (parts.size() != 2)
			return false;
		value["payloadType"] = (uint8_t) atoi(STR(parts[0]));
		map<string, string> temp = mapping(parts[1], ";", "=", false);

		FOR_MAP(temp, string, string, i) {
			value[MAP_KEY(i)] = MAP_VAL(i);
		}

		return true;
	} else if (attributeName == "framerate") {
		// Axis camera issue
		value = (double) strtod(STR(rawValue), NULL);
		return true;
	} else {
		WARN("Attribute `%s` with value `%s` not parsed", STR(attributeName), STR(rawValue));
		value = rawValue;
		return true;
	}
}

bool SDP::ParseSDPLineB(Variant &result, string line) {
	//b=<modifier>:<bandwidth-value>
	result.Reset();

	vector<string> parts;
	split(line, ":", parts);
	if (parts.size() != 2)
		return false;

	result["modifier"] = parts[0];
	result["value"] = parts[1];

	if (parts[0] == "AS") {
		uint32_t val = (((uint32_t) atoi(STR(parts[1])))*1024);
		result = (uint32_t) val;
	} else {
		WARN("Bandwidth modifier %s not implemented", STR(result["modifier"]));
		result = (uint32_t) 0;
	}

	return true;
}

bool SDP::ParseSDPLineC(Variant &result, string line) {
	//c=<network type> <address type> <connection address>
	result.Reset();

	vector<string> parts;
	split(line, " ", parts);
	if (parts.size() != 3)
		return false;

	result["networkType"] = parts[0];
	result["addressType"] = parts[1];
	if (parts[2].find("/") == string::npos) {
		result["connectionAddress"] = parts[2];
	} else {
		vector<string> connectParts;
		split(parts[2], "/", connectParts);
		result["connectionAddress"] = connectParts[0];
		result["ttl"] = connectParts[1];
	}
	return true;
}

bool SDP::ParseSDPLineE(Variant &result, string line) {
	result.Reset();
	result = line;
	return true;
}

bool SDP::ParseSDPLineI(Variant &result, string line) {
	result.Reset();
	result = line;
	return true;
}

bool SDP::ParseSDPLineK(Variant &result, string line) {
	result.Reset();
	NYIR;
}

bool SDP::ParseSDPLineM(Variant &result, string line) {
	//m=<media> <port> <transport> <fmt list>
	result.Reset();

	vector<string> parts;
	trim(line);
	split(line, " ", parts);
	if (parts.size() < 4)
		return false;

	result["media_type"] = parts[0];
	result["ports"] = parts[1];
	result["transport"] = parts[2];
	for (size_t i = 3; i < parts.size(); i++)
		result["payloadTypes"].PushToArray((uint32_t) atoi(parts[i].c_str()));
	return true;
}

bool SDP::ParseSDPLineO(Variant &result, string line) {
	//o=<username> <session id> <version> <network type> <address type> <address>
	result.Reset();

	vector<string> parts;
	split(line, " ", parts);
	if (parts.size() != 6)
		return false;

	result["username"] = parts[0];
	result["sessionId"] = parts[1];
	result["version"] = parts[2];
	result["networkType"] = parts[3];
	result["addressType"] = parts[4];
	result["address"] = parts[5];

	if (result["networkType"] != "IN") {
		FATAL("Unsupported network type: %s", STR(result["networkType"]));
		return false;
	}

	if (result["addressType"] != "IP4") {
		if (result["addressType"] != "IPV4") {
			FATAL("Unsupported address type: %s", STR(result["addressType"]));
			return false;
		} else {
			WARN("Tolerate IPV4 value inside line %s", STR(result["addressType"]));
			result["addressType"] = "IP4";
		}
	}

	string ip = getHostByName(result["address"]);
	if (ip == "") {
		WARN("Invalid address: %s", STR(result["address"]));
	}
	result["ip_address"] = ip;

	return true;
}

bool SDP::ParseSDPLineP(Variant &result, string line) {
	result.Reset();
	result = line;
	return true;
}

bool SDP::ParseSDPLineR(Variant &result, string line) {
	result.Reset();
	NYIR;
}

bool SDP::ParseSDPLineS(Variant &result, string line) {
	result.Reset();
	result = line;
	return true;
}

bool SDP::ParseSDPLineT(Variant &result, string line) {
	//t=<start time>  <stop time>
	result.Reset();

	vector<string> parts;
	split(line, " ", parts);
	if (parts.size() != 2)
		return false;

	result["startTime"] = parts[0];
	result["stopTime"] = parts[1];

	return true;
}

bool SDP::ParseSDPLineU(Variant &result, string line) {
	result.Reset();
	result = line;
	return true;
}

bool SDP::ParseSDPLineV(Variant &result, string line) {
	result.Reset();
	if (line != "0")
		return false;
	result = (uint32_t) 0;
	return true;
}

bool SDP::ParseSDPLineZ(Variant &result, string line) {
	result.Reset();
	NYIR;
}

void SDP::CalcVideoSourceIndex() {
	Variant &tracks = (*this)[SDP_MEDIATRACKS];
	std::map<int32_t, Variant &, std::greater<int32_t> > m;
	FOR_MAP(tracks, string, Variant, i) {
		Variant attribs = MAP_VAL(i)[SDP_A];
		if (attribs.HasKey("x-avg-params")) {
			string xAvgParams = attribs["x-avg-params"];
			size_t widthPosBegin = xAvgParams.find(" width=");
			if (widthPosBegin != string::npos) {
				widthPosBegin += 7;
				size_t widthPosEnd = xAvgParams.find(";", widthPosBegin);
				if (widthPosEnd != string::npos) {
					string sWidth = xAvgParams.substr(widthPosBegin, 
						widthPosEnd - widthPosBegin);
					uint32_t width = (uint32_t)atol(STR(sWidth));
					m.insert(std::pair<int32_t, Variant &>(width, MAP_VAL(i)));
				}
			}
		}
	}

	if (!m.empty()) {	//set everything to the first entry
		/* 
		Upon parsing the SDP, the video sources will be ordered by image width.
		If there is 1 video source, it will be used regardless of the 
			videoSourceIndex value.
		If there are 2 video sources, high will select the larger, med and low 
			will select the smaller.
		If there are 3 video sources, then high for large, med for mid, low for 
			smallest.
		if there are 4 or more video sources, then high will select the largest, 
			med will select the second largest, and low will select the 
			smallest. The rest of the sources will be inaccessible.
		*/
		std::map<int32_t, Variant &>::iterator iter = m.begin();
		tracks[SDP_VIDSRCINDEX]["high"] = iter->second;	//first video source
		if (++iter != m.end()) {	//do we have a second video source?
			tracks[SDP_VIDSRCINDEX]["med"] = iter->second;
			if (++iter != m.end()) {	//do we have a third video source?
				//get the last video source
				tracks[SDP_VIDSRCINDEX]["low"] = (--m.end())->second;	
			} else {
				tracks[SDP_VIDSRCINDEX]["low"] = tracks[SDP_VIDSRCINDEX]["med"];
			}
		} else {
			tracks[SDP_VIDSRCINDEX]["low"] = tracks[SDP_VIDSRCINDEX]["med"] = 
				tracks[SDP_VIDSRCINDEX]["high"];
		}
	}
}

Variant SDP::GetTrack(string const &videoSourceIndex, string type) {
	Variant result;
	Variant &tracks = (*this)[SDP_MEDIATRACKS];
	if (tracks.HasKey(SDP_VIDSRCINDEX)) {
		Variant &track = tracks[SDP_VIDSRCINDEX][videoSourceIndex];
		if (track[SDP_M]["media_type"] == type) {
			if (type == "video") {
				result = ParseVideoTrack(track);
			} else if (type == "audio") {
				result = ParseAudioTrack(track);
			}
		}
	} else {	//old way of doing things
		result = GetTrack(0, type);
	}

	return result;
}

Variant SDP::GetTrack(uint32_t index, string type) {
	uint32_t videoTracksCount = 0;
	uint32_t audioTracksCount = 0;
	uint32_t globalTrackIndex = 0;
	Variant result;

	FOR_MAP((*this)[SDP_MEDIATRACKS], string, Variant, i) {
		if (MAP_VAL(i)[SDP_M]["media_type"] == type) {
			if (type == "video") {
				videoTracksCount++;
				if (videoTracksCount == (index + 1)) {
					result = ParseVideoTrack(MAP_VAL(i));
					break;
				}
			} else if (type == "audio") {
				audioTracksCount++;
				if (audioTracksCount == (index + 1)) {
					result = ParseAudioTrack(MAP_VAL(i));
					break;
				}
			}
		}
		globalTrackIndex++;
	}
	if (result != V_NULL) {
		SDP_TRACK_GLOBAL_INDEX(result) = globalTrackIndex;
	}
	return result;
}

Variant SDP::GetTrackLine(uint32_t index, string type) {
	uint32_t videoTracksCount = 0;
	uint32_t audioTracksCount = 0;
	Variant result;

	FOR_MAP((*this)[SDP_MEDIATRACKS], string, Variant, i) {
		if (MAP_VAL(i)[SDP_M]["media_type"] == type) {
			if (type == "video") {
				videoTracksCount++;
				INFO("\n\nSDP::GetTrackLine Video found videoTracksCount %d index %d\n\n", videoTracksCount, index);
				if (videoTracksCount == (index + 1)) {
					result = MAP_VAL(i);
					break;
				}
			} else if (type == "audio") {
				audioTracksCount++;
				INFO("\n\nSDP::GetTrack Audio found audioTracksCount %d index %d\n\n", audioTracksCount, index);
				if (audioTracksCount == (index + 1)) {
					result = MAP_VAL(i);
					break;
				}
			}
		}
	}

	return result;
}

Variant SDP::ParseVideoTrack(Variant &track) {
	Variant result = track;
	if ((uint64_t)result[SDP_A]["rtpmap"]["encodingName"] == CONTAINER_AUDIO_VIDEO_MP2T)
		return result;

	if (!result.HasKey(SDP_A)) {
		FATAL("Track with no attributes");
		return Variant();
	}
	if (!result[SDP_A].HasKey("control", false)) {
		FATAL("Track with no control uri");
		return Variant();
	}
	if (!result[SDP_A].HasKey("rtpmap", false)) {
		FATAL("Track with no rtpmap");
		return Variant();
	}
	if (!result[SDP_A].HasKey("fmtp", false)) {
		FATAL("Track with no fmtp");
		return Variant();
	}
	Variant &fmtp = result[SDP_A].GetValue("fmtp", false);

	if (!fmtp.HasKey("sprop-parameter-sets", false)) {
		FATAL("Video doesn't have sprop-parameter-sets");
		return Variant();
	}
	Variant &temp = fmtp.GetValue("sprop-parameter-sets", false);
	vector<string> parts;
	split((string) temp, ",", parts);
	if (parts.size() != 2) {
		FATAL("Video doesn't have sprop-parameter-sets");
		return Variant();
	}
	temp.Reset();
	temp["SPS"] = parts[0];
	temp["PPS"] = parts[1];

	return result;
}

Variant SDP::ParseAudioTrack(Variant &track) {
	Variant result = track;

	if (!result.HasKey(SDP_A)) {
		FATAL("Track with no attributes");
		return Variant();
	}
	if (!result[SDP_A].HasKey("control", false)) {
		FATAL("Track with no control uri");
		return Variant();
	}
	if (!result[SDP_A].HasKey("rtpmap", false)) {
		FATAL("Track with no rtpmap");
		return Variant();
	}

	string encodingName = "";

#ifdef HAS_G711
	uint8_t payloadType = (uint8_t)result[SDP_A]["rtpmap"]["payloadType"];
	if (!STATIC_TYPE(payloadType)) {
#endif	/* HAS_G711 */
		if (!result[SDP_A].HasKey("fmtp", false)) {
			FATAL("Track with no fmtp uri");
			return Variant();
		}

		if (!track.HasKeyChain(V_STRING, true, 3, SDP_A, "rtpmap", "encodingNameString")) {
			FATAL("Track with no encoding name");
			return Variant();
		}
		encodingName = (string) track[SDP_A]["rtpmap"]["encodingNameString"];
#ifdef HAS_G711
	} else {
		// check if there is no encodingNameString
		if (track.HasKeyChain(V_MAP, false, 2, SDP_A, "rtpmap")) {
			encodingName = (string) track[SDP_A]["rtpmap"]["encodingNameString"];
		} else {
			if (payloadType == PAYLOAD_TYPE_PCMA) {
				result[SDP_A]["rtpmap"]["encodingName"] = CODEC_AUDIO_G711A;
				result[SDP_A]["rtpmap"]["encodingNameString"] = "pcma";
				encodingName = "pcma";
			} else if (payloadType == PAYLOAD_TYPE_PCMU) {
				result[SDP_A]["rtpmap"]["encodingName"] = CODEC_AUDIO_G711U;
				result[SDP_A]["rtpmap"]["encodingNameString"] = "pcmu";
				encodingName = "pcmu";
			}
		}
	}
#endif	/* HAS_G711 */

	Variant &fmtp = result[SDP_A].GetValue("fmtp", false);

	if (encodingName == "mpeg4-generic") {
		if (!fmtp.HasKey("config", false)) {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
		if (!fmtp.HasKey("mode", false)) {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
		if (lowerCase((string) fmtp.GetValue("mode", false)) != "aac-hbr") {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
		if (!fmtp.HasKey("SizeLength", false)) {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
		if (fmtp.GetValue("sizelength", false) != "13") {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
		if (fmtp.HasKey("IndexLength", false)) {
			if (fmtp.GetValue("IndexLength", false) != "3") {
				FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
				return Variant();
			}
		}
		if (fmtp.HasKey("IndexDeltaLength", false)) {
			if (fmtp.GetValue("IndexDeltaLength", false) != "3") {
				FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
				return Variant();
			}
		}
	} else if (encodingName == "mp4a-latm") {
		bool cpresent = true;
		if (fmtp.HasKey("cpresent", false)) {
			cpresent = fmtp.GetValue("cpresent", false) == "1";
		}

		if (cpresent) {
			FATAL("cpresent not yet implemented");
			return Variant();
		}

		if ((!fmtp.HasKeyChain(V_STRING, false, 1, "config"))
				|| ((string) fmtp.GetValue("config", false) == "")) {
			FATAL("Invalid fmtp line:\n%s", STR(fmtp.ToString()));
			return Variant();
		}
#ifdef HAS_G711
	} else if (encodingName.length() == 4 &&
			(encodingName.substr(0, 4) == "pcma" || encodingName.substr(0, 4) == "pcmu")) {
		if (false)
			return Variant();
#endif	/* HAS_G711 */
	} else {

		FATAL("Track encoding not supported %s", STR(encodingName));
		return Variant();
	}

	return result;
}

void SDP::NormalizeMedia(Variant &dest, Variant &src, bool includeAll) {

	FOR_MAP(src[SDP_M]["payloadTypes"], string, Variant, i) {
		Variant media = src;
		media[SDP_M].RemoveKey("payloadTypes");
		media[SDP_M]["payloadType"] = MAP_VAL(i);
		media[SDP_A]["rtpmap"] = src[SDP_A]["rtpmap"][(uint32_t)MAP_VAL(i)];
#ifdef HAS_G711
		SetDefaultAttributes(media);
#endif	/* HAS_G711 */
		media[SDP_A]["fmtp"] = src[SDP_A]["fmtp"][(uint32_t)MAP_VAL(i)];
		if (!includeAll) {
			if ((VariantType)(media[SDP_A]["rtpmap"]["encodingName"]) != V_UINT64) {
				continue;
			}
		}
		dest.PushToArray(media);
	}
}
#ifdef HAS_G711
bool SDP::AllowedCodec(uint64_t codecType) {
	return (codecType == CODEC_AUDIO_AAC)
		|| (codecType == CODEC_AUDIO_G711A)
		|| (codecType == CODEC_AUDIO_G711U);
}
void SDP::SetDefaultAttributes(Variant &media) {
	if (media[SDP_M]["rtpmap"] != V_NULL)
		return;
	switch((uint64_t) media[SDP_M]["payloadType"]) {
		case 0:	
			media[SDP_A]["rtpmap"]["encodingName"] = CODEC_AUDIO_G711U;
			media[SDP_A]["rtpmap"]["encodingNameString"] = "pcmu";
			media[SDP_A]["rtpmap"]["clockRate"] = 8000;
			media[SDP_A]["rtpmap"]["encodingParameters"] = 1;
			break;
		case 1:
			media[SDP_A]["rtpmap"]["encodingName"] = CODEC_AUDIO_G711A;
			media[SDP_A]["rtpmap"]["encodingNameString"] = "pcma";
			media[SDP_A]["rtpmap"]["clockRate"] = 8000;
			media[SDP_A]["rtpmap"]["encodingParameters"] = 1;
			break;
	}
}
#endif	/* HAS_G711 */
#endif /* HAS_PROTOCOL_RTP */
