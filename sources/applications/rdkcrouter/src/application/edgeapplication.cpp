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

#include "application/edgeapplication.h"
#include "protocols/rtmp/rtmpappprotocolhandler.h"
#include "protocols/rtmp/streaming/rtmpplaylist.h"
#include "protocols/variant/edgevariantappprotocolhandler.h"
#include "protocols/variant/statusreportconstant.h"
#include "netio/netio.h"
#include "streaming/streamstypes.h"
#include "streaming/baseoutnetstream.h"
#include "streaming/baseinstream.h"
#include "protocols/timer/jobstimerprotocol.h"
#include "protocols/protocolmanager.h"
#include "protocols/timer/jobstimerappprotocolhandler.h"
#include "eventlogger/sinktypes.h"
#include "eventlogger/rpcsink.h"
#include "protocols/rpc/baserpcappprotocolhandler.h"
#include "protocols/http/httpappprotocolhandler.h"
using namespace app_rdkcrouter;

#define SD_NOT_SUPPORTED 0
#define SD_PUBLISHER_EDGE 1
#define SD_EDGE_ORIGIN 2
#define SD_ORIGIN_EDGE 3
#define SD_EDGE_PLAYER 4

EdgeApplication::EdgeApplication(Variant &configuration)
: BaseClientApplication(configuration) {
	_pHTTPHandler = NULL;
	_pRPCHandler = NULL;
	_pRTMPHandler = NULL;
	_pVariantHandler = NULL;
	_pTimerHandler = NULL;
	_retryTimerProtocolId = 0;
	_pushId = 1;
}

EdgeApplication::~EdgeApplication() {
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(_retryTimerProtocolId);
	if (pProtocol != NULL) {
		pProtocol->EnqueueForDelete();
	}

	UnRegisterAppProtocolHandler(PT_INBOUND_HTTP_FOR_RTMP);
	if (_pHTTPHandler != NULL) {
		delete _pHTTPHandler;
		_pHTTPHandler = NULL;
	}

	UnRegisterAppProtocolHandler(PT_INBOUND_RTMP);
	UnRegisterAppProtocolHandler(PT_OUTBOUND_RTMP);
	UnRegisterAppProtocolHandler(PT_INBOUND_RTMPS_DISC);
	if (_pRTMPHandler != NULL) {
		delete _pRTMPHandler;
		_pRTMPHandler = NULL;
	}

	UnRegisterAppProtocolHandler(PT_BIN_VAR);
	if (_pVariantHandler != NULL) {
		delete _pVariantHandler;
		_pVariantHandler = NULL;
	}

	UnRegisterAppProtocolHandler(PT_TIMER);
	if (_pTimerHandler != NULL) {
		delete _pTimerHandler;
		_pTimerHandler = NULL;
	}

	for (size_t i = 0; i < _pRPCSinks.size(); i++)
		_pRPCSinks[i]->SetRpcProtocolHandler(NULL);
	UnRegisterAppProtocolHandler(PT_OUTBOUND_RPC);
	if (_pRPCHandler != NULL) {
		delete _pRPCHandler;
		_pRPCHandler = NULL;
	}
}

bool EdgeApplication::Initialize() {
	if (!BaseClientApplication::Initialize()) {
		FATAL("Unable to initialize application");
		return false;
	}

	_pHTTPHandler = new HTTPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_INBOUND_HTTP_FOR_RTMP, _pHTTPHandler);

	_pRPCHandler = new BaseRPCAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_OUTBOUND_RPC, _pRPCHandler);
	for (size_t i = 0; i < _pRPCSinks.size(); i++)
		_pRPCSinks[i]->SetRpcProtocolHandler(_pRPCHandler);

	_pRTMPHandler = new RTMPAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_INBOUND_RTMP, _pRTMPHandler);
	RegisterAppProtocolHandler(PT_OUTBOUND_RTMP, _pRTMPHandler);
	RegisterAppProtocolHandler(PT_INBOUND_RTMPS_DISC, _pRTMPHandler);

	_pVariantHandler = new EdgeVariantAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_BIN_VAR, _pVariantHandler);

	_pTimerHandler = new JobsTimerAppProtocolHandler(_configuration);
	RegisterAppProtocolHandler(PT_TIMER, _pTimerHandler);

	Variant varAcceptor;
	varAcceptor[CONF_IP] = "127.0.0.1";
	varAcceptor[CONF_PORT] = (uint32_t) 0;
	varAcceptor[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_BIN_VARIANT;
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_INBOUND_BIN_VARIANT);
	if (chain.size() == 0) {
		FATAL("Unable to get chain "CONF_PROTOCOL_INBOUND_BIN_VARIANT);
		return false;
	}

	TCPAcceptor *pAcceptor = new TCPAcceptor(
			varAcceptor[CONF_IP],
			varAcceptor[CONF_PORT],
			varAcceptor,
			chain);
	if (!pAcceptor->Bind()) {
		FATAL("Unable to bind acceptor");
		return false;
	}
	if (!ActivateAcceptor(pAcceptor)) {
		FATAL("Unable to activate acceptor");
		return false;
	}
	if (!pAcceptor->StartAccept()) {
		FATAL("Unable to start acceptor");
		return false;
	}

	_pVariantHandler->SetConnectorInfo(pAcceptor->GetParameters());

	BaseTimerProtocol *pProtocol = new JobsTimerProtocol(false);
	_retryTimerProtocolId = pProtocol->GetId();
	pProtocol->SetApplication(this);
	pProtocol->EnqueueForTimeEvent(1);

	if ((_configuration["ingestPointsPersistenceFile"] != V_STRING)
			|| (_configuration["ingestPointsPersistenceFile"] == "")) {
		_configuration["ingestPointsPersistenceFile"] = "./ingestpoints.xml";
	}

	//4. Process initial ingest points persistence file
	if (!ProcessIngestPointsList()) {
		WARN("Unable to process pushPull list");
	}

	return _pVariantHandler->SendHello();
}

bool EdgeApplication::ActivateAcceptor(IOHandler *pIOHandler) {
	switch (pIOHandler->GetType()) {
		case IOHT_UDP_CARRIER:
		{
			UDPCarrier *pUDPCarrier = (UDPCarrier *) pIOHandler;
			if (pUDPCarrier->GetProtocol() != NULL)
				pUDPCarrier->GetProtocol()->GetNearEndpoint()->EnqueueForDelete();
			return true;
		}
		case IOHT_ACCEPTOR:
		{
			TCPAcceptor *pAcceptor = (TCPAcceptor *) pIOHandler;
			Variant &params = pAcceptor->GetParameters();
			if ((params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMP)
					&& (params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_BIN_VARIANT)
					&& (params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMPS)
					)
				return true;
			bool clustering = false;
			if (params.HasKeyChain(V_BOOL, false, 1, "clustering"))
				clustering = (bool)params.GetValue("clustering", false);
			if (clustering)
				return true;
			pAcceptor->SetApplication(this);
			return true;
		}
		default:
		{
			return true;
		}
	}
}

void EdgeApplication::InsertPlaylistItem(string &playlistName, string &localStreamName,
		double insertPoint, double sourceOffset, double duration) {
	const map<uint32_t, RTMPPlaylist *> _playlists = RTMPPlaylist::GetPlaylists(playlistName);
	
	//Insert the item in all of them
	map<uint32_t, RTMPPlaylist *>::const_iterator i = _playlists.begin();
	for (; i != _playlists.end(); i++) {
		MAP_VAL(i)->InsertItem(localStreamName, insertPoint, sourceOffset, duration);
	}
}

void EdgeApplication::SignalStreamRegistered(BaseStream *pStream) {
	BaseClientApplication::SignalStreamRegistered(pStream);
	_pVariantHandler->RegisterInfoStream(pStream->GetUniqueId());

	uint64_t streamType = pStream->GetType();
	BaseProtocol *pProtocol = pStream->GetProtocol();
	Variant &customParameters = pProtocol != NULL ? pProtocol->GetCustomParameters() : _dummy;

	switch (GetClusterStreamDirection(streamType, pProtocol, customParameters)) {
		case SD_PUBLISHER_EDGE:
		{
			//WARN("**** SD_PUBLISHER_EDGE created");
			Variant pushConfig;

			pushConfig["targetUri"] = "rtmp://localhost:1936/live";
			pushConfig["localStreamName"] = pStream->GetName();
			pushConfig["emulateUserAgent"] = HTTP_HEADERS_SERVER_US;
			string publicName = pStream->GetName();
			pushConfig["targetStreamName"] = GetIngestPointPrivateName(publicName);
			pushConfig["targetStreamType"] = "live";
			pushConfig["swfUrl"] = "";
			pushConfig["pageUrl"] = "";
			pushConfig["keepAlive"] = (bool)false;
			pushConfig["ttl"] = (uint32_t) 0x100;
			pushConfig["tos"] = (uint32_t) 0x100;
			pushConfig["configId"] = (uint32_t) (_pushId++);
			pushConfig["inputProtocolId"] = pProtocol->GetId();

			EnqueueTimedOperation(pushConfig);
			break;
		}
		case SD_EDGE_ORIGIN:
		{
			//WARN("**** SD_EDGE_ORIGIN created");
			BaseProtocol *pInputProtocol = ProtocolManager::GetProtocol(customParameters["customParameters"]["localStreamConfig"]["inputProtocolId"]);
			if (pInputProtocol != NULL) {
				pInputProtocol->GetCustomParameters()["outputProtocol"] = pProtocol->GetId();
			}
			BaseProtocol *pProtocol = pStream->GetProtocol();
			if (pProtocol != NULL) {
				Variant &config = pProtocol->GetCustomParameters();
				config["_isInternal"] = true;
			}
			break;
		}
		case SD_ORIGIN_EDGE:
		{
			//WARN("**** SD_ORIGIN_EDGE created");
			BaseProtocol *pProtocol = pStream->GetProtocol();
			if (pProtocol != NULL) {
				Variant &config = pProtocol->GetCustomParameters();
				config["_isInternal"] = true;
			}
			break;
		}
		case SD_EDGE_PLAYER:
		{
			//WARN("**** SD_EDGE_PLAYER created");
			break;
		}
		default:
		{
			break;
		}
	}
}

void EdgeApplication::SignalStreamUnRegistered(BaseStream *pStream) {
	BaseClientApplication::SignalStreamUnRegistered(pStream);
	_pVariantHandler->UnRegisterInfoStream(pStream->GetUniqueId());

	uint64_t streamType = pStream->GetType();
	BaseProtocol *pProtocol = pStream->GetProtocol();
	Variant &customParameters = pProtocol != NULL ? pProtocol->GetCustomParameters() : _dummy;

	switch (GetClusterStreamDirection(streamType, pProtocol, customParameters)) {
		case SD_PUBLISHER_EDGE:
		{
			//WARN("**** SD_PUBLISHER_EDGE deleted");
			if (customParameters.HasKeyChain(_V_NUMERIC, true, 1, "outputProtocol")) {
				BaseProtocol *pOutputProtocol = ProtocolManager::GetProtocol(customParameters["outputProtocol"]);
				if (pOutputProtocol != NULL)
					pOutputProtocol->EnqueueForDelete();
			}
			break;
		}
		case SD_EDGE_ORIGIN:
		{
			//WARN("**** SD_EDGE_ORIGIN deleted");
			pProtocol->EnqueueForDelete();
			break;
		}
		case SD_ORIGIN_EDGE:
		{
			//WARN("**** SD_ORIGIN_EDGE deleted");
			pProtocol->EnqueueForDelete();
			break;
		}
		case SD_EDGE_PLAYER:
		{
			//WARN("**** SD_EDGE_PLAYER deleted");
			break;
		}
		default:
		{
			break;
		}
	}
}

void EdgeApplication::RegisterProtocol(BaseProtocol *pProtocol) {
	BaseClientApplication::RegisterProtocol(pProtocol);
	_pVariantHandler->RegisterInfoProtocol(pProtocol->GetId());
}

void EdgeApplication::UnRegisterProtocol(BaseProtocol *pProtocol) {
	_pVariantHandler->UnRegisterInfoProtocol(pProtocol->GetId());
	BaseClientApplication::UnRegisterProtocol(pProtocol);

	Variant &parameters = pProtocol->GetCustomParameters();
	if (parameters.HasKeyChain(_V_NUMERIC, true, 3, "customParameters", "localStreamConfig", "inputProtocolId")) {
		BaseProtocol *pInputProtocol = ProtocolManager::GetProtocol(parameters["customParameters"]["localStreamConfig"]["inputProtocolId"]);
		if (pInputProtocol != NULL)
			pInputProtocol->EnqueueForDelete();
	}
}

void EdgeApplication::SignalUnLinkingStreams(BaseInStream *pInStream, BaseOutStream *pOutStream) {
	if (pOutStream == NULL)
		return;
	uint64_t outStreamType = pOutStream->GetType();
	BaseProtocol *pOutProtocol = pOutStream->GetProtocol();
	Variant &outCustomParameters = pOutProtocol != NULL ? pOutProtocol->GetCustomParameters() : _dummy;
	if (GetClusterStreamDirection(outStreamType, pOutProtocol, outCustomParameters) != SD_EDGE_PLAYER) {
		//WARN("****** NOT EDGE PLAYER");
		return;
	}

	if (pInStream == NULL)
		return;
	uint64_t inStreamType = pInStream->GetType();
	BaseProtocol *pInProtocol = pInStream->GetProtocol();
	Variant &inCustomParameters = pInProtocol != NULL ? pInProtocol->GetCustomParameters() : _dummy;

	if (GetClusterStreamDirection(inStreamType, pInProtocol, inCustomParameters) != SD_ORIGIN_EDGE) {
		//WARN("****** NOT ORIGIN EDGE");
		return;
	}

	if (pInStream->GetOutStreams().size() > 1) {
		//WARN("****** STILL HAS CONSUMERS");
		return;
	}

	//WARN("****** CRUSH IT");
	pInStream->GetProtocol()->EnqueueForDelete();
}

void EdgeApplication::SignalApplicationMessage(Variant &message) {
	if ((!message.HasKeyChain(V_STRING, false, 1, "header"))
			|| (!message.HasKey("payload", false)))
		return;
	string header = message["header"];
	Variant &payload = message.GetValue("payload", false);
	if (header == "signalOutStreamAttached") {
		_pVariantHandler->SendSignalOutStreamAttached(payload["streamName"]);
	} else if (header == "eventLog") {
		_pVariantHandler->SendEventLog(payload);
	} else {
		WARN("Unhandled message: %s", STR(header));
	}
}

void EdgeApplication::SignalOriginInpuStreamAvailable(string streamName) {
	map<uint32_t, BaseStream *> streams = GetStreamsManager()->FindByTypeByName(
			ST_IN_NET_RTMP, streamName + "?", true, true);
	if (streams.size() > 0) {
		//WARN("*** Already have it in long name");
		return;
	}
	streams = GetStreamsManager()->FindByTypeByName(
			ST_IN_NET_RTMP, streamName, true, false);
	if (streams.size() > 0) {
		//WARN("*** Already have it in short name");
		return;
	}

	//WARN("*** This edge doesn't have it");

	map<uint32_t, BaseOutStream *> subscribedOutStreams =
			GetStreamsManager()->GetWaitingSubscribers(
			streamName, ST_IN_NET_RTMP, true);

	if (subscribedOutStreams.size() == 0) {
		//WARN("*** This edge doesn't need it");
		return;
	}

	//WARN("*** This edge needs it");
	Variant pullSettings;
	pullSettings["uri"] = "rtmp://localhost:1936/live/" + streamName;
	pullSettings["keepAlive"] = (bool)false;
	pullSettings["localStreamName"] = streamName;
	pullSettings["forceTcp"] = (bool)true;
	pullSettings["swfUrl"] = "";
	pullSettings["pageUrl"] = "";
	pullSettings["ttl"] = (uint32_t) 0x100;
	pullSettings["tos"] = (uint32_t) 0x100;
	pullSettings["width"] = (uint32_t) 0;
	pullSettings["height"] = (uint32_t) 0;
	pullSettings["rangeStart"] = (int64_t) - 2;
	pullSettings["rangeEnd"] = (int64_t) - 1;

	//6. Do the request
	if (!PullExternalStream(pullSettings)) {
		FATAL("Unable to pull stream %s", STR(streamName));
		return;
	}
}

void EdgeApplication::SignalEventSinkCreated(BaseEventSink *pSink) {
	if (pSink->GetSinkType() == SINK_RPC)
		ADD_VECTOR_END(_pRPCSinks, (RpcSink *) pSink);
}

void EdgeApplication::EnqueueTimedOperation(Variant &request) {
	//1. Get the timer protocol
	JobsTimerProtocol * pTimer = (JobsTimerProtocol *) ProtocolManager::GetProtocol(
			_retryTimerProtocolId);
	if (pTimer == NULL) {
		FATAL("Unable to get the timer protocol");
		return;
	}

	//2. Enqueue the operation
	pTimer->EnqueueOperation(request, OPERATION_TYPE_PUSH);
}

uint8_t EdgeApplication::GetClusterStreamDirection(uint64_t streamType,
		BaseProtocol *pProtocol, Variant &customParameters) {
	if (TAG_KIND_OF(streamType, ST_IN_NET_RTMP)) {
		//INPUT STREAM
		if (customParameters.HasKeyChain(V_MAP, false, 2, "customParameters", "externalStreamConfig")) {
			//FROM ORIGIN
			return SD_ORIGIN_EDGE;
		} else {
			//FROM PUBLISHER
			return SD_PUBLISHER_EDGE;
		}
	} else if (TAG_KIND_OF(streamType, ST_OUT_NET_RTMP)) {
		//OUTPUT STREAM
		if (customParameters.HasKeyChain(V_MAP, false, 2, "customParameters", "localStreamConfig")) {
			//TO ORIGIN
			return SD_EDGE_ORIGIN;
		} else {
			//TO PLAYER
			return SD_EDGE_PLAYER;
		}
	} else {
		return SD_NOT_SUPPORTED;
	}
}

bool EdgeApplication::ProcessIngestPointsList() {
	string filePath = _configuration["ingestPointsPersistenceFile"];

	Variant setup;
	if (!Variant::DeserializeFromXmlFile(filePath, setup)) {
		FATAL("Unable to load file %s", STR(filePath));
		return false;
	}

	if (setup != V_MAP) {
		FATAL("Invalid ingest points persistence file");
		return false;
	}

	FOR_MAP(setup, string, Variant, i) {
		if (MAP_VAL(i) != V_STRING)
			continue;
		string privateStreamName = MAP_KEY(i);
		string publicStreamName = MAP_VAL(i);
		if (!CreateIngestPoint(privateStreamName, publicStreamName)) {
			WARN("Unable to create ingest point %s -> %s",
					STR(privateStreamName),
					STR(publicStreamName));
		}
	}

	return true;
}

void EdgeApplication::GatherStats(uint64_t flags, Variant &stats) {

	if (((flags & REPORT_FLAG_RTMP_OUTBOUND) > 0) && (_pRTMPHandler != NULL)) {
		_pRTMPHandler->GetRtmpOutboundStats(stats);
	}

}

