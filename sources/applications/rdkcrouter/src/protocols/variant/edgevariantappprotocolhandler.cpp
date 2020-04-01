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


#include "protocols/variant/edgevariantappprotocolhandler.h"
#include "protocols/variant/clusteringmessages.h"
#include "protocols/protocolmanager.h"
#include "netio/netio.h"
#include "streaming/streamsmanager.h"
#include "application/baseclientapplication.h"
#include "streaming/basestream.h"
#include "streaming/streamstypes.h"
#include "protocols/variant/basevariantprotocol.h"
#include "application/filters.h"
#include "application/edgeapplication.h"
using namespace app_rdkcrouter;

EdgeVariantAppProtocolHandler::EdgeVariantAppProtocolHandler(Variant &configuration)
: BaseVariantAppProtocolHandler(configuration) {
	_streamInfos.IsArray(false);
	_protocolInfos.IsArray(false);
	_protocolId = 0;
}

EdgeVariantAppProtocolHandler::~EdgeVariantAppProtocolHandler() {
}

void EdgeVariantAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	BaseVariantAppProtocolHandler::RegisterProtocol(pProtocol);
	_protocolId = pProtocol->GetId();
}

bool EdgeVariantAppProtocolHandler::ProcessMessage(BaseVariantProtocol *pProtocol,
		Variant &lastSent, Variant &lastReceived) {
	if (!lastReceived.HasKeyChain(V_STRING, true, 1, "type")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		return false;
	}
	if (!lastReceived.HasKeyChain(V_MAP, true, 1, "payload")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		return false;
	}
	string type = lastReceived["type"];
	if (type == "accept") {
		return ProcessMessageAccept(lastReceived["payload"]);
	} else if (type == "getStats") {
		return ProcessMessageGetStats(lastReceived["payload"]);
	} else if (type == "listStreamsIds") {
		return ProcessMessageListStreamsIds(lastReceived["payload"]);
	} else if (type == "getStreamInfo") {
		return ProcessMessageGetStreamInfo(lastReceived["payload"]);
	} else if (type == "listStreams") {
		return ProcessMessageListStreams(lastReceived["payload"]);
	} else if (type == "listInboundStreamNames") {
		return ProcessMessageListInboundStreamNames(lastReceived["payload"]);
	} else if (type == "isStreamRunning") {
		return ProcessMessageIsStreamRunning(lastReceived["payload"]);
	} else if (type == "listInboundStreams") {
		return ProcessMessageListInboundStreams(lastReceived["payload"]);
	} else if (type == "listConnectionsIds") {
		return ProcessMessageListConnectionsIds(lastReceived["payload"]);
	} else if (type == "getConnectionInfo") {
		return ProcessMessageGetConnectionInfo(lastReceived["payload"]);
	} else if (type == "listConnections") {
		return ProcessMessageListConnections(lastReceived["payload"]);
	} else if (type == "resetMaxFdCounters") {
		return ProcessMessageResetMaxFdCounters(lastReceived["payload"]);
	} else if (type == "resetTotalFdCounters") {
		return ProcessMessageResetTotalFdCounters(lastReceived["payload"]);
	} else if (type == "setStreamAlias") {
		return ProcessMessageSetStreamAlias(lastReceived["payload"]);
	} else if (type == "shutdownStream") {
		return ProcessMessageShutdownStream(lastReceived["payload"]);
	} else if (type == "statusReport") {
		return ProcessMessageStatusReport(lastReceived["payload"]);
	} else if (type == "signalInpuStreamAvailable") {
		return ProcessMessageSignalInpuStreamAvailable(lastReceived["payload"]);
	} else if (type == "removeStreamAlias") {
		return ProcessMessageRemoveStreamAlias(lastReceived["payload"]);
	} else if (type == "createIngestPoint") {
		return ProcessMessageCreateIngestPoint(lastReceived["payload"]);
	} else if (type == "insertPlaylistItem") {
		return ProcessMessageInsertPlaylistItem(lastReceived["payload"]);
	} else if (type == "clientsConnected") {
		return ProcessMessageClientsConnected(lastReceived["payload"]);
	} else if (type == "getStreamsCount") {
		return ProcessMessageGetStreamsCount(lastReceived["payload"]);
	} else {
		WARN("Invalid message type: %s", STR(type));
		return false;
	}
}

void EdgeVariantAppProtocolHandler::SetConnectorInfo(Variant &info) {
	_info = info;
}

bool EdgeVariantAppProtocolHandler::SendHello() {
	Variant message = ClusteringMessages::Hello(_info["ip"], _info["port"]);
	return Send(message);
}

bool EdgeVariantAppProtocolHandler::SendSignalOutStreamAttached(string streamName) {
	Variant message = ClusteringMessages::SignalOutStreamAttached(streamName);
	return Send(message);
}

bool EdgeVariantAppProtocolHandler::SendEventLog(Variant &eventDetails) {
	WARN("Sending log event to origin");
	Variant message = ClusteringMessages::SendEventLog(eventDetails);
	return Send(message);
}

void EdgeVariantAppProtocolHandler::RegisterInfoStream(uint32_t uniqeStreamId) {
	if (!_streamInfos.HasIndex(uniqeStreamId))
		_streamInfos[uniqeStreamId] = uniqeStreamId;
}

void EdgeVariantAppProtocolHandler::UnRegisterInfoStream(uint32_t uniqeStreamId) {
	_streamInfos.RemoveAt(uniqeStreamId);
}

void EdgeVariantAppProtocolHandler::RegisterInfoProtocol(uint32_t protocolId) {
	if (!_protocolInfos.HasIndex(protocolId))
		_protocolInfos[protocolId] = protocolId;
}

void EdgeVariantAppProtocolHandler::UnRegisterInfoProtocol(uint32_t protocolId) {
	_protocolInfos.RemoveAt(protocolId);
}

bool EdgeVariantAppProtocolHandler::Send(Variant &message) {
	BaseVariantProtocol *pProtocol =
			(BaseVariantProtocol *) ProtocolManager::GetProtocol(_protocolId);
#if 0 //randomly crash connections
	if (pProtocol != NULL) {
		if ((rand() % 4) == 0) {
			pProtocol->EnqueueForDelete();
			pProtocol = NULL;
			_protocolId = 0;
		}
	}
#endif
	if (pProtocol == NULL) {
		return BaseVariantAppProtocolHandler::Send("127.0.0.1", 1113, message,
				VariantSerializer_BIN);
	} else {
		return pProtocol->Send(message);
	}
}

bool EdgeVariantAppProtocolHandler::ProcessMessageAccept(Variant &message) {
#if 0
	if ((rand() % 3) == 0) {
		FINEST("Drop out tests");
		return false;
	}
#endif /* 0 */
	uint32_t id = (uint32_t) message["id"];
	map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();
	if (!MAP_HAS1(handlers, id))
		return Send(ClusteringMessages::AcceptFailed(message));
	if (handlers[id]->GetType() != IOHT_ACCEPTOR)
		return Send(ClusteringMessages::AcceptFailed(message));
	if (!((TCPAcceptor *) handlers[id])->Accept())
		return Send(ClusteringMessages::AcceptFailed(message));
	return Send(ClusteringMessages::AcceptSucceeded(message));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageGetStats(Variant &message) {
	return Send(ClusteringMessages::Stats(IOHandlerManager::GetStats(true),
			(uint32_t) GetApplication()->GetStreamsManager()->GetAllStreams().size()));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageListStreamsIds(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pSM->GetAllStreams();

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		info.Reset();
		MAP_VAL(i)->GetStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetListStreamsIdsResult(result,
		(uint64_t)message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageClientsConnected(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> allStreams = pSM->GetAllStreams();

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		info.Reset();
		MAP_VAL(i)->GetStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetClientsConnectedResult(result,
		(uint64_t)message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageGetStreamsCount(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> allStreams = pSM->GetAllStreams();

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(allStreams, uint32_t, BaseStream *, i) {
		info.Reset();
		MAP_VAL(i)->GetStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetStreamsCountResult(result,
		(uint64_t)message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageGetStreamInfo(Variant &message) {
	//1. Prepare the stream info
	StreamsManager *pSM = GetApplication()->GetStreamsManager();

	//2. Get the stream
	BaseStream *pStream = pSM->FindByUniqueId(message["streamId"]);

	//3. Get the info
	Variant info;
	if (pStream != NULL)
		pStream->GetStats(info, GetPid());

	return Send(ClusteringMessages::GetStreamInfoResult(info,
			(uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageListStreams(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pSM->GetAllStreams();

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		info.Reset();
		MAP_VAL(i)->GetStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetListStreamsResult(result,
			(uint64_t) message["uniqueRequestId"]));
}

/**
 * Process the listInboundStreamNames API command sent by origin
 */
bool EdgeVariantAppProtocolHandler::ProcessMessageListInboundStreamNames(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pSM->FindByType(ST_IN, true);

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		info.Reset();
		info["name"] =  MAP_VAL(i)->GetName();
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetListInboundStreamNamesResult(result,
			(uint64_t) message["uniqueRequestId"]));
}

/**
 * Process the listInboundStreams API command sent by origin
 */
bool EdgeVariantAppProtocolHandler::ProcessMessageListInboundStreams(Variant &message) {
	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	map<uint32_t, BaseStream *> streams = pSM->FindByType(ST_IN, true);

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		info.Reset();
		MAP_VAL(i)->GetStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetListInboundStreamsResult(result,
			(uint64_t) message["uniqueRequestId"]));
}
bool EdgeVariantAppProtocolHandler::ProcessMessageInsertPlaylistItem(Variant &message) {
	Variant &params = message["parameters"];
	EdgeApplication *pApp = (EdgeApplication *) GetApplication();
	string playlistName = params["playlistName"];
	string localStreamName = params["localStreamName"];
	double insertPoint = params["insertPoint"];
	double sourceOffset = params["sourceOffset"];
	double duration = params["duration"];

	pApp->InsertPlaylistItem(playlistName, localStreamName, insertPoint, sourceOffset, duration);
	return true;
}
bool EdgeVariantAppProtocolHandler::ProcessMessageStatusReport(Variant &message) {
	Variant stats;
	EdgeApplication *pApp = (EdgeApplication *) GetApplication();
	if (pApp == NULL)
		return false;
	Variant parameters = message["parameters"];
	if (parameters.HasKeyChain(_V_NUMERIC, false, 1, "queryFlags")) {
		pApp->GatherStats((uint64_t) parameters["queryFlags"], stats);
	}
	Send(ClusteringMessages::StatusReport(stats));
	return true;
}
/**
 * Process the isStreamRunning API command sent by origin
 */
bool EdgeVariantAppProtocolHandler::ProcessMessageIsStreamRunning(Variant &message) {
	Variant info;
	bool status = false;

	StreamsManager *pSM = GetApplication()->GetStreamsManager();
	BaseStream *pStream = pSM->FindByUniqueId(message["streamId"]);
	if (pStream) {
		status = true;
	}

	info["streamStatus"] = status;

	return Send(ClusteringMessages::GetIsStreamRunningResult(info, (uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageListConnectionsIds(Variant &message) {
	//1. Get all protocols
	map<uint32_t, BaseProtocol *> endpointProtocols;
	ProtocolManager::GetActiveProtocols(endpointProtocols, protocolManagerProtocolTypeFilter);

	Variant allIds;
	allIds.IsArray(false);

	FOR_MAP(endpointProtocols, uint32_t, BaseProtocol *, i) {
		allIds[format("%"PRIu32, MAP_KEY(i))] = (uint32_t) MAP_KEY(i);
	}

	return Send(ClusteringMessages::GetListConnectionsIdsResult(allIds,
			(uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageGetConnectionInfo(Variant &message) {
	//1. Get the connection
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(message["connectionId"]);

	//3. Get the info
	Variant info;
	if (pProtocol != NULL)
		pProtocol->GetStackStats(info, GetPid());

	return Send(ClusteringMessages::GetConnectionInfoResult(info,
			(uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageListConnections(Variant &message) {
	map<uint32_t, BaseProtocol *> protocols;
	ProtocolManager::GetActiveProtocols(protocols, protocolManagerProtocolTypeFilter);

	Variant result;
	result.IsArray(true);
	Variant info;

	FOR_MAP(protocols, uint32_t, BaseProtocol *, i) {
		info.Reset();
		MAP_VAL(i)->GetStackStats(info, GetPid());
		result.PushToArray(info);
	}
	return Send(ClusteringMessages::GetListConnectionsResult(result,
			(uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageResetMaxFdCounters(
		Variant &message) {
	IOHandlerManager::GetStats(false).ResetMax();
	return true;
}

bool EdgeVariantAppProtocolHandler::ProcessMessageResetTotalFdCounters(
		Variant &message) {
	IOHandlerManager::GetStats(false).ResetTotal();
	return true;
}

bool EdgeVariantAppProtocolHandler::ProcessMessageSetStreamAlias(
		Variant &message) {
	string localStreamName = message["localStreamName"];
	string alias = message["aliasName"];
	int64_t expirePeriod = message["expirePeriod"];
	return GetApplication()->SetStreamAlias(localStreamName, alias, expirePeriod, NULL);
}

bool EdgeVariantAppProtocolHandler::ProcessMessageShutdownStream(Variant &message) {
	Variant result;
	BaseStream *pStream = NULL;

	// Get the stream manager
	StreamsManager *pSM = GetApplication()->GetStreamsManager();

	// Get the stream
	//TODO: modify this part if we need to support localstreamname as well
	pStream = pSM->FindByUniqueId(message["streamId"]);

	// Check if pStream is valid
	if (pStream != NULL) {
		// Get the protocol associated with the stream
		BaseProtocol *pProtocol = pStream->GetProtocol();
		if (pProtocol != NULL) {
			//TODO: should consider 'permanently' flag?

			// Get the last stats about the protocol and the stream
			pProtocol->GetStackStats(result["protocolStackInfo"], 0);
			pStream->GetStats(result["streamInfo"], 0);

			// Enqueue for delete
			pProtocol->EnqueueForDelete();
		}
	}

	return Send(ClusteringMessages::ShutdownStreamResult(result,
			(uint64_t) message["uniqueRequestId"]));
}

bool EdgeVariantAppProtocolHandler::ProcessMessageSignalInpuStreamAvailable(Variant &message) {
	string streamName = message["streamName"];
	vector<string> parts;
	split(streamName, "?", parts);
	if (parts.size() < 1)
		return true;
	streamName = parts[0];
	((EdgeApplication *) GetApplication())->SignalOriginInpuStreamAvailable(streamName);
	return true;
}

bool EdgeVariantAppProtocolHandler::ProcessMessageRemoveStreamAlias(Variant &message) {
	string aliasName = message["aliasName"];
	GetApplication()->RemoveStreamAlias(aliasName);
	return true;
}

bool EdgeVariantAppProtocolHandler::ProcessMessageCreateIngestPoint(Variant &message) {
	string privateStreamName = message["privateStreamName"];
	string publicStreamName = message["publicStreamName"];
	GetApplication()->CreateIngestPoint(privateStreamName, publicStreamName);
	return true;
}
