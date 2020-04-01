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
#ifndef _DASHFRAGMENT_H
#define	_DASHFRAGMENT_H

#include "common.h"

class Atom_styp;
class Atom_sidx;
class Atom_moof;
class Atom_mdat;

class DLLEXP DASHFragment {
private:
    /*!
     * @brief full path to the fragment file
     */
    string _fullPath;
    /*!
     * @brief relative path to the fragment file (used in SegmentList)
     */
    string _relativePath;
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
	Atom_styp* _pStyp;
	Atom_sidx* _pSidx;
    Atom_moof* _pMoof;
    Atom_mdat* _pMdat;

    // File to be used for the actual fragment
    File _fileFragment;

    const uint8_t _DEFAULT_TRACK_ID;

    uint32_t _fragmentNum;
    Variant _startTime;
    uint64_t _frameCount;
    uint64_t _moofOffset;
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
    DASHFragment(string fullPath, uint32_t fragmentNum, double targetDuration,
            int64_t rate, double additionalFramesPercent,
            double startPts, double startDts, bool isAudio);

    /*!
     * @brief Standard destructor
     */
    virtual ~DASHFragment();

    /*!
     * @brief Forces the fragment to be closed on the next MustClose() inspection
     */
    inline void ForceClose() {
        _forceClose = true;
    }

    /*!
     * @brief This function should initialize all the dynamic parts
     * (pointers) of this class.
     */
    bool Initialize();

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
     * DASHFragment *pFragemnt;
     * DASHFragment fragment;
     *
     */
    operator string();

    void Finalize();

    string GetPath();
    void SetLastTS(double lastTS, bool isAudio);
	//tfdt setters
    void SetFragmentDecodeTime(uint64_t fragmentDecodeTime);
	//sidx setters
	void SetTimeScale(uint32_t value);
	void SetEarliestPresentationTime(uint64_t value);
	void SetFirstOffset(uint64_t value);
	void SetReferenceTypeSize(uint32_t value, size_t index = 0);
	void SetSubSegmentDuration(uint32_t value, size_t index = 0);
	void SetSAPFlags(uint32_t value, size_t index = 0);
    uint64_t GetFragmentDecodeTime();
    uint32_t GetTotalDuration(bool isAudio);
    uint32_t GetTotalSize();
    inline string GetRelativePath() const { return _relativePath; }
    inline void SetRelativePath(string const& value) {_relativePath = value; }
    inline double GetStartDts() const { return _startDts; }
    inline double GetStartPts() const { return _startPts; }
    inline Variant GetStartTime() const { return _startTime; }
    inline uint64_t GetFrameCount() const { return _frameCount; }
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

#endif	/* _DASHFRAGMENT_H */
#endif	/* HAS_PROTOCOL_DASH */
