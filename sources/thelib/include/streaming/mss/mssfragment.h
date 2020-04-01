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
#ifndef _MSSFRAGMENT_H
#define _MSSFRAGMENT_H

//#define FLV_FILEHEADERSIZE 13
#define FLV_FILEHEADERSIZE 0

#include "common.h"

/*
 *  url="{bitrate}/streamname_{start_time}"
 */
class Atom_moof;
class Atom_mdat;
class Atom_isml;
class Atom_ftyp;
class Atom_isml;
class Atom_mfra;
struct GenericProcessDataSetup;
namespace MicrosoftSmoothStream {
    struct GroupState;
}
class BaseClientApplication;
class BaseInStream;
class OutFileMP4InitSegment;

class DLLEXP MSSFragment {
private:
    /*!
     * @brief full path to the fragment file
     */
    string _fullPath;

    /*!
     * @brief maximum amount of time this fragment should accumulate
     */
    double _targetDuration;

    /*!
     * @brief the video rate (frames/second) Used to compute the number of
     * frames needed to accommodate _targetDuration seconds worth of video
     * data
     */
    int64_t _rate;

    /*!
     * @brief maximum number of frames that this fragment will accept.
     */
    uint64_t _maxFrameCount;

    /*!
     * @brief how many additional frames should be reserved. Expressed as a
     * percentage.
     */
    double _additionalFramesPercent;

    /*!
     * @brief how many frames were currently accumulated by this fragment
     * It must be incremented by the PushXXXData function on each
     * successfully stored frame.
     */
    uint64_t _currentFrameCount;

    /*!
     * @brief this field stores the first pts the fragment will store.
     */
    double _startPts;
    /*!
     * @brief this field stores the first dts the fragment will store.
     */
    double _startDts;
    /*!
     * @brief this field stores the last seen dts the fragment stored.
     */
    double _lastDts;
    /*!
     * @brief this field stores the last seen pts the fragment stored.
     */
    double _lastPts;
    /*!
     * @brief this field indicates that MustClose will return true no matter what
     * Used to force closing the current segment when the audio/video codecs changed
     */
    bool _forceClose;

    /*
     * Include the needed atoms/boxes for this fragment
     */
    Atom_moof* _pMoof;
    Atom_mdat* _pMdat;
    Atom_ftyp* _pFtyp;
    Atom_isml* _pUuid;
    Atom_mfra* _pMfra;

    // File to be used for the actual fragment
    File* _fileFragment;

    const uint8_t _DEFAULT_TRACK_ID;

    uint32_t _fragmentNum;
    bool _isAudio;
    Variant _startTime;
    uint64_t _frameCount;
    string _manifestEntry;
    string _xmlString;
    bool _isFirstFragment;
    uint32_t _ftypSize;
    uint32_t _ismlmoovSize;
    uint32_t _moofmdatSize;
    uint32_t _mfraSize;
    MicrosoftSmoothStream::GroupState& _currentGroupState;
	uint64_t _tfxdTimeStamp;
	uint64_t _tfxdDuration;
public:

    /*!
     * @brief creates a new instance
     *
     * @param fullPath - full path to the fragment file
     *
     * @param fixedHeaderSize -  size of the fixed atoms to be reserved
     *
     * @param targetDuration - maximum amount of time this fragment should
     * accumulate
     *
     * @param audioRate - the audio rate (samples/second) expressed in HZ.
     * Used to compute the number of frames needed to accommodate
     * targetDuration seconds worth of audio data
     *
     * @param videoRate - the video rate (frames/second) Used to compute the
     * number of frames needed to accommodate targetDuration seconds worth
     * of video data
     *
     * @param additionalFramesPercent - how many additional frames should be
     * reserved. Expressed as a percentage.
     *
     * @param startPts - the first pts the fragment will store. Used to
     * compute the duration of the fragment
     *
     * @param startDts - the first dts the fragment will store.
     */
    MSSFragment(string fullPath, uint32_t fragmentNum, double targetDuration,
            int64_t rate, double additionalFramesPercent,
            double startPts, double startDts, bool isAudio,
            MicrosoftSmoothStream::GroupState& groupState);

    /*!
     * @brief Standard destructor
     */
    virtual ~MSSFragment();

    /*!
     * @brief Forces the fragment to be closed on the next MustClose() inspection
     */
    void ForceClose();

    /*!
     * @brief This function should initialize all the dynamic parts
     * (pointers) of this class.
     */
    bool Initialize(GenericProcessDataSetup* pGenericProcessDataSetup,
        BaseClientApplication* pApplication, BaseInStream* pInStream, string const& pISMType, 
        Variant& streamNames, uint16_t trackId);

    /*!
     * @brief see the documentation on BaseOutStream::PushVideoData
     *
     * @discussion The form of the data inside the buffer is:
     * 0x17|0x27 0x01 0xXX 0xXX 0xXX (0xYY 0xYY 0xYY 0xYY NAL_PAYLOAD)+
     * (...)+ denotes one or many instances of "..."
     */
    bool PushVideoData(IOBuffer &buffer, double pts, double dts,
            bool isKeyFrame);

    /*!
     * @brief see the documentation on BaseOutStream::PushAudioData
     *
     * @discussion The form of the data inside the buffer is:
     * 0xAF 0x01 AAC_PAYLOAD
     */
    bool PushAudioData(IOBuffer &buffer, double pts, double dts);

    /*!
     * @brief returns the number of milliseconds of accumulated data. The
     * returned value is the maximum between AudioDuration() and
     * VideoDuration()
     */
    double Duration();

    /*!
     * @brief returns true if all the frames slots were used. Nothing to do
     * with the relation between Duration() and _targetDuration. In other
     * words, this function should return true ONLY if there is no more
     * slots for any additional frames. It is externally used to properly
     * adjust _additionalFramesPercent on the next fragment so the next
     * fragment will have enough space(self learning
     * _additionalFramesPercent).
     */
    bool MustClose();

    /*!
     * @brief Used externally to compute new value for
     * _additionalFramesPercent in combination with MustClose() function
     */
    double AdditionalFramesPercent();

    /*!
     * @brief standard string operator so we can write things like:
     *
     * MSSFragment *pFragemnt;
     * MSSFragment fragment;
     *
     */
    operator string();

    void Finalize();
    string GetPath();
    double GetStartPts();
    double GetStartDts();
    void SetLastTS(double lastTS, bool isAudio);
    void SetXmlString(const string &xmlString);
    uint64_t GetTotalDuration(bool isAudio);
    Variant GetStartTime();
    uint64_t GetFrameCount();
    bool AddMfraBox();
    void SetTfxdAbsoluteTimestamp(uint64_t timeStamp);
    void SetTfxdFragmentDuration(uint64_t duration);
    Variant GetLiveIngestMessageSizes();
	inline uint64_t GetTfxdAbsoluteTimestamp() { return _tfxdTimeStamp; }
	inline uint64_t GetTfxdFragmentDuration() { return _tfxdDuration;  }
	uint64_t GetTfxdOffset();
	uint64_t GetFreeOffset();
private:

    /**
     * Adds the actual sample (video and audio) with its corresponding timestamp
     * to the current fragment
     *
     * @param buffer Container for the sample
     * @param time Timestamp of this sample
     * @param isAudio Indicates that the sample is audio, otherwise video
     * @param isKeyFrame Indicate frame as such
     */
    void AddSample(IOBuffer &buffer, double pts, double dts, bool isAudio,
            bool isKeyFrame = false);
    bool PushData(IOBuffer &buffer, double pts, double dts, bool isAudio,
            bool isKeyFrame = false);
};

#endif /* _MSSFRAGMENT_H */
#endif /* HAS_PROTOCOL_MSS */
