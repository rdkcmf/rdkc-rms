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


#ifdef HAS_PROTOCOL_MSS
#ifndef _OUTFILRMSSSTREAM_H
#define	_OUTFILRMSSSTREAM_H

#include  "streaming/baseoutfilestream.h"

class BaseClientApplication;
class MSSFragment;
class MSSManifest;
class MSSPublishingPoint;

namespace MicrosoftSmoothStream {
    struct LookAhead {
        string fileName;
		uint64_t freeOffset;
        uint64_t tfxdOffset;
        uint64_t timeStamp; //for live ingest stamp in tfxd
        uint64_t duration;
		uint64_t manifestStamp; //for tfrf & manifest timestamp
    };

    struct RetainedFragmentInfo {
        string fragmentFullPath;
        string manifestEntry;
		//for VOD
		uint64_t fragmentDuration;
    };

    struct SessionInfo {
        uint32_t fragmentSequence;
        string prevAudioPath;
        string prevVideoPath;
        string videoFolder;
        string audioFolder;
        MSSManifest* pManifest;
        bool isFirstRun;
        uint32_t videoChunkCount;
        uint32_t audioChunkCount;
		//for VOD
        uint64_t totalAudioDuration;
        uint64_t totalVideoDuration;
		//<== VOD
		uint64_t startAudioTimeTicks;
		uint64_t startVideoTimeTicks;
		//for live ingest
        map<bool, uint16_t> trackIDs;
        uint16_t trackIdCounter;
        bool manifestInitialized;
		//<== liveingest
        vector<LookAhead> audioLookAheadQue;
        vector<LookAhead> videoLookAheadQue;
        queue<RetainedFragmentInfo> retainedAudioFragments;
        queue<RetainedFragmentInfo> retainedVideoFragments;
        queue<RetainedFragmentInfo> staleAudioFragments;
        queue<RetainedFragmentInfo> staleVideoFragments;
		//for live ingest
        MSSPublishingPoint* pPublishingPoint;

        SessionInfo();
        void Reset();
		void ClearQueue(queue<RetainedFragmentInfo>& refQueue);
    };

    struct GroupState {
        bool moovHeaderFlushed;
        uint16_t trakCounts;
        GroupState()
        : moovHeaderFlushed(false),
          trakCounts(0) {
        }
    };

    class DLLEXP OutFileMSSStream
    : public BaseOutFileStream {
    public:
        static map<string, SessionInfo> _sessionInfo;
        static map<string, MSSManifest*> _prevManifest;
        static map<string, MSSPublishingPoint*> _prevPublishingPoint;
        static map<string, GroupState> _groupStates;
        static string _prevGroupName;
    private:
        //audio
        int64_t _audioRate;
        //video
        int64_t _videoRate;
        Variant _settings;
        GenericProcessDataSetup *_pGenericProcessDataSetup;
        BaseClientApplication* _pApplication;
        bool _chunkOnIDR;
        MSSFragment *_pVideoFragment;
        MSSFragment *_pAudioFragment;
        double _chunkLength;
        string _targetFolder;
        bool _isLastFragment;
        Variant _chunkInfo;
        // Indicates the fragments can be be overwritten or not
        bool _overwriteDestination;
        // Indicates if MSS is of rolling type (fragments generated are limited)
        bool _isRolling;
		// Indicates the type of manifest MSS will create (live or not)
		bool _isLive;
        uint32_t _playlistLength;
        uint32_t _staleRetentionCount;
        double _lastAudioTS;
        double _lastVideoTS;
        string _localStreamName;
        string _groupName;
        string _sessionKey;
        bool _videoChunked;
		uint8_t _addStreamFlag;
		uint8_t _removeStreamFlag;
        string _ismType;
        static string _prevLocalStreamName;
    private:
        OutFileMSSStream(BaseProtocol *pProtocol, string name, Variant &settings);

        bool OpenFragment(double pts, double dts, double additionalFramesPercent, bool isAudio);
        bool CloseFragment(bool isAudio, double pts = -1);
        bool NewFragment(double pts, double dts, bool forced, bool isAudio);
        bool InsertSequenceHeaders(double videoTS, double audioTS);
        void InitializeManifest();
    public:
        static OutFileMSSStream* GetInstance(BaseClientApplication *pApplication,
                string name, Variant &settings);
        virtual ~OutFileMSSStream();

        virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
        virtual bool IsCompatibleWithType(uint64_t type);
        virtual bool SignalPlay(double &dts, double &length);
        virtual bool SignalPause();
        virtual bool SignalResume();
        virtual bool SignalSeek(double &dts);
        virtual bool SignalStop();
        virtual void SignalAttachedToInStream();
        virtual void SignalDetachedFromInStream();
        virtual void SignalStreamCompleted();
        virtual void SignalAudioStreamCapabilitiesChanged(
                StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
                AudioCodecInfo *pNew);
        virtual void SignalVideoStreamCapabilitiesChanged(
                StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
                VideoCodecInfo *pNew);
        virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
                uint32_t processedLength, uint32_t totalLength,
                double pts, double dts, bool isAudio);
    protected:
        virtual bool FinishInitialization(
                GenericProcessDataSetup *pGenericProcessDataSetup);
        virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
                bool isKeyFrame);
        virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
        virtual bool IsCodecSupported(uint64_t codec);
    };
}
#endif	/* _OUTFILRMSSSTREAM_H */
#endif	/* HAS_PROTOCOL_MSS */
