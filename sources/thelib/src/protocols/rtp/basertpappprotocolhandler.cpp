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
#include "application/clientapplicationmanager.h"
#include "protocols/rtp/basertpappprotocolhandler.h"
#include "protocols/rtp/inboundrtpprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "netio/netio.h"
#include "protocols/rtp/streaming/innetrtpstream.h"
#include "protocols/rtp/sdp.h"
#include "streaming/codectypes.h"

BaseRTPAppProtocolHandler::BaseRTPAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

BaseRTPAppProtocolHandler::~BaseRTPAppProtocolHandler() {
}

void BaseRTPAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
}

void BaseRTPAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}

bool BaseRTPAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	Variant customParameters;
	customParameters["customParameters"]["externalStreamConfig"] = streamConfig;

	// Create inbound RTP from factory
	InboundRTPProtocol *pResult =
			(InboundRTPProtocol *) ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_INBOUND_UDP_RTP, customParameters);

	if (pResult == NULL) {
		FATAL("Unable to create the inbound RTP protocol chain!");
		return false;
	}

	// Create the needed UDP carrier
	UDPCarrier *pCarrier = UDPCarrier::Create(
			streamConfig["uri"]["ip"], //bindIp,
			streamConfig["uri"]["port"], //bindPort,
			pResult,
			streamConfig["ttl"], //ttl,
			streamConfig["tos"], //tos
			streamConfig["ssmIp"] //ssmIp
			);

	if (pCarrier == NULL) {
		FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
		pResult->EnqueueForDelete();
		return false;
	}

	// Start accepting traffic
	if (!pCarrier->StartAccept()) {
		FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
		pResult->EnqueueForDelete();
		return false;
	}

	// Setup the application
	pResult->SetApplication(GetApplication());

	bool isAudio = (bool)streamConfig["isAudio"];
	Variant videoTrack;
	Variant audioTrack;

	// Assign the necessary codec setup bytes
	if (isAudio) {
		SDP_AUDIO_CODEC_SETUP(audioTrack) = streamConfig["audioCodecBytes"]; //hex
	} else {
		SDP_VIDEO_CODEC_H264_SPS(videoTrack) = streamConfig["spsBytes"]; //b64
		SDP_VIDEO_CODEC_H264_PPS(videoTrack) = streamConfig["ppsBytes"]; //b64
	}

	int16_t a = -1;
	int16_t b = -1;

	if (uri.parameters().HasKeyChain(V_STRING, false, 1, "quality")) {
		string raw = (string) uri.parameters().GetValue("quality", false);
		int16_t value = (int16_t) atoi(STR(raw));
		if (format("%"PRId16, value) == raw) {
			a = value;
		}
	}
	if (uri.parameters().HasKeyChain(V_STRING, false, 1, "resolution")) {
		string raw = (string) uri.parameters().GetValue("resolution", false);
		int16_t value = (int16_t) atoi(STR(raw));
		if (format("%"PRId16, value) == raw) {
			b = value;
		}
	}

	// Instantiate the inbound stream from the just created protocol
	InNetRTPStream *pInStream = new InNetRTPStream(
			pResult, //pProtocol
			streamConfig["localStreamName"], //name
			videoTrack, //videoTrack
			audioTrack, //audioTrack
			0, //bandwidthHint
			0, //rtcpDetectionInterval
			a,
			b
			);

	// Assign the stream manager
	if (!pInStream->SetStreamsManager(GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		pResult->EnqueueForDelete();
		delete pInStream;
		return false;
	}

	// Initialize the stream itself
	pResult->SetStream(
			pInStream, //pInStream
			isAudio, //isAudio
			true //unsolicited
			);

	// Everything went well, return true
	return true;
}

#endif /* HAS_PROTOCOL_RTP */
