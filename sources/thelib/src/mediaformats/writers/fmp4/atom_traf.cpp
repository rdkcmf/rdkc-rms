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

#include "mediaformats/writers/fmp4/atom_traf.h"
#include "mediaformats/writers/fmp4/atom_tfhd.h"
#include "mediaformats/writers/fmp4/atom_trun.h"
#include "mediaformats/writers/fmp4/atom_free.h"
#include "mediaformats/writers/fmp4/atom_tfdt.h"
#include "mediaformats/writers/fmp4/atom_msstfxd.h"

Atom_traf::Atom_traf(HttpStreamingType streamingType) : Atom(0x74726166, false) {

    _useMultipleTrunBoxes = true;
    _trunBoxes = 0;
    _pTfhd = NULL;
    _pTrun = NULL;
    _pFree = NULL;
    _pTfdt = NULL;
    _pMssTfxd = NULL;
    _tfhdPos = 0;
    _firstTrunPos = 0;
    _streamingType = streamingType;
}

Atom_traf::~Atom_traf() {

    if (NULL != _pTfhd) {
        delete _pTfhd;
        _pTfhd = NULL;
    }

    if (NULL != _pTrun) {
        delete _pTrun;
        _pTrun = NULL;
    }

    if (NULL != _pFree) {
        delete _pFree;
        _pFree = NULL;
    }

    if (NULL != _pTfdt) {
        delete _pTfdt;
        _pTfdt = NULL;
    }

    if (NULL != _pMssTfxd) {
        delete _pMssTfxd;
        _pMssTfxd = NULL;
    }
}

void Atom_traf::SetTrackId(uint32_t trackId) {

    _pTfhd->SetTrackID(trackId);
}

uint32_t Atom_traf::GetTrackId() {

    return _pTfhd->GetTrackID();
}

void Atom_traf::AddTrunSample(double startTime, uint32_t size, bool isKeyFrame,
        uint32_t compositionTimeOffset, bool isAudio) {

    _pTrun->AddSample(startTime, size, isKeyFrame, compositionTimeOffset, isAudio);

    if (_useMultipleTrunBoxes) {
        // Adjust the start position if we're using multiple TRUNs
        _pTrun->SetPosition(_firstTrunPos + (_pTrun->GetSize() * _trunBoxes));

        // Increment the number of TRUN boxes/atoms
        _trunBoxes++;

        //TODO, this needs to be completed for multiple TRUNs
        NYIA;
    }
}

void Atom_traf::SetMultipleTrun(bool useMultipleTruns) {

    _useMultipleTrunBoxes = useMultipleTruns;
}

bool Atom_traf::GetMultipleTrunFlag() {

    return _useMultipleTrunBoxes;
}

bool Atom_traf::WriteFields() {

    // Write TFHD
    if (!_pTfhd->Write()) {
        FATAL("Unable to write TFHD box/atom!");
        return false;
    }

    if (_streamingType == DASH) {
        // Write TFDT
        if (!_pTfdt->Write()) {
            FATAL("Unable to write TFDT box/atom!");
            return false;
        }
    }

    // Write TRUN
    if (!_pTrun->Write()) {
        FATAL("Unable to write TRUN box/atom!");
        return false;
    }

    if (_isDirectFileWrite) {
        // Reserve some space if this is a direct file write
        if (_reservedSpace < Atom_free::MINIMUM)
            _reservedSpace = Atom_free::MINIMUM;
        _pFree->SetFreeSpace((uint32_t) _reservedSpace);

        // Write FREE
        if (!_pFree->Write()) {
            FATAL("Unable to write FREE box/atom!");
            return false;
        }
    }

	if (_streamingType == MSS || _streamingType == MSSINGEST) {
		// Write true TFXD
		if (!_pMssTfxd->Write()) {
			FATAL("Unable to write true TFXD box/atom!");
			return false;
		}
    }

    return true;
}

uint64_t Atom_traf::GetFieldsSize() {
    /*
     * Compute the size of this atom.
     */
    uint64_t size = 0;

    // TFHD size
    size += _pTfhd->GetSize();

    if (_streamingType == DASH) {
        // TFDT size
        size += _pTfdt->GetSize();
    }

    // TRUN size
    if (_useMultipleTrunBoxes) {
        size += (((uint64_t) _pTrun->GetSize()) * _trunBoxes);
    } else {
        size += _pTrun->GetSize();
    }

    if (_isDirectFileWrite) {
        // Add the reserved space if this is a direct file write
        size += _reservedSpace;
    }

	if (_streamingType == MSS || _streamingType == MSSINGEST) {
        // TFXD size
		size += _pMssTfxd->GetSize();
    }

    return size;
}

uint64_t Atom_traf::GetTrunBoxSize() {

    return _pTrun->GetSize();
}

uint8_t Atom_traf::GetTrunEntrySize() {

    return _pTrun->GetEntrySize();
}

void Atom_traf::Update() {

    if (_streamingType == DASH) {
        _pTfdt->UpdateSequenceNo();
        _pTfdt->UpdateFragmentDecodeTime();
        _pTrun->SetPosition(_pTfdt->GetEndPosition());
    }

    _pTrun->Update();

    // Update the reserved space
    _reservedSpace -= (((uint64_t) _pTrun->GetEntrySize()) * _pTrun->GetSampleCount());

    if (_reservedSpace >= _pFree->MINIMUM) {
        _pFree->SetPosition(_pTrun->GetEndPosition());
        _pFree->SetFreeSpace((uint32_t) _reservedSpace);
        _pFree->UpdateSize();
    }

	if (_streamingType == MSS || _streamingType == MSSINGEST) {
		if (_reservedSpace >= _pFree->MINIMUM) {
			_pMssTfxd->SetPosition(_pFree->GetEndPosition());
		}
		else {
			_pMssTfxd->SetPosition(_pTrun->GetEndPosition());
		}
		_pMssTfxd->UpdateAbsoluteTimestamp();
		_pMssTfxd->UpdateFragmentDuration();
    }
}

void Atom_traf::Initialize(bool writeDirect, File* pFile, uint64_t position) {

    // Initialize the common members
    Atom::Initialize(writeDirect, pFile, position);

    // Initialize TFHD
    _tfhdPos = _position + GetHeaderSize();
    _pTfhd = new Atom_tfhd;
    _pTfhd->Initialize(writeDirect, pFile, _tfhdPos);
    if (_streamingType == DASH) {
        _pTfhd->SetSampleDescriptionIndex(1);
        _pTfhd->SetDefaultSampleDuration(0);
        _pTfhd->SetDefaultSampleSize(0);
        _pTfdt = new Atom_tfdt;
        _pTfdt->Initialize(writeDirect, pFile, _tfhdPos + _pTfhd->GetSize());
    }

    // Initialize TRUN
    _firstTrunPos = _tfhdPos + _pTfhd->GetSize();
    if (_streamingType == DASH)
        _firstTrunPos += _pTfdt->GetSize();

    _pTrun = new Atom_trun(_streamingType);
    if (_streamingType == DASH)
        _pTrun->SetFirstSampleFlags(0);
    _pTrun->Initialize(writeDirect, pFile, _firstTrunPos);

    // Initialize FREE
    _pFree = new Atom_free;
    _pFree->Initialize(writeDirect, pFile,
            _firstTrunPos + _pTrun->GetSize());

	if (_streamingType == MSS || _streamingType == MSSINGEST) {
		_pMssTfxd = new Atom_MssTfxd;
		_pMssTfxd->Initialize(writeDirect, pFile,
			_firstTrunPos + _pTrun->GetSize() + _pFree->GetSize());
    }
}

void Atom_traf::SetTfhdBaseOffset(uint64_t offset) {

    _pTfhd->SetBaseDataOffset(offset);
}

void Atom_traf::SetTrunDataOffset(int32_t offset) {
    if (_pTrun == NULL) return;
    _pTrun->SetDataOffset(offset);
}

void Atom_traf::SetLastTS(double lastTS, bool isAudio) {
    if (_pTrun != NULL)
        _pTrun->SetLastTS(lastTS, isAudio);
}

uint64_t Atom_traf::GetTotalDuration(bool isAudio) {
    if (_pTrun != NULL)
        return _pTrun->GetTotalDuration(isAudio);
    return 0;
}

void Atom_traf::SetFragmentDecodeTime(uint64_t fragmentDecodeTime) {
    if (_pTfdt != NULL)
        _pTfdt->SetFragmentDecodeTime(fragmentDecodeTime);
}

void Atom_traf::SetTfxdAbsoluteTimestamp(uint64_t timeStamp) {
    if (NULL != _pMssTfxd)
        _pMssTfxd->SetAbsoluteTimestamp(timeStamp);
}

void Atom_traf::SetTfxdFragmentDuration(uint64_t duration) {
    if (NULL != _pMssTfxd)
        _pMssTfxd->SetFragmentDuration(duration);
}

uint64_t Atom_traf::GetTfxdOffset() {
	if (NULL != _pFree)
		return _pFree->GetEndPosition();
	return 0;
}

uint64_t Atom_traf::GetFreeOffset() {
	if (NULL != _pTrun)
		return _pTrun->GetEndPosition();
	return 0;
}

