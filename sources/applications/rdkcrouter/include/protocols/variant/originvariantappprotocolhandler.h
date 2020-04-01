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


#ifndef _ORIGINVARIANTAPPPROTOCOLHANDLER_H
#define	_ORIGINVARIANTAPPPROTOCOLHANDLER_H

#include "protocols/variant/basevariantappprotocolhandler.h"
#include "protocols/variant/edgeserver.h"

class TCPAcceptor;
class BaseVariantProtocol;

namespace app_rdkcrouter {

	class DLLEXP OriginVariantAppProtocolHandler
	: public BaseVariantAppProtocolHandler {
	private:
		EdgeServer _dummy;
		map<uint64_t, EdgeServer> _edges;
		uint64_t _idGenerator;
		map<uint64_t, Variant> _pendingCallbacks;
	public:
		OriginVariantAppProtocolHandler(Variant &configuration);
		virtual ~OriginVariantAppProtocolHandler();

		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
		virtual void ConnectionFailed(Variant &parameters);

		virtual bool ProcessMessage(BaseVariantProtocol *pProtocol,
				Variant &lastSent, Variant &lastReceived);

		bool AcceptTCPConnection(TCPAcceptor *pTCPAcceptor);
		void RefreshConnectionsCount();
		map<uint64_t, EdgeServer> &GetEdges();

		void QueryEdgeStatus(Variant &reportInfo);
		bool EnqueueCall(Variant callbackInfo);
		bool FinishCallbacks();
		void ResetMaxFdCounters();
		void ResetTotalFdCounters();
		void SetStreamAlias(string &localStreamName, string &aliasName, int64_t expirePeriod);
		void RemoveStreamAlias(const string &aliasName);
		void ShutdownEdges();
		void SignalInpuStreamAvailable(string streamName);
		void CreateIngestPoint(string &privateStreamName, string &publicStreamName);
		void InsertPlaylistItem(string &playlistName, string &localStreamName,
				double insertPoint, double sourceOffset, double duration);
#ifdef HAS_WEBSERVER
		bool ProcessWebServerMessage(uint64_t pid, Variant &message);
		bool AreEdgesWebServers();
		uint8_t CountWebServers();
#endif /*HAS_WEBSERVER*/
	private:
		string GetEdgesInfo();
		void SetIncrement(uint64_t pid, bool ok);
		void SetIncrement(EdgeServer &edge, bool ok);
		void SetProtocolId(uint64_t pid, uint32_t protocolId);
		void IncrementConnectionsCount(uint64_t pid);
		void IncrementConnectionsCount(EdgeServer &edge);
		void SetConnectionsCount(uint64_t pid, uint32_t connectionsCount);
		void SetConnectionsCount(EdgeServer &edge, uint32_t connectionsCount);
		bool EnableAcceptor(uint32_t id);
		bool Send(EdgeServer &edge, Variant &message);
		bool ProcessMessageHello(uint64_t pid, Variant &message);
		bool ProcessMessageAcceptFailed(uint64_t pid, Variant &message);
		bool ProcessMessageAcceptSucceeded(uint64_t pid, Variant &message);
		bool ProcessMessageStats(uint64_t pid, Variant &message);
		bool ProcessMessageStatusReport(uint64_t pid, Variant &message);
		bool ProcessMessageListStreamsIds(uint64_t pid, Variant &message);
		bool ProcessMessageGetStreamInfo(uint64_t pid, Variant &message);
		bool ProcessMessageGetStreamsCount(uint64_t pid, Variant &message);
		bool ProcessMessageListStreams(uint64_t pid, Variant &message);
		bool ProcessMessageClientsConnected(uint64_t pid, Variant &message);
		/**
		 * Process the listInboundStreamNames API command result from edges
		 */
		bool ProcessMessageListInboundStreamNames(uint64_t pid, Variant &message);
		/**
		 * Process the listInboundStreams API command result from edges
		 */
		bool ProcessMessageListInboundStreams(uint64_t pid, Variant &message);
		/**
		 * Process the isStreamRunning API command result from edges
		 */
		bool ProcessMessageIsStreamRunning(uint64_t pid, Variant &message);
		bool ProcessMessageListConnectionsIds(uint64_t pid, Variant &message);
		bool ProcessMessageGetConnectionInfo(uint64_t pid, Variant &message);
		bool ProcessMessageListConnections(uint64_t pid, Variant &message);
		bool ProcessMessageShutdownStream(uint64_t pid, Variant &message);
		bool ProcessMessageSignalOutStreamAttached(uint64_t pid, Variant &message);
		bool ProcessMessageLogEvent(uint64_t pid, Variant &message);
		bool FinishCallback(Variant &callbackInfo);
	};
}

#endif	/* _VARIANTAPPPROTOCOLHANDLER_H */

