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


#include "protocols/variant/clusteringmessages.h"
#include "netio/netio.h"
using namespace app_rdkcrouter;

Variant ClusteringMessages::_message;
Variant ClusteringMessages::_payload;

Variant & ClusteringMessages::Hello(string ip, uint16_t port) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["ip"] = ip;
	_payload["port"] = (uint16_t) port;
	return Message("hello");
}

Variant & ClusteringMessages::SignalOutStreamAttached(string streamName) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamName"] = streamName;
	return Message("signalOutStreamAttached");
}

Variant & ClusteringMessages::Accept(TCPAcceptor *pAcceptor) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload = pAcceptor->GetParameters();
	_payload["id"] = (uint32_t) pAcceptor->GetId();
	return Message("accept");
}

Variant & ClusteringMessages::AcceptFailed(Variant &acceptRequest) {
	_payload = acceptRequest;
	return Message("acceptFailed");
}

Variant & ClusteringMessages::AcceptSucceeded(Variant &acceptRequest) {
	_payload = acceptRequest;
	return Message("acceptSucceeded");
}

Variant & ClusteringMessages::GetStats() {
	_payload.Reset();
	_payload.IsArray(false);
	return Message("getStats");
}

Variant & ClusteringMessages::GetResetMaxFdCounters() {
	_payload.Reset();
	_payload.IsArray(false);
	return Message("resetMaxFdCounters");
}

Variant & ClusteringMessages::GetResetTotalFdCounters() {
	_payload.Reset();
	_payload.IsArray(false);
	return Message("resetTotalFdCounters");
}

Variant & ClusteringMessages::Stats(FdStats &stats, uint32_t streamsCount) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["connectionsCount"] = (int64_t) stats.GetManagedTcp().Current() + stats.GetManagedUdp().Current();
	_payload["streamsCount"] = (uint32_t) streamsCount;
	_payload["IOStats"] = stats.ToVariant();
	_payload["IOStats"]["edgeId"] = (uint64_t) GetPid();
	return Message("stats");
}

Variant & ClusteringMessages::GetListStreamsIds(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listStreamsIds");
}

Variant & ClusteringMessages::GetListStreamsIdsResult(Variant &allStreams, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["allStreams"] = allStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listStreamsIds");
}

Variant & ClusteringMessages::GetStreamInfo(uint32_t streamId, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamId"] = streamId;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getStreamInfo");
}

Variant & ClusteringMessages::GetStreamInfoResult(Variant &streamInfo, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamInfo"] = streamInfo;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getStreamInfo");
}

Variant & ClusteringMessages::GetListStreams(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listStreams");
}

Variant & ClusteringMessages::GetClientsConnected(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("clientsConnected");
}

Variant & ClusteringMessages::GetClientsConnectedResult(Variant &allStreams, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["allStreams"] = allStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("clientsConnected");
}

Variant & ClusteringMessages::GetStreamsCount(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getStreamsCount");
}

Variant & ClusteringMessages::GetStreamsCountResult(Variant &allStreams, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["allStreams"] = allStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getStreamsCount");
}

/**
* Used by listInboundStreamNames API command
* Provides a list of all the current inbound localstreamnames.
*/
Variant & ClusteringMessages::GetListInboundStreamNames(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listInboundStreamNames");
}

/**
* Used by listInboundStreams API command
* Provides a list of inbound streams that are not intermediate.
*/
Variant & ClusteringMessages::GetListInboundStreams(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listInboundStreams");
}

/**
* Used by isStreamRunning API command
* Checks a specific stream if it is running or not.
*/
Variant & ClusteringMessages::GetIsStreamRunning(uint32_t streamId, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamId"] = streamId;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("isStreamRunning");
}

/**
* Used by isStreamRunning API command result from edges
*/
Variant & ClusteringMessages::GetIsStreamRunningResult(Variant &result, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	//_payload["streamId"] = result["streamId"];
	if (result.HasKey("streamStatus", true)) {
		_payload["streamStatus"] = result["streamStatus"];
	}
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("isStreamRunning");
}

Variant & ClusteringMessages::GetListStreamsResult(Variant &allStreams,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["allStreams"] = allStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listStreams");
}

/**
* Used by listInboundStreamNames API command result from edges
*/
Variant & ClusteringMessages::GetListInboundStreamNamesResult(Variant &inboundStreams,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["inboundStreams"] = inboundStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listInboundStreamNames");
}

/**
* Used by listInboundStreams API command result from edges
*/
Variant & ClusteringMessages::GetListInboundStreamsResult(Variant &inboundStreams,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["inboundStreams"] = inboundStreams;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listInboundStreams");
}

Variant & ClusteringMessages::GetListConnectionsIds(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listConnectionsIds");
}

Variant & ClusteringMessages::GetListConnectionsIdsResult(Variant &allIds,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["connectionsIds"] = allIds;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listConnectionsIds");
}

Variant & ClusteringMessages::GetConnectionInfo(uint32_t connectionId,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["connectionId"] = connectionId;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getConnectionInfo");
}

Variant & ClusteringMessages::GetConnectionInfoResult(Variant &connectionInfo,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["connectionInfo"] = connectionInfo;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("getConnectionInfo");
}

Variant & ClusteringMessages::GetListConnections(uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listConnections");
}

Variant & ClusteringMessages::GetSetStreamAlias(string localStreamName, string aliasName, int64_t expirePeriod) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["localStreamName"] = localStreamName;
	_payload["aliasName"] = aliasName;
        _payload["expirePeriod"] = expirePeriod;
	return Message("setStreamAlias");
}

Variant & ClusteringMessages::GetRemoveStreamAlias(string aliasName) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["aliasName"] = aliasName;
	return Message("removeStreamAlias");
}

Variant & ClusteringMessages::GetSignalInpuStreamAvailable(string streamName) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamName"] = streamName;
	return Message("signalInpuStreamAvailable");
}

Variant & ClusteringMessages::GetListConnectionsResult(Variant &allConnections,
		uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["allConnections"] = allConnections;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("listConnections");
}

Variant & ClusteringMessages::ShutdownStream(uint32_t streamId, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["streamId"] = streamId;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("shutdownStream");
}

Variant & ClusteringMessages::ShutdownStreamResult(Variant &result, uint64_t uniqueRequestId) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["result"] = result;
	_payload["uniqueRequestId"] = uniqueRequestId;
	return Message("shutdownStream");
}

Variant & ClusteringMessages::GetCreateIngestPoint(string &privateStreamName, string &publicStreamName) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["privateStreamName"] = privateStreamName;
	_payload["publicStreamName"] = publicStreamName;
	return Message("createIngestPoint");
}

Variant & ClusteringMessages::SendEventLog(Variant &eventDetails) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["eventDetails"] = eventDetails;
	return Message("edgeEventDetails");
}
Variant & ClusteringMessages::InsertPlaylistItem(Variant &params) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["parameters"] = params;
	return Message("insertPlaylistItem");

}

Variant & ClusteringMessages::StatusReport(Variant &params) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["parameters"] = params;
	return Message("statusReport");

}

//#ifdef HAS_WEBSERVER
//used by origin to ask a web server for currently streaming sessions
Variant & ClusteringMessages::SendRequestToWebServers(string const &type, Variant &request) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = request["uniqueRequestId"];
	if (type == "getHttpStreamingSessions" ||
		type == "flushGroupNameAliases" ||
		type == "listGroupNameAliases") {

	} else if (type == "addGroupNameAlias") {
		_payload["groupName"] = request["groupName"];
		_payload["aliasName"] = request["aliasName"];
	} else if (type == "getGroupNameByAlias" ||
		type == "removeGroupNameAlias") {
			_payload["aliasName"] = request["aliasName"];
	} else if (type == "getHttpClientsConnected") {
		_payload["groupName"] = request["groupName"];
	}
	return Message(type);
}

//used by a web server to send results to origin
Variant & ClusteringMessages::SendResultToOrigin(string const &type, Variant &result) {
	_payload.Reset();
	_payload.IsArray(false);
	_payload["uniqueRequestId"] = result["payload"]["uniqueRequestId"];
	_payload["result"] = result["result"];
	if (type == "httpStreamingSessions") {
		_payload["httpStreamingSessions"] = result["httpStreamingSessions"];
	} else if (type == "addGroupNameAlias" || 
		type == "getGroupNameByAlias" ||
		type == "removeGroupNameAlias" ||
		type == "flushGroupNameAliases") {
			_payload["groupName"] = result["groupName"];
			_payload["aliasName"] = result["aliasName"];
	} else if (type == "httpClientsConnected") {
		_payload["httpClientsConnected"] = result["httpClientsConnected"];
	} else if (type == "listGroupNameAliases")
		_payload["groupNameAliases"] = result["groupNameAliases"];
	return Message(type);
}

//#endif /*HAS_WEBSERVER*/

Variant & ClusteringMessages::Message(string type) {
	_message["type"] = type;
	_message["pid"] = (uint64_t) GetPid();
	_message["payload"] = _payload;
	return _message;
}
