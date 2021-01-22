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


#ifndef _ORIGINAPPLICATION_H
#define _ORIGINAPPLICATION_H

#include "application/baseclientapplication.h"
#include "streaming/streamstatus.h"
#ifdef HAS_PROTOCOL_RAWMEDIA
#include "protocols/rawmedia/rawmediaprotocolhandler.h"
#endif /* HAS_PROTOCOL_RAWMEDIA */
class PassThroughAppProtocolHandler;
#ifdef HAS_PROTOCOL_EXTERNAL
class ExternalModuleAPI;
#endif /* HAS_PROTOCOL_EXTERNAL */
class BaseRPCAppProtocolHandler;
class RpcSink;
#ifdef HAS_PROTOCOL_NMEA
class NMEAAppProtocolHandler;
#endif /* HAS_PROTOCOL_NMEA */
#ifdef HAS_PROTOCOL_METADATA
class JsonMetadataAppProtocolHandler;
#endif
class OutVmfAppProtocolHandler;
#ifdef HAS_PROTOCOL_WS
class BaseWSAppProtocolHandler;
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_WEBRTC
class WrtcAppProtocolHandler;
#endif /* HAS_PROTOCOL_WEBRTC */
#ifdef HAS_PROTOCOL_API
#include "api3rdparty/apiprotocolhandler.h"
#endif /* HAS_PROTOCOL_API */

namespace app_rdkcrouter {
	class RTMPAppProtocolHandler;
	class HTTPAppProtocolHandler;
	class CLIAppProtocolHandler;
	class RTPAppProtocolHandler;
	class RTSPAppProtocolHandler;
	class JobsTimerAppProtocolHandler;
	class TSAppProtocolHandler;
	class LiveFLVAppProtocolHandler;
	class OriginVariantAppProtocolHandler;
	class V4L2DeviceProtocolHandler;
    class StatsVariantAppProtocolHandler;
	class SDPAppProtocolHandler;
#ifdef HAS_PROTOCOL_DRM
	class DRMAppProtocolHandler;
#endif /* HAS_PROTOCOL_DRM */

    class DLLEXP OriginApplication
    : public BaseClientApplication {
    private:
        static uint32_t _idGenerator;
#ifdef HAS_PROTOCOL_EXTERNAL
        ExternalModuleAPI *_pExternalAPI;
#endif /* HAS_PROTOCOL_EXTERNAL */
		RTMPAppProtocolHandler *_pRTMPHandler;
		HTTPAppProtocolHandler *_pHTTPHandler;
		CLIAppProtocolHandler *_pCLIHandler;
		RTPAppProtocolHandler *_pRTPHandler;
		RTSPAppProtocolHandler *_pRTSPHandler;
		PassThroughAppProtocolHandler *_pPassThrough;
		JobsTimerAppProtocolHandler *_pTimerHandler;
		TSAppProtocolHandler *_pTSHandler;
		LiveFLVAppProtocolHandler *_pLiveFLV;
		BaseRPCAppProtocolHandler *_pRPCHandler;
#ifdef HAS_V4L2
                V4L2DeviceProtocolHandler *_pDevHandler;
#endif /* HAS_V4L2 */		
        StatsVariantAppProtocolHandler *_pStatsHandler;
		SDPAppProtocolHandler *_pSDPHandler;
		vector<RpcSink *> _pRPCSinks;
#ifdef HAS_PROTOCOL_DRM
		DRMAppProtocolHandler *_pDRMHandler;
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_PROTOCOL_NMEA
                NMEAAppProtocolHandler *_pNMEAHandler;
#endif /* HAS_PROTOCOL_NMEA */
#ifdef HAS_PROTOCOL_METADATA
        JsonMetadataAppProtocolHandler * _pJsonMetadataHandler;
#endif
		OutVmfAppProtocolHandler * _pOutVmfAppProtocolHandler;
#ifdef HAS_PROTOCOL_WEBRTC
		WrtcAppProtocolHandler * _pWrtcAppProtocolHandler;
#endif /* HAS_PROTOCOL_WEBRTC */
#ifdef HAS_PROTOCOL_RAWMEDIA
		RawMediaProtocolHandler *_pRawMediaHandler;
#endif /* HAS_PROTOCOL_RAWMEDIA */
#ifdef HAS_PROTOCOL_WS
		BaseWSAppProtocolHandler *_pWS;
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_API
		ApiProtocolHandler *_pApiHandler;
#endif /* HAS_PROTOCOL_API */

		uint32_t _connectionsCountLimit;
		uint32_t _hardConnectionLimitCount;
		double _inBandwidthLimit;
		double _outBandwidthLimit;
		double _hardLimitIn;
		double _hardLimitOut;
		OriginVariantAppProtocolHandler *_pVariantHandler;
		uint32_t _retryTimerProtocolId;
		uint32_t _startwebrtcTimerProtocolId;
		uint32_t _keepAliveTimerProtocolId;
		uint32_t _clusteringTimerProtocolId;
		uint32_t _processWatchdogTimerProtocolId;
#ifdef HAS_PROTOCOL_DRM
		uint32_t _drmTimerProtocolId;
#endif /* HAS_PROTOCOL_DRM */
		Variant _pushSetup;
		Variant _configurations;
		bool _isServer;
		Variant _dummy;
		Variant _delayedProcesses;
		Variant _autoHlsTemplate;
		Variant _autoHdsTemplate;
		Variant _autoMssTemplate;
        Variant _autoDashTemplate;
		static map<string, double> _hlsTimestamps;
        uint32_t _hlsVersion;
        static map<string, RecordingSession> _recordSession;
	public:
		OriginApplication(Variant &configuration);
		virtual ~OriginApplication();
		virtual bool Initialize();
		bool IsServer();
		virtual bool AcceptTCPConnection(TCPAcceptor *pTCPAcceptor);
		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
		virtual bool PushLocalStream(Variant &streamConfig);
		virtual bool PullExternalStream(Variant &streamConfig);

		virtual void SignalEventSinkCreated(BaseEventSink *pSink);
		virtual void EventLogEntry(BaseEventSink *pEventSink, Variant &event);
        void InsertPlaylistItem(string &playlistName, string &localStreamName,
                double insertPoint, double sourceOffset, double duration);
		/**
		 * Check if the stream already exists in memory
		 *
		 * @param type The type of connection (HLS, HDS, MSS, etc.)
		 * @param settings To be used for comparing the settings
		 * @return true if it exists, false otherwise
		 */
		bool IsStreamAlreadyCreated(OperationType operationType, Variant &settings);

#ifdef HAS_PROTOCOL_HLS
		bool CreateHLSStream(Variant &settings);
		static bool ComputeHLSPaths(Variant &settings, Variant &result);
		bool CreateHLSStreamPart(Variant &settings);
		void RemoveHLSGroup(string groupName, bool deleteHLSFiles);
#endif /* HAS_PROTOCOL_HLS */
#ifdef HAS_PROTOCOL_HDS
		bool CreateHDSStream(Variant &settings);
		static bool ComputeHDSPaths(Variant settings, Variant &result);
		bool CreateHDSStreamPart(Variant &settings);
		void RemoveHDSGroup(string groupName, bool deleteHDSFiles);
#endif /* HAS_PROTOCOL_HDS */
#ifdef HAS_PROTOCOL_MSS
		bool CreateMSSStream(Variant &settings);
		static bool ComputeMSSPaths(Variant settings, Variant &result);
		bool CreateMSSStreamPart(Variant &settings);
		void RemoveMSSGroup(string groupName, bool deleteMSSFiles);
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        bool CreateDASHStream(Variant &settings);
        static bool ComputeDASHPaths(Variant settings, Variant &result);
        bool CreateDASHStreamPart(Variant &settings);
        void RemoveDASHGroup(string groupName, bool deleteDASHFiles);
#endif /* HAS_PROTOCOL_DASH */
		void RemoveProcessGroup(string groupName);
		bool GetConfig(Variant &config, uint32_t configId);
		void RemoveConfig(Variant streamConfig, OperationType operationType,
				bool deleteHlsHdsFiles);
		void RemoveConfigId(uint32_t configId, bool deleteHLSFiles);
		Variant GetStreamsConfig();
		virtual void SignalLinkedStreams(BaseInStream *pInStream, BaseOutStream *pOutStream);
		virtual void SignalUnLinkingStreams(BaseInStream *pStream,
				BaseOutStream *pOutStream);
		virtual void SignalStreamRegistered(BaseStream *pStream, bool registerStreamExpiry = true);
		virtual bool OutboundConnectionFailed(Variant &customParameters);
		void SetStreamStatus(Variant &config, StreamStatus status,
				uint64_t uniqueStreamId);
		virtual BaseAppProtocolHandler *GetProtocolHandler(string &scheme);
		uint32_t GetConnectionsCountLimit();
		int64_t GetConnectionsCount();
		void SetConnectionsCountLimit(uint32_t connectionsCountLimit);
		void GetBandwidth(double &in, double &out, double &maxIn, double &maxOut);
		void SetBandwidthLimit(double in, double out);
		virtual bool StreamNameAvailable(string streamName, BaseProtocol *pProtocol,
				bool bypassIngestPoints);
		bool RecordStream(Variant &settings);
		bool SendWebRTCCommand(Variant &settings);
		bool LaunchProcess(Variant &settings);
		bool SetCustomTimer(Variant &settings);
		virtual bool CreateIngestPoint(string &privateStreamName, string &publicStreamName);
		virtual void RemoveIngestPoint(string &privateStreamName, string &publicStreamName);
		virtual void SignalApplicationMessage(Variant &message);
		virtual void SignalServerBootstrapped();
		virtual string GetStreamNameByAlias(const string &streamName, bool remove = true);
		void ShutdownEdges();

		bool PushMetadata(Variant &settings);
		bool ShutdownMetadata(Variant &settings);
#ifdef HAS_PROTOCOL_WEBRTC
		bool StartWebRTC(Variant & settings);
		bool StopWebRTC(Variant & settings);
#endif // HAS_PROTOCOL_WEBRTC
#ifdef HAS_PROTOCOL_METADATA
		bool AddMetadataListener(Variant & settings, TCPAcceptor* &pAcceptor);
#endif // HAS_PROTOCOL_METADATA
        void GatherStats(Variant &reportInfo);
        void SignalEdgeFeedback();
        map<string, RecordingSession>& GetRecordSession() {return _recordSession;}
        uint32_t& GetHLSVersion() { return _hlsVersion; }
		bool RestartService(Variant& params);
	private:
		void SetHardLimits(uint32_t connections,
				double inBandwidth, double outBandwidth);
		bool InitializeForServing();
		bool InitializeForMetaFileGenerator();
		void EnqueueTimedOperation(Variant &request, OperationType operationType);
		void RegisterExpireStream(uint32_t uniqueId);
		bool ProcessIngestPointsList();
		bool ProcessPushPullList();
		bool ProcessAuthPersistenceFile();
		bool ProcessConnectionsLimitPersistenceFile();
		bool ProcessBandwidthLimitPersistenceFile();
		void SaveConfig(Variant &streamConfig, OperationType operationType);
		bool IsActive(Variant &config);
		bool ValidateConfigChecksum(Variant &setup);
		bool ApplyChecksum(Variant &setup);
		bool SerializePullPushConfig(Variant &config);
		bool SerializeIngestPointsConfig();
		bool DropConnection(TCPAcceptor *pTCPAcceptor);
		void RemoveFromPushSetup(Variant &settings);
		void RemoveFromConfigStatus(OperationType operationType, Variant &settings);
		bool RecordedFileActive(string path);
		bool RecordedFileScheduled(string path, uint32_t configId);
		bool RecordedFileHasPermissions(string path);
		void ReadAutoHLSTemplate();
		void ReadAutoHDSTemplate();
		void ProcessAutoHLS(string streamName);
		void ProcessAutoHDS(string streamName);
#ifdef HAS_PROTOCOL_MSS
		void ReadAutoMSSTemplate();
		void ProcessAutoMSS(string streamName);
#endif /* HAS_PROTOCOL_MSS */
#ifdef HAS_PROTOCOL_DASH
        void ReadAutoDASHTemplate();
        void ProcessAutoDASH(string streamName);
#endif /* HAS_PROTOCOL_DASH */
		bool UpgradePullPushConfigFile(Variant &setup);
		bool UpgradePullPushConfigFile_1_2(Variant &setup);
		bool UpgradePullPushConfigFile_2_3(Variant &setup);
		bool UpgradePullPushConfigFile_3_4(Variant &setup);
		bool UpgradePullPushConfigFile_4_5(Variant &setup);
		bool UpgradePullPushConfigFile_5_6(Variant &setup);
		bool UpgradePullPushConfigFile_6_7(Variant &setup);
		bool UpgradePullPushConfigFile_7_8(Variant &setup);
		bool UpgradePullPushConfigFile_8_9(Variant &setup);
		bool UpgradePullPushConfigFile_9_10(Variant &setup);
		bool UpgradePullPushConfigFile_10_11(Variant &setup);
		bool UpgradePullPushConfigFile_11_12(Variant &setup);
		bool UpgradePullPushConfigFile_12_13(Variant &setup);
		bool UpgradePullPushConfigFile_13_14(Variant &setup);
		bool UpgradePullPushConfigFile_14_15(Variant &setup);
		bool UpgradePullPushConfigFile_15_16(Variant &setup);
		bool UpgradePullPushConfigFile_16_17(Variant &setup);
		bool UpgradePullPushConfigFile_17_18(Variant &setup);
		bool UpgradePullPushConfigFile_18_19(Variant &setup);
		bool UpgradePullPushConfigFile_19_20(Variant &setup);
		bool UpgradePullPushConfigFile_20_21(Variant &setup);
		bool UpgradePullPushConfigFile_21_22(Variant &setup);
		bool UpgradePullPushConfigFile_22_23(Variant &setup);
		bool UpgradePullPushConfigFile_23_24(Variant &setup);
        bool UpgradePullPushConfigFile_24_25(Variant &setup);
        bool UpgradePullPushConfigFile_25_26(Variant &setup);
		bool UpgradePullPushConfigFile_26_27(Variant &setup);
		
		/**
		 * Check if the conditions of a lazyPull stream is satisfied.
		 * 
         * @param streamName Stream to check if it is created by lazy pull
         * @return True if lazy pull is active, false otherwise
         */
		bool IsLazyPullActive(string streamName);
		
		void ProcessVodRequest(Variant &details);
	};
}

#endif	/* _ORIGINAPPLICATION_H */
