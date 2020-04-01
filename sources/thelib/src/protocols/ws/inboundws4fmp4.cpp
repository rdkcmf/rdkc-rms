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

#ifdef HAS_PROTOCOL_WS
#ifdef HAS_PROTOCOL_WS_FMP4
#include "protocols/ws/inboundws4fmp4.h"
#include "streaming/baseinnetstream.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "protocols/external/externalmoduleapi.h"
#include "streaming/baseoutstream.h"
#include "streaming/fmp4/outnetfmp4stream.h"
#include "streaming/mp4/outnetmp4stream.h"
#include "protocols/ws/inboundwsprotocol.h"
#include "application/clientapplicationmanager.h"
#include "mediaformats/readers/streammetadataresolver.h"

InboundWS4FMP4::InboundWS4FMP4()
: BaseProtocol(PT_INBOUND_WS_FMP4) {
	_outputStreamId = 0;
	_fullMp4 = false;
	_streamName = "";
	_isPlaylist = false;
}

InboundWS4FMP4::~InboundWS4FMP4() {
	//unregister from the outbound stream
	BaseClientApplication *pApp = NULL;
	StreamsManager *pSM = NULL;
	
	if (((pApp = GetLastKnownApplication()) == NULL)
			|| ((pSM = pApp->GetStreamsManager()) == NULL)) {
		RemovePlaylist();
		return;
	}

	if (_fullMp4) {
		OutNetMP4Stream *pOutNetMP4Stream = NULL;
		if ((pOutNetMP4Stream = (OutNetMP4Stream *)pSM->FindByUniqueId(
				(uint32_t)_outputStreamId)) == NULL) {
			RemovePlaylist();
			return;
		}
		
		pOutNetMP4Stream->UnRegisterOutputProtocol(GetId());
	} else {
		OutNetFMP4Stream *pOutNetFMP4Stream = NULL;
		if ((pOutNetFMP4Stream = (OutNetFMP4Stream *)pSM->FindByUniqueId(
				(uint32_t)_outputStreamId)) == NULL) {
			RemovePlaylist();
			return;
		}
		
		pOutNetFMP4Stream->UnRegisterOutputProtocol(GetId());
	}

	RemovePlaylist();
}

void InboundWS4FMP4::RemovePlaylist() {
	if (_isPlaylist && !_streamName.empty()) {
		BaseClientApplication *pApp = GetLastKnownApplication();
		if (pApp) {
			Variant message;
			message["header"] = "removeConfig";
			message["payload"]["command"] = message["header"];
			message["payload"]["localStreamName"] = _streamName;
			pApp->SignalApplicationMessage(message);
		}
	}
}

bool InboundWS4FMP4::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool InboundWS4FMP4::AllowFarProtocol(uint64_t type) {
	return type == PT_INBOUND_WS;
}

bool InboundWS4FMP4::AllowNearProtocol(uint64_t type) {
	FATAL("This is an endpoint protocol");
	return false;
}

bool InboundWS4FMP4::SignalInputData(int32_t recvAmount) {
	FATAL("Operation not supported");
	return false;
}

bool InboundWS4FMP4::SignalInputData(IOBuffer &buffer) {
	FATAL("Operation not supported");
	return false;
}

bool InboundWS4FMP4::SendOutOfBandData(const IOBuffer &buffer, void *pUserData) {
	return _pFarProtocol == NULL ? false : _pFarProtocol->SendOutOfBandData(buffer, pUserData);
}

WSInterface* InboundWS4FMP4::GetWSInterface() {
	return this;
}

bool InboundWS4FMP4::SignalInboundConnection(Variant &request) {
	//get the output stream
	bool repeat = true;
	while (repeat) {
		_outputStreamId = FindStream(request[HTTP_FIRST_LINE][HTTP_URL], 
			repeat);
	}
	return _outputStreamId != 0;
	//return (_outputStreamId = FindStream(request[HTTP_FIRST_LINE][HTTP_URL])) != 0;
}

bool InboundWS4FMP4::SignalTextData(const uint8_t *pBuffer, size_t length) {
	//FINEST("%s", STR(string((const char *) pBuffer, length)));
	BaseClientApplication* pApp = GetApplication();
	if (pApp) {
		// Try to get the pointer to the outbound stream that has the _outputStreamId
		string command = string((const char*)pBuffer, length);

		// for now the supported command is swapstream=<name>
		string action;
		string parameter;
		size_t index = command.find("=");

		if (index != string::npos) {
			action = command.substr(0, index);
			parameter = command.substr(index + 1);
		} else {
			action = command;
			parameter = "";
		}

		// NOTE : Assumes that _outputStreamsId != 0... otherwise, we won't be in this function call ...
		StreamsManager* pSM = pApp->GetStreamsManager();
		OutNetFMP4Stream* pOutNetStream =
			(OutNetFMP4Stream*)pSM->FindByUniqueId((uint32_t)_outputStreamId);
		if (pOutNetStream) {
			if (action == "pause") {
				pOutNetStream->SignalPause();
				INFO("Stream playback paused");
			} else if (action == "resume") {
				pOutNetStream->SignalResume();
				INFO("Stream playback resumed");
			} else if (action == "ping") {
				pOutNetStream->SignalPing();
				INFO("Responding to player ping");
			} else {
				WARN("Unknown command received: %s", STR(command));
			}
		}

	}

	return true;
}

bool InboundWS4FMP4::SignalBinaryData(const uint8_t *pBuffer, size_t length) {
	//FINEST("Received %"PRIz"u bytes",length);
	return true;
}

bool InboundWS4FMP4::SignalPing(const uint8_t *pBuffer, size_t length) {
	//FINEST("ping: %s", STR(string((const char *) pBuffer, length)));
	return true;
}

void InboundWS4FMP4::SignalConnectionClose(uint16_t code, const uint8_t *pReason, size_t length) {
	//FINEST("code: %"PRIu16"; reason: %s", code, STR(string((const char *) pReason, length)));
}

bool InboundWS4FMP4::DemaskBuffer() {
	return true;
}

uint64_t InboundWS4FMP4::FindStream(string requestedStreamName, 
		bool& repeat) {
	bool progressive = false;

	repeat = false;

	string::size_type offset = requestedStreamName.find("?");
	if (offset != string::npos) {
		URI uri;
		if (!URI::FromString("http://a" + requestedStreamName, false, uri)) {
			FATAL("Requested stream name is invalid");
			EnqueueForDelete();
			return 0;
		}
		requestedStreamName = uri.fullDocumentPath();
		progressive = uri.parameters().HasKey("progressive", false);
		_fullMp4 = uri.parameters().HasKey("fullMp4", false);
	}

	//get the application
	BaseClientApplication *pApp = NULL;
	if ((pApp = GetApplication()) == NULL) {
		FATAL("Unable to get the application");
		return 0;
	}

	//get the stream name from the HTTP request
	if ((requestedStreamName.size() > 0)&&(requestedStreamName[0] == '/'))
		requestedStreamName = requestedStreamName.substr(1);

	string rsn = lowerCase(requestedStreamName);
	_isPlaylist = (rsn.rfind(".lst") == rsn.length() - 4);
	bool isLazyPullVod = _isPlaylist ? false : (rsn.rfind(".vod") == 
		rsn.length() - 4);

	uint64_t outputStreamType = ST_OUT_NET_FMP4;
	if (_fullMp4) {
		outputStreamType = ST_OUT_NET_MP4;
	}

	//get the private stream name
	string privateStreamName = pApp->GetStreamNameByAlias(requestedStreamName, false);
	if (privateStreamName == "") {
		FATAL("Stream name alias value not found: %s", STR(requestedStreamName));
		return 0;
	}

	//try get the output stream
	StreamsManager *pSM = NULL;
	if ((pSM = pApp->GetStreamsManager()) == NULL) {
		FATAL("Unable to get the streams manager");
		return 0;
	}

	BaseOutStream *pOutNetStream = NULL;
	map<uint32_t, BaseStream *> outStreams = pSM->FindByTypeByName(
		outputStreamType, privateStreamName, false, false);
	if (!outStreams.empty()) {
		if (_fullMp4) {
			pOutNetStream = (OutNetMP4Stream *) MAP_VAL(outStreams.begin());
		} else {
			pOutNetStream = (OutNetFMP4Stream *) MAP_VAL(outStreams.begin());
		}
	}

	//we may have a valid outgoing stream for <privateStreamName>.vod
	bool streamExists = (pSM->FindByTypeByName(ST_IN_NET, privateStreamName, 
		true, false).size() > 0);
	//make sure there is no stream exists before going the lazypull route
	if (pOutNetStream == NULL && !streamExists && !_isPlaylist) {
		if (!isLazyPullVod) {
			requestedStreamName = format("%s.vod",
				STR(requestedStreamName));
			privateStreamName = pApp->GetStreamNameByAlias(
				requestedStreamName, false);
			if (privateStreamName == "") {
				FATAL("Stream name alias value not found: %s",
					STR(requestedStreamName));
				return 0;
			}
		}
		outStreams = pSM->FindByTypeByName(outputStreamType, privateStreamName, 
			false, false);
		if (!outStreams.empty()) {
			if (_fullMp4) {
				pOutNetStream = (OutNetMP4Stream *)MAP_VAL(outStreams.begin());
			} else {
				pOutNetStream = (OutNetFMP4Stream *)MAP_VAL(outStreams.begin());
			}
		}
	}

	if (pOutNetStream == NULL) {
		//there is no output stream with this name. Create one
		if (_fullMp4) {
			pOutNetStream = OutNetMP4Stream::GetInstance(pApp, privateStreamName);
		} else {
			pOutNetStream = OutNetFMP4Stream::GetInstance(pApp, privateStreamName, progressive);
		}

		if (pOutNetStream == NULL) {
			FATAL("Unable to create the output stream");
			return 0;
		}

		//and also try to bind it to the possibly present input stream
		BaseInStream *pInStream = NULL;
		map<uint32_t, BaseStream *> inStreams = pSM->FindByTypeByName(
			ST_IN_NET, privateStreamName, true, false);
		if (inStreams.size() != 0) {
			pInStream = (BaseInStream *)MAP_VAL(inStreams.begin());
		} else if (_isPlaylist) {
			Variant message;
			message["header"] = "pullStream";
			message["payload"]["command"] = message["header"];
			//do not save to pushpull config
			message["payload"]["parameters"]["serializeToConfig"] = "0";
			message["payload"]["parameters"]["localStreamName"] =
				requestedStreamName;
			message["payload"]["parameters"]["uri"] =
				format("rtmp://localhost/vod/%s", STR(requestedStreamName));
			_streamName = requestedStreamName;
			ClientApplicationManager::BroadcastMessageToAllApplications(
				message);
			if ((!message.HasKeyChain(V_BOOL, true, 3, "payload",
					"parameters", "ok")) ||
					(!((bool)message["payload"]["parameters"]["ok"]))) {
				FATAL("Unable to pull the requested stream %s",
					STR(requestedStreamName));
			} else {
				repeat = true;
			}
		} else if (!isLazyPullVod) {
			isLazyPullVod = true;
		}
		if (isLazyPullVod) {
			StreamMetadataResolver *pSMR = pApp->GetStreamMetadataResolver();
			if (pSMR == NULL) {
				FATAL("No stream metadata resolver found");
				return false;
			}

			Metadata metadata;
			if (!pSMR->ResolveMetadata(requestedStreamName, metadata, true)) {
				FATAL("Unable to lazy pull the requested vod file %s", 
					STR(requestedStreamName));
			} else {
				repeat = true;
			}
		}
		if ((pInStream != NULL)&&(pInStream->IsCompatibleWithType(outputStreamType))) {
			if (!pInStream->Link(pOutNetStream)) {
				FATAL("Unable to link the in/out streams");
				pOutNetStream->EnqueueForDelete();
				return 0;
			}
			else {
				//WARN("$b2$ InboundWS4FMP4 linked stream: %s", STR(*pInStream));
			}
		}else{
			if (!pInStream) {
				FATAL("Can't find input stream: %s", STR(privateStreamName));
			}else if (!pInStream->IsCompatibleWithType(outputStreamType)) {
				FATAL("Input stream: %s Not compatible with %s!",
						STR(*pInStream), STR(tagToString(outputStreamType)));
			}
		}
	}

	//register this WebSocket connection as a consumer
	if (_fullMp4) {
		((OutNetMP4Stream *)pOutNetStream)->RegisterOutputProtocol(GetId(), NULL);
	} else {
		((OutNetFMP4Stream *)pOutNetStream)->RegisterOutputProtocol(GetId(), NULL);
	}

	//done
	return pOutNetStream->GetUniqueId();
}

#endif	/* HAS_PROTOCOL_WS_FMP4 */
#endif	/* HAS_PROTOCOL_WS */
