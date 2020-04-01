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

#include "mediaformats/writers/fmp4/atom_moof.h"
#include "mediaformats/writers/fmp4/atom_mfhd.h"
#include "mediaformats/writers/fmp4/atom_free.h"
#include "mediaformats/writers/fmp4/atom_traf.h"

Atom_moof::Atom_moof(HttpStreamingType streamingType) : Atom(0x6D6F6F66, false) {
    _pMfhd = NULL;
    _pTraf = NULL;
    _trafBoxes = 0;
    _mfhdPos = 0;
    _firstTrafPos = 0;
    _trackID = 1;
    _streamingType = streamingType;
    _tfdtFragmentDecodeTime = 0;
}

Atom_moof::~Atom_moof() {

    if (NULL != _pMfhd) {
        delete _pMfhd;
        _pMfhd = NULL;
    }

    if (NULL != _pTraf) {
        delete _pTraf;
        _pTraf = NULL;
    }
}

void Atom_moof::SetTfdtFragmentDecodeTime(uint64_t fragmentDecodeTime) {
    if (_pTraf != NULL)
        _pTraf->SetFragmentDecodeTime(fragmentDecodeTime);

    _tfdtFragmentDecodeTime = fragmentDecodeTime;
}

void Atom_moof::AddTrackFragment() {

    if (_trafBoxes > 0) {
        // Readjust the position if this is the second or more TRAF
        _pTraf->SetPosition(_firstTrafPos + (_pTraf->GetSize() * _trafBoxes));
    }

    _pTraf->SetTrackId(_trackID);

    _trackID++;
    _trafBoxes++;
}

void Atom_moof::AddTrunSample(uint32_t trackId, double startTime, uint32_t size,
        bool isKeyFrame, uint32_t compositionTimeOffset, bool isAudio) {

    // Adjust the position of TRAF that holds the sample to be added
    if (trackId > 1) {
        _pTraf->SetPosition(_firstTrafPos + (_pTraf->GetSize() * (trackId - 1)));
    }

    // Add this to the TRAF atom/box which has the matching track ID
    _pTraf->AddTrunSample(startTime, size, isKeyFrame, compositionTimeOffset, isAudio);
}

void Atom_moof::SetMultipleTrun(bool useMultipleTruns) {

    // Set the setting with the target track ID
    _pTraf->SetMultipleTrun(useMultipleTruns);
}

bool Atom_moof::WriteFields() {

    // Write MFHD
    if (!_pMfhd->Write()) {
        FATAL("Unable to write MFHD box/atom!");
        return false;
    }

    // Write TRAF
    if (!_pTraf->Write()) {
        FATAL("Unable to write TRAF box/atom!");
        return false;
    }

    return true;
}

uint64_t Atom_moof::GetFieldsSize() {
    /*
     * Compute the size of this atom.
     */
    uint64_t size = 0;

    // MFHD size
    size += _pMfhd->GetSize();

    // TRAF size
    size += _pTraf->GetSize() * _trafBoxes;

    return size;
}

uint64_t Atom_moof::GetTrunBoxSize() {

    return _pTraf->GetTrunBoxSize();
}

uint8_t Atom_moof::GetTrunEntrySize() {

    return _pTraf->GetTrunEntrySize();
}

void Atom_moof::Update() {

    _pTraf->Update();
    UpdateSize();
}

void Atom_moof::Initialize(bool writeDirect, File* pFile, uint64_t position, uint32_t mfhdSequence) {

    // Initialize the common members
    Atom::Initialize(writeDirect, pFile, position);

    // Initialize MFHD
    _mfhdPos = _position + GetHeaderSize();
    _pMfhd = new Atom_mfhd;
    _pMfhd->SetSequenceNumber(mfhdSequence);
    _pMfhd->Initialize(writeDirect, pFile, _mfhdPos);

    // Initialize TRAF
    _firstTrafPos = _mfhdPos + _pMfhd->GetSize();
    _pTraf = new Atom_traf(_streamingType);
    _pTraf->Initialize(writeDirect, pFile, _firstTrafPos);
}

void Atom_moof::ReserveTrafSpace(uint64_t space) {

    _pTraf->ReserveSpace(space);
}

void Atom_moof::SetTfhdBaseOffset(uint64_t offset) {

    _pTraf->SetTfhdBaseOffset(offset);
}

void Atom_moof::SetTrunDataOffset(int32_t offset) {
    if (_pTraf == NULL) return;
    _pTraf->SetTrunDataOffset(offset);
}

void Atom_moof::SetLastTS(double lastTS, bool isAudio) {
    if (_pTraf != NULL)
        _pTraf->SetLastTS(lastTS, isAudio);
}

uint64_t Atom_moof::GetTotalDuration(bool isAudio) {
    if (_pTraf != NULL)
        return _pTraf->GetTotalDuration(isAudio);
    return 0;
}

void Atom_moof::SetTfxdAbsoluteTimestamp(uint64_t timeStamp) {
    if (_pTraf != NULL)
        _pTraf->SetTfxdAbsoluteTimestamp(timeStamp);
}

void Atom_moof::SetTfxdFragmentDuration(uint64_t duration) {
    if (_pTraf != NULL)
        _pTraf->SetTfxdFragmentDuration(duration);
}

uint64_t Atom_moof::GetTfxdOffset() {
	if (_pTraf != NULL)
		return _pTraf->GetTfxdOffset();
	return 0;
}

uint64_t Atom_moof::GetFreeOffset() {
	if (_pTraf != NULL)
		return _pTraf->GetFreeOffset();
	return 0;
}

void Atom_moof::SetTrackId(uint32_t trackId) {
    if (NULL != _pTraf)
        _pTraf->SetTrackId(trackId);
}

