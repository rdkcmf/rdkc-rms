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


#include "application/clientapplicationmanager.h"
#include "protocols/rtp/sdpappprotocolhandler.h"
#include "protocols/rtp/inboundrtpprotocol.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/rtp/streaming/innetrtpstream.h"
#include "protocols/rtp/sdp.h"
#include "netio/netio.h"

namespace app_rdkcrouter {

	SDPAppProtocolHandler::SDPAppProtocolHandler(Variant &configuration)
	: BaseAppProtocolHandler(configuration) {
	}

	SDPAppProtocolHandler::~SDPAppProtocolHandler() {

	}

	bool SDPAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
		Variant customParameters;
		customParameters["customParameters"]["externalStreamConfig"] = streamConfig;

		// Read the SDP from file
		File sdpFile;
		string sdpPath = uri.fullDocumentPath();
		if (!sdpFile.Initialize(sdpPath, FILE_OPEN_MODE_READ)) {
			FATAL("Cannot open SDP file");
			return false;
		}
		string sdpContent;
		if (!sdpFile.ReadAll(sdpContent)) {
			FATAL("Cannot read SDP file contents");
			return false;
		}
		sdpFile.Close();

		SDP sdp;
		if (!SDP::ParseSDP(sdp, sdpContent, false)) {
			FATAL("Unable to parse the SDP");
			return false;
		}

		// Get the first video track
		Variant videoTrack = sdp.GetVideoTrack(0, uri.baseURI());
		Variant audioTrack = sdp.GetAudioTrack(0, uri.baseURI());
		if ((videoTrack == V_NULL) && (audioTrack == V_NULL)) {
			FATAL("No compatible tracks found");
			return false;
		}

		string srcAddress;
		string audioPort;
		string videoPort;
		string mediaType;

		if (sdp.HasKeyChain(V_STRING, true, 3, "session", "owner", "address")) {
			srcAddress = (string) sdp["session"]["owner"]["address"];
		}
		if (sdp.HasKeyChain(V_STRING, true, 4, "mediaTracks", "0x00000000", "media", "media_type")) {
			mediaType = (string) sdp["mediaTracks"]["0x00000000"]["media"]["media_type"];
			if (sdp.HasKeyChain(V_STRING, true, 4, "mediaTracks", "0x00000000", "media", "ports")) {
				if (mediaType == "audio") {
					audioPort = (string) sdp["mediaTracks"]["0x00000000"]["media"]["ports"];
				} else if (mediaType == "video") {
					videoPort = (string) sdp["mediaTracks"]["0x00000000"]["media"]["ports"];
				}
			}
		}
		if (sdp.HasKeyChain(V_STRING, true, 4, "mediaTracks", "0x00000001", "media", "media_type")) {
			mediaType = (string) sdp["mediaTracks"]["0x00000001"]["media"]["media_type"];
			if (sdp.HasKeyChain(V_STRING, true, 4, "mediaTracks", "0x00000001", "media", "ports")) {
				if (mediaType == "audio") {
					audioPort = (string) sdp["mediaTracks"]["0x00000001"]["media"]["ports"];
				} else if (mediaType == "video") {
					videoPort = (string) sdp["mediaTracks"]["0x00000001"]["media"]["ports"];
				}
			}
		}
		
		// Create inbound RTP for Audio from factory
		InboundRTPProtocol* pInboundRTPAudio = (InboundRTPProtocol*)ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_INBOUND_UDP_RTP, customParameters);

		if (pInboundRTPAudio == NULL) {
			FATAL("Unable to create the audio inbound RTP protocol chain!");
			return false;
		}

		uint16_t audioPortNum = (uint16_t) atoi(audioPort.c_str());

		// Create the needed UDP carrier for the Audio protocol
		UDPCarrier* pCarrierAudio = UDPCarrier::Create(
				srcAddress, //bindIp,
				audioPortNum,  //bindPort,
				pInboundRTPAudio,
				streamConfig["ttl"], //ttl,
				streamConfig["tos"], //tos
				streamConfig["ssmIp"] //ssmIp
				);

		if (pCarrierAudio == NULL) {
			FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
			pInboundRTPAudio->EnqueueForDelete();
			return false;
		}

		// Start accepting traffic
		if (!pCarrierAudio->StartAccept()) {
			FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
			pInboundRTPAudio->EnqueueForDelete();
			return false;
		}

		// Create inbound RTP for Audio from factory
		InboundRTPProtocol* pInboundRTPVideo = (InboundRTPProtocol*)ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_INBOUND_UDP_RTP, customParameters);

		if (pInboundRTPVideo == NULL) {
			FATAL("Unable to create the audio inbound RTP protocol chain!");
			return false;
		}

		uint16_t videoPortNum = (uint16_t)atoi(videoPort.c_str());

		// Create the needed UDP carrier for the Video protocol
		UDPCarrier* pCarrierVideo = UDPCarrier::Create(
				srcAddress, //bindIp,
				videoPortNum,  //bindPort,
				pInboundRTPVideo,
				streamConfig["ttl"], //ttl,
				streamConfig["tos"], //tos
				streamConfig["ssmIp"] //ssmIp
				);

		if (pCarrierVideo == NULL) {
			FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
			pInboundRTPVideo->EnqueueForDelete();
			return false;
		}

		// Start accepting traffic
		if (!pCarrierVideo->StartAccept()) {
			FATAL("Unable to bind on %s", STR(streamConfig["uri"]["ip"]));
			pInboundRTPVideo->EnqueueForDelete();
			return false;
		}
		
		// Setup the application
		pInboundRTPAudio->SetApplication(GetApplication());
		pInboundRTPVideo->SetApplication(GetApplication());
	
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
		InNetRTPStream* pInStream = new InNetRTPStream(
				pInboundRTPVideo, //pProtocol
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
			pInboundRTPVideo->EnqueueForDelete();
			delete pInStream;
			return false;
		}

		// Initialize the audio stream
		pInboundRTPAudio->SetStream(
				pInStream, //pInStream
				true, //isAudio
				true //unsolicited
				);

		// Initialize the video stream
		pInboundRTPVideo->SetStream(
				pInStream, //pInStream
				false, //isAudio
				false //unsolicited
				);

		return true;
	}

	void SDPAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
		//1. Is this a client RTSP protocol?
		if (pProtocol->GetType() != PT_RTSP)
			return;
		Variant &parameters = pProtocol->GetCustomParameters();
		if ((!parameters.HasKeyChain(V_BOOL, true, 1, "isClient"))
				|| (!((bool)parameters["isClient"]))) {
			return;
		}

		//2. Get the pull/push stream config
		if ((!parameters.HasKeyChain(V_MAP, true, 2, "customParameters", "externalStreamConfig"))
				&& (!parameters.HasKeyChain(V_MAP, true, 2, "customParameters", "localStreamConfig"))) {
			WARN("Bogus connection. Terminate it");
			pProtocol->EnqueueForDelete();
			return;
		}

	}

	void SDPAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
	}

}

