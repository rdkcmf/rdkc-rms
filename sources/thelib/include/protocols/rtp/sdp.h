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


#ifdef HAS_PROTOCOL_RTP
#ifndef _SDP_H
#define	_SDP_H

#include "common.h"

#define SDP_SESSION		"session"
#define SDP_MEDIATRACKS	"mediaTracks"
#define SDP_A			"attributes"
#define SDP_B			"bandwidth"
#define SDP_C			"connection"
#define SDP_E			"email"
#define SDP_I			"sessionInfo"
#define SDP_K			"encryptionKey"
#define SDP_M			"media"
#define SDP_O			"owner"
#define SDP_P			"phone"
#define SDP_R			"repeat"
#define SDP_S			"sessionName"
#define SDP_T			"startStopTime"
#define SDP_U			"descriptionUri"
#define SDP_V			"version"
#define SDP_Z			"timeZone"
#define SDP_VIDSRCINDEX	"vsi"

#define SDP_TRACK_GLOBAL_INDEX(x)	((x)["globalTrackIndex"])
#define SDP_TRACK_IS_AUDIO(x)		((x)["isAudio"])
#define SDP_TRACK_BANDWIDTH(x)		((x)["bandwidth"])
#define SDP_TRACK_CLOCKRATE(x)		((x)["clockRate"])
#define SDP_VIDEO_SERVER_IP(x)		((x)["ip"])
#define SDP_VIDEO_CONTROL_URI(x)	((x)["controlUri"])
#define SDP_VIDEO_CODEC(x)			((x)["codec"])
#define SDP_VIDEO_CODEC_H264_SPS(x)	((x)["h264SPS"])
#define SDP_VIDEO_CODEC_H264_PPS(x)	((x)["h264PPS"])
#define SDP_VIDEO_FRAME_RATE(x)		((x)["framerate"])
#define SDP_AUDIO_SERVER_IP(x)		((x)["ip"])
#define SDP_AUDIO_CONTROL_URI(x)	((x)["controlUri"])
#define SDP_AUDIO_CODEC(x)			((x)["codec"])
#define SDP_AUDIO_CODEC_SETUP(x)	((x)["codecSetup"])
#define SDP_AUDIO_TRANSPORT(x)		((x)["encodingNameString"])

class DLLEXP SDP
: public Variant {
public:
	SDP();
	virtual ~SDP();

	static bool ParseSDP(SDP &sdp, string &raw, bool includeAll);
	Variant GetVideoTrack(string const &videoSourceIndex, string contentBase);
	Variant GetAudioTrack(string const &videoSourceIndex, string contentBase);
	string GetStreamName();
	uint32_t GetTotalBandwidth();
	static bool ParseTransportLine(string raw, Variant &result);
	static bool ParseTransportLinePart(string raw, Variant &result);

	// to get the track line regardless of its contents
	Variant GetTrackLine(uint32_t index, string type);

private:
	static bool ParseSection(Variant &result, vector<string> &lines,
			uint32_t start, uint32_t length);
	static bool ParseSDPLine(Variant &result, string &line);
	static bool ParseSDPLineA(string &attributeName, Variant &value, string line);
	static bool ParseSDPLineB(Variant &result, string line);
	static bool ParseSDPLineC(Variant &result, string line);
	static bool ParseSDPLineE(Variant &result, string line);
	static bool ParseSDPLineI(Variant &result, string line);
	static bool ParseSDPLineK(Variant &result, string line);
	static bool ParseSDPLineM(Variant &result, string line);
	static bool ParseSDPLineO(Variant &result, string line);
	static bool ParseSDPLineP(Variant &result, string line);
	static bool ParseSDPLineR(Variant &result, string line);
	static bool ParseSDPLineS(Variant &result, string line);
	static bool ParseSDPLineT(Variant &result, string line);
	static bool ParseSDPLineU(Variant &result, string line);
	static bool ParseSDPLineV(Variant &result, string line);
	static bool ParseSDPLineZ(Variant &result, string line);
	Variant GetTrack(uint32_t index, string type);
	Variant GetTrack(string const &videoSourceIndex, string type);
	Variant ParseVideoTrack(Variant &track);
	Variant ParseAudioTrack(Variant &track);
	static void NormalizeMedia(Variant &dest, Variant &src, bool includeAll);
#ifdef HAS_G711
	static bool AllowedCodec(uint64_t codecType);
	static void SetDefaultAttributes(Variant &media);
#endif	/* HAS_G711 */
	void CalcVideoSourceIndex();
};


#endif	/* _SDP_H */
#endif /* HAS_PROTOCOL_RTP */


