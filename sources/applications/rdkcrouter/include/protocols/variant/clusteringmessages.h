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


#ifndef _CLUSTERINGMESSAGES_H
#define	_CLUSTERINGMESSAGES_H

#include "common.h"
#include "netio/netio.h"
class TCPAcceptor;

namespace app_rdkcrouter {

	class ClusteringMessages {
	private:
		static Variant _message;
		static Variant _payload;
	public:
		static Variant & Hello(string ip, uint16_t port);
		static Variant & SignalOutStreamAttached(string streamName);
		static Variant & Accept(TCPAcceptor *pAcceptor);
		static Variant & AcceptFailed(Variant &acceptRequest);
		static Variant & AcceptSucceeded(Variant &acceptRequest);
		static Variant & GetStats();
		static Variant & GetResetMaxFdCounters();
		static Variant & GetResetTotalFdCounters();
		static Variant & Stats(FdStats &stats, uint32_t streamsCount);
		static Variant & GetListStreamsIds(uint64_t uniqueRequestId);
		static Variant & GetListStreamsIdsResult(Variant &allStreams, uint64_t uniqueRequestId);
		static Variant & GetStreamInfo(uint32_t streamId, uint64_t uniqueRequestId);
		static Variant & GetStreamInfoResult(Variant &streamInfo, uint64_t uniqueRequestId);
		static Variant & GetListStreams(uint64_t uniqueRequestId);
		/**
		 * Used by listInboundStreamNames API command
		 * Provides a list of all the current inbound localstreamnames.
		 */
		static Variant & GetListInboundStreamNames(uint64_t uniqueRequestId);
		/**
		 * Used by listInboundStreamNames API command
		 * Used to carry the result from the edges.
		 */
		static Variant & GetListInboundStreamNamesResult(Variant &inboundStreams, uint64_t uniqueRequestId);
		/**
		 * Used by listInboundStreams API command
		 * Provides a list of inbound streams that is not intermediate.
		 */
		static Variant & GetListInboundStreams(uint64_t uniqueRequestId);
		/**
		 * Used by listInboundStreams API command
		 * Used to carry the result from the edges.
		 */
		static Variant & GetListInboundStreamsResult(Variant &inboundStreams, uint64_t uniqueRequestId);
		/**
		 * Used by isStreamRunning API command
		 * Checks a specific stream if it is running or not.
		 */
		static Variant & GetIsStreamRunning(uint32_t streamId, uint64_t uniqueRequestId);
		/**
		 * Used by isStreamRunning API command
		 * Used to carry the result from the edges.
		 */
		static Variant & GetIsStreamRunningResult(Variant &result, uint64_t uniqueRequestId);
		static Variant & GetListStreamsResult(Variant &allStreams, uint64_t uniqueRequestId);
		static Variant & GetListConnectionsIds(uint64_t uniqueRequestId);
		static Variant & GetListConnectionsIdsResult(Variant &allIds, uint64_t uniqueRequestId);
		static Variant & GetConnectionInfo(uint32_t connectionId, uint64_t uniqueRequestId);
		static Variant & GetConnectionInfoResult(Variant &connectionInfo, uint64_t uniqueRequestId);
		static Variant & GetListConnections(uint64_t uniqueRequestId);
		static Variant & GetClientsConnected(uint64_t uniqueRequestId);
		static Variant & GetClientsConnectedResult(Variant &allStreams, uint64_t uniqueRequestId);
		static Variant & GetStreamsCount(uint64_t uniqueRequestId);
		static Variant & GetStreamsCountResult(Variant &allStreams, uint64_t uniqueRequestId);
		static Variant & GetListConnectionsResult(Variant &allConnections, uint64_t uniqueRequestId);
		static Variant & GetSetStreamAlias(string localStreamName, string aliasName, int64_t expirePeriod);
		static Variant & GetRemoveStreamAlias(string aliasName);
		static Variant & GetSignalInpuStreamAvailable(string streamName);
		static Variant & ShutdownStream(uint32_t streamId, uint64_t uniqueRequestId);
		static Variant & ShutdownStreamResult(Variant &result, uint64_t uniqueRequestId);
		static Variant & GetCreateIngestPoint(string &privateStreamName, string &publicStreamName);
		static Variant & SendEventLog(Variant &eventDetails);
		static Variant & InsertPlaylistItem(Variant &params);
		static Variant & StatusReport(Variant &params);
//#ifdef HAS_WEBSERVER
		static Variant & SendRequestToWebServers(string const &type, Variant &request);
		static Variant & SendResultToOrigin(string const &type, Variant &result);
//#endif /*HAS_WEBSERVER*/
	private:
		static Variant & Message(string type);
	};
}


#endif	/* _CLUSTERINGMESSAGES_H */

