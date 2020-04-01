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

#include "mediaformats/writers/fmp4/atom_trun.h"

Atom_trun::Atom_trun(HttpStreamingType streamingType) : Atom(0x7472756E, 0, 0x0F01) {
    _sampleCount = 0;
    _dataOffset = 0;
    _streamingType = streamingType;
    _firstSampleFlagBits = 0x00000000;

    _afterLastTrunEntryPos = 0;
    _lastTrunAudioEntryPos = _afterLastTrunEntryPos;
    _lastTrunVideoEntryPos = _afterLastTrunEntryPos;
    _trunEntrySize = 0;

    _lastAudioTS = 0;
    _lastVideoTS = 0;
    _lastDuration = 0;

    _flagBits = 0;
    _totalAudioDuration = 0;
    _totalVideoDuration = 0;
}

Atom_trun::~Atom_trun() {

}

void Atom_trun::AddSample(double startTime, uint32_t size, bool isKeyFrame,
        uint32_t compositionTimeOffset, bool isAudio) {
    if (!_isDirectFileWrite) {
        //TODO, in case we want to support buffering
        ASSERT("Support for buffering needs to be implemented!");
    }

    /*
     * Set the default flagbit as non-key/non-sync sample and does depend
     * on others (not an I picture)
     *
     * See page 36 of Adobe Flash Video File Format Specification v10.1
     */
    _flagBits = 0x01010000;

    if (isKeyFrame) {
        // If this is a key frame, we change the flags accordingly
        _flagBits = 0x02000000;
    }

    EHTONLP(_trunSample, _lastDuration);
    EHTONLP(_trunSample + 4, size);
    EHTONLP(_trunSample + 8, _flagBits);
    EHTONLP(_trunSample + 12, compositionTimeOffset);

    _pFile->SeekTo(_afterLastTrunEntryPos);
    _pFile->WriteBuffer(_trunSample, 16);
    // Update the duration of the previous sample
    uint64_t previousTrunPos = isAudio ? _lastTrunAudioEntryPos : _lastTrunVideoEntryPos;
    double lastTS = (isAudio) ? _lastAudioTS : _lastVideoTS;
    _lastDuration = (uint32_t)startTime - (uint32_t)lastTS;

    // Update the duration
	if (_streamingType == MSSINGEST)
        WriteUI32(_lastDuration * MSSLI_ADJUST_TIMESCALE, previousTrunPos);
    else 
        WriteUI32(_lastDuration, previousTrunPos);

    // Update the member variables
    if (isAudio) {
        _lastTrunAudioEntryPos = _afterLastTrunEntryPos;
        _lastAudioTS = startTime;
        _totalAudioDuration += _lastDuration;
    } else {
        _lastTrunVideoEntryPos = _afterLastTrunEntryPos;
        _lastVideoTS = startTime;
        _totalVideoDuration += _lastDuration;
    }

    _afterLastTrunEntryPos += _trunEntrySize;
    _sampleCount++;
}

bool Atom_trun::WriteFields() {
    // Sample count
    _buffer.ReadFromRepeat(0, 4);
    EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
            _sampleCount);

    // data offset
    if (_flags & 0x0001) {
        _buffer.ReadFromRepeat(0, 4);
        EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
                _dataOffset);
    }

    // first sample flag
    if (_flags & 0x0004) {
        _buffer.ReadFromRepeat(0, 4);
        EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
                _firstSampleFlagBits);
    }

    return true;
}

uint64_t Atom_trun::GetFieldsSize() {
    /*
     * Compute the size of the this atom.
     */
    uint64_t size = 0;

    size += 4; // sampleCount

    if (_flags & 0x0001) {
        size += 4; // dataOffset
    }

    if (_flags & 0x0004) {
        size += 4; // firstSampleFlagBits
    }

    size += (((uint64_t) _trunEntrySize) * _sampleCount);

    return size;
}

uint8_t Atom_trun::GetEntrySize() {
    uint8_t entrySize = 0;

    if (_flags & 0x0100) {
        entrySize += 4; // duration
    }

    if (_flags & 0x0200) {
        entrySize += 4; // size
    }

    if (_flags & 0x0400) {
        entrySize += 4; // flagBits
    }

    if (_flags & 0x0800) {
        entrySize += 4; // compositionTimeOffset
    }

    return entrySize;
}

uint64_t Atom_trun::GetFirstEntryPosition() {
    uint64_t pos = _position;

    pos += GetHeaderSize();

    pos += 4; // sampleCount

    if (_flags & 0x0001) {
        pos += 4; // data offset
    }

    if (_flags & 0x0004) {
        pos += 4; // first sample flag
    }

    return pos;
}

void Atom_trun::Update() {
    UpdateSampleCount();
    UpdateSize();
}

bool Atom_trun::UpdateSampleCount() {
    if (_isDirectFileWrite) {
        if (!_pFile->SeekTo(_position + GetHeaderSize())) {
            FATAL("Could not seek to desired file position!");
            return false;
        }


        if (!_pFile->WriteUI32(_sampleCount)) {
            FATAL("Could not write updated sample count!");
            return false;
        }

        if (_streamingType == HDS) {
            // Update the first two samples of TRUN to set the duration to zero
            if (!_pFile->SeekTo(GetFirstEntryPosition())) {
                FATAL("Could not seek to desired file position!");
                return false;
            }

            if (!_pFile->WriteUI32(0)) {
                FATAL("Could not update duration!");
                return false;
            }

            if (!_pFile->SeekAhead(_trunEntrySize - 4)) {
                FATAL("Could not seek to desired file position!");
                return false;
            }

            if (!_pFile->WriteUI32(0)) {
                FATAL("Could not update duration!");
                return false;
            }
        }
    } else {
        ASSERT("Buffer support needs to be implemented!");
    }

    return true;
}

void Atom_trun::Initialize(bool writeDirect, File* pFile, uint64_t position) {

    // Initialize the common members
    Atom::Initialize(writeDirect, pFile, position);

    _afterLastTrunEntryPos = GetFirstEntryPosition();
    _trunEntrySize = GetEntrySize();

    // Initialize container of TRUN sample
    memset(_trunSample, 0, sizeof (_trunSample));

    _lastTrunAudioEntryPos = _afterLastTrunEntryPos;
    _lastTrunVideoEntryPos = _afterLastTrunEntryPos;
}
