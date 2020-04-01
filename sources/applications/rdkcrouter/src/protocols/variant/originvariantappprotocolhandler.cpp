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


#include "protocols/variant/originvariantappprotocolhandler.h"
#include "protocols/variant/basevariantprotocol.h"
#include "netio/netio.h"
#include "protocols/variant/clusteringmessages.h"
#include "protocols/cli/cliappprotocolhandler.h"
#include "application/baseclientapplication.h"
#ifdef HAS_WEBSERVER
#include "application/originapplication.h"
#endif /*HAS_WEBSERVER*/
#include "protocols/protocolmanager.h"
#include "protocols/variant/basevariantprotocol.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "protocols/rtp/rtspprotocol.h"
#include "eventlogger/eventlogger.h"
using namespace app_rdkcrouter;

OriginVariantAppProtocolHandler::OriginVariantAppProtocolHandler(Variant &configuration)
: BaseVariantAppProtocolHandler(configuration) {
	_idGenerator = 1;
}

OriginVariantAppProtocolHandler::~OriginVariantAppProtocolHandler() {
}

void OriginVariantAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
	BaseVariantAppProtocolHandler::RegisterProtocol(pProtocol);
}

void OriginVariantAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
	BaseVariantAppProtocolHandler::UnRegisterProtocol(pProtocol);
	//FATAL("Unblock all pending connections. One edge went down");
	map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();

	FOR_MAP(handlers, uint32_t, IOHandler *, i) {
		IOHandler *pIOHandler = MAP_VAL(i);
		if (pIOHandler->GetType() != IOHT_ACCEPTOR)
			continue;
		bool dropResult = false;
		do {
			dropResult = ((TCPAcceptor *) pIOHandler)->Drop();
		} while (dropResult);
		IOHandlerManager::EnableAcceptConnections(pIOHandler);
	}
}

void OriginVariantAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	Variant &message = parameters["payload"];
	uint64_t pid = (uint64_t) message["targetPid"];
	MAP_ERASE1(_edges, pid);
	WARN("Edge with pid %"PRIu64" went down", pid);
}

bool OriginVariantAppProtocolHandler::ProcessMessage(BaseVariantProtocol *pProtocol,
		Variant &lastSent, Variant &lastReceived) {
	if (pProtocol->GetType() != PT_BIN_VAR) {
		FATAL("Incorrect protocol type");
		return false;
	}
	if (!lastReceived.HasKeyChain(V_STRING, true, 1, "type")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		pProtocol->EnqueueForDelete();
		return false;
	}
	if (!lastReceived.HasKeyChain(_V_NUMERIC, true, 1, "pid")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		pProtocol->EnqueueForDelete();
		return false;
	}
	if (!lastReceived.HasKeyChain(V_MAP, true, 1, "payload")) {
		WARN("Invalid message:\n%s", STR(lastReceived.ToString()));
		pProtocol->EnqueueForDelete();
		return false;
	}
	string type = lastReceived["type"];
	uint64_t pid = (uint64_t) lastReceived["pid"];
	lastReceived["payload"]["pid"] = pid;
	bool result = false;
	SetProtocolId(pid, pProtocol->GetId());
	if (type == "hello") {
		result = ProcessMessageHello(pid, lastReceived["payload"]);
	} else if (type == "acceptFailed") {
		result = ProcessMessageAcceptFailed(pid, lastReceived["payload"]);
	} else if (type == "acceptSucceeded") {
		result = ProcessMessageAcceptSucceeded(pid, lastReceived["payload"]);
	} else if (type == "stats") {
		result = ProcessMessageStats(pid, lastReceived["payload"]);
	} else if (type == "statusReport") {
		result = ProcessMessageStatusReport(pid, lastReceived["payload"]);
	} else if (type == "listStreamsIds") {
		result = ProcessMessageListStreamsIds(pid, lastReceived["payload"]);
	} else if (type == "getStreamInfo") {
		result = ProcessMessageGetStreamInfo(pid, lastReceived["payload"]);
	} else if (type == "listStreams") {
		result = ProcessMessageListStreams(pid, lastReceived["payload"]);
	} else if (type == "listInboundStreamNames") {
		result = ProcessMessageListInboundStreamNames(pid, lastReceived["payload"]);
	} else if (type == "isStreamRunning") {
		result = ProcessMessageIsStreamRunning(pid, lastReceived["payload"]);
	} else if (type == "listInboundStreams") {
		result = ProcessMessageListInboundStreams(pid, lastReceived["payload"]);
	} else if (type == "listConnectionsIds") {
		result = ProcessMessageListConnectionsIds(pid, lastReceived["payload"]);
	} else if (type == "getConnectionInfo") {
		result = ProcessMessageGetConnectionInfo(pid, lastReceived["payload"]);
	} else if (type == "listConnections") {
		result = ProcessMessageListConnections(pid, lastReceived["payload"]);
	} else if (type == "shutdownStream") {
		result = ProcessMessageShutdownStream(pid, lastReceived["payload"]);
	} else if (type == "signalOutStreamAttached") {
		result = ProcessMessageSignalOutStreamAttached(pid, lastReceived["payload"]);
	} else if (type == "edgeEventDetails") {
		result = ProcessMessageLogEvent(pid, lastReceived["payload"]);
	} else if (type == "clientsConnected") {
		result = ProcessMessageClientsConnected(pid, lastReceived["payload"]);
	} else if (type == "getStreamsCount") {
		result = ProcessMessageGetStreamsCount(pid, lastReceived["payload"]);
#ifdef HAS_WEBSERVER
	} else if (type == "httpStreamingSessions" ||
		type == "addGroupNameAlias" ||
		type == "getGroupNameByAlias" ||
		type == "removeGroupNameAlias" ||
		type == "flushGroupNameAliases" ||
		type == "listGroupNameAliases" ||
		type == "httpClientsConnected") {
			result = ProcessWebServerMessage(pid, lastReceived["payload"]);
#endif /*HAS_WEBSERVER*/
	} else {
		WARN("Invalid message type: %s", STR(type));
		result = false;
	}
	SetProtocolId(pid, pProtocol->GetId());
	return result;
}

bool OriginVariantAppProtocolHandler::AcceptTCPConnection(TCPAcceptor *pTCPAcceptor) {
	if (_edges.size() == 0)
		return pTCPAcceptor->Accept();
#ifdef HAS_WEBSERVER
	else if (AreEdgesWebServers())
		return pTCPAcceptor->Accept();
#endif
	EdgeServer &edge = _dummy;
	uint32_t min = 0xffffffff;

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (MAP_VAL(i).IsWebServer())
			continue;
#endif		
		if (min > (uint32_t) MAP_VAL(i).Load()) {
			edge = MAP_VAL(i);
			min = (uint32_t) MAP_VAL(i).Load();
		}
	}

	IOHandlerManager::DisableAcceptConnections(pTCPAcceptor);

	return Send(edge, ClusteringMessages::Accept(pTCPAcceptor));
}

void OriginVariantAppProtocolHandler::RefreshConnectionsCount() {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif		
		Send(MAP_VAL(i), ClusteringMessages::GetStats());
	}
}

map<uint64_t, EdgeServer> &OriginVariantAppProtocolHandler::GetEdges() {
	return _edges;
}

bool OriginVariantAppProtocolHandler::EnqueueCall(Variant callbackInfo) {
	//1. Prepare the callback
	callbackInfo["lastUpdate"] = (uint64_t) time(NULL);
#ifdef HAS_WEBSERVER	
	callbackInfo["edgesCount"] = (uint32_t) _edges.size() - CountWebServers();
#else
	callbackInfo["edgesCount"] = (uint32_t) _edges.size();
#endif
	callbackInfo["uniqueRequestId"] = ++_idGenerator;
	callbackInfo["edges"].IsArray(true);

	//2. Do we have any edges?
	if (_edges.size() == 0)
		return FinishCallback(callbackInfo);
#ifdef HAS_WEBSERVER
	else if (AreEdgesWebServers()) {
		//Finish immediately if we only have web servers and the operation is not intended for web servers
		string operation = callbackInfo["operation"];
		if  (!(operation == "getHttpStreamingSessions" || operation == "addGroupNameAlias" ||
				operation == "getGroupNameByAlias" || operation == "removeGroupNameAlias" ||
				operation == "flushGroupNameAliases" || operation == "listGroupNameAliases" ||
				operation == "getHttpClientsConnected"))
			return FinishCallback(callbackInfo);
	}
#endif /*HAS_WEBSERVER*/
	//2. Get the operation
	string operation = callbackInfo["operation"];
	if (operation == "listStreamsIds") {
		_pendingCallbacks[(uint64_t)callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListStreamsIds((uint64_t)callbackInfo["uniqueRequestId"]);
		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif				
				Send(MAP_VAL(i), callbackInfo["request"]);
		}
		return true;
	}
	else if (operation == "getStreamInfo") {
		uint64_t edgeId = (uint64_t)callbackInfo["edgeId"];
		if (!MAP_HAS1(_edges, edgeId))
			return FinishCallback(callbackInfo);
		_pendingCallbacks[(uint64_t)callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetStreamInfo(
			(uint32_t)callbackInfo["streamId"],
			(uint64_t)callbackInfo["uniqueRequestId"]);
		callbackInfo["edgesCount"] = (uint32_t)1;
		return Send(_edges[edgeId], callbackInfo["request"]);
	} else if (operation == "getStreamsCount") {
		_pendingCallbacks[(uint64_t)callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetStreamsCount((uint64_t)callbackInfo["uniqueRequestId"]);
		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif				
				Send(MAP_VAL(i), callbackInfo["request"]);
		}
		return true;
	} else if (operation == "listStreams") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListStreams((uint64_t) callbackInfo["uniqueRequestId"]);
		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer()) 
#endif				
			Send(MAP_VAL(i), callbackInfo["request"]);
		}
		return true;
	/**
	 * Used by listInboundStreamNames API command
	 * Provides a list of all the current inbound localstreamnames.
	 */
	} else if (operation == "listInboundStreamNames") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListInboundStreamNames((uint64_t) callbackInfo["uniqueRequestId"]);

		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif
			Send(MAP_VAL(i), callbackInfo["request"]);
		}

		return true;
	/**
	 * Used by listInboundStreams API command
	 * Provides a list of inbound streams that are not intermediate.
	 */
	} else if (operation == "listInboundStreams") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListInboundStreams((uint64_t) callbackInfo["uniqueRequestId"]);

		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif
			Send(MAP_VAL(i), callbackInfo["request"]);
		}

		return true;
	/**
	 * Used by isStreamRunning API command
	 * Checks a specific stream if it is running or not.
	 */
	} else if (operation == "isStreamRunning") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetIsStreamRunning((uint32_t) callbackInfo["streamId"], 
																		 (uint64_t) callbackInfo["uniqueRequestId"]);

		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif
			Send(MAP_VAL(i), callbackInfo["request"]);
		}

		return true;
	} else if (operation == "listConnectionsIds") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListConnectionsIds((uint64_t) callbackInfo["uniqueRequestId"]);

		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif
			Send(MAP_VAL(i), callbackInfo["request"]);
		}

		return true;
	} else if (operation == "getConnectionInfo") {
		uint64_t edgeId = (uint64_t) callbackInfo["edgeId"];
		if (!MAP_HAS1(_edges, edgeId))
			return FinishCallback(callbackInfo);
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetConnectionInfo(
				(uint32_t) callbackInfo["connectionId"],
				(uint64_t) callbackInfo["uniqueRequestId"]);
		callbackInfo["edgesCount"] = (uint32_t) 1;
		return Send(_edges[edgeId], callbackInfo["request"]);
	} else if (operation == "listConnections") {
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetListConnections((uint64_t) callbackInfo["uniqueRequestId"]);

		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif
			Send(MAP_VAL(i), callbackInfo["request"]);
		}

		return true;
	} else if (operation == "shutdownStream") {
		uint64_t edgeId = (uint64_t) callbackInfo["edgeId"];
		if (!MAP_HAS1(_edges, edgeId))
			return FinishCallback(callbackInfo);
		_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::ShutdownStream(
				(uint32_t) callbackInfo["streamId"],
				(uint64_t) callbackInfo["uniqueRequestId"]);
		callbackInfo["edgesCount"] = (uint32_t) 1;
		return Send(_edges[edgeId], callbackInfo["request"]);
#ifdef HAS_WEBSERVER
	} else if (operation == "getHttpStreamingSessions" ||
		operation == "addGroupNameAlias" ||
		operation == "getGroupNameByAlias" ||
		operation == "removeGroupNameAlias" ||
		operation == "flushGroupNameAliases" ||
		operation == "listGroupNameAliases" ||
		operation == "getHttpClientsConnected") {
			_pendingCallbacks[(uint64_t) callbackInfo["uniqueRequestId"]] = callbackInfo;
			callbackInfo["request"] = ClusteringMessages::SendRequestToWebServers(operation, callbackInfo);
			FOR_MAP(_edges, uint64_t, EdgeServer, i) {
				if (MAP_VAL(i).IsWebServer())
					Send(MAP_VAL(i), callbackInfo["request"]);
			}
			return true;
#endif /*HAS_WEBSERVER*/
	} else if (operation == "clientsConnected") {
		_pendingCallbacks[(uint64_t)callbackInfo["uniqueRequestId"]] = callbackInfo;
		callbackInfo["request"] = ClusteringMessages::GetClientsConnected((uint64_t)callbackInfo["uniqueRequestId"]);
		FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
			if (!MAP_VAL(i).IsWebServer())
#endif				
				Send(MAP_VAL(i), callbackInfo["request"]);
		}
		return true;
	} else {
		FATAL("Invalid operation %s", STR(operation));
		return false;
	}
}

bool OriginVariantAppProtocolHandler::FinishCallbacks() {
	map<uint64_t, Variant> temp = _pendingCallbacks;
	uint64_t now = (uint64_t) time(NULL);

	FOR_MAP(temp, uint64_t, Variant, i) {
		uint64_t wait = now - (uint64_t) MAP_VAL(i)["lastUpdate"];
		if (wait > 30) {
			WARN("callback took too long time to complete. Forcing it...");
			FinishCallback(MAP_VAL(i));
		}
	}

	return true;
}
void OriginVariantAppProtocolHandler::InsertPlaylistItem(string &playlistName, 
		string &localStreamName,	double insertPoint, double sourceOffset, double duration) {
	Variant params;
	params["playlistName"] = playlistName;
	params["localStreamName"] = localStreamName;
	params["insertPoint"] = insertPoint;
	params["sourceOffset"] = sourceOffset;
	params["duration"] = duration;

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::InsertPlaylistItem(params));
	}
}

void OriginVariantAppProtocolHandler::ResetMaxFdCounters() {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetResetMaxFdCounters());
	}
}

void OriginVariantAppProtocolHandler::ResetTotalFdCounters() {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetResetTotalFdCounters());
	}
}

void OriginVariantAppProtocolHandler::SetStreamAlias(string &localStreamName, string &aliasName, int64_t expirePeriod) {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetSetStreamAlias(localStreamName, aliasName, expirePeriod));
	}
}

void OriginVariantAppProtocolHandler::RemoveStreamAlias(const string &aliasName) {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetRemoveStreamAlias(aliasName));
	}
}

void OriginVariantAppProtocolHandler::ShutdownEdges() {
#ifndef WIN32
	if (_edges.size() == 0)
		return;

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (MAP_VAL(i).IsWebServer())
			continue;
#endif		
		WARN("Stopping edge %"PRIu64, MAP_KEY(i));
		kill(MAP_KEY(i), SIGTERM);
	}

	uint32_t iterations = 0;

	while ((iterations++) < 10) {
		int status = 0;
		// In order for the loop to work, we need to pass the option WNOHANG
		pid_t pid = waitpid(-1, &status, WNOHANG);
		int err = errno;
		if (pid > 0)
			continue;
		if ((pid == -1) && (err == ECHILD)) {
			//			WARN("*****************************");
			//			WARN("* All edges are now stopped *");
			//			WARN("*****************************");
			_edges.clear();
			return;
		}
		sleep(1);
	}
	FATAL("Not all child processes could be stopped");
	_edges.clear();
#endif /* WIN32 */
}

void OriginVariantAppProtocolHandler::SignalInpuStreamAvailable(string streamName) {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetSignalInpuStreamAvailable(streamName));
	}
}

void OriginVariantAppProtocolHandler::CreateIngestPoint(string &privateStreamName,
		string &publicStreamName) {

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		Send(MAP_VAL(i), ClusteringMessages::GetCreateIngestPoint(
				privateStreamName, publicStreamName));
	}
}

string OriginVariantAppProtocolHandler::GetEdgesInfo() {
	string result = "";

	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (!MAP_VAL(i).IsWebServer())
#endif
		result += format("%"PRIu32"(%"PRIu64" - %"PRIu32"), ",
				MAP_VAL(i).Load(), MAP_VAL(i).Pid(), MAP_VAL(i).LoadIncrement());
	}
	return result;
}

void OriginVariantAppProtocolHandler::SetIncrement(uint64_t pid, bool ok) {
	if (MAP_HAS1(_edges, pid)) {
		SetIncrement(_edges[pid], ok);
	}
}

void OriginVariantAppProtocolHandler::SetIncrement(EdgeServer &edge, bool ok) {
	if (ok) {
		edge.LoadIncrement(1);
	} else {
		uint64_t temp = edge.LoadIncrement()*10;
		if (temp > 1000)
			edge.LoadIncrement(1000);
		else
			edge.LoadIncrement((uint32_t) temp);
	}
}

void OriginVariantAppProtocolHandler::SetProtocolId(uint64_t pid,
		uint32_t protocolId) {
	if (MAP_HAS1(_edges, pid)) {
		_edges[pid].ProtocolId(protocolId);
	}
}

void OriginVariantAppProtocolHandler::IncrementConnectionsCount(uint64_t pid) {
	if (MAP_HAS1(_edges, pid)) {
		IncrementConnectionsCount(_edges[pid]);
	}
}

void OriginVariantAppProtocolHandler::IncrementConnectionsCount(EdgeServer &edge) {
	if ((edge.Load() + edge.LoadIncrement()) > 0xffffffff)
		return;
	edge.Load(edge.Load() + edge.LoadIncrement());
}

void OriginVariantAppProtocolHandler::SetConnectionsCount(uint64_t pid,
		uint32_t connectionsCount) {
	if (MAP_HAS1(_edges, pid)) {
		SetConnectionsCount(_edges[pid], connectionsCount);
	}
}

void OriginVariantAppProtocolHandler::SetConnectionsCount(EdgeServer &edge,
		uint32_t connectionsCount) {
	SetIncrement(edge, true);
	edge.Load(connectionsCount);
}

bool OriginVariantAppProtocolHandler::EnableAcceptor(uint32_t id) {
	map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();
	if (!MAP_HAS1(handlers, id)) {
		return false;
	}
	if (handlers[id]->GetType() != IOHT_ACCEPTOR) {
		return false;
	}
	return IOHandlerManager::EnableAcceptConnections(handlers[id]);
}

bool OriginVariantAppProtocolHandler::Send(EdgeServer &edge, Variant &message) {
	// Before sending, add the target pid of the edge server
	message["targetPid"] = edge.Pid();

	BaseVariantProtocol *pProtocol =
			(BaseVariantProtocol *) ProtocolManager::GetProtocol(edge.ProtocolId());
	if (pProtocol == NULL) {
		return BaseVariantAppProtocolHandler::Send(edge.Ip(), edge.Port(),
				message, VariantSerializer_BIN);
	} else {
		return pProtocol->Send(message);
	}
}

bool OriginVariantAppProtocolHandler::ProcessMessageHello(
		uint64_t pid, Variant &message) {

#ifndef HAS_WEBSERVER
	EdgeServer es;
	es.Ip(message["ip"]);
	es.Port(message["port"]);
	es.Pid(message["pid"]);
	es.LoadIncrement(1);
	es.Load(0);
	_edges[pid] = es;
	INFO("New edge available: ip/port: %s:%"PRIu16"; pid: %"PRIu64,
		STR(es.Ip()),
		es.Port(),
		es.Pid());
#else
	if (!MAP_HAS1(_edges, pid)) {
		EdgeServer es;
		es.Ip(message["ip"]);
		es.Port(message["port"]);
		es.Pid(message["pid"]);
		es.LoadIncrement(1);
		es.Load(0);
		es.IsWebServer(message["isWebServer"]);
		_edges[pid] = es;
		INFO("New edge available%s: ip/port: %s:%"PRIu16"; pid: %"PRIu64,
			es.IsWebServer() ? " (Web Server)" : "",
			STR(es.Ip()),
			es.Port(),
			es.Pid());
	}
#endif
	return true;
}

bool OriginVariantAppProtocolHandler::ProcessMessageAcceptFailed(
		uint64_t pid, Variant &message) {
	SetIncrement(pid, false);
	IncrementConnectionsCount(pid);
	return EnableAcceptor(message["id"]);
}

bool OriginVariantAppProtocolHandler::ProcessMessageAcceptSucceeded(
		uint64_t pid, Variant &message) {
	SetIncrement(pid, true);
	IncrementConnectionsCount(pid);
	return EnableAcceptor(message["id"]);
}

bool OriginVariantAppProtocolHandler::ProcessMessageStats(
		uint64_t pid, Variant &message) {
	SetConnectionsCount(pid, (uint32_t) message["connectionsCount"]);
	if (MAP_HAS1(_edges, pid)) {
		_edges[pid].StreamsCount(message["streamsCount"]);
		_edges[pid].ConnectionsCount(message["connectionsCount"]);
		message["IOStats"]["edgeId"] = (uint64_t) pid;
		_edges[pid].IOStats(message["IOStats"]);
	}

	return true;
}
bool OriginVariantAppProtocolHandler::ProcessMessageStatusReport(
		uint64_t pid, Variant &message) {
	// TODO:
	BaseClientApplication *pApp = GetApplication();
	Variant feedback;
	feedback["header"] = "statusReport";
	feedback["payload"]["pid"] = pid;
	feedback["payload"]["stats"] = message["parameters"];
	pApp->SignalApplicationMessage(feedback);
	return true;
}
bool OriginVariantAppProtocolHandler::ProcessMessageListStreamsIds(
		uint64_t pid, Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t)callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t)edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["allStreams"] = message["allStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageGetStreamInfo(
		uint64_t pid, Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	callbackInfo["streamInfo"] = message["streamInfo"];

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageListStreams(uint64_t pid,
		Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["allStreams"] = message["allStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

/**
* Process the clientsConnected API command result from edges
*/

bool OriginVariantAppProtocolHandler::ProcessMessageClientsConnected(uint64_t pid,
	Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t)callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t)edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["allStreams"] = message["allStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

/**
* Process the GetStreamsCount API command result from edges
*/

bool OriginVariantAppProtocolHandler::ProcessMessageGetStreamsCount(uint64_t pid,
	Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t)callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t)edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["allStreams"] = message["allStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

/**
 * Process the listInboundStreamNames API command result from edges
 */
bool OriginVariantAppProtocolHandler::ProcessMessageListInboundStreamNames(uint64_t pid,
		Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["inboundStreams"] = message["inboundStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

/**
 * Process the listInboundStreams API command result from edges
 */
bool OriginVariantAppProtocolHandler::ProcessMessageListInboundStreams(uint64_t pid,
		Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["inboundStreams"] = message["inboundStreams"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

/**
 * Process the isStreamRunning API command result from edges
 */
bool OriginVariantAppProtocolHandler::ProcessMessageIsStreamRunning(uint64_t pid,
		Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	if (message.HasKey("streamStatus", true)) {
		edge["streamStatus"] = message["streamStatus"];
	}
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageListConnectionsIds(
		uint64_t pid, Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["connectionsIds"] = message["connectionsIds"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageGetConnectionInfo(
		uint64_t pid, Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	callbackInfo["connectionInfo"] = message["connectionInfo"];

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageListConnections(uint64_t pid,
		Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	uint32_t edgesCount = (uint32_t) callbackInfo["edgesCount"] - 1;
	callbackInfo["edgesCount"] = (uint32_t) edgesCount;

	Variant edge;
	edge["pid"] = pid;
	edge["allConnections"] = message["allConnections"];
	callbackInfo["edges"].PushToArray(edge);

	if (edgesCount != 0) {
		return true;
	}

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageShutdownStream(
		uint64_t pid, Variant &message) {
	uint64_t uniqueId = message["uniqueRequestId"];
	if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
		return true;
	}
	Variant &callbackInfo = _pendingCallbacks[uniqueId];
	callbackInfo["result"] = message["result"];

	return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::ProcessMessageSignalOutStreamAttached(
		uint64_t pid, Variant &message) {
	string streamName = message["streamName"];
	BaseClientApplication *pApp = NULL;
	StreamsManager *pSM = NULL;
	if (((pApp = GetApplication()) == NULL)
			|| ((pSM = pApp->GetStreamsManager()) == NULL)) {
		FATAL("Unable to get the streams manager");
		return false;
	}

	map<uint32_t, BaseStream *> streams = pSM->FindByTypeByName(
			ST_IN_NET_RTP, streamName, false, false);
	if (streams.size() == 0)
		return true;
	BaseProtocol *pProtocol = MAP_VAL(streams.begin())->GetProtocol();
	if ((pProtocol == NULL) || (pProtocol->GetType() != PT_RTSP))
		return true;
	((RTSPProtocol *) pProtocol)->SignalOutStreamAttached();
	return true;
}

bool OriginVariantAppProtocolHandler::ProcessMessageLogEvent(
		uint64_t pid, Variant &message) {
	Variant &eventDetails = message["eventDetails"];
	BaseClientApplication * pApp = GetApplication();
	if (pApp == NULL)
		return false;
	EventLogger *pEvtLogger = pApp->GetEventLogger();
	if (pEvtLogger == NULL)
		return false;
	pEvtLogger->LogEdgeRelayedEvent(eventDetails);
	return true;
}

#ifdef HAS_WEBSERVER
bool OriginVariantAppProtocolHandler::ProcessWebServerMessage(
	uint64_t pid, Variant &message) {
		uint64_t uniqueId = message["uniqueRequestId"];
		if (!MAP_HAS1(_pendingCallbacks, uniqueId)) {
			return true;
		}
		Variant &callbackInfo = _pendingCallbacks[uniqueId];
		string operation = callbackInfo["operation"];
		callbackInfo["result"] = message["result"];
		if (operation == "getHttpStreamingSessions") {
			callbackInfo["httpStreamingSessions"] = message["httpStreamingSessions"];		
		} else if (operation == "addGroupNameAlias" ||
			operation == "removeGroupNameAlias" ||
			operation == "flushGroupNameAliases") {

		} else if (operation == "getGroupNameByAlias") {
			callbackInfo["groupName"] = message["groupName"];
		} else if (operation == "listGroupNameAliases") {
			callbackInfo["groupNameAliases"] = message["groupNameAliases"];
		} else if (operation == "getHttpClientsConnected") {
			callbackInfo["httpClientsConnected"] = message["httpClientsConnected"];
		}
		return FinishCallback(callbackInfo);
}

bool OriginVariantAppProtocolHandler::AreEdgesWebServers() {
	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
		if (!MAP_VAL(i).IsWebServer()) 
			return false;
	}
	return true;
}

uint8_t OriginVariantAppProtocolHandler::CountWebServers() {
	uint8_t webServerCount = 0;
	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
		if (MAP_VAL(i).IsWebServer()) 
			webServerCount++;
	}
	return webServerCount;	
}

#endif /*HAS_WEBSERVER*/


bool OriginVariantAppProtocolHandler::FinishCallback(Variant &callbackInfo) {
	bool result = true;
	CLIAppProtocolHandler *pCLI =
			(CLIAppProtocolHandler *) GetApplication()->GetProtocolHandler((uint64_t) PT_INBOUND_JSONCLI);
	if (pCLI == NULL) {
		FATAL("Unable to get the handler");
		result = false;
	}

	if (result) {
		//2. Get the operation
		string operation = callbackInfo["operation"];
		if (operation == "listStreamsIds") {
			result = pCLI->ListStreamsIdsResult(callbackInfo);
		} else if (operation == "getStreamInfo") {
			result = pCLI->GetStreamInfoResult(callbackInfo);
		} else if (operation == "listStreams") {
			result = pCLI->ListStreamsResult(callbackInfo);
		} else if (operation == "listInboundStreamNames") {
			result = pCLI->ListInboundStreamNamesResult(callbackInfo);
		} else if (operation == "listInboundStreams") {
			result = pCLI->ListInboundStreamsResult(callbackInfo);
		} else if (operation == "isStreamRunning") {
			result = pCLI->GetStreamStatus(callbackInfo);
		} else if (operation == "listConnectionsIds") {
			result = pCLI->ListConnectionsIdsResult(callbackInfo);
		} else if (operation == "getConnectionInfo") {
			result = pCLI->GetConnectionInfoResult(callbackInfo);
		} else if (operation == "listConnections") {
			result = pCLI->ListConnectionsResult(callbackInfo);
		} else if (operation == "shutdownStream") {
			result = pCLI->ShutdownStreamResult(callbackInfo);
#ifdef HAS_WEBSERVER
		} else if (operation == "getHttpStreamingSessions") {
			result = pCLI->GetListHttpStreamingSessionsResult(callbackInfo);
		} else if (operation == "getHttpClientsConnected") {
			result = pCLI->GetHttpClientsConnectedResult(callbackInfo);
		} else if (operation == "addGroupNameAlias") {
			result = pCLI->GetAddGroupNameAliasResult(callbackInfo);
		} else if (operation == "getGroupNameByAlias") {
			result = pCLI->GetGroupNameByAliasResult(callbackInfo);
		} else if (operation == "removeGroupNameAlias") {
			result = pCLI->GetRemoveGroupNameAliasResult(callbackInfo);
		} else if (operation == "flushGroupNameAliases") {
			result = pCLI->GetFlushGroupNameAliasesResult(callbackInfo);
		} else if (operation == "listGroupNameAliases") {
			result = pCLI->GetListGroupNameAliasesResult(callbackInfo);
		} else if (operation == "clientsConnected") {
			result = pCLI->GetClientsConnectedResult(callbackInfo);
		} else if (operation == "getStreamsCount") {
			result = pCLI->GetStreamsCountResult(callbackInfo);
#endif /*HAS_WEBSERVER*/
		} else {
			FATAL("Invalid operation %s", STR(operation));
			result = false;
		}
	}

	uint64_t uniqueRequestId = callbackInfo["uniqueRequestId"];
	if (MAP_HAS1(_pendingCallbacks, uniqueRequestId)) {
		_pendingCallbacks.erase(uniqueRequestId);
	}

	return result;
}

void OriginVariantAppProtocolHandler::QueryEdgeStatus(Variant &reportInfo) {
	Variant message;
	if (_edges.size() == 0) {
		message["header"] = "statusReport";
		message["payload"] = reportInfo;
		GetApplication()->SignalApplicationMessage(message);
		return;
	}
#ifdef HAS_WEBSERVER
	else if (AreEdgesWebServers()) {
		message["header"] = "statusReport";
		message["payload"] = reportInfo;
		GetApplication()->SignalApplicationMessage(message);
		return;
	}
#endif
	
	message["queryFlags"] = reportInfo["queryFlags"];
	FOR_MAP(_edges, uint64_t, EdgeServer, i) {
#ifdef HAS_WEBSERVER
		if (MAP_VAL(i).IsWebServer())
			continue;
#endif		
		Send(MAP_VAL(i), ClusteringMessages::StatusReport(message));
		reportInfo["edgeIds"][format("%"PRIu64, MAP_VAL(i).Pid())] = MAP_VAL(i).Pid();
	}
}

