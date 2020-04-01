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

#include "streaming/hds/hdsfragment.h"
#include "mediaformats/writers/fmp4/atom_afra.h"
#include "mediaformats/writers/fmp4/atom_abst.h"
#include "mediaformats/writers/fmp4/atom_moof.h"
#include "mediaformats/writers/fmp4/atom_mdat.h"

HDSFragment::HDSFragment(string fullPath, uint32_t fragmentNum, Atom_abst *abst,
		double targetDuration, int64_t audioRate, int64_t videoRate, double additionalFramesPercent,
		double startPts, double startDts) : _DEFAULT_TRACK_ID(1) {
	//1. save input parameters
	_fullPath = fullPath;
	_fragmentNum = fragmentNum;
	_pAbst = abst;
	_targetDuration = targetDuration;
	_audioRate = audioRate;
	_videoRate = videoRate;
	_startPts = startPts;
	_startDts = startDts;
	_additionalFramesPercent = additionalFramesPercent;

	//2. Initialize the rest of the member variables to their default
	//values
	_maxFrameCount = 0;
	_currentFrameCount = 0;
	_lastAudioPts = -1;
	_lastAudioDts = -1;
	_lastVideoPts = -1;
	_lastVideoDts = -1;
	_forceClose = false;
	_maxAudioFramesCount = 0;
	_maxVideoFramesCount = 0;

	//3. compute the maximum frames counters
	if (_audioRate > 0) {
		//ideal number of frames
		_maxAudioFramesCount = ((uint64_t) ((_targetDuration * (double) audioRate) / 1024.0)) + 1;

		//additional number of frames
		_maxAudioFramesCount = (uint64_t) (_additionalFramesPercent * (double) _maxAudioFramesCount);
	}
	if (_videoRate > 0) {
		//ideal number of frames
		_maxVideoFramesCount = ((uint64_t) ((_targetDuration * (double) videoRate))) + 1;

		//additional number of frames
		_maxVideoFramesCount = (uint64_t) (_additionalFramesPercent * (double) _maxVideoFramesCount);
	}
	_maxFrameCount = _maxAudioFramesCount + _maxVideoFramesCount;

	//FINEST("MA: %"PRIu64"; MV: %"PRIu64"; M: %"PRIu64"; AFP: %.3f%%",
	//		_maxAudioFramesCount,
	//		_maxVideoFramesCount,
	//		_maxFrameCount,
	//		_additionalFramesPercent * 100.0);

	_pAfra = NULL;
	_pMoof = NULL;
	_pMdat = NULL;

	_startTime = Variant::Now();
	_frameCount = 0;
}

HDSFragment::~HDSFragment() {
	// Check if fragment is still open
	if (_fileFragment.IsOpen()) {
		_fileFragment.Close();
	}

	if (NULL != _pAfra) {
		delete _pAfra;
		_pAfra = NULL;
	}

	if (NULL != _pMoof) {
		delete _pMoof;
		_pMoof = NULL;
	}

	if (NULL != _pMdat) {
		delete _pMdat;
		_pMdat = NULL;
	}

	//INFO("MA: %"PRIu64"; MV: %"PRIu64"; M: %"PRIu64"; U: %"PRIu64"(%.6f%%)",
	//		_maxAudioFramesCount,
	//		_maxVideoFramesCount,
	//		_maxFrameCount,
	//		_currentFrameCount,
	//		(double) _currentFrameCount / (double) _maxFrameCount * 100.0
	//		);
}

void HDSFragment::ForceClose() {
	_forceClose = true;
}

bool HDSFragment::Initialize() {

	// Create a new fragment file
	//FINEST("Creating %s...", STR(_fullPath));

	if (!_fileFragment.Initialize(_fullPath, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to initialize new fragment: %s", STR(_fullPath));
		return false;
	}

	/*
	 * Initialize the following atoms
	 *
	 * AFRA
	 * ABST
	 * MOOF
	 *  - MFHD
	 *  - TRAF
	 *  -- TFHD
	 *  -- TRUN
	 *  -- FREE -> (_maxFrameCount + 2 sequence headers/codec setup) * size of TRUN entry
	 * MDAT
	 */

	// Initialize the atoms/boxes
	// AFRA
	if (NULL == _pAfra) {
		_pAfra = new Atom_afra;
		_pAfra->Initialize(true, &_fileFragment, 0);
		_pAfra->AddAfraEntry((uint64_t) _startDts, 0); // dummy offset
		_pAfra->Write();
	}

	// ABST
	_pAbst->SetPosition(_pAfra->GetEndPosition());
	_pAbst->SetFile(&_fileFragment);
	_pAbst->WriteBufferToFile();

	// MOOF
	if (NULL == _pMoof) {
		_pMoof = new Atom_moof;
		_pMoof->Initialize(true, &_fileFragment, _pAbst->GetEndPosition(), _fragmentNum);
		_pMoof->AddTrackFragment();
		_pMoof->SetMultipleTrun(false);
		//_pMoof->SetTfhdBaseOffset(0x3780 + 8 + FLV_FILEHEADERSIZE);
		// Reserve TRAF box for estimated TRUN entries
		_pMoof->ReserveTrafSpace(((uint32_t) _maxFrameCount) * _pMoof->GetTrunEntrySize());
		_pMoof->Write();
	}

	// MDAT
	if (NULL == _pMdat) {
		_pMdat = new Atom_mdat;
		_pMdat->Initialize(true, &_fileFragment, _pMoof->GetEndPosition());
		//_pMdat->HasFLVHeader(true);
		_pMdat->Write();
	}

	//TODO support this later on
	// Add the FLV header
	//	uint8_t _flvHeader[] = {'F', 'L', 'V', 1, 0, 0, 0, 0, 9, 0, 0, 0, 0};
	//	if (-1 != _audioRate)
	//		_flvHeader[4] |= 0x04;
	//	if (-1 != _videoRate)
	//		_flvHeader[4] |= 0x01;
	//	if (!_fileFragment.WriteBuffer(_flvHeader, sizeof (_flvHeader))) {
	//		FATAL("Unable to write flv header!");
	//		return false;
	//	}

	// Close the file and reopen as MODE_WRITE
	_fileFragment.Close();

	if (!_fileFragment.Initialize(_fullPath, FILE_OPEN_MODE_WRITE)) {
		FATAL("Unable to reinitialize new fragment: %s", STR(_fullPath));
		return false;
	}

	// Initialize the FLV tag headers
	memset(_tagHeader, 0, sizeof (_tagHeader));

	return true;
}

bool HDSFragment::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	if (_currentFrameCount >= _maxFrameCount) {
		FATAL("Frames count exceeded. Frame dropped");
		return false;
	}

	// Add the sample inside MDAT and references inside TRUN
	AddSample(buffer, pts, dts, false, isKeyFrame);

	// Update the necessary members
	_lastVideoPts = pts;
	_lastVideoDts = dts;
	_currentFrameCount++;
	_frameCount++;

	return true;
}

bool HDSFragment::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	if (_currentFrameCount >= _maxFrameCount) {
		FATAL("Frames count exceeded. Frame dropped");
		return false;
	}

	// Add the sample inside MDAT and references inside TRUN
	AddSample(buffer, pts, dts, true);

	// Update the necessary members
	_lastAudioPts = pts;
	_lastAudioDts = dts;
	_currentFrameCount++;

	return true;
}

double HDSFragment::AudioDuration() {
	return (_lastAudioPts >= 0) ? (_lastAudioPts - _startPts) : (-1.0);
}

double HDSFragment::VideoDuration() {
	return (_lastVideoPts >= 0) ? (_lastVideoPts - _startPts) : (-1.0);
}

double HDSFragment::Duration() {
	double audioDuration = AudioDuration();
	double videoDuration = VideoDuration();
	return audioDuration > videoDuration ? audioDuration : videoDuration;
}

bool HDSFragment::MustClose() {
	return _forceClose || (_currentFrameCount >= _maxFrameCount);
}

double HDSFragment::AdditionalFramesPercent() {
	return _additionalFramesPercent;
}

HDSFragment::operator string() {
	return format("PTS: %8.2f; DTS: %8.2f; DA: %8.2f; DV: %8.2f; D: %8.2f; %s",
			_startPts,
			_startDts,
			AudioDuration(),
			VideoDuration(),
			Duration(),
			STR(_fullPath)
			);
}

void HDSFragment::AddSample(IOBuffer &buffer, double pts, double dts,
		bool isAudio, bool isKeyFrame) {

	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(buffer);

	// Add the FLV tag header
	EHTONLP(_tagHeader, dataLength);

	if (isAudio) {
		_tagHeader[0] = 0x08;
	} else {
		_tagHeader[0] = 0x09;
	}

	EHTONAP(_tagHeader + 4, (uint32_t) dts);

	EHTONLP(_tagHeader + 11, dataLength + 11);

	_pMoof->AddTrunSample(
			_DEFAULT_TRACK_ID,
			dts,
			dataLength + 11 + 4,
			isKeyFrame,
			(uint32_t) (pts - dts),
			isAudio);

	_pMdat->AddSample(buffer, _tagHeader);
}

void HDSFragment::UpdateAbst(bool isFirstFragment, double lastTS) {

	double nextTS = lastTS;

	if (nextTS == -1) {
		nextTS = _startDts + Duration();
	}

	if (!isFirstFragment) {
		// Not the first fragment, so either add or modify the second AFRT entry
		_pAbst->ModifyFragmentRunEntry(0, 1, _fragmentNum, (uint64_t) _startDts,
				(uint32_t) (nextTS - _startDts), 0);
	} else {
		// Update the fragment run entry representing the first fragment
		_pAbst->ModifyFragmentRunEntry(0, 0, _fragmentNum, (uint64_t) _startDts,
				(uint32_t) (nextTS - _startDts), 0);
	}

	_pAbst->UpdateFragments(_fragmentNum, _startDts, lastTS);

	// Set the current media time stamp of the next fragment
	_pAbst->SetMediaTimeStamp((uint64_t) nextTS);
}

void HDSFragment::FinalizeAbst() {

	/*
	 * Finalize ABST when end of stream is encountered
	 * NOTE: This is the same behavior with the new HDS demo (Hillman)
	 */

	// Add a second fragment run entry (that of the last fragment)
	// UPDATE: no need since we're adding this on the start, just update it
	_pAbst->ModifyFragmentRunEntry(0, 1, _fragmentNum, (uint64_t) _startDts,
			(uint32_t) Duration(), 0);

	// We add a third entry which is all zero to terminate the run
	_pAbst->AddFragmentRunEntry(0, 0, 0, 0, 0);

	// This is also the same behavior with the new HDS demo (Hillman)
	_pAbst->SetInfoVersion(0x02);

	/* We start with 'live' stream type. Once everything is recorded or the
	 * inbound stream dies, replace the stream type as recoded.
	 */
	_pAbst->SetLiveFlag(false);
}

void HDSFragment::Finalize() {
	// Close the actual fragment file
	if (_fileFragment.IsOpen()) {
		_fileFragment.Flush();

		// Write down all header atoms to the file as well as the needed updates

		// Update size of MDAT
		_pMdat->UpdateSize();

		// Update MOOF
		_pMoof->Update();

		// Write the updates on ABST
		_pAbst->WriteBufferToFile(true);

		// Update AFRA
		// recompute offset = mdat position + 8 (mdat header) + 13 (FLV header)
		_pAfra->ModifyAfraRunEntry(0, (uint64_t) _startDts,
				_pMdat->GetStartPosition() + _pMdat->GetHeaderSize() + FLV_FILEHEADERSIZE);

		// Do the closing of this file on the destructor instead
		//_fileFragment.Close();
	}
}

string HDSFragment::GetPath() {
	return _fileFragment.GetPath();
}

Variant HDSFragment::GetStartTime() {
	return _startTime;
}

uint64_t HDSFragment::GetFrameCount() {
	return _frameCount;
}

#endif	/* HAS_PROTOCOL_HDS */
