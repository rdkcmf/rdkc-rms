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


#ifdef HAS_PROTOCOL_DASH
#ifndef _OUTFILEDASHSTREAM_H
#define	_OUTFILEDASHSTREAM_H

#include  "streaming/baseoutfilestream.h"

class BaseClientApplication;

class DASHFragment;
class DASHManifest;

namespace MpegDash {

    struct RetainedFragmentInfo {
        string absoluteFragmentPath;
        uint64_t fragmentDecodeTime;
		//for VOD
        string relativeFragmentPath;
        double fragmentDuration;
    };

    struct SessionInfo {
        uint32_t fragmentSequence;
        string prevAudioPath;
        string prevVideoPath;
        string videoFolder;
        string audioFolder;
        DASHManifest* pManifest;
        bool isFirstRun;
		//==> used for VOD
		double totalAudioDuration;
		double totalVideoDuration;
		//<== VOD
		uint64_t startAudioTimeTicks;
		uint64_t startVideoTimeTicks;
        queue<RetainedFragmentInfo> retainedAudioFragments;
        queue<RetainedFragmentInfo> retainedVideoFragments;
        queue<RetainedFragmentInfo> staleAudioFragments;
        queue<RetainedFragmentInfo> staleVideoFragments;

        SessionInfo();
        void Reset();
		void ClearQueue(queue<RetainedFragmentInfo>& refQueue);
    };

    class DLLEXP OutFileDASHStream
    : public BaseOutFileStream {
    public:
        static map<string, SessionInfo> _sessionInfo;
        static map<string, DASHManifest*> _prevManifest;
        static string _prevGroupName;
    private:
        int64_t _audioRate;
        int64_t _videoRate;
        Variant _settings;
        GenericProcessDataSetup* _pGenericProcessDataSetup;
        bool _chunkOnIDR;
        DASHFragment* _pVideoFragment;
        DASHFragment* _pAudioFragment;
        double _chunkLength;
        string _targetFolder;
        string _videoRelativeFolder;
        string _audioRelativeFolder;
        bool _isLastFragment;
        Variant _chunkInfo;
        // Indicates the fragments can be be overwritten or not
        bool _overwriteDestination;
        // Indicates if DASH is of rolling type (fragments generated are limited)
        bool _isRolling;
        // Indicates whether the DASH content to create is 'dynamic' or 'static'
        bool _isLive;
        uint32_t _playlistLength;
        uint32_t _staleRetentionCount;
        double _lastAudioTS;
        double _lastVideoTS;
        ostringstream _repAudioId;
        ostringstream _repVideoId;
        BaseClientApplication* _pApplication;
        string _bandwidth;
        string _localStreamName;
        string _groupName;
        string _sessionKey;
		double _lastVideoDuration;
        uint8_t _addStreamFlag;
        uint8_t _removeStreamFlag;
        static string _prevLocalStreamName;
		bool _isAutoDash;
		double _avDurationDelta;
    private:
        OutFileDASHStream(BaseProtocol *pProtocol, string name, Variant &settings);
        bool OpenFragment(double pts, double dts, double additionalFramesPercent, bool isAudio);
        bool CloseFragment(bool isAudio, double pts = -1);
        bool NewFragment(double pts, double dts, bool forced, bool isAudio);
        void InitializeManifest();
    public:
        static OutFileDASHStream* GetInstance(BaseClientApplication *pApplication,
                string name, Variant &settings);
        virtual ~OutFileDASHStream();

        virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
        virtual bool IsCompatibleWithType(uint64_t type);
        inline bool SignalPlay(double &dts, double &length) { return true; }
        inline bool SignalPause() { return true; }
        inline bool SignalResume() { return true; }
        inline bool SignalSeek(double &dts) { return true; }
        inline bool SignalStop() { return true; }
        inline void SignalAttachedToInStream() {}
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
        inline void SetApplication(BaseClientApplication* value) { _pApplication = value; }
    protected:
        virtual bool FinishInitialization(
                GenericProcessDataSetup *pGenericProcessDataSetup);
        virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
                bool isKeyFrame);
        virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
        virtual bool IsCodecSupported(uint64_t codec);
    };
}

#endif	/* _OUTFILEDASHSTREAM_H */
#endif	/* HAS_PROTOCOL_DASH */
