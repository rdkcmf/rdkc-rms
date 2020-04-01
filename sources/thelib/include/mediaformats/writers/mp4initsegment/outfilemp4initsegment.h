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


#ifndef _OUTFILEMP4INITSEGMENT_H
#define _OUTFILEMP4INITSEGMENT_H
#define DASH_TIMESCALE (1000)
#define MSS_MDHD_TIMESCALE (10000000)
#define MSS_MVHD_TIMESCALE (1000)

#include "recording/mp4/outfilemp4base.h"

class DLLEXP OutFileMP4InitSegment
: private OutFileMP4Base {
public:
    OutFileMP4InitSegment(BaseProtocol *pProtocol, string name, Variant &settings);
    static OutFileMP4InitSegment* GetInstance(BaseClientApplication *pApplication,
            string name, Variant &settings);
    virtual ~OutFileMP4InitSegment();
    bool CreateInitSegment(BaseInStream* pInStream, bool isAudio);
    bool CreateMoovHeader(File* pFile, BaseInStream* pInStream, bool hasAudio, bool hasVideo, 
        Variant& streamNames, uint16_t& trakCount);

protected:
    virtual bool FinishInitialization(bool hasAudio, bool hasVideo);
    inline bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) { return true; }
    inline bool PushAudioData(IOBuffer &buffer, double pts, double dts) { return true; }

private:
    bool WriteFTYP(File *pFile);
    bool WriteMVEX(File *pFile, uint8_t trexCount);
    bool WriteTREX(File *pFile, uint8_t trackId);
    bool WriteMOOVAUDIO(File *pFile);
    bool WriteMOOVVIDEO(File *pFile);
    bool WriteSTSC(File *pFile, Track *track);
    bool OpenMP4();
    bool CloseMP4();
    uint64_t Duration(uint32_t timescale);

private:
    enum UsageMode {
        DASH = 1,
        MSS
    };

private:
    BaseProtocol *_pDummyProtocol;
    static uint8_t _ftyp[32];
    Track *_pAudio;
    Track *_pVideo;
    bool _withFtype;
    UsageMode _Mode;
};

#endif	/* _OUTFILEMP4INITSEGMENT_H */
