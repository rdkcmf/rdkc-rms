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

#include "streaming/mss/mssfragment.h"
#include "mediaformats/writers/fmp4/atom_moof.h"
#include "mediaformats/writers/fmp4/atom_mdat.h"
#include "mediaformats/writers/fmp4/atom_mfra.h"
#include "mediaformats/writers/fmp4/atom_ftyp.h"
#include "mediaformats/writers/mp4initsegment/outfilemp4initsegment.h"
#include "mediaformats/writers/fmp4/atom_isml.h"
#include "streaming/mss/outfilemssstream.h"
#define CHUNKFRAME_TOLERANCE 4000 // maximum no. of frames

MSSFragment::MSSFragment(string fullPath, uint32_t fragmentNum, double targetDuration,
        int64_t rate, double additionalFramesPercent,
        double startPts, double startDts, bool isAudio,
        MicrosoftSmoothStream::GroupState& groupState) 
        : _DEFAULT_TRACK_ID(1),
          _currentGroupState(groupState) {
    //1. save input parameters
    _fullPath = fullPath;
    _fragmentNum = fragmentNum;
    _targetDuration = targetDuration;
    _rate = rate;
    _startPts = startPts;
    _startDts = startDts;
    _additionalFramesPercent = additionalFramesPercent;
    //_fragmentTimeTicks = 0;
    _isAudio = isAudio;

    //2. Initialize the rest of the member variables to their default
    //values
    _maxFrameCount = 0;
    _currentFrameCount = 0;
    _lastDts = -1;
    _lastPts = -1;
    _forceClose = false;

    //3. compute the maximum frames counters
    if (_rate > 0) {
        //ideal number of frames
        if (isAudio)
            _maxFrameCount = ((uint64_t)((_targetDuration * (double)_rate) / 1024.0)) + 1 + CHUNKFRAME_TOLERANCE;
        else
            _maxFrameCount = ((uint64_t)(_targetDuration * (double)_rate)) + 1 + CHUNKFRAME_TOLERANCE;

        //additional number of frames
        _maxFrameCount = (uint64_t) (_additionalFramesPercent * (double) _maxFrameCount);
    }
    _pFtyp = NULL;
    _pMoof = NULL;
    _pMdat = NULL;
    _pUuid = NULL;
    _pMfra = NULL;

    _startTime = Variant::Now();
    _frameCount = 0;
    _fileFragment = NULL;
    _isFirstFragment = false;
    _ftypSize = 0;
    _ismlmoovSize = 0;
    _moofmdatSize = 0;
    _mfraSize = 0;
}

MSSFragment::~MSSFragment() {
    // Check if fragment is still open
    if (NULL != _fileFragment && _fileFragment->IsOpen()) {
        _fileFragment->Close();
        delete _fileFragment;
        _fileFragment = NULL;
    }

    if (NULL != _pMoof) {
        delete _pMoof;
        _pMoof = NULL;
    }

    if (NULL != _pMdat) {
        delete _pMdat;
        _pMdat = NULL;
    }

    if (NULL != _pFtyp) {
        delete _pFtyp;
        _pFtyp = NULL;
    }
    
    if (NULL != _pUuid) {
        delete _pUuid;
        _pUuid = NULL;
    }

    if (NULL != _pMfra) {
        delete _pMfra;
        _pMfra = NULL;
    }
}

void MSSFragment::ForceClose() {
    _forceClose = true;
}

bool MSSFragment::Initialize(GenericProcessDataSetup* pGenericProcessDataSetup, 
    BaseClientApplication* pApplication, BaseInStream* pInStream, string const& pISMType, 
    Variant& streamNames, uint16_t trackId) {

    NULL == _fileFragment ? _fileFragment = new File() : _fileFragment;
    // Create a new fragment file
    if (!_fileFragment->Initialize(_fullPath, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to initialize new fragment: %s", STR(_fullPath));
        return false;
    }

    /*
     * Initialize the following atoms
     *==== for live ingest include the ff. atoms ===>
     * FTYP
     * MOOV
     *  - MVHD
     *  - TRAK
     *  -- TKHD
     *  -- MDIA
     *  --- MDHD
     *  --- HDLR
     *  --- MINF
     *  ---- VMHD
     *  ---- DINF
     *  ----- DREF
     *  ------ URL
     *  ---- STBL
     *  ----- STSD
     *  ------ AVC1/MP4A
     *  ----- STTS
     *  ----- STSS
     *  ----- CTTS
     *  ----- STSC
     *  ----- STSZ
     *  ----- STCO
     *  - MVEX
     *  -- TREX
     *<====
     * MOOF
     *  - MFHD
     *  - TRAF
     *  -- TFHD
     *  -- TRUN
     *  -- FREE
     * MDAT
     */

    // Initialize the atoms/boxes

    if (pISMType == "isml" && !_currentGroupState.moovHeaderFlushed) {
        if (NULL == _pFtyp) {
            _pFtyp = new Atom_ftyp;
            _pFtyp->Initialize(true, _fileFragment, 0);
            _pFtyp->Write();
        }

        _ftypSize = (uint32_t)_pFtyp->GetSize();
        if (NULL == _pUuid) {
            uint64_t pFtypEndPos = _fileFragment->Cursor();
            _pUuid = new Atom_isml;
            _pUuid->Initialize(true, _fileFragment, pFtypEndPos);
            //put user types, 4 blocks offset then isml here
            _pUuid->SetXmlString(_xmlString);
            _pUuid->Write();
        }

        Variant settings;
        string name = "MoovHeaderWriter";
        settings["computedPathToFile"] = _fullPath;
        settings["chunkLength"] = 0;
        OutFileMP4InitSegment* pMoovHeader = OutFileMP4InitSegment::GetInstance(pApplication, name, settings);
        if (!pMoovHeader->CreateMoovHeader(_fileFragment, pInStream, pGenericProcessDataSetup->_hasAudio, 
                                            pGenericProcessDataSetup->_hasVideo, streamNames, _currentGroupState.trakCounts)) {
            FATAL("Unable to write Moov header");
            return false;
        }
        _ismlmoovSize = (uint32_t)_fileFragment->Size() - _ftypSize;
        _currentGroupState.moovHeaderFlushed = true;
        _isFirstFragment = true;
	}

    if (NULL == _pMoof) {
        uint64_t moofOffset = _fileFragment->Cursor();
        HttpStreamingType sType = MSS;
        if (pISMType == "isml")
            sType = MSSINGEST;

        _pMoof = new Atom_moof(sType);
        _pMoof->Initialize(true, _fileFragment, moofOffset, _fragmentNum);
        _pMoof->AddTrackFragment();
        _pMoof->SetMultipleTrun(false);
        if (pISMType == "ismc")
            trackId = 1;
        _pMoof->SetTrackId(trackId);
        // Reserve TRAF box for estimated TRUN entries
        _pMoof->ReserveTrafSpace(((uint32_t) _maxFrameCount) * _pMoof->GetTrunEntrySize());
        _pMoof->SetTrunDataOffset((int32_t) _pMoof->GetSize() + 8);
        _pMoof->Write();
    }

    // MDAT
    if (NULL == _pMdat) {
        _pMdat = new Atom_mdat;
        _pMdat->Initialize(true, _fileFragment, _pMoof->GetEndPosition());
        _pMdat->Write();
    }

    // Close the file and reopen as MODE_WRITE
    _fileFragment->Close();

    if (!_fileFragment->Initialize(_fullPath, FILE_OPEN_MODE_WRITE)) {
        FATAL("Unable to reinitialize new fragment: %s", STR(_fullPath));
        return false;
    }

    return true;
}

bool MSSFragment::PushVideoData(IOBuffer &buffer, double pts, double dts,
        bool isKeyFrame) {
    _frameCount++;
    return PushData(buffer, pts, dts, false, isKeyFrame);
}

bool MSSFragment::PushAudioData(IOBuffer &buffer, double pts, double dts) {
    return PushData(buffer, pts, dts, true);
}

bool MSSFragment::PushData(IOBuffer &buffer, double pts, double dts,
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

double MSSFragment::Duration() {
    return (_lastPts >= 0) ? (_lastPts - _startPts) : (-1.0);
}

bool MSSFragment::MustClose() {
    return _forceClose || (_currentFrameCount >= _maxFrameCount);
}

double MSSFragment::AdditionalFramesPercent() {
    return _additionalFramesPercent;
}

MSSFragment::operator string() {
    return format("PTS: %8.2f; DTS: %8.2f; D: %8.2f; %s",
            _startPts,
            _startDts,
            Duration(),
            STR(_fullPath)
            );
}

void MSSFragment::AddSample(IOBuffer &buffer, double pts, double dts,
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

void MSSFragment::Finalize() {
    // Close the actual fragment file
    if (_fileFragment->IsOpen()) {
        _fileFragment->Flush();

        // Write down all header atoms to the file as well as the needed updates

        // Update size of MDAT
        _pMdat->UpdateSize();

        // Update MOOF
        _pMoof->Update();
    }
}

string MSSFragment::GetPath() {
    return _fileFragment->GetPath();
}

double MSSFragment::GetStartDts() {
    return _startDts;
}

double MSSFragment::GetStartPts() {
    return _startPts;
}

void MSSFragment::SetLastTS(double lastTS, bool isAudio) {
    if (_pMoof != NULL)
        _pMoof->SetLastTS(lastTS, isAudio);
}

uint64_t MSSFragment::GetTotalDuration(bool isAudio) {
    if (_pMoof != NULL)
        return _pMoof->GetTotalDuration(isAudio);
    return 0;
}

Variant MSSFragment::GetStartTime() {
    return _startTime;
}

uint64_t MSSFragment::GetFrameCount() {
    return _frameCount;
}

bool MSSFragment::AddMfraBox() {
    if (NULL == _fileFragment) {
        FATAL("Can't write to _fileFragment, value is NULL");
        return false;
    }
    _pMfra = new Atom_mfra;
    _pMfra->Initialize(true, _fileFragment, _pMdat->GetEndPosition());
    _pMfra->Write();
    return true;
}

void MSSFragment::SetXmlString(const string &xmlString) {
    _xmlString = xmlString;
}

void MSSFragment::SetTfxdAbsoluteTimestamp(uint64_t timeStamp) {
    if (_pMoof != NULL)
        _pMoof->SetTfxdAbsoluteTimestamp(timeStamp);
	_tfxdTimeStamp = timeStamp;
}

void MSSFragment::SetTfxdFragmentDuration(uint64_t duration) {
    if (_pMoof != NULL)
        _pMoof->SetTfxdFragmentDuration(duration);
	_tfxdDuration = duration;
}

Variant MSSFragment::GetLiveIngestMessageSizes() {
    Variant result;
    if (_isFirstFragment) {
        result["firstFragment"] = _isFirstFragment;
        result["ftyp"] = _ftypSize;
        result["ismlmoov"] = _ismlmoovSize;
    }
    _moofmdatSize = (uint32_t)(_pMoof->GetSize() + _pMdat->GetSize());
    result["moofmdat"] = _moofmdatSize;
    if (_currentGroupState.trakCounts == 1 && NULL != _pMfra) {
        _mfraSize = (uint32_t) _pMfra->GetSize();
        result["mfra"] = _mfraSize;
    }

    return result;
}

uint64_t MSSFragment::GetTfxdOffset() {
	if (_pMoof != NULL)
		return _pMoof->GetTfxdOffset();
	return 0;
}

uint64_t MSSFragment::GetFreeOffset() {
	if (_pMoof != NULL)
		return _pMoof->GetFreeOffset();
	return 0;
}
#endif	/* HAS_PROTOCOL_MSS */
