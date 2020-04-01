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


#ifndef _EDGEVARIANTAPPPROTOCOLHANDLER_H
#define	_EDGEVARIANTAPPPROTOCOLHANDLER_H

#include "protocols/variant/basevariantappprotocolhandler.h"

namespace app_rdkcrouter {

	class DLLEXP EdgeVariantAppProtocolHandler
	: public BaseVariantAppProtocolHandler {
	private:
		Variant _info;
		Variant _streamInfos;
		Variant _protocolInfos;
		uint32_t _protocolId;
	public:
		EdgeVariantAppProtocolHandler(Variant &configuration);
		virtual ~EdgeVariantAppProtocolHandler();

		virtual void RegisterProtocol(BaseProtocol *pProtocol);

		virtual bool ProcessMessage(BaseVariantProtocol *pProtocol,
				Variant &lastSent, Variant &lastReceived);
		void SetConnectorInfo(Variant &info);
		bool SendHello();
		bool SendEventLog(Variant &eventDetails);
		bool SendSignalOutStreamAttached(string streamName);
		void RegisterInfoStream(uint32_t uniqeStreamId);
		void UnRegisterInfoStream(uint32_t uniqeStreamId);
		void RegisterInfoProtocol(uint32_t protocolId);
		void UnRegisterInfoProtocol(uint32_t protocolId);
	private:
		bool Send(Variant &message);
		bool ProcessMessageAccept(Variant &message);
		bool ProcessMessageGetStats(Variant &message);
		bool ProcessMessageListStreamsIds(Variant &message);
		bool ProcessMessageGetStreamInfo(Variant &message);
		bool ProcessMessageListStreams(Variant &message);
		bool ProcessMessageClientsConnected(Variant &message);
		bool ProcessMessageInsertPlaylistItem(Variant &message);
		bool ProcessMessageStatusReport(Variant &message);
		bool ProcessMessageGetStreamsCount(Variant &message);
		/**
		 * Process the listInboundStreamNames API command sent by origin
		 */
		bool ProcessMessageListInboundStreamNames(Variant &message);
		/**
		 * Process the listInboundStreams API command sent by origin
		 */
		bool ProcessMessageListInboundStreams(Variant &message);
		/**
		 * Process the isStreamRunning API command sent by origin
		 */
		bool ProcessMessageIsStreamRunning(Variant &message);
		/**
		 * Used by isStreamRunning API command
		 * Checks a specific stream using streamId if it is running or not.
		 * Return the status in boolean form i.e. true or false
		 */
		bool GetStreamStatus(Variant& callbackInfo);
		/**
		 * Used by isStreamRunning API command
		 * Checks a specific stream using localStreamName if it is running or not.
		 * Return the status in boolean form i.e. true or false
		 */
		//bool GetStreamStatus(const string& localStreamName, Variant& callbackInfo);
		bool ProcessMessageListConnectionsIds(Variant &message);
		bool ProcessMessageGetConnectionInfo(Variant &message);
		bool ProcessMessageListConnections(Variant &message);
		bool ProcessMessageResetMaxFdCounters(Variant &message);
		bool ProcessMessageResetTotalFdCounters(Variant &message);
		bool ProcessMessageSetStreamAlias(Variant &message);
		bool ProcessMessageShutdownStream(Variant &message);
		bool ProcessMessageSignalInpuStreamAvailable(Variant &message);
		bool ProcessMessageRemoveStreamAlias(Variant &message);
		bool ProcessMessageCreateIngestPoint(Variant &message);
	};
}


#endif	/* _EDGEVARIANTAPPPROTOCOLHANDLER_H */

