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


#include "protocols/passthrough/streaming/outnetpassthroughstream.h"
#include "protocols/baseprotocol.h"
#include "utils/udpsenderprotocol.h"
#include "streaming/streamstypes.h"
#include "eventlogger/eventlogger.h"

OutNetPassThroughStream::OutNetPassThroughStream(BaseProtocol *pProtocol, string name)
: BaseOutNetStream(pProtocol, ST_OUT_NET_PASSTHROUGH, name) {
	_pSender = NULL;
	_enabled = false;
}

OutNetPassThroughStream::~OutNetPassThroughStream() {
	GetEventLogger()->LogStreamClosed(this);
	if (_pSender != NULL) {
		_pSender->ResetReadyForSendInterface();
		_pSender->EnqueueForDelete();
		_pSender = NULL;
	}
}

void OutNetPassThroughStream::Enable(bool value) {
	_enabled = value;
}

void OutNetPassThroughStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseOutNetStream::GetStats(info, namespaceId);
	info["pushUri"] = _pushUri;
}

void OutNetPassThroughStream::SignalAttachedToInStream() {

}

void OutNetPassThroughStream::SignalDetachedFromInStream() {
	if (_pProtocol != NULL)
		_pProtocol->EnqueueForDelete();
}

void OutNetPassThroughStream::SignalStreamCompleted() {

}

void OutNetPassThroughStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
}

void OutNetPassThroughStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
}

bool OutNetPassThroughStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutNetPassThroughStream::SignalPause() {
	return true;
}

bool OutNetPassThroughStream::SignalResume() {
	return true;
}

bool OutNetPassThroughStream::SignalSeek(double &dts) {
	return true;
}

bool OutNetPassThroughStream::SignalStop() {
	return true;
}

bool OutNetPassThroughStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!_enabled)
		return true;
	_stats.video.packetsCount++;
	_stats.video.bytesCount += dataLength;
	if (_pSender == NULL) {
		FATAL("No sink defined");
		return false;
	}
	return _pSender->SendChunked(pData, dataLength, 1500);
}

bool OutNetPassThroughStream::IsCompatibleWithType(uint64_t type) {
	return (type == ST_IN_NET_PASSTHROUGH
			|| type == ST_IN_FILE_PASSTHROUGH);
}

bool OutNetPassThroughStream::InitUdpSink(Variant &config) {
	if ((config["customParameters"] != V_MAP)
			|| (config["customParameters"]["localStreamConfig"] != V_MAP)
			|| (config["customParameters"]["localStreamConfig"]["targetUri"] != V_MAP)
			|| (config["customParameters"]["localStreamConfig"]["targetUri"]["ip"] != V_STRING)
			|| (config["customParameters"]["localStreamConfig"]["targetUri"]["port"] != _V_NUMERIC)
			|| (config["customParameters"]["localStreamConfig"]["ttl"] != _V_NUMERIC)
			|| (config["customParameters"]["localStreamConfig"]["tos"] != _V_NUMERIC)) {
		FATAL("Invalid config\n%s", STR(config.ToString()));
		return false;
	}
	_pSender = UDPSenderProtocol::GetInstance(
			"", //bindIp
			0, //bindPort
			config["customParameters"]["localStreamConfig"]["targetUri"]["ip"], //targetIp
			config["customParameters"]["localStreamConfig"]["targetUri"]["port"], //targetPort
			config["customParameters"]["localStreamConfig"]["ttl"], //ttl
			config["customParameters"]["localStreamConfig"]["tos"], //tos
			NULL //pReadyForSend
			);
	if (_pSender == NULL) {
		FATAL("Unable to initialize the UDP sender");
		return false;
	}
	if (config["customParameters"]["localStreamConfig"]["targetUri"]["originalUri"] == V_STRING)
		_pushUri = (string) config["customParameters"]["localStreamConfig"]["targetUri"]["originalUri"];
	_farIp = _nearIp = (string) config["customParameters"]["localStreamConfig"]["targetUri"]["ip"];
	_farPort = _nearPort = (uint16_t) config["customParameters"]["localStreamConfig"]["targetUri"]["port"];
	return true;
}

uint16_t OutNetPassThroughStream::GetUdpBindPort() {
	if (_pSender == NULL)
		return 0;
	return _pSender->GetUdpBindPort();
}

bool OutNetPassThroughStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetPassThroughStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetPassThroughStream::IsCodecSupported(uint64_t codec) {
	ASSERT("Operation not supported");
	return false;
}

