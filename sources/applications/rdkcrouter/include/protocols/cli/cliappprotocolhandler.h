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


#ifdef HAS_PROTOCOL_CLI
#ifndef _CLIAPPPROTOCOLHANDLER_H
#define	_CLIAPPPROTOCOLHANDLER_H

#include "protocols/cli/basecliappprotocolhandler.h"

class StreamsManager;
namespace app_rdkcrouter {

    class OriginApplication;

    class DLLEXP CLIAppProtocolHandler
    : public BaseCLIAppProtocolHandler {
    private:
        Variant _help;
        OriginApplication *_pApp;
        StreamsManager *_pSM;
        string _shutdownKey;
    public:
        CLIAppProtocolHandler(Variant &configuration);
        virtual ~CLIAppProtocolHandler();

        virtual void SetApplication(BaseClientApplication *pApplication);

        bool ListStreamsIdsResult(Variant &callbackInfo);
        bool GetStreamInfoResult(Variant &callbackInfo);
        /**
         * Used by getStreamInfo API command
         * Used the streamId to find a specific stream.
         * Returns all the information of the specified stream
         */
        bool GetLocalStreamInfo(uint32_t streamId, Variant &result);
        /**
         * Used by getStreamInfo API command
         * Used the localStreamName to find a specific stream.
         * Returns all the information of the specified stream
         */
        bool GetLocalStreamInfo(const string& streamName, Variant &result);
        bool ListStreamsResult(Variant &callbackInfo);
        /**
         * Used by listInboundStreamNames API command
         * Provides a list of all the current inbound localstreamnames.
         */
        bool ListInboundStreamNamesResult(Variant &callbackInfo);
        /**
         * Used by listInboundStreams API command
         * Provides a list of inbound streams that are not intermediate.
         */
        bool ListInboundStreamsResult(Variant &callbackInfo);
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
        bool GetStreamStatus(const string& localStreamName, Variant& callbackInfo);
        bool ListConnectionsIdsResult(Variant &callbackInfo);
        bool GetConnectionInfoResult(Variant &callbackInfo);
        bool ListConnectionsResult(Variant &callbackInfo);
        bool ShutdownStreamResult(Variant &callbackInfo);
		bool GetClientsConnectedResult(Variant &callbackInfo);
		bool GetStreamsCountResult(Variant &callbackInfo);
#ifdef HAS_WEBSERVER
		bool GetListHttpStreamingSessionsResult(Variant &callbackInfo);
		bool GetHttpClientsConnectedResult(Variant &callbackInfo);
		bool GetAddGroupNameAliasResult(Variant &callbackInfo);
		bool GetRemoveGroupNameAliasResult(Variant &callbackInfo);
		bool GetListGroupNameAliasesResult(Variant &callbackInfo);
		bool GetGroupNameByAliasResult(Variant &callbackInfo);
		bool GetFlushGroupNameAliasesResult(Variant &callbackInfo);
#endif /*HAS_WEBSERVER*/
#ifdef HAS_PROTOCOL_HLS
        static bool NormalizeHLSParameters(BaseProtocol *pFrom,
                Variant &parameters, Variant &settings, bool &result);
#endif /* HAS_PROTOCOL_HLS */
        virtual bool ProcessMessage(BaseProtocol *pFrom, Variant &message);
    private:
        void GatherCustomParameters(Variant &src, Variant &dst);
        virtual bool ProcessMessageHelp(BaseProtocol *pFrom, Variant &parameters);
        virtual bool ProcessMessageQuit(BaseProtocol *pFrom, Variant &parameters);
        virtual bool ProcessMessageSetAuthentication(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageVersion(BaseProtocol *pFrom, Variant &parameters);
        virtual bool ProcessMessagePullStream(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessagePushStream(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListStreamsIds(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetStreamsCount(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetStreamInfo(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListStreams(BaseProtocol *pFrom,
                Variant &parameters);
        /**
         * Process the clientsConnected API command
         */
		virtual bool ProcessMessageClientsConnected(BaseProtocol *pFrom,
				Variant &parameters);
		/**
         * Process the httpClientsConnected API command
         */
		virtual bool ProcessMessageHttpClientsConnected(BaseProtocol *pFrom,
				Variant &parameters);
        /**
         * Process the listInboundStreamNames API command
         */
        virtual bool ProcessMessageListInboundStreamNames(BaseProtocol *pFrom,
                Variant &parameters);
        /**
         * Process the listInboundStreams API command
         */
        virtual bool ProcessMessageListInboundStreams(BaseProtocol *pFrom,
                Variant &parameters);
        /**
         * Process the isStreamRunning API command
         */
        virtual bool ProcessMessageIsStreamRunning(BaseProtocol *pFrom,
                Variant &parameters);
        /**
         * Process the getServerInfo API command
         */
        virtual bool ProcessMessageGetServerInfo(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageShutdownStream(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListConfig(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetConfigInfo(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveConfig(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListServices(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageEnableService(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageShutdownService(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageCreateService(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListConnectionsIds(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetExtendedConnectionCounters(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetConnectionsCount(BaseProtocol *pFrom,
                Variant &parameters);
        bool GetLocalConnectionInfo(uint32_t connectionId, Variant &result);
        virtual bool ProcessMessageGetConnectionInfo(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListConnections(BaseProtocol *pFrom,
                Variant &parameters);
		virtual bool ProcessMessageSendWebRTCCommand(BaseProtocol *pFrom,
			Variant &parameters);
#ifdef HAS_PROTOCOL_HLS
        virtual bool ProcessMessageCreateHLSStream(BaseProtocol *pFrom,
                Variant &parameters);
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
        virtual bool ProcessMessageCreateHDSStream(BaseProtocol *pFrom,
                Variant &parameters);
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
        virtual bool ProcessMessageCreateMSSStream(BaseProtocol *pFrom,
                Variant &parameters);
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        virtual bool ProcessMessageCreateDASHStream(BaseProtocol *pFrom,
                Variant &parameters);
#endif /* HAS_PROTOCOL_DASH */
#ifdef HAS_WEBSERVER
		virtual bool ProcessMessageAddGroupNameAlias(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageGetGroupNameByAlias(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageRemoveGroupNameAlias(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageListGroupNameAliases(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageFlushGroupNameAliases(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageListHTTPStreamingSessions(BaseProtocol *pFrom,
			Variant &parameters);
#endif /* HAS_WEBSERVER */
        virtual bool ProcessMessageGetConnectionsCountLimit(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageSetConnectionsCountLimit(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGetBandwidth(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageSetBandwidthLimit(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListEdges(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageSetLogLevel(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageResetMaxFdCounters(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageResetTotalFdCounters(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRecordStream(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageAddStreamAlias(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveStreamAlias(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListStreamAliases(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageFlushStreamAliases(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageShutdownServer(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageLaunchProcess(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListStorage(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageAddStorage(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveStorage(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageSetTimer(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListTimers(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveTimer(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageCreateIngestPoint(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveIngestPoint(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListIngestPoints(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageInsertPlaylistItem(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageTranscode(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageListMediaFiles(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageRemoveMediaFile(BaseProtocol *pFrom,
                Variant &parameters);
        virtual bool ProcessMessageGenerateLazyPullFile(BaseProtocol *pFrom,
                Variant &parameters);
		virtual bool ProcessMessageGetMetadata(BaseProtocol *pFrom,
				Variant &parameters);
		virtual bool ProcessMessagePushMetadata(BaseProtocol *pFrom,
				Variant &parameters);
		virtual bool ProcessMessageShutdownMetadata(BaseProtocol *pFrom,
				Variant &parameters);
#ifdef HAS_PROTOCOL_METADATA
		virtual bool ProcessMessageAddMetadataListener(BaseProtocol *pFrom,
				Variant &parameters);
		virtual bool ProcessMessageListMetadataListeners(BaseProtocol *pFrom,
				Variant &parameters);
#endif // HAS_PROTOCOL_METADATA
#ifdef HAS_PROTOCOL_WEBRTC
		virtual bool ProcessMessageStartWebRTC(BaseProtocol *pFrom,
			Variant &parameters);
		virtual bool ProcessMessageStopWebRTC(BaseProtocol *pFrom,
			Variant &parameters);
#endif // HAS_PROTOCOL_WEBRTC

		/**
		* Api that can be called to open a new http acceptor that waits for mp4 upload.
		* It will then save the uploaded mp4 to a directory in the server.
		*/
		virtual bool ProcessMessageUploadMedia(BaseProtocol *pFrom,
			Variant &parameters);

        virtual bool ProcessMessageGenerateServerPlaylist(BaseProtocol *pFrom,
            Variant &parameters);

		virtual bool ProcessMessageCustomRTSPHeaders(BaseProtocol* pFrom,
			Variant &parameters);

        /**
         * Checks if the passed URI is valid or if it is a stream name.
         *
         * @param uri URI to be tested for validity
         * @param name Contains the stream name if such is the case
         * @return true if valid URI or stream name, false if invalid URI
         */
        bool IsValidURI(string uri, string &name);

        /**
         * Initializes the parameters with "" to zero (0) and checks if the number
         * of elements matches that of the reference parameter.
         *
         * @param settings Variant containing the set of parameters
         * @param paramRef The reference to which the target parameter will be based with
         * @param paramName Actual name of the parameter to be initialized
         * @return true if successfully initialized, false if number of elements did not match
         */
        bool InitializeEmptyParams(Variant &settings, string paramRef, string paramName);

        /**
         * Forms an argument parameter that will be used by the launchProcess command.
         *
         * @param settings Variant containing the set of parameters
         * @param destIndex Destination index currently being used (for multiple destinations)
         * @param forceRE Flag to indicate if the -re option to avconv needs to be included
         * @return string The arguments that was formed
         */
        string FormTranscodeArgs(Variant &settings, uint32_t destIndex, bool &forceRE);

		/**
		* Used by getServerInfo API command
		* Returns a bunch of information regarding the configuration of the running RMS.
		*/
		bool GetServerInfo(Variant &serverInfo);
    };
}

#endif	/* _CLIAPPPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_CLI */
