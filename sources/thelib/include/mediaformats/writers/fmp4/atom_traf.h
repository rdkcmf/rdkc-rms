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


#ifndef _ATOM_TRAF_H
#define	_ATOM_TRAF_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_tfhd;
class Atom_trun;
class Atom_free;
class Atom_tfdt;
class Atom_MssTfxd;

class Atom_traf : public Atom {
public:
    /*
     * TRAF atom/box (see 2.12.2 of Adobe Flash Video File Format
     * Specification v10.1)
     */
    Atom_traf(HttpStreamingType streamingType);
    virtual ~Atom_traf();

    // Setter and getter for the track ID contained on the TFHD atom/box
    void SetTrackId(uint32_t trackId);
    uint32_t GetTrackId();

    /**
     * Adds the trun samples on the TRUN atom/box
     *
     * @param startTime Timestamp of the sample
     * @param size Size of the sample
     * @param isKeyFrame Indicates if frame as such
     * @param compositionTimeOffset Composition Time Offset
     * @param isAudio Indicates that the sample to be added is audio
     */
    void AddTrunSample(double startTime, uint32_t size, bool iskeyFrame,
            uint32_t compositionTimeOffset, bool isAudio);

    /**
     * Sets the flag that determines if this atom (TRAF) uses multiple TRUN
     * atoms/boxes
     *
     * @param isMultiple Flag to indicate above stated behavior
     */
    void SetMultipleTrun(bool isMultiple);

    /**
     * Returns the flag that determines the use of multiple TRUN atoms/boxes
     *
     * @return true if it is the case, false otherwise
     */
    bool GetMultipleTrunFlag();

    uint64_t GetTrunBoxSize();
    uint8_t GetTrunEntrySize();

    void Update();
    void Initialize(bool writeDirect = false, File* pFile = NULL,
            uint64_t position = 0);

    void SetTfhdBaseOffset(uint64_t offset);
    void SetTrunDataOffset(int32_t offset);
    void SetLastTS(double lastTS, bool isAudio);
    uint64_t GetTotalDuration(bool isAudio);
    void SetFragmentDecodeTime(uint64_t fragmentDecodeTime);
    void SetTfxdAbsoluteTimestamp(uint64_t timeStamp);
    void SetTfxdFragmentDuration(uint64_t duration);
	uint64_t GetTfxdOffset();
	uint64_t GetFreeOffset();
private:
    bool WriteFields();
    uint64_t GetFieldsSize();

    // TFHD child atom/box of this atom/box
    Atom_tfhd* _pTfhd;

    // TRUN child atom/box of this atom/box
    Atom_trun* _pTrun;

    // FREE child atom/box of this atom/box
    Atom_free* _pFree;

    Atom_tfdt* _pTfdt; //Use in MPEG-DASH

    Atom_MssTfxd* _pMssTfxd; //Use in MSS

    // In case of multiple TRUN atom/boxes
    uint32_t _trunBoxes;

    /*
     * This flag will indicate if this TRAF atom/box contains multiple instances
     * of TRUN atoms/boxes or just a single TRUN for all the samples
     *
     * TODO: multiple TRUNs not yet supported!
     */
    bool _useMultipleTrunBoxes;

    // Positions
    uint64_t _tfhdPos;
    uint64_t _firstTrunPos;
    HttpStreamingType _streamingType;
};
#endif	/* _ATOM_TRAF_H */
