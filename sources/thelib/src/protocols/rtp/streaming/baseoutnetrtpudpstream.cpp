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
#include "protocols/rtp/streaming/baseoutnetrtpudpstream.h"
#include "streaming/streamstypes.h"
#include "protocols/protocolmanager.h"
#include "protocols/baseprotocol.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "eventlogger/eventlogger.h"
#include "streaming/codectypes.h"

BaseOutNetRTPUDPStream::BaseOutNetRTPUDPStream(BaseProtocol *pProtocol,
		string name, bool zeroTimeBase, bool isSrtp)
: BaseOutNetStream(pProtocol, ST_OUT_NET_RTP, name) {
	_isSrtp = isSrtp;
	_audioSsrc = 0x80000000 | (rand()&0x00ffffff);
	_videoSsrc = _audioSsrc + 1;
	_pConnectivity = NULL;
	_videoCounter = isSrtp ? 0 : (uint16_t) rand();
	_audioCounter = isSrtp ? 0 : (uint16_t) rand();
	_hasAudio = false;
	_hasVideo = false;
	_enabled = false;
	_zeroTimeBase = zeroTimeBase;
}

BaseOutNetRTPUDPStream::~BaseOutNetRTPUDPStream() {
	GetEventLogger()->LogStreamClosed(this);
}

void BaseOutNetRTPUDPStream::Enable(bool value) {
	_enabled = value;
}

OutboundConnectivity *BaseOutNetRTPUDPStream::GetConnectivity() {
	return _pConnectivity;
}

void BaseOutNetRTPUDPStream::SetConnectivity(OutboundConnectivity *pConnectivity) {
	_pConnectivity = pConnectivity;
}

void BaseOutNetRTPUDPStream::HasAudioVideo(bool hasAudio, bool hasVideo) {
	_hasAudio = hasAudio;
	_hasVideo = hasVideo;
}

uint32_t BaseOutNetRTPUDPStream::AudioSSRC() {
	return _audioSsrc;
}

uint32_t BaseOutNetRTPUDPStream::VideoSSRC() {
	return _videoSsrc;
}

uint16_t BaseOutNetRTPUDPStream::VideoCounter() {
	return _videoCounter;
}

uint16_t BaseOutNetRTPUDPStream::AudioCounter() {
	return _audioCounter;
}

bool BaseOutNetRTPUDPStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool BaseOutNetRTPUDPStream::SignalPause() {
	return true;
}

bool BaseOutNetRTPUDPStream::SignalResume() {
	return true;
}

bool BaseOutNetRTPUDPStream::SignalSeek(double &dts) {
	return true;
}

bool BaseOutNetRTPUDPStream::SignalStop() {
	return true;
}

bool BaseOutNetRTPUDPStream::IsCompatibleWithType(uint64_t type) {
	return type == ST_IN_NET_RTMP
			|| type == ST_IN_NET_TS
			|| type == ST_IN_NET_RTP
			|| type == ST_IN_NET_LIVEFLV
			|| type == ST_IN_FILE_RTSP
			|| type == ST_IN_FILE_TS
			|| type == ST_IN_NET_RAW
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
            || type == ST_IN_NET_EXT
			;
}

void BaseOutNetRTPUDPStream::SignalDetachedFromInStream() {
	_pConnectivity->SignalDetachedFromInStream();
}

void BaseOutNetRTPUDPStream::SignalStreamCompleted() {
	EnqueueForDelete();
}

bool BaseOutNetRTPUDPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!_enabled)
		return true;
	return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
}

bool BaseOutNetRTPUDPStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_NONE, //naluMarkerType,
			false, //insertPDNALU,
			false, //insertRTMPPayloadHeader,
			false, //insertSPSPPSBeforeIDR,
			false //aggregateNALU
			);

	//audio setup
#ifdef HAS_G711
	if (pGenericProcessDataSetup->_audioCodec == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
	pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
	pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;
#ifdef HAS_G711
	} else if ((pGenericProcessDataSetup->_audioCodec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
		pGenericProcessDataSetup->audio.g711._aLaw = (pGenericProcessDataSetup->_audioCodec == CODEC_AUDIO_G711A);
	}
#endif	/* HAS_G711 */
	//misc setup
	pGenericProcessDataSetup->_timeBase = _zeroTimeBase ? 0 : -1;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

	pGenericProcessDataSetup->_hasAudio = _hasAudio;
	pGenericProcessDataSetup->_hasVideo = _hasVideo;

	return true;
}
void BaseOutNetRTPUDPStream::SetRTPBlockSize(uint32_t blockSize) {
}

uint32_t BaseOutNetRTPUDPStream::RetransmitFrames(uint32_t ssrc, vector<uint16_t> pids) {
//	INFO("videoSsrc=%"PRIu32" ssrc=%"PRIu32, _videoSsrc, ssrc);
	uint32_t transmitted = 0;
	if (ssrc == _videoSsrc) {
		FOR_VECTOR(pids, i) {
			IOBuffer packet;
			if (_videoPacketBuffer.GetPacket(pids[i], packet) == 0)
				continue;
			if (_pConnectivity != NULL) {
				_pConnectivity->SendDirect(packet);
				transmitted++;
			}
		}
		FINE("No: of frames requested for retransmit: %"PRIu32", No of frames retransmitted: %"PRIu32"", (uint32_t)pids.size(), transmitted);
	}
	return transmitted;
}

bool BaseOutNetRTPUDPStream::PacketIsCached(uint32_t ssrc, uint16_t pid) {
	if (ssrc == _videoSsrc) {
		return _videoPacketBuffer.WithinRange(pid);
	}
	return false;
}

bool BaseOutNetRTPUDPStream::PacketIsNewerThanCache(uint32_t ssrc, uint16_t pid) {
	if (ssrc == _videoSsrc) {
		return _videoPacketBuffer.IsHigherThanRange(pid);
	}
	return false;
}

void BaseOutNetRTPUDPStream::EnablePacketCache(bool enabled) {
	_videoPacketBuffer.Enable(enabled);
}
#endif /* HAS_PROTOCOL_RTP */
