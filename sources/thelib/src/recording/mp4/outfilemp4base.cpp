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


#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "streaming/codectypes.h"
#include "streaming/baseinstream.h"
#include "recording/mp4/outfilemp4base.h"
#include "application/baseappprotocolhandler.h"
#include "eventlogger/eventlogger.h"

OutFileMP4Base::OutFileMP4Base(BaseProtocol *pProtocol, string name, Variant &settings)
: BaseOutRecording(pProtocol, ST_OUT_FILE_MP4, name, settings),
_pMdatFile(NULL),
_mdatDataSize(0),
_mdatDataSizeOffset(0),
_nextTrackId(1),
_infoWriteBegin(0),
_winQtCompat(true),
_chunkLength(0),
_onlyAudio(false),
_mvhdTimescale(1000) {
    _creationTime = (uint64_t) time(NULL) + (uint64_t) 0x7C25B080; // 1970 based -> 1904 based
}

OutFileMP4Base::~OutFileMP4Base() {
}

uint64_t OutFileMP4Base::Duration(uint32_t timescale) {
    uint64_t audio = 0; //_chunkLength * timescale;//_pAudio->Duration(timescale);
    uint64_t video = 0; //_chunkLength * timescale;//_pVideo->Duration(timescale);
    uint64_t result = audio < video ? video : audio;
    if (_winQtCompat) {
        if (result >= 0x100000000LL)
            result = 0xffffffff;
    }

    return result;
}

bool OutFileMP4Base::IsCodecSupported(uint64_t codec) {
    return (codec == CODEC_VIDEO_H264) || (codec == CODEC_AUDIO_AAC);
}

bool OutFileMP4Base::UpdateSize(File *pFile, uint64_t sizeOffset) {
    uint64_t cursor = pFile->Cursor();
    uint32_t size = (uint32_t) (cursor - sizeOffset);
    if ((!pFile->SeekTo(sizeOffset))
            || (!pFile->WriteUI32(size, true))
            || (!pFile->SeekTo(cursor))
            ) {
        FATAL("Unable to update size");
        return false;
    }
    return true;
}

bool OutFileMP4Base::WriteMVHD(File *pFile) {
    /*
     * 	mvhd
     *		-version			1
     *		-flags				0
     *		-creation_time		time(NULL)
     *		-modification_time	creation_time
     *		-timescale			1000
     *		-duration			max value from all tkhd:duration in mvhd:timescale
     *		-rate				0x00010000
     *		-volume				0x0100
     *		-reserved			0
     *		-reserved			0
     *		-matrix				0x0001000000000000LL,0x0000000000000000LL,0x0001000000000000,0x0000000000000000LL,0x40000000
     *		-pre_defined		0
     *		-next_track_ID		max(trak:track_ID)+1
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D766864LL, true)) //XX XX XX XX 'm' 'v' 'h' 'd'
            || (!pFile->WriteUI32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version and flags
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //modification_time
            || (!pFile->WriteUI32(_mvhdTimescale, true)) //timescale
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) Duration(_mvhdTimescale), true)) : (!pFile->WriteUI64(Duration(_mvhdTimescale), true))) //duration
            || (!pFile->WriteUI32(0x00010000, true)) //rate
            || (!pFile->WriteUI16(0x0100, true)) //volume
            || (!pFile->WriteUI16(0, true)) //reserved
            || (!pFile->WriteUI64(0, true)) //reserved
            || (!pFile->WriteUI64(0x0001000000000000LL, true)) //matrix
            || (!pFile->WriteUI64(0, true)) //matrix
            || (!pFile->WriteUI64(0x0001000000000000LL, true)) //matrix
            || (!pFile->WriteUI64(0, true)) //matrix
            || (!pFile->WriteUI32(0x40000000, true)) //matrix
            || (!pFile->WriteUI64(0, true)) //pre_defined
            || (!pFile->WriteUI64(0, true)) //pre_defined
            || (!pFile->WriteUI64(0, true)) //pre_defined
            || (!pFile->WriteUI32(_nextTrackId, true)) //next_track_ID
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write mvhd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteTRAK(File *pFile, Track *track) {
    if (track->_id == 0)
        return true;
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000007472616BLL, true)) //XX XX XX XX 't' 'r' 'a' 'k'
            || (!WriteTKHD(pFile, track)) //tkhd
            || (!WriteMDIA(pFile, track)) //mdia
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write trak");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteTKHD(File *pFile, Track *track) {
    /*
     * 		tkhd
     *			-version			1
     *			-flags				0x0f
     *			-creation_time		mvhd:creation_time
     *			-modification_time	mvhd:creation_time
     *			-track_ID			incremental number starting from 1
     *			-reserved			0
     *			-duration			duration in mvhd:timescale
     *			-reserved			0
     *			-layer				0 (this is the Z order)
     *			-alternate_group	a unique number for each track. This is how we can group multiple tracks on the same logical track (audio in multiple languages for example)
     *			-volume				if track is audio 0x0100 else 0
     *			-reserved			0
     *			-matrix				0x00010000,0,0,0,0x00010000,0,0,0,0x40000000
     *			-width				0 for non-video otherwise see libavformat/movenc.c
     *			-height				0 for non-video otherwise see libavformat/movenc.c
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x00000000746B6864LL, true)) //XX XX XX XX 't' 'k' 'h' 'd'
            || (!pFile->WriteUI32(_winQtCompat ? 0x0000000f : 0x0100000f, true)) //version 1 and flags 0x0f (enabled,inMovie,inPreview)
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //modification_time
            || (!pFile->WriteUI32(track->_id, true)) //track_ID
            || (!pFile->WriteUI32(0, true)) //reserved
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) Duration(_mvhdTimescale), true)) : (!pFile->WriteUI64(Duration(_mvhdTimescale), true))) //duration
            || (!pFile->WriteUI64(0, true)) //reserved
            || (!pFile->WriteUI16(0, true)) //layer
            || (!pFile->WriteUI16(!track->_isAudio, true)) //alternate_group
            || (!pFile->WriteUI16(track->_isAudio ? 0x0100 : 0, true)) //volume
            || (!pFile->WriteUI16(0, true)) //reserved
            || (!pFile->WriteUI64(0x0001000000000000LL, true)) //matrix
            || (!pFile->WriteUI64(0, true)) //matrix
            || (!pFile->WriteUI64(0x0001000000000000LL, true)) //matrix
            || (!pFile->WriteUI64(0, true)) //matrix
            || (!pFile->WriteUI32(0x40000000, true)) //matrix
            || (!pFile->WriteUI32((track->_width << 16), true)) //width
            || (!pFile->WriteUI32((track->_height << 16), true)) //height
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write tkhd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteMDIA(File *pFile, Track *track) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D646961LL, true)) //XX XX XX XX 'm' 'd' 'i' 'a'
            || (!WriteMDHD(pFile, track)) //mdhd
            || (!WriteHDLR(pFile, track)) //hdlr
            || (!WriteMINF(pFile, track)) //minf
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write mdia");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteMDHD(File *pFile, Track *track) {
    /*
     * 			mdhd
     *				-version			1
     *				-flags				0
     *				-creation_time		mvhd:creation_time
     *				-modification_time	mvhd:creation_time
     *				-timescale			1000 for video and audio rate for audio.
     *				-duration			duration in mdhd:timescale
     *				-language			default it to 'und'
     *				-pre_defined		0
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D646864LL, true)) //XX XX XX XX 'm' 'd' 'h' 'd'
            || (!pFile->WriteUI32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version and flags
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) _creationTime, true)) : (!pFile->WriteUI64(_creationTime, true))) //modification_time
            || (!pFile->WriteUI32(track->_timescale, true)) //timescale
            || (_winQtCompat ? (!pFile->WriteUI32((uint32_t) Duration(track->_timescale), true)) : (!pFile->WriteUI64(Duration(track->_timescale), true))) //duration
            || (!pFile->WriteUI16(0x55C4, true)) //language which is 'und'
            || (!pFile->WriteUI16(0, true)) //pre_defined
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write mdhd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteHDLR(File *pFile, Track *track) {
    /*
     * 			hdlr(vide)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		soun/vide
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000068646C72LL, true)) //XX XX XX XX 'h' 'd' 'l' 'r'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI32(0, true)) //pre_defined
            || (!pFile->WriteUI32(track->_isAudio ? 0x736F756E : 0x76696465, true)) //handler_type
            || (!pFile->WriteUI64(0, true)) //reserved
            || (!pFile->WriteUI32(0, true)) //reserved
            || (!pFile->WriteUI8(0)) //name
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write hdlr");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteMINF(File *pFile, Track *track) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D696E66LL, true)) //XX XX XX XX 'm' 'i' 'n' 'f'
            || ((track->_isAudio) ? (!WriteSMHD(pFile, track)) : (!WriteVMHD(pFile, track))) //smhd/vmhd
            || (!WriteDINF(pFile, track)) //dinf
            || (!WriteSTBL(pFile, track)) //stbl
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write minf");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSMHD(File *pFile, Track *track) {
    /*
     * 				smhd (sound media header, overall information (sound track only))
     *					-version			0
     *					-flags				0
     *					-balance			0
     *					-reserved			0
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x00000000736D6864LL, true)) //XX XX XX XX 's' 'm' 'h' 'd'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI16(0, true)) //balance
            || (!pFile->WriteUI16(0, true)) //reserved
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write smhd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteVMHD(File *pFile, Track *track) {
    /*
     * 				vmhd
     *					-version			0
     *					-flags				1
     *					-graphicsmode		0
     *					-opcolor			0
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x00000000766D6864LL, true)) //XX XX XX XX 'v' 'm' 'h' 'd'
            || (!pFile->WriteUI32(0x00000001, true)) //version and flags
            || (!pFile->WriteUI64(0, true)) //graphicsmode,opcolor
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write vmhd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteDINF(File *pFile, Track *track) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000064696E66LL, true)) //XX XX XX XX 'd' 'i' 'n' 'f'
            || (!WriteDREF(pFile, track)) //dref
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write dinf");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTBL(File *pFile, Track *track) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000007374626CLL, true)) //XX XX XX XX 's' 't' 'b' 'l'
            || (!WriteSTSD(pFile, track)) //stsd
            || (!WriteSTTS(pFile, track)) //stts
            || ((track->_isAudio) ? false : (!WriteSTSS(pFile, track)))
            || ((track->_isAudio) ? false : (!WriteCTTS(pFile, track)))
            || (!WriteSTSC(pFile, track))
            || (!WriteSTSZ(pFile, track))
            || (!WriteCO64(pFile, track))
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stbl");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteDREF(File *pFile, Track *track) {
    /*
     * 					dref
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						url
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000064726566LL, true)) //XX XX XX XX 'd' 'r' 'e' 'f'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI32(1, true)) //entry_count;
            || (!WriteURL(pFile, track)) //url
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write dref");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteURL(File *pFile, Track *track) {
    /*
     * 						url
     *							-version			0
     *							-flags				1
     *							-location			NULL(0)
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000075726C20LL, true)) //XX XX XX XX 'u' 'r' 'l' ' '
            || (!pFile->WriteUI32(0x00000001, true)) //version and flags
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write url");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTSD(File *pFile, Track *track) {
    /*
     * 					stsd
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						avc1/mp4a
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000073747364LL, true)) //XX XX XX XX 's' 't' 's' 'd'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI32(1, true)) //entry_count;
            || (track->_isAudio ? (!WriteMP4A(pFile, track)) : (!WriteAVC1(pFile, track))) //avc1/mp4a
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stsd");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTTS(File *pFile, Track *track) {
    /*
     * 					stts (time-to-sample, and DTS must be used, not PTS)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_delta)		DTS delta expressed in mdia:timescale
     */
#ifdef DEBUG_MP4
    track->_stts.Finish();
#endif
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000073747473LL, true)) //XX XX XX XX 's' 't' 't' 's'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI32(track->_stts._entryCount, true)) //entry_count;
            || (!pFile->WriteBuffer(track->_stts.Content(), track->_stts.ContentSize())) //the table
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stts");
        return false;
    }
    return true;
}

bool OutFileMP4Base::WriteCTTS(File *pFile, Track *track) {
    /*
     * 					ctts (compositionTime-to-sample)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_offset)		CTS expressed in mdia:timescale
     */
#ifdef DEBUG_MP4
    track->_ctts.Finish();
#endif
    uint32_t contentSize = track->_ctts.ContentSize();
    if (contentSize == 0)
        return true;
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000063747473LL, true)) //XX XX XX XX 'c' 't' 't' 's'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteBuffer(track->_ctts.Content(), contentSize)) //the table
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write ctts");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTSS(File *pFile, Track *track) {
    /*
     * 					stss (sync sample table (random access points))
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						-sample_number		the ordinal number of the sample which is also a key frame
     */
#ifdef DEBUG_MP4
    track->_stss.Finish();
#endif
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000073747373LL, true)) //XX XX XX XX 's' 't' 's' 's'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteBuffer(track->_stss.Content(), track->_stss.ContentSize())) //the table
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stss");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTSC(File *pFile, Track *track) {
    /*
     * 					stsc (sample-to-chunk, partial data-offset information). We are going to have only one chunk per file
     *						-version					0
     *						-flags						0
     *						-entry_count				1
     *						-first_chunk				1
     *						-samples_per_chunk			1
     *						-sample_description_index	1
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000073747363LL, true)) //XX XX XX XX 's' 't' 's' 'c'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteUI32(1, true)) //entry_count
            || (!pFile->WriteUI32(1, true)) //first_chunk
            || (!pFile->WriteUI32(1, true)) //samples_per_chunk
            || (!pFile->WriteUI32(1, true)) //sample_description_index
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stsc");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteSTSZ(File *pFile, Track *track) {
    /*
     * 					stsz (sample sizes (framing))
     *						-version					0
     *						-flags						0
     *						-sample_size				0 (we ware going to always specify the sample sizes)
     *						-sample_count				total frames count in this track
     *						*entry_size					size of each frame
     */
#ifdef DEBUG_MP4
    track->_stsz.Finish();
#endif
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000007374737ALL, true)) //XX XX XX XX 's' 't' 's' 'z'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteBuffer(track->_stsz.Content(), track->_stsz.ContentSize())) //the table
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stsz");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteCO64(File *pFile, Track *track) {
    /*
     * 					co64 (64-bit chunk offset, we are going to always use 64 bits, not stco)
     *						-version					0
     *						-flags						0
     *						-entry_count				1 (we are going to have only one chunk)
     *						-chunk_offset				where the first frame begins, regardless of the audio/video type
     */
#ifdef DEBUG_MP4
    track->_co64.Finish();
#endif
    uint64_t sizeOffset = pFile->Cursor();
    uint64_t boxHeader = track->_co64._is64 ? 0x00000000636F3634LL : 0x000000007374636FLL; //XX XX XX XX ('c' 'o' '6' '4' | 's' 't' 'c' 'o')
    if ((!pFile->WriteUI64(boxHeader, true))
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteBuffer(track->_co64.Content(), track->_co64.ContentSize())) //the table
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stsz");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteMP4A(File *pFile, Track *track) {
    /*
     * 						mp4a
     *							-reserved				0
     *							-data_reference_index	1
     *							-reserved				0
     *							-channelcount			2
     *							-samplesize				16
     *							-pre_defined			0
     *							-reserved				0
     *							-samplerate				(audio_sample_rate)<<16
     * 							esds
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D703461LL, true)) //XX XX XX XX 'm' 'p' '4' 'a'
            || (!pFile->WriteUI32(0, true)) //reserved
            || (!pFile->WriteUI32(1, true)) //reserved, data_reference_index
            || (!pFile->WriteUI64(0, true)) //reserved
            || (!pFile->WriteUI16(2, true)) //channelcount
            || (!pFile->WriteUI16(16, true)) //samplesize
            || (!pFile->WriteUI16(0, true)) //pre_defined
            || (!pFile->WriteUI16(0, true)) //reserved
            || (!pFile->WriteUI32((track->_timescale << 16), true)) //samplerate
            || (!WriteESDS(pFile, track)) //esds
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write mp4a");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteAVC1(File *pFile, Track *track) {
    /*
     * 						avc1
     *							-reserved				0
     *							-data_reference_index	1
     *							-pre_defined			0
     *							-reserved				0
     *							-pre_defined			0
     *							-width					width of video
     *							-height					height of video
     *							-horizresolution		0x00480000 (72DPI)
     *							-vertresolution			0x00480000 (72DPI)
     *							-reserved				0
     *							-frame_count			1 (how many frames per sample)
     *							-compressorname			0x16, 'E','v','o',	'S','t','r','e','a','m',' ','M','e','d','i','a',' ','S','e','r','v','e','r',0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
     *							-depth					0x18
     *							-pre_defined			0xffff
     *							avcC
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000061766331LL, true)) //XX XX XX XX 'a' 'v' 'c' '1'
            || (!pFile->WriteUI32(0, true)) //reserved
            || (!pFile->WriteUI32(1, true)) //reserved, data_reference_index
            || (!pFile->WriteUI64(0, true)) //pre_defined,reserved,pre_defined
            || (!pFile->WriteUI64(0, true)) //pre_defined,reserved,pre_defined
            || (!pFile->WriteUI16((uint16_t) track->_width, true)) //width
            || (!pFile->WriteUI16((uint16_t) track->_height, true)) //height
            || (!pFile->WriteUI32(0x00480000, true)) //horizresolution
            || (!pFile->WriteUI32(0x00480000, true)) //vertresolution
            || (!pFile->WriteUI32(0, true)) //reserved
            || (!pFile->WriteUI16(1, true)) //frame_count
            || (!pFile->WriteUI64(0, true)) //compressorname
            || (!pFile->WriteUI64(0, true)) //compressorname
            || (!pFile->WriteUI64(0, true)) //compressorname
            || (!pFile->WriteUI64(0, true)) //compressorname
            || (!pFile->WriteUI16(0x18, true)) //depth
            || (!pFile->WriteUI16(0xffff, true)) //pre_defined
            || (!WriteAVCC(pFile, track)) //avcC
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write avc1");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteESDS(File *pFile, Track *track) {
    /*
     * 							esds
     *								-version			0
     *								-flags				0
     *								-ES_Descriptor
     */
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000065736473LL, true)) //XX XX XX XX 'e' 's' 'd' 's'
            || (!pFile->WriteUI32(0x00000000, true)) //version and flags
            || (!pFile->WriteBuffer(GETIBPOINTER(track->_esDescriptor), GETAVAILABLEBYTESCOUNT(track->_esDescriptor))) //ES_Descriptor
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write esds");
        return false;
    }

    return true;
}

bool OutFileMP4Base::WriteAVCC(File *pFile, Track *track) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x0000000061766343LL, true)) //XX XX XX XX 'a' 'v' 'c' 'C'
            || (!pFile->WriteBuffer(GETIBPOINTER(track->_esDescriptor), GETAVAILABLEBYTESCOUNT(track->_esDescriptor))) //ES_Descriptor
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write avcC");
        return false;
    }

    return true;
}
