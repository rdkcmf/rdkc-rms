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

#ifndef _ATOM_TRUN_H
#define	_ATOM_TRUN_H

#include "mediaformats/writers/fmp4/atom.h"

/*
 * Structure of a TRUN atom/box sample (see 2.12.2.2 of Adobe Flash Video
 * File Format Specification v10.1)
 */

class Atom_trun : public Atom {
    // Sample Flags structure used by TRUN, TREX and TFHD atoms/boxes

    typedef struct _SampleFlags {
        //reserved
        uint8_t dependsOn;
        uint8_t dependedOn;
        uint8_t hasRedundancy;
        uint8_t paddingValue;
        bool nonKeySyncSample;
        //uint16_t degradationPriority; //reserved
    } SampleFlags;

public:
    /*
     * TRUN atom/box (see 2.12.2.2 of Adobe Flash Video File Format
     * Specification v10.1)
     */
    Atom_trun(HttpStreamingType streamingType);
    virtual ~Atom_trun();

    // Setters for the TRUN internal fields
    inline void SetSampleCount(uint32_t sampleCount) { _sampleCount = sampleCount; }
    inline void SetDataOffset(int32_t dataOffset) { 
        _flags |= 0x0001;
        _dataOffset = dataOffset; 
    }
    inline void SetFirstSampleFlags(uint32_t sampleFlagBits) { 
        _flags |= 0x0004;
        _firstSampleFlagBits = sampleFlagBits; 
    }
    inline uint32_t GetSampleCount() { return _sampleCount; }

    /**
     * Adds a TRUN sample for each sample returned by PushVideo/PushAudio functions
     *
     * @param startTime Timestamp of this sample
     * @param size Size of the sample
     * @param isKeyFrame Indicates if the frame as such
     * @param compositionTimeOffset Composition Time Offset
     * @param isAudio Indicates that the sample to be added is audio
     */
    void AddSample(double startTime, uint32_t size, bool isKeyFrame,
            uint32_t compositionTimeOffset, bool isAudio);

    uint8_t GetEntrySize();

    void Update();
    bool UpdateSampleCount();
    void Initialize(bool writeDirect = false, File* pFile = NULL,
            uint64_t position = 0);
    inline void SetLastTS(double lastTS, bool isAudio) {
        if (isAudio)
            _lastAudioTS = lastTS;
        else
            _lastVideoTS = lastTS;
    }
    inline uint64_t GetTotalDuration(bool isAudio) {
        if (isAudio)
            return _totalAudioDuration;
        else
            return _totalVideoDuration;
    }
private:
    bool WriteFields();
    uint64_t GetFieldsSize();
    uint64_t GetFirstEntryPosition();

    uint32_t _sampleCount;
    int32_t _dataOffset;
    HttpStreamingType _streamingType;

    /*
     * Note that when this was implemented, _firstSampleFlags and its
     * corresponding uint32_t format is not being used.
     */
    //SampleFlags _firstSampleFlags; //not used, reported by compiler
    uint32_t _firstSampleFlagBits;

    uint64_t _afterLastTrunEntryPos;
    uint64_t _lastTrunAudioEntryPos;
    uint64_t _lastTrunVideoEntryPos;
    uint8_t _trunEntrySize;

    double _lastAudioTS;
    double _lastVideoTS;
    uint32_t _lastDuration;

    uint8_t _trunSample[16];

    uint32_t _flagBits;
    uint64_t _totalAudioDuration;
    uint64_t _totalVideoDuration;
};
#endif	/* _ATOM_TRUN_H */
