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

#include "streaming/dash/dashfragment.h"
#include "mediaformats/writers/fmp4/atom_styp.h"
#include "mediaformats/writers/fmp4/atom_sidx.h"
#include "mediaformats/writers/fmp4/atom_moof.h"
#include "mediaformats/writers/fmp4/atom_mdat.h"
#define CHUNKFRAME_TOLERANCE 4000 // maximum no. of frames

DASHFragment::DASHFragment(string fullPath, uint32_t fragmentNum, double targetDuration,
        int64_t rate, double additionalFramesPercent,
        double startPts, double startDts, bool isAudio)
:
_fullPath(fullPath),
_targetDuration(targetDuration),
_rate(rate),
_maxFrameCount(0),
_additionalFramesPercent(additionalFramesPercent),
_currentFrameCount(0),
_startPts(startPts),
_startDts(startDts),
_lastDts(-1),
_lastPts(-1),
_forceClose(false),
_pStyp(NULL),
_pSidx(NULL),
_pMoof(NULL),
_pMdat(NULL),
_DEFAULT_TRACK_ID(1),
_fragmentNum(fragmentNum),
//_fragmentTimeTicks(0),
_frameCount(0) {
    // compute the maximum frames counters
    if (_rate > 0) {
        //ideal number of frames
        if (isAudio)
            _maxFrameCount = ((uint64_t) ((_targetDuration * (double) _rate) / 1024.0)) + 1 + CHUNKFRAME_TOLERANCE;
        else
            _maxFrameCount = ((uint64_t) (_targetDuration * (double) _rate)) + 1 + CHUNKFRAME_TOLERANCE;

        //additional number of frames
        _maxFrameCount = (uint64_t) (_additionalFramesPercent * (double) _maxFrameCount);
    }

    _startTime = Variant::Now();
}

DASHFragment::~DASHFragment() {
    // Check if fragment is still open
    if (_fileFragment.IsOpen()) {
        _fileFragment.Close();
    }

	if (NULL != _pStyp) {
		delete _pStyp;
		_pStyp = NULL;
	}

	if (NULL != _pSidx) {
		delete _pSidx;
		_pSidx = NULL;
	}

    if (NULL != _pMoof) {
        delete _pMoof;
        _pMoof = NULL;
    }

    if (NULL != _pMdat) {
        delete _pMdat;
        _pMdat = NULL;
    }

}

bool DASHFragment::Initialize() {

    // Create a new fragment file
    if (!_fileFragment.Initialize(_fullPath, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to initialize new fragment: %s", STR(_fullPath));
        return false;
    }
	
	/*
	* Initialize the following atoms
	*
	* STYP
	* SIDX
	* MOOF
	*  - MFHD
	*  - TRAF
	*  -- TFHD
	*  -- TFDT
	*  -- TRUN
	*  -- FREE
	* MDAT
	*/

    // Initialize the atoms/boxes
	// STYP
	if (NULL == _pStyp) {
		_pStyp = new Atom_styp;
		_pStyp->Initialize(true, &_fileFragment, _fileFragment.Cursor());
		_pStyp->Write();
	}

	// SIDX
	if (NULL == _pSidx) {
		_pSidx = new Atom_sidx;
		_pSidx->Initialize(true, &_fileFragment, _pStyp->GetEndPosition());
		_pSidx->Write();
	}

    // MOOF
    if (NULL == _pMoof) {
        _pMoof = new Atom_moof(DASH);
		_moofOffset = _pSidx->GetEndPosition();
		_pMoof->Initialize(true, &_fileFragment, _moofOffset, _fragmentNum);
        _pMoof->AddTrackFragment();
        _pMoof->SetMultipleTrun(false);
        // Reserve TRAF box for estimated TRUN entries
        _pMoof->ReserveTrafSpace(((uint32_t) _maxFrameCount) * _pMoof->GetTrunEntrySize());
        uint64_t mdatOffset = _pMoof->GetSize() + 8;
        _pMoof->SetTrunDataOffset((int32_t)mdatOffset);
        _pMoof->Write();
    }

    // MDAT
    if (NULL == _pMdat) {
        _pMdat = new Atom_mdat;
        _pMdat->Initialize(true, &_fileFragment, _pMoof->GetEndPosition());
        _pMdat->Write();
    }

    // Close the file and reopen as MODE_WRITE
    _fileFragment.Close();

    if (!_fileFragment.Initialize(_fullPath, FILE_OPEN_MODE_WRITE)) {
        FATAL("Unable to reinitialize new fragment: %s", STR(_fullPath));
        return false;
    }

    return true;
}

bool DASHFragment::PushVideoData(IOBuffer &buffer, double pts, double dts,
        bool isKeyFrame) {
    _frameCount++;
    return PushData(buffer, pts, dts, false, isKeyFrame);
}

bool DASHFragment::PushAudioData(IOBuffer &buffer, double pts, double dts) {
    return PushData(buffer, pts, dts, true);
}

bool DASHFragment::PushData(IOBuffer &buffer, double pts, double dts,
        bool isAudio, bool isKeyFrame) {
    if (_currentFrameCount >= _maxFrameCount) {
        FATAL("Frames count exceeded. Frame dropped");
        return false;
    }

    // Add the sample inside MDAT and references inside TRUN
    AddSample(buffer, pts, dts, isAudio, isKeyFrame);

    // Update the necessary members
    _lastPts = pts;
    _lastDts = dts;
    _currentFrameCount++;

    return true;
}

double DASHFragment::Duration() {
    return (_lastPts >= 0) ? (_lastPts - _startPts) : (-1.0);
}

bool DASHFragment::MustClose() {
    return _forceClose || (_currentFrameCount >= _maxFrameCount);
}

double DASHFragment::AdditionalFramesPercent() {
    return _additionalFramesPercent;
}

DASHFragment::operator string() {
    return format("PTS: %8.2f; DTS: %8.2f; D: %8.2f; %s",
            _startPts,
            _startDts,
            Duration(),
            STR(_fullPath)
            );
}

void DASHFragment::AddSample(IOBuffer &buffer, double pts, double dts,
        bool isAudio, bool isKeyFrame) {

    uint32_t dataLength = GETAVAILABLEBYTESCOUNT(buffer);

	if (dataLength > 0) {
		_pMoof->AddTrunSample(
			_DEFAULT_TRACK_ID,
			dts,
			dataLength,
			isKeyFrame,
			(uint32_t)(pts - dts),
			isAudio);

		_pMdat->AddSample(buffer);
	}

}

void DASHFragment::Finalize() {
    // Close the actual fragment file
    if (_fileFragment.IsOpen()) {
        _fileFragment.Flush();

        // Write down all header atoms to the file as well as the needed updates

        // Update size of MDAT
        _pMdat->UpdateSize();

        // Update MOOF
        _pMoof->Update();

		// Update SIDX
		uint32_t currentSize = GetTotalSize();
		SetReferenceTypeSize((0 << 31) | (currentSize & 0x7fffffff));
		_pSidx->Update();
    }
}

string DASHFragment::GetPath() {
    return _fileFragment.GetPath();
}

void DASHFragment::SetLastTS(double lastTS, bool isAudio) {
    if (_pMoof != NULL)
        _pMoof->SetLastTS(lastTS, isAudio);
}

void DASHFragment::SetFragmentDecodeTime(uint64_t fragmentDecodeTime) {
    if (_pMoof != NULL)
        _pMoof->SetTfdtFragmentDecodeTime(fragmentDecodeTime);
}

void DASHFragment::SetTimeScale(uint32_t value) {
	if (_pSidx != NULL)
		_pSidx->SetTimeScale(value);
}

void DASHFragment::SetEarliestPresentationTime(uint64_t value) {
	if (_pSidx != NULL)
		_pSidx->SetEarliestPresentationTime(value);
}

void DASHFragment::SetFirstOffset(uint64_t value) {
	if (_pSidx != NULL)
		_pSidx->SetFirstOffset(value);
}

void DASHFragment::SetReferenceTypeSize(uint32_t value, size_t index) {
	if (_pSidx != NULL)
		_pSidx->SetReferenceTypeSize(index, value);
}

void DASHFragment::SetSubSegmentDuration(uint32_t value, size_t index) {
	if (_pSidx != NULL)
		_pSidx->SetSubSegmentDuration(index, value);
}

void DASHFragment::SetSAPFlags(uint32_t value, size_t index) {
	if (_pSidx != NULL)
		_pSidx->SetSAPFlags(index, value);
}

uint64_t DASHFragment::GetFragmentDecodeTime() {
    if (_pMoof != NULL)
        return _pMoof->GetTfdtFragmentDecodeTime();
    return 0;
}

uint32_t DASHFragment::GetTotalDuration(bool isAudio) {
    if (_pMoof != NULL)
        return (uint32_t) _pMoof->GetTotalDuration(isAudio);
    return 0;
}

uint32_t DASHFragment::GetTotalSize() {
    if (_pMoof != NULL && _pMdat != NULL)
        return (uint32_t)_pMdat->GetSize() + (uint32_t)(_pMoof->GetSize() + _moofOffset);
    return 0;
}

#endif	/* HAS_PROTOCOL_DASH */
