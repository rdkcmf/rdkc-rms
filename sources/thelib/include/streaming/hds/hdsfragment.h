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


#ifdef HAS_PROTOCOL_HDS
#ifndef _HDSFRAGMENT_H
#define	_HDSFRAGMENT_H

//#define FLV_FILEHEADERSIZE 13
#define FLV_FILEHEADERSIZE 0

#include "common.h"

class Atom_afra;
class Atom_abst;
class Atom_moof;
class Atom_mdat;

class DLLEXP HDSFragment {
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
	 * @brief the audio rate (samples/second) expressed in HZ. Used to
	 * compute the number of frames needed to accommodate _targetDuration
	 * seconds worth of audio data
	 */
	int64_t _audioRate;

	/*!
	 * @brief the video rate (frames/second) Used to compute the number of
	 * frames needed to accommodate _targetDuration seconds worth of video
	 * data
	 */
	int64_t _videoRate;

	/*!
	 * @brief maximum number of audio frames that this fragment will accept.
	 * Computed from _audioRate and _targetDuration with an additional
	 * _additionalFramesPercent number of frames
	 */
	uint64_t _maxAudioFramesCount;

	/*!
	 * @brief maximum number of video frames that this fragment will accept.
	 * Computed from _videoRate and _targetDuration with an additional
	 * _additionalFramesPercent number of frames
	 */
	uint64_t _maxVideoFramesCount;

	/*!
	 * @brief maximum number of frames that this fragment will accept.
	 * It is the sum of _maxAudioFramesCount and _maxVideoFramesCount
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
	 * @brief this field stores the first pts the fragment will store. Used
	 * to compute the duration of the fragment
	 */
	double _startPts;

	/*!
	 * @brief this field stores the first dts the fragment will store.
	 */
	double _startDts;

	/*!
	 * @brief this field stores the last seen audio pts the fragment stored.
	 * Used to compute the duration of the fragment
	 */
	double _lastAudioPts;

	/*!
	 * @brief this field stores the last seen audio dts the fragment stored.
	 */
	double _lastAudioDts;

	/*!
	 * @brief this field stores the last seen video pts the fragment stored.
	 * Used to compute the duration of the fragment
	 */
	double _lastVideoPts;

	/*!
	 * @brief this field stores the last seen video dts the fragment stored.
	 */
	double _lastVideoDts;

	/*!
	 * @brief this field indicates that MustClose will return true no matter what
	 * Used to force closing the current segment when the audio/video codecs changed
	 */
	bool _forceClose;

	/*
	 * Include the needed atoms/boxes for this fragment
	 */
	Atom_afra* _pAfra;
	Atom_abst* _pAbst;
	Atom_moof* _pMoof;
	Atom_mdat* _pMdat;

	// File to be used for the actual fragment
	File _fileFragment;

	const uint8_t _DEFAULT_TRACK_ID;

	uint32_t _fragmentNum;

	uint8_t _tagHeader[15];

	Variant _startTime;
	uint64_t _frameCount;
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
	HDSFragment(string fullPath, uint32_t fragmentNum, Atom_abst *abst,
			double targetDuration, int64_t audioRate, int64_t videoRate,
			double additionalFramesPercent, double startPts, double startDts);

	/*!
	 * @brief Standard destructor
	 */
	virtual ~HDSFragment();

	/*!
	 * @brief Forces the fragment to be closed on the next MustClose() inspection
	 */
	void ForceClose();

	/*!
	 * @brief This function should initialize all the dynamic parts
	 * (pointers) of this class. Upon initialization, pass the fixed header
	 * box sizes as well as the ABST box position and ABST itself
	 *
	 * @param headerSize Size occupied by boxes before MOOF
	 * @param abstPos Start position of ABST box
	 * @param abst ABST box/atom
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
	 * @brief returns the number of milliseconds of accumulated audio data
	 * or -1 if no audio data was stored so far
	 */
	double AudioDuration();

	/*!
	 * @brief returns the number of milliseconds of accumulated video data
	 * or -1 if no video data was stored so far
	 */
	double VideoDuration();

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
	 * HDSFragment *pFragemnt;
	 * HDSFragment fragment;
	 *
	 * FINEST("fragment: %s",STR(*pFragment));
	 * //or
	 * FINEST("fragment: %s",STR(fragment));
	 */
	operator string();

	//void InitializeAbst();
	void UpdateAbst(bool isFirstFragment, double lastTS);
	void FinalizeAbst();

	void Finalize();
	
	string GetPath();
	Variant GetStartTime();
	uint64_t GetFrameCount();
private:

	/**
	 * Writes the FLV tag header of the sample
	 *
	 * @param buffer Container for the sample
	 * @param dataLen Length of the sample without the headers
	 * @param ts Timestamp of this sample
	 * @param isAudio Flag to indicate if the sample is audio or not
	 */
	void WriteFlvTagHeader(IOBuffer &buffer, uint32_t dataLen, double ts,
			bool isAudio);

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
};

#endif	/* _HDSFRAGMENT_H */
#endif	/* HAS_PROTOCOL_HDS */
