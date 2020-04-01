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



#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
#include "streaming/basestream.h"
#include "application/clientapplicationmanager.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "mediaformats/readers/ts/tspacketpat.h"
#include "mediaformats/readers/ts/tspacketpmt.h"
#include "protocols/ts/basetsappprotocolhandler.h"
#include "protocols/ts/innettsstream.h"
#include "mediaformats/readers/ts/tsboundscheck.h"
#include "protocols/rtmp/header_le_ba.h"
#include "streaming/baseoutstream.h"
#include "mediaformats/readers/ts/tsparser.h"
#include "streaming/codectypes.h"
#include "mediaformats/readers/ts/avcontext.h"

InboundTSProtocol::InboundTSProtocol()
: BaseProtocol(PT_INBOUND_TS) {
	//1. Set the chunk size
	_chunkSize = 0;

	_pProtocolHandler = NULL;
	_chunkSizeDetectionCount = 0;
	_pParser = NULL;
	_pInStream = NULL;
}

InboundTSProtocol::~InboundTSProtocol() {
	if (_pParser != NULL) {
		delete _pParser;
		_pParser = NULL;
	}
	if (_pInStream != NULL) {
		delete _pInStream;
		_pInStream = NULL;
	}
}

bool InboundTSProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;

	bool fireOnlyOnce = false;
#ifdef HAS_MULTIPROGRAM_TS
	bool hasFilters = false;
#endif	/* HAS_MULTIPROGRAM_TS */

	if (parameters["customParameters"]["externalStreamConfig"].HasKeyChain(V_BOOL, true, 1, "fireOnlyOnce"))
		fireOnlyOnce = (bool)parameters["customParameters"]["externalStreamConfig"]["fireOnlyOnce"];

	if (fireOnlyOnce) {
		if (parameters["customParameters"]["externalStreamConfig"].HasKeyChain(_V_NUMERIC, true, 1, "id")) {
			uint32_t id = (uint32_t)parameters["customParameters"]["externalStreamConfig"]["id"];
			map<uint32_t, IOHandler *>& handlers = IOHandlerManager::GetActiveHandlers();
			if (MAP_HAS1(handlers, id)) {
				IOHandlerManager::EnqueueForDelete(handlers[id]);
			}
		}
	}
#ifdef HAS_MULTIPROGRAM_TS
	if (parameters.HasKeyChain(V_MAP, false, 2, "customParameters", "externalStreamConfig")) {
		Variant streamConfig = parameters["customParameters"]["externalStreamConfig"];
		if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "videoPID")) {
			uint16_t videoPID = streamConfig.GetValue("videoPID", false);
			if (videoPID != 0) {
				_filters["videoPID"] = videoPID;
				_filters["hasAVPID"] = true;
				hasFilters = true;
			}
		}
		if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "audioPID")) {
			uint16_t audioPID = streamConfig.GetValue("audioPID", false);
			if (audioPID != 0) {
				_filters["audioPID"] = audioPID;
				_filters["hasAVPID"] = true;
				hasFilters = true;
			}
		}
		if (!_filters.HasKeyChain(V_BOOL, false, 1, "hasAVPID")) {
			if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "programPID")) {
				uint16_t programPID = streamConfig.GetValue("programPID", false);
				if (programPID != 0) {
					_filters["programPID"] = streamConfig.GetValue("programPID", false);
					hasFilters = true;
				}
			}
			if (streamConfig.HasKeyChain(V_STRING, false, 1, "audioLanguage")) {
				string language = streamConfig.GetValue("audioLanguage", false);
				if (language != "") {
					_filters["audioLanguage"] = streamConfig.GetValue("audioLanguage", false);
					hasFilters = true;
				}
			}
		}
	}
	if (hasFilters) {
		_pParser->SetAVFilters(_filters);
	}
#endif	/* HAS_MULTIPROGRAM_TS */
	_pParser = new TSParser(this);
	return true;
}

bool InboundTSProtocol::AllowFarProtocol(uint64_t type) {
	return true;
}

bool InboundTSProtocol::AllowNearProtocol(uint64_t type) {
	FATAL("This protocol doesn't allow any near protocols");
	return false;
}

bool InboundTSProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool InboundTSProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	return SignalInputData(buffer);
}

bool InboundTSProtocol::SignalInputData(IOBuffer &buffer) {
	//	if ((rand() % 500) == 0) {
	//		FINEST("LOSS");
	//		if (GETAVAILABLEBYTESCOUNT(buffer) > 0)
	//			buffer.Ignore(1);
	//	}
	if (_chunkSize == 0) {
		if (!DetermineChunkSize(buffer)) {
			FATAL("Unable to determine chunk size");
			return false;
		}
	}

	if (_chunkSize == 0) {
		return true;
	}

	if (!_pParser->ProcessBuffer(buffer, false)) {
		FATAL("Unable to parse TS data");
		return false;
	}

	if (_chunkSize == 0) {
		return SignalInputData(buffer);
	}

	return true;
}

void InboundTSProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pProtocolHandler = (BaseTSAppProtocolHandler *)
				pApplication->GetProtocolHandler(this);
	} else {
		_pProtocolHandler = NULL;
	}
}

BaseTSAppProtocolHandler *InboundTSProtocol::GetProtocolHandler() {
	return _pProtocolHandler;
}

uint32_t InboundTSProtocol::GetChunkSize() {
	return _chunkSize;
}

BaseInStream *InboundTSProtocol::GetInStream() {
	return _pInStream;
}

void InboundTSProtocol::SignalResetChunkSize() {
	_chunkSizeDetectionCount = 0;
	_chunkSize = 0;
}

void InboundTSProtocol::SignalPAT(TSPacketPAT *pPAT) {
}

void InboundTSProtocol::SignalPMT(TSPacketPMT *pPMT) {
	if ((pPMT == NULL) || (_pInStream != NULL))
		return;

	//Set the default stream name if required
	string streamName = "";
	if (GetCustomParameters()["customParameters"]["externalStreamConfig"].HasKeyChain(V_STRING, true, 1, "localStreamName"))
		streamName = (string)GetCustomParameters()["customParameters"]["externalStreamConfig"]["localStreamName"];
	else
		streamName = format("ts_%"PRIu32"_%s", GetId(), STR(generateRandomString(4)));

	//Get the public stream name
	string publicStreamName = GetApplication()->GetIngestPointPublicName(streamName);
	if (publicStreamName == "") {
		FATAL("No ingest point found for %s", STR(streamName));
		return;
	}

	//Create the in stream
	_pInStream = new InNetTSStream(this, publicStreamName, pPMT->GetBandwidth());
	if (!_pInStream->SetStreamsManager(GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete _pInStream;
		_pInStream = NULL;
		EnqueueForDelete();
	}
}

void InboundTSProtocol::SignalPMTComplete() {
	if (_pInStream == NULL) {
		FATAL("No TS in stream");
		EnqueueForDelete();
		return;
	}

	//7. Pickup all outbound waiting streams
	map<uint32_t, BaseOutStream *> subscribedOutStreams =
			GetApplication()->GetStreamsManager()->GetWaitingSubscribers(
			_pInStream->GetName(), _pInStream->GetType(), true);

	//8. Bind the waiting subscribers

	FOR_MAP(subscribedOutStreams, uint32_t, BaseOutStream *, i) {
		BaseOutStream *pBaseOutStream = MAP_VAL(i);
		pBaseOutStream->Link(_pInStream);
	}

	_pInStream->Enable(true);
}

bool InboundTSProtocol::SignalStreamsPIDSChanged(map<uint16_t, TSStreamInfo> &streams) {
	return true;
}

bool InboundTSProtocol::SignalStreamPIDDetected(TSStreamInfo &streamInfo,
		BaseAVContext *pContext, PIDType type, bool &ignore) {
	if ((_pInStream == NULL) || (pContext == NULL)) {
		FATAL("Input TS stream not allocated");
		return false;
	}
	StreamCapabilities *pCapabilities = _pInStream->GetCapabilities();
	if (pCapabilities == NULL) {
		FATAL("Unable to get stream capabilities");
		return false;
	}

	if (type == PID_TYPE_AUDIOSTREAM) {
		if (!_pInStream->HasAudio()) {
			pContext->_pStreamCapabilities = pCapabilities;
			ignore = false;
			_pInStream->HasAudio(true);
		}
		return true;
	} else if (type == PID_TYPE_VIDEOSTREAM) {
		if (!_pInStream->HasVideo()) {
			pContext->_pStreamCapabilities = pCapabilities;
			ignore = false;
			_pInStream->HasVideo(true);
		}
		return true;
	} else {
		FATAL("Invalid stream type detected");
		return false;
	}
}

void InboundTSProtocol::SignalUnknownPIDDetected(TSStreamInfo &streamInfo) {
}

bool InboundTSProtocol::FeedData(BaseAVContext *pContext, uint8_t *pData,
		uint32_t dataLength, double pts, double dts, bool isAudio) {
	if (_pInStream == NULL) {
		FATAL("No in ts stream");
		return false;
	}
	return _pInStream->FeedData(pData, dataLength, 0, dataLength, pts, dts,
			isAudio);
}

bool InboundTSProtocol::DetermineChunkSize(IOBuffer &buffer) {
	while (GETAVAILABLEBYTESCOUNT(buffer) >= 3 * TS_CHUNK_208 + 2) {
		if (_chunkSizeDetectionCount >= TS_CHUNK_208) {
			FATAL("I give up. I'm unable to detect the ts chunk size");
			return false;
		}
		if (GETIBPOINTER(buffer)[0] != 0x47) {
			_chunkSizeDetectionCount++;
			buffer.Ignore(1);
			continue;
		}

		if ((GETIBPOINTER(buffer)[TS_CHUNK_188] == 0x47)
				&& (GETIBPOINTER(buffer)[2 * TS_CHUNK_188] == 0x47)) {
			_chunkSize = TS_CHUNK_188;
			return true;
		} else if ((GETIBPOINTER(buffer)[TS_CHUNK_204] == 0x47)
				&& (GETIBPOINTER(buffer)[2 * TS_CHUNK_204] == 0x47)) {
			_chunkSize = TS_CHUNK_204;
			return true;
		} else if ((GETIBPOINTER(buffer)[TS_CHUNK_208] == 0x47)
				&& (GETIBPOINTER(buffer)[2 * TS_CHUNK_208] == 0x47)) {
			_chunkSize = TS_CHUNK_208;
			return true;
		} else {
			_chunkSizeDetectionCount++;
			buffer.Ignore(1);
		}
	}
	if (_chunkSize != 0)
		_pParser->SetChunkSize(_chunkSize);
	return true;
}
#endif	/* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */

