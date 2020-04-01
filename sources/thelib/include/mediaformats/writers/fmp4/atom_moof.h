/*
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
 */

#ifndef _ATOM_MOOF_H
#define	_ATOM_MOOF_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_mfhd;
class Atom_traf;
class Atom_free;

class Atom_moof : public Atom {
    // Track type to be used on this atom (can be moved to main atom class)

    enum TRACK_TYPE {
        AUDIO,
        VIDEO,
        HINT
    };

public:
    /*
     * MOOF atom/box (see 2.12 of Adobe Flash Video File Format
     * Specification v10.1)
     */
    Atom_moof(HttpStreamingType streamingType = HDS);
    virtual ~Atom_moof();

    /**
     * Adds the track atom/box and sets its track ID which is contained in
     * the TFHD atom/box
     */
    void AddTrackFragment();

    /**
     * Adds a TRUN atom/box sample
     *
     * @param trackId Track ID of the TFHD atom/box containing this sample
     * @param startTime Timestamp of this sample
     * @param size Size of the sample
     * @param isKeyFrame Flag to indicate if the sample to be added is a keyframe
     * @param compositionTimeOffset Composition Time Offset
     * @param isAudio Indicates that the sample to be added is audio
     */
    void AddTrunSample(uint32_t trackId, double startTime, uint32_t size,
            bool isKeyFrame, uint32_t compositionTimeOffset, bool isAudio);

    /**
     * Sets the flag that determines if this atom (TRAF) uses multiple TRUN
     * atoms/boxes
     *
     * @param isMultiple Flag to indicate above stated behavior
     */
    void SetMultipleTrun(bool isMultiple);

    uint64_t GetTrunBoxSize();
    uint8_t GetTrunEntrySize();

    void Update();

    void Initialize(bool writeDirect = false, File* file = NULL,
            uint64_t position = 0, uint32_t mfhdSequence = 1);

    void ReserveTrafSpace(uint64_t space);

    void SetTfhdBaseOffset(uint64_t offset);
    void SetTrunDataOffset(int32_t offset);
    void SetLastTS(double lastTS, bool isAudio);
    uint64_t GetTotalDuration(bool isAudio);
    void SetTfdtFragmentDecodeTime(uint64_t fragmentDecodeTime);
    inline uint64_t GetTfdtFragmentDecodeTime() { return _tfdtFragmentDecodeTime; }
    void SetTfxdAbsoluteTimestamp(uint64_t timeStamp);
    void SetTfxdFragmentDuration(uint64_t duration);
    void SetTrackId(uint32_t trackId);
	uint64_t GetTfxdOffset();
	uint64_t GetFreeOffset();
private:
    bool WriteFields();
    uint64_t GetFieldsSize();

    // MFHD child atom/box of this atom/box
    Atom_mfhd* _pMfhd;

    // TRAF child atom/box of this atom/box
    Atom_traf* _pTraf;

    // In case of multiple TRAF atom/boxes
    // TODO: multiple TRAFs is not supported yet
    uint32_t _trafBoxes;

    // Positions
    uint64_t _mfhdPos;
    uint64_t _firstTrafPos;

    uint8_t _trackID;

    HttpStreamingType _streamingType;

    uint64_t _tfdtFragmentDecodeTime;
};
#endif	/* _ATOM_MOOF_H */
