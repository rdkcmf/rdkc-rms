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

#ifndef __WRTCSDP_H
#define __WRTCSDP_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef RDKC
#include "sysUtils.h"
#endif
#ifdef __cplusplus
}
#endif

//#define GENERATE_PCAP_FILTER_FROM_SDP

class X509Certificate;
class OutNetRTPUDPH264Stream;
typedef enum {
	SDP_TYPE_CONTROLLING,
	SDP_TYPE_CONTROLLED
} SDPType;

struct SDPInfo {
	string fingerprint;
	string iceUsername;
	string icePassword;
	string mid;
	string sessionInfo;

	SDPInfo() {
		mid = "data";
	}

	~SDPInfo() {
	}
};

struct StreamInfo {
	struct {
		bool active;
		uint32_t dynamicType;
		uint32_t ssrc;
		string fmtp;
		string ptAttrib;
		Variant attributes;
	} video, audio;
	StreamInfo() {
		video.active = false;
		audio.active = false;
	}
};
class WrtcSDP {
private:
	SDPType _type;
	X509Certificate *_pCertificate;
	uint16_t _sctpPort;
	uint16_t _maxSctpChannels;
	uint32_t _id;
	uint32_t _version;
	bool _enabled;
	string _sessionName;
	string _iceUsername;
	string _icePassword;
	string _sdp;
	StreamInfo _streamInfo;
public:
	WrtcSDP(SDPType type, X509Certificate *pCertificate, uint16_t sctpPort = 5000,
			uint16_t maxSctpChannels = 4);
	virtual ~WrtcSDP();

	SDPType GetType() const;
	void Enable();
	const bool IsEnabled() const;
	string & GenerateSDP(SDPInfo *pInfo, bool hasVideo, bool hasAudio, bool hasDataChannel);
	void SetSourceStream(OutNetRTPUDPH264Stream *stream);
	const string &GetContent() const;
	uint32_t GetVersion() const;
	X509Certificate * GetCertificate() { return _pCertificate;};
	string &GetICEUsername() ;
	string &GetICEPassword() ;
	const string &GetFingerprint();
	uint16_t GetSCTPPort() const;
	uint16_t GetSCTPMaxChannels() const;
//	static SDPInfo *ParseSdpStr(const string &sdpStr);
	static SDPInfo *ParseSdpStr2(const string &sdpStr);
private:
	static bool GetValue(const string & sdp, string keyName, string & val, bool skipNextChar =true);
	static bool ReadToken(const string &src, string &dst, const char *pName,
			size_t nameLength);
	string GenerateVideoDescription();
	string GenerateAudioDescription();
	string GenerateDataChannelDescription(SDPInfo *pInfo);
	string GenerateConnectionDescriptionAttributes();
};

#endif // __WRTCSDP_H

