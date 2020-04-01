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

#include "protocols/wrtc/wrtcsdp.h"
#include "utils/misc/x509certificate.h"
#include "protocols/rtp/streaming/outnetrtpudph264stream.h"
#include "streaming/codectypes.h"

#define SDP_TOKEN_FINGERPRINT_SHA256 "a=fingerprint:sha-256"
#define SDP_TOKEN_FINGERPRINT_SHA256_LEN 21
#define SDP_TOKEN_FINGERPRINT_SHA1 "a=fingerprint:sha-1"
#define SDP_TOKEN_FINGERPRINT_SHA1_LEN 19
#define SDP_TOKEN_CANDIDATE "a=candidate:"
#define SDP_TOKEN_CANDIDATE_LEN 12
#define SDP_TOKEN_ICE_UFRAG "a=ice-ufrag:"
#define SDP_TOKEN_ICE_UFRAG_LEN 12
#define SDP_TOKEN_ICE_PWD "a=ice-pwd:"
#define SDP_TOKEN_ICE_PWD_LEN 10
#define SDP_TOKEN_M_APPLICATION "m=application "
#define SDP_TOKEN_M_APPLICATION_LEN 14

//Redmine 2269
#if OPENSSL_API_COMPAT < 0x10100000L
#else
#define RAND_pseudo_bytes RAND_bytes
#endif

//#define NO_AUDIO

WrtcSDP::WrtcSDP(SDPType type, X509Certificate *pCertificate, uint16_t sctpPort,
		uint16_t maxSctpChannels) {
	_type = type;
	_pCertificate = pCertificate;
	_sctpPort = sctpPort;
	_maxSctpChannels = maxSctpChannels;
	_id = 0;
	_version = 2;
	_enabled = false;
	_sessionName = "RDKC_WebRTC";
	uint8_t randBytes[18]; //the passwd must be 24 character after b64
	RAND_pseudo_bytes((uint8_t *) & randBytes, sizeof (randBytes));
	_iceUsername = b64(randBytes, 12);
	RAND_pseudo_bytes((uint8_t *) & randBytes, sizeof (randBytes));
	_icePassword = b64(randBytes, 18);
}

WrtcSDP::~WrtcSDP() {
}

SDPType WrtcSDP::GetType() const {
	return _type;
}

void WrtcSDP::Enable() {
	_enabled = true;
}

const bool WrtcSDP::IsEnabled() const {
	return _enabled;
}

void WrtcSDP::SetSourceStream(OutNetRTPUDPH264Stream *stream) {
	if (stream == NULL) {
		WARN("SetSourceStream was called with null input.");
		return;
	}
	
	_streamInfo.audio.active = stream->GetCapabilities()->GetAudioCodec() != NULL;
	if (_streamInfo.audio.active) {
		_streamInfo.audio.ssrc = stream->AudioSSRC();
#ifdef HAS_G711
		uint64_t audioCodecType = stream->GetCapabilities()->GetAudioCodecType();
		if ((audioCodecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
			//AudioCodecInfoG711 *pInfo = stream->GetCapabilities()->GetAudioCodec<AudioCodecInfoG711>();

			_streamInfo.audio.dynamicType = audioCodecType == CODEC_AUDIO_G711U ? 0 : 8;
			stream->SetPayloadType(_streamInfo.audio.dynamicType, true);
		}
		else {
#endif /* HAS_G711 */
			AudioCodecInfoAAC *pInfo = stream->GetCapabilities()->GetAudioCodec<AudioCodecInfoAAC>();
			if (pInfo != NULL) {
				// By default, we mask AAC as PCMU
				_streamInfo.audio.dynamicType = 0;
				stream->SetPayloadType(_streamInfo.audio.dynamicType, true);
				_streamInfo.audio.attributes["rtpmap"] = format("a=rtpmap:%"PRIu8" PCMU/8000\\r\\n", _streamInfo.audio.dynamicType);
				_streamInfo.audio.attributes["ext-rtpmap"] = format("a=ext-rtpmap:%"PRIu8" mpeg4-generic/%"PRIu32"/2\\r\\n",
					_streamInfo.audio.dynamicType, pInfo->_samplingRate);
				_streamInfo.audio.attributes["ext-ftmp"] = format("a=ext-fmtp:%"PRIu8" streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3;\\r\\n",
					_streamInfo.audio.dynamicType, STR(hex(pInfo->_pCodecBytes, (uint32_t) pInfo->_codecBytesLength)));
			}
#ifdef HAS_G711
		}
#endif	/* HAS_G711 */

		string ssrcAttrib = format("a=ssrc:%"PRIu32, _streamInfo.audio.ssrc);
		string ssrc = ssrcAttrib + " cname:sSOBdBnFEEGXD121\\r\\n";
		ssrc += ssrcAttrib + " msid:ECFIs7KiWTbrCE99uqsqDeR3lwMZWJaokoCP ef59c9e2-23b7-4d93-a852-4a20a448d682\\r\\n";
		ssrc += ssrcAttrib + " mslabel:ECFIs7KiWTbrCE99uqsqDeR3lwMZWJaokoCP\\r\\n";
		ssrc += ssrcAttrib + " label:ef59c9e2-23b7-4d93-a852-4a20a448d682\\r\\n";
		_streamInfo.audio.attributes["ssrc"] = ssrc;

#ifdef HAS_G711

#endif	/* HAS_G711 */
	} else {
		WARN("Audio stream info is NOT active.");
	}
	
	_streamInfo.video.active = stream->GetCapabilities()->GetVideoCodec() != NULL;
	if (_streamInfo.video.active) {
		_streamInfo.video.ssrc = stream->VideoSSRC();
		_streamInfo.video.dynamicType = stream->GetPayloadType(false);
		VideoCodecInfoH264 *pInfo = stream->GetCapabilities()->GetVideoCodec<VideoCodecInfoH264 >();
		if (pInfo != NULL) {
#if 0
			_streamInfo.video.fmtp = format("a=fmtp:%"PRIu8" profile-level-id=", _streamInfo.video.dynamicType);
			_streamInfo.video.fmtp += hex(pInfo->_pSPS + 1, 3);
			_streamInfo.video.fmtp += "; packetization-mode=1; sprop-parameter-sets=";
			_streamInfo.video.fmtp += b64(pInfo->_pSPS, pInfo->_spsLength) + ",";
			_streamInfo.video.fmtp += b64(pInfo->_pPPS, pInfo->_ppsLength);
			_streamInfo.video.ptAttrib = format("a=rtpmap:%"PRIu8" H264/90000\\r\\n", _streamInfo.video.dynamicType);
#else
			string fmtp = format("a=fmtp:%"PRIu8" profile-level-id=", _streamInfo.video.dynamicType);
			// We are hard coding this value because of two reasons:
			// 1. SRTP browsers would fail on the webrtc establishment because it fails to somehow support some of the values;
			//    but would still decode it properly
			// 2. iOS players would fail to render when they receive certain profile level IDs; but would decode it anyway
			//    even though a different ID was passed
			// Value is from our beloved bunny source file
			string SPS = hex(pInfo->_pSPS + 1, 3);
			INFO("WrtcSDP::SetSourceStream SPS is: %s", STR(SPS));
			fmtp += "42c01e";
			fmtp += "; packetization-mode=1; sprop-parameter-sets=";
			fmtp += b64(pInfo->_pSPS, pInfo->_spsLength) + ",";
			fmtp += b64(pInfo->_pPPS, pInfo->_ppsLength) + "\\r\\n";
			string ssrcAttrib = format("a=ssrc:%"PRIu32, _streamInfo.video.ssrc);
			string ssrc = ssrcAttrib + " cname:sSOBdBnFEEGXD121\\r\\n";
			ssrc += ssrcAttrib + " msid:ECFIs7KiWTbrCE99uqsqDeR3lwMZWJaokoCP ea2bbfae-6d4f-44ee-b0e2-da56677b6f27\\r\\n";
			ssrc += ssrcAttrib + " mslabel:ECFIs7KiWTbrCE99uqsqDeR3lwMZWJaokoCP\\r\\n";
			ssrc += ssrcAttrib + " label:ea2bbfae-6d4f-44ee-b0e2-da56677b6f27\\r\\n";
			_streamInfo.video.attributes["ftmp"] = fmtp;
			_streamInfo.video.attributes["rtpmap"] = format("a=rtpmap:%"PRIu8" H264/90000\\r\\n", _streamInfo.video.dynamicType);
			_streamInfo.video.attributes["rtcp-fb"] = format("a=rtcp-fb:%"PRIu8" nack\\r\\n", _streamInfo.video.dynamicType);
			_streamInfo.video.attributes["ssrc"] = ssrc;
#endif
		}
	} else {
		WARN("Video stream info is NOT active.");
	}
}

string & WrtcSDP::GenerateSDP(SDPInfo *pInfo, bool hasVideo, bool hasAudio, bool hasDataChannel) {
	//see if the SDP generation is enabled
	//if (!_enabled)
	//	return WRTC_ERR_OK;

	//reset the enabled flag
	_enabled = false;

#ifdef NO_VIDEO
	hasVideo = false;
#endif
#ifdef NO_AUDIO
	hasAudio = false;
#endif

	hasVideo &= _streamInfo.video.active;
	hasAudio &= _streamInfo.audio.active;

	//compute the session id if needed, and make sure is a big
	//number by setting the very first bit of it
	if (_id == 0) {
		RAND_pseudo_bytes((uint8_t *) & _id, sizeof (_id));
		_id |= 0x80000000ULL;
	}

	//reset the currently computed sdp
	_sdp = "";

	//https://tools.ietf.org/html/rfc4566#section-5.1
	_sdp += "v=0\\r\\n";

	//https://tools.ietf.org/html/rfc4566#section-5.2
	_sdp += format("o=- %"PRIu64" %"PRIu32" IN IP4 127.0.0.1\\r\\n", _id, _version);
	_version++;

#ifdef RDKC
	char fw_name[FW_NAME_MAX_LENGTH] = "";
        if (getCameraFirmwareVersion(fw_name) == -1) {
		FATAL(" ERROR in reading camera firmware version.");
	}
#else
	char fw_name[23] = "RDKC_Media_Server";
#endif
	//https://tools.ietf.org/html/rfc4566#section-5.3
	_sdp += format("s=%s %s\\r\\n", fw_name, _sessionName.c_str());

	//https://tools.ietf.org/html/rfc4566#section-5.9
	_sdp += "t=0 0\\r\\n";

	//https://tools.ietf.org/html/draft-alvestrand-rtcweb-msid-02#section-3
	_sdp += "a=msid-semantic: WMS\\r\\n";
	//TODO: might need to generate string after WMS from here https://tools.ietf.org/html/draft-ietf-mmusic-msid-13
	
	// Support trickle
	_sdp += "a=ice-options:trickle\\r\\n";

	if (hasAudio || hasVideo || hasDataChannel) {
		_sdp += "a=group:BUNDLE";
		_sdp += hasAudio ? " audio" : "";
		_sdp += hasVideo ? " video" : "";
		_sdp += hasDataChannel ? " data" : "";
		_sdp += "\\r\\n";
	} else {
		FATAL("Could not create valid SDP offer!");
	}

	if (hasAudio) {
		_sdp += GenerateAudioDescription();
	}

	if (hasVideo) {
		_sdp += GenerateVideoDescription();
	}

	if (hasDataChannel) {
		_sdp += GenerateDataChannelDescription(pInfo);
	}

	return _sdp;
}

string WrtcSDP::GenerateVideoDescription() {
	if (!_streamInfo.video.active)
		return "";

	string videoDesc = "";
	//https://tools.ietf.org/html/rfc5124
	//http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-1
	videoDesc += "m=video 9 UDP/TLS/RTP/SAVPF 107\\r\\n";

	//https://tools.ietf.org/html/rfc4566#section-5.7
	videoDesc += format("c=IN IP4 0.0.0.0\\r\\n");

	// RTCP port
	videoDesc += "a=rtcp:9 IN IP4 0.0.0.0\\r\\n";

	videoDesc += GenerateConnectionDescriptionAttributes();

	//https://tools.ietf.org/html/rfc3388
	videoDesc += "a=mid:video\\r\\n";

	//TODO: check if extmap is needed
	//a=extmap:2 urn:ietf:params:rtp-hdrext:toffset
	//a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time

	// Stream will be one direction only
	videoDesc += "a=sendonly\\r\\n";

	// Allow RTCP to be multiplexed with RTP port
	videoDesc += "a=rtcp-mux\\r\\n";

	FOR_MAP(_streamInfo.video.attributes, string, Variant, i) {
		videoDesc += (string)MAP_VAL(i);
	}
	return videoDesc;
}

string WrtcSDP::GenerateAudioDescription() {
	if (!_streamInfo.audio.active)
		return "";
	string audioDesc = "";
	//https://tools.ietf.org/html/rfc5124
	//http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-1
	audioDesc += format("m=audio 9 UDP/TLS/RTP/SAVPF %"PRIu8"\\r\\n", _streamInfo.audio.dynamicType);

	//https://tools.ietf.org/html/rfc4566#section-5.7
	audioDesc += format("c=IN IP4 0.0.0.0\\r\\n");

	// RTCP port
	audioDesc += "a=rtcp:9 IN IP4 0.0.0.0\\r\\n";

	audioDesc += GenerateConnectionDescriptionAttributes();

	//https://tools.ietf.org/html/rfc3388
	audioDesc += "a=mid:audio\\r\\n";

	//TODO: check if extmap is needed
	//a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
	//a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time

#ifdef PLAYBACK_SUPPORT
	// Stream will be both direction only
	audioDesc += "a=sendrecv\\r\\n";
#else
	// Stream will be one direction only
	audioDesc += "a=sendonly\\r\\n";
#endif

	// Allow RTCP to be multiplexed with RTP port
	audioDesc += "a=rtcp-mux\\r\\n";

	FOR_MAP(_streamInfo.audio.attributes, string, Variant, i) {
		audioDesc += (string)MAP_VAL(i);
	}
	if (_streamInfo.audio.attributes != V_NULL) {
		FOR_MAP(_streamInfo.audio.attributes, string, Variant, i) {
			audioDesc += (string)MAP_VAL(i);
		}
	}
	audioDesc += "a=maxptime:60\\r\\n";

	return audioDesc;
}

string WrtcSDP::GenerateDataChannelDescription(SDPInfo *pInfo) {
	string dataDesc = "";

	//https://tools.ietf.org/html/rfc4566#section-5.14
	dataDesc += format("m=application 9 DTLS/SCTP %"PRIu16"\\r\\n", _sctpPort);

	//https://tools.ietf.org/html/rfc4566#section-5.7
	dataDesc += format("c=IN IP4 0.0.0.0\\r\\n");

	// observed in Mozilla answer
	//b2: not in chrome 17-aug
	//dataDesc += "a=sendrecv\\r\\n";

	// Optional bandwidth
	dataDesc += "b=AS:30\\r\\n";

	dataDesc += GenerateConnectionDescriptionAttributes();

	//https://tools.ietf.org/html/rfc3388
	//string midData = pInfo ? pInfo->mid : "sdparta_0";
	//b2: 18aug - change our default to "data"
	string midData = pInfo ? pInfo->mid : "data";
	dataDesc += format("a=mid:%s\\r\\n", STR(midData));

	//https://tools.ietf.org/html/rfc2327
	dataDesc += format("a=sctpmap:%"PRIu16" webrtc-datachannel %"PRIu16"\\r\\n", _sctpPort,
		_maxSctpChannels);

	return dataDesc;
}

string WrtcSDP::GenerateConnectionDescriptionAttributes() {
	string attrib = "";
	//https://tools.ietf.org/html/rfc5245#section-15.4
	//same for all tracks
	attrib += format("a=ice-ufrag:%s\\r\\n", _iceUsername.c_str());

	//https://tools.ietf.org/html/rfc5245#section-15.4
	//same for all tracks
	attrib += format("a=ice-pwd:%s\\r\\n", _icePassword.c_str());

	//	//https://tools.ietf.org/html/rfc5245#section-15.5
	//	attrib += format("a=ice-options:google-ice\\r\\n");

	if (NULL != _pCertificate) {
		//http://tools.ietf.org/html/rfc4572#section-5
		attrib += format("a=fingerprint:sha-256 %s\\r\\n", _pCertificate->GetSHA256FingerprintString().c_str());
	}

	//https://tools.ietf.org/html/rfc6135#section-4.2
	attrib += format("a=setup:%s\\r\\n", _type == SDP_TYPE_CONTROLLING ? "actpass" : "active");

	return attrib;
}

const string &WrtcSDP::GetContent() const {
	return _sdp;
}

uint32_t WrtcSDP::GetVersion() const {
	return _version - 1;
}

string &WrtcSDP::GetICEUsername()  {
	return _iceUsername;
}

string &WrtcSDP::GetICEPassword()  {
	return _icePassword;
}

const string & WrtcSDP::GetFingerprint() {
	return _pCertificate->GetSHA1FingerprintString();
}

uint16_t WrtcSDP::GetSCTPPort() const {
	return _sctpPort;
}

uint16_t WrtcSDP::GetSCTPMaxChannels() const {
	return _maxSctpChannels;
}

/****************************************
SDPInfo *WrtcSDP::ParseSdpStr(const string &sdpStr) {
	//check if we have any content in the sdpStr
	if (sdpStr.size() == 0) {
		FATAL("Wrtc ParseSdpStr called with null SDP to parse");
		return NULL;
	}

	//local variables needed for parsing
	vector<string> lines;
	vector<string> sessionAttributes;
	vector<vector<string> > mediaStreamsAttributes;
	ssize_t tracksIndex = -1;

	//split the sdpStr into lines
	split(sdpStr, "\n", lines);

	//distribute the lines over the session attributes and streams attributes
	for (size_t i = 0; i < lines.size(); i++) {
		std::string &line = lines[i];
		trim(line);
		if (line == "")
			continue;
		WARN("Checking SDP Line: %s", STR(line));
		if ((line.size() > 2)&&(line[0] == 'm')&&(line[1] == '=')) {
			mediaStreamsAttributes.push_back(vector<string>());
			tracksIndex = mediaStreamsAttributes.size() - 1;
		}
		if (tracksIndex < 0)
			sessionAttributes.push_back(line);
		else
			mediaStreamsAttributes[tracksIndex].push_back(line);
	}

	SDPInfo *pInfo = new SDPInfo();

	//cycle over the session attributes and gather required information
	for (size_t i = 0; i < sessionAttributes.size(); i++) {
		trim(sessionAttributes[i]);
		if ((sessionAttributes[i].size() < 2)
				|| (sessionAttributes[i][0] != 'a')
				|| (sessionAttributes[i][1] != '='))
			continue;
		if ((pInfo->iceUsername.size() == 0)
				&& ReadToken(sessionAttributes[i], pInfo->iceUsername, SDP_TOKEN_ICE_UFRAG, SDP_TOKEN_ICE_UFRAG_LEN))
			continue;
		if ((pInfo->icePassword.size() == 0)
				&& ReadToken(sessionAttributes[i], pInfo->icePassword, SDP_TOKEN_ICE_PWD, SDP_TOKEN_ICE_PWD_LEN))
			continue;
		if ((pInfo->fingerprint.size() == 0)
				&& ReadToken(sessionAttributes[i], pInfo->fingerprint, SDP_TOKEN_FINGERPRINT_SHA256, SDP_TOKEN_FINGERPRINT_SHA256_LEN))
			continue;
		if ((pInfo->fingerprint.size() == 0)
				&& ReadToken(sessionAttributes[i], pInfo->fingerprint, SDP_TOKEN_FINGERPRINT_SHA1, SDP_TOKEN_FINGERPRINT_SHA1_LEN))
			continue;
	}

	//compute the m= token
	string dummy;

	//cycle over the tracks attributes and gather the required information
	for (size_t i = 0; i < mediaStreamsAttributes.size(); i++) {
		//ignore all media streams with the wrong componentId
		if ((mediaStreamsAttributes[i].size() < 1)
				|| (mediaStreamsAttributes[i][0].size() < 2)
				|| (mediaStreamsAttributes[i][0][0] != 'm')
				|| (mediaStreamsAttributes[i][0][1] != '=')
				|| (!ReadToken(mediaStreamsAttributes[i][0], dummy, SDP_TOKEN_M_APPLICATION, SDP_TOKEN_M_APPLICATION_LEN)))
			continue;

		for (size_t j = 1; j < mediaStreamsAttributes[i].size(); j++) {
			//ignore all media attributes not starting with "a="
			if ((mediaStreamsAttributes[i][j].size() < 2)
					|| (mediaStreamsAttributes[i][j][0] != 'a')
					|| (mediaStreamsAttributes[i][j][1] != '='))
				continue;

			//read the username, password and fingerprint
			if ((pInfo->iceUsername.size() == 0)
					&& ReadToken(mediaStreamsAttributes[i][j], pInfo->iceUsername, SDP_TOKEN_ICE_UFRAG, SDP_TOKEN_ICE_UFRAG_LEN))
				continue;
			if ((pInfo->icePassword.size() == 0)
					&& ReadToken(mediaStreamsAttributes[i][j], pInfo->icePassword, SDP_TOKEN_ICE_PWD, SDP_TOKEN_ICE_PWD_LEN))
				continue;
			if ((pInfo->fingerprint.size() == 0)
					&& ReadToken(mediaStreamsAttributes[i][j], pInfo->fingerprint, SDP_TOKEN_FINGERPRINT_SHA256, SDP_TOKEN_FINGERPRINT_SHA256_LEN))
				continue;
			if ((pInfo->fingerprint.size() == 0)
					&& ReadToken(mediaStreamsAttributes[i][j], pInfo->fingerprint, SDP_TOKEN_FINGERPRINT_SHA1, SDP_TOKEN_FINGERPRINT_SHA1_LEN))
				continue;
		}
	}

	//do the final integrity checks
	if ((pInfo->fingerprint == "")
			|| (pInfo->iceUsername == "")
			|| (pInfo->icePassword == "")) {
		delete pInfo;
		FATAL("Wrtc ParseSdpStr bad Parse!!");		if (!pInfo->fingerprint.size()) {
			FATAL("No Fingerprint found");
		}
		if (!pInfo->iceUsername.size()) {
			FATAL("No iceUsername found");
		}
		if (!pInfo->icePassword.size()) {
			FATAL("No icePassword found");
		}
		//errorCode = WRTC_ERR_SDP_PARSE_ERROR;
		return NULL;
	}else {
		WARN("$b2$ ParsedSDP: Usr: %s, Pwd: %s",
				STR(pInfo->iceUsername), STR(pInfo->icePassword));
	}

	//done
	//errorCode = WRTC_ERR_OK;
	return pInfo;
}
*************************************************/

bool WrtcSDP::ReadToken(const string &src, string &dst, const char *pName, size_t nameLength) {
	//FINEST("src: `%s`; pName: %s", src.c_str(), pName);
	if ((src.size() < nameLength) || (src.substr(0, nameLength) != pName))
		return false;
	dst = src.substr(nameLength);
	trim(dst);
	return true;
}

SDPInfo *WrtcSDP::ParseSdpStr2(const string &sdpStr) {
	SDPInfo *pInfo = new SDPInfo();

	if(!GetValue(sdpStr,string("s="), pInfo->sessionInfo, false)) {
		FATAL("No s= line found");
	}

	if ( GetValue(sdpStr,string("a=fingerprint"), pInfo->fingerprint)
	  && GetValue(sdpStr,string("a=ice-ufrag"), pInfo->iceUsername)
	  && GetValue(sdpStr,string("a=ice-pwd"), pInfo->icePassword)
	  ){
		// all a=mid to fail
		GetValue(sdpStr, string("a=mid"), pInfo->mid);
//#ifdef WEBRTC_DEBUG
		DEBUG("ParsedSDP: Usr: %s, Mid: %s",
				STR(pInfo->iceUsername), STR(pInfo->mid));
//#endif
	} else if (sdpStr.find("a=recvonly") != string::npos) {
		// This is a case of a unidirectional (from RMS, if this is an answer) stream
		FATAL("sdp: %s", STR(sdpStr));
		
		GetValue(sdpStr, string("a=mid"), pInfo->mid);
	} else {
		FATAL("Could not parse SDP!");

		if (!pInfo->fingerprint.size()) {
			FATAL("No Fingerprint found");
		}
		
		if (!pInfo->iceUsername.size()) {
			FATAL("No iceUsername found");
		}
		
		if (!pInfo->icePassword.size()) {
			FATAL("No icePassword found");
		}

		// hey dont delete if we have a valid s= line
		if (!pInfo->sessionInfo.size()) {
			delete pInfo;
			pInfo = NULL;
		}

	}
	return pInfo;
}

bool WrtcSDP::GetValue(const string & sdp, string keyName, string & val,  bool skipNextChar) {
	// find the key, skip the colon, collect up to end of line
	size_t k = sdp.find(keyName);
	if (k == string::npos) {
		return false; 	// ===can't find the key======>
	}

	if (skipNextChar) {
		k += keyName.size() + 1;	// skip the name and the colon
	}
	else {
		k += keyName.size();
	}

	//look for the leftmost terminator
	size_t rns = sdp.find("\\r", k);	// return escaped
	size_t lfs = sdp.find("\\n", k);    // linefeed escaped
	size_t rn = sdp.find("\r", k);	// return
	size_t lf = sdp.find("\n", k);    // linefeed
	// find the smallest
	size_t z = rns;
	if (lfs < z) z = lfs;
	if (lf < z) z = lfs;
	if (rn < z) z = lfs;
	//
	if (z == string::npos) {
		return false;
	}
	//
	val = sdp.substr(k, z-k);
	//WARN("$b2$ ParseSdp2: Key: %s, Value: %s", STR(keyName), STR(val));
	return true;
}
