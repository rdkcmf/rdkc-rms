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


#ifndef _EVENTLOGGER_H
#define	_EVENTLOGGER_H

#include "common.h"

class BaseEventSink;
class BaseClientApplication;
class BaseStream;
class BaseOutStream;
class BaseProtocol;
class BaseAppProtocolHandler;
class FdStats;

class DLLEXP EventLogger {
private:
    static EventLogger _defaultLogger;
    static uint32_t _idGen;

    vector<BaseEventSink *> _eventSinkTable;
    BaseClientApplication *_pApplication;
    Variant _message;
    uint32_t _id;
    map<string, bool> _canLogEventCache;
    uint64_t _eventId;

    Variant _customData;
public:
    EventLogger();
    virtual ~EventLogger();

    static EventLogger *GetDefaultLogger();
    static bool InitializeDefaultLogger(Variant &config);
    static void SignalShutdown();

    uint32_t GetId();
    bool Initialize(Variant &config, BaseClientApplication *pApplication);
    Variant &GetCustomData();

    void LogStreamCreated(BaseStream *pStream);
    void LogStreamCodecsUpdated(BaseStream *pStream);
    void LogStreamClosed(BaseStream *pStream);
    void LogAudioFeedStopped(BaseStream *pStream);
    void LogVideoFeedStopped(BaseStream *pStream);
    void LogCLIRequest(Variant &request);
    void LogCLIResponse(Variant &response);
    void LogApplicationStart(BaseClientApplication *pApplication);
    void LogApplicationStop(BaseClientApplication *pApplication);
    void LogCarrierCreated(Variant &stats);
    void LogCarrierClosed(Variant &stats);
    void LogProtocolRegisteredToApplication(BaseProtocol *pProtocol);
    void LogProtocolUnregisteredFromApplication(BaseProtocol *pProtocol);
    void LogServerStarted();
    void LogServerStopping();
    void LogProcessStarted(Variant &processInfo);
    void LogProcessStopped(Variant &processInfo);
    void LogTimerCreated(Variant &info);
    void LogTimerTriggered(Variant &info);
    void LogTimerClosed(Variant &info);
#ifdef HAS_PROTOCOL_HLS
    void LogHLSChunkCreated(const string filename, double playlistStartTs,
            double segmentStartTs);
    void LogHLSChunkClosed(const string filename, double playlistStartTs,
            double segmentStartTs, double segmentDuration);
    void LogHLSChunkError(string errorMsg);
    void LogHLSChildPlaylistUpdated(string fileName);
    void LogHLSMasterPlaylistUpdated(string fileName);
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
    void LogHDSChunkCreated(Variant &chunkInfo);
    void LogHDSChunkClosed(Variant &chunkInfo);
    void LogHDSChunkError(string errorMsg);
    void LogHDSChildPlaylistUpdated(string fileName);
    void LogHDSMasterPlaylistUpdated(string fileName);
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
    void LogMSSChunkCreated(Variant &chunkInfo);
    void LogMSSChunkClosed(Variant &chunkInfo);
    void LogMSSChunkError(string errorMsg);
    void LogMSSPlaylistUpdated(string fileName);
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
    void LogDASHChunkCreated(Variant &chunkInfo);
    void LogDASHChunkClosed(Variant &chunkInfo);
    void LogDASHChunkError(string errorMsg);
    void LogDASHPlaylistUpdated(string fileName);
#endif /* HAS_PROTOCOL_DASH */
#ifdef HAS_WEBSERVER
	void LogMediaFileDownloaded(Variant &sessionInfo);
	void LogStreamingSessionStarted(Variant &sessionInfo);
	void LogStreamingSessionEnded(Variant &sessionInfo);
#endif /* HAS_WEBSERVER */
    // RECORD section
    void LogRecordChunkCreated(Variant &chunkInfo);
    void LogRecordChunkClosed(Variant &chunkInfo);
	void LogRecordChunkError(Variant &chunkInfo);
    void LogEdgeRelayedEvent(Variant &edgeEvent);
    // RECORD section end

	void LogRTMPPlaylistItemStart(Variant &info);
	void LogWebRTCCommandReceived(Variant &commandInfo);
	void LogWebRtcServiceConnected(Variant &params);
	void LogPullStreamFailed(Variant &details);
    void LogPlayerJoinedRoom(Variant& payload);
    void LogOutStreamFirstFrameSent (BaseOutStream* pOutStream);
private:
    bool EnabledForEvent(uint64_t sinkType, string eventtype);
    bool CanLogEvent(string eventType);
    void Log(Variant &event);
    /*!
            @brief Spawns new EventSink based on config
            @param config contains:
                    type
                    prop1,
                    prop2, ...
     */
    BaseEventSink * SpawnEventSink(Variant &config);

};

#endif	/* _EVENTLOGGER_H */

