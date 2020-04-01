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

//TODO: Refactor this to merge with recording/mp4/outfilemp4base to practice code reuse!

#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "streaming/codectypes.h"
#include "streaming/baseinstream.h"
#include "streaming/mp4/mp4streamwriter.h"
#include "application/baseappprotocolhandler.h"
#include "eventlogger/eventlogger.h"

Mp4StreamWriter::Mp4StreamWriter() :
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

Mp4StreamWriter::~Mp4StreamWriter() {
}

uint64_t Mp4StreamWriter::Duration(uint32_t timescale) {
    uint64_t audio = 0; //_chunkLength * timescale;//_pAudio->Duration(timescale);
    uint64_t video = 0; //_chunkLength * timescale;//_pVideo->Duration(timescale);
    uint64_t result = audio < video ? video : audio;
    if (_winQtCompat) {
        if (result >= 0x100000000LL)
            result = 0xffffffff;
    }

    return result;
}

bool Mp4StreamWriter::IsCodecSupported(uint64_t codec) {
    return (codec == CODEC_VIDEO_H264) || (codec == CODEC_AUDIO_AAC);
}

bool Mp4StreamWriter::UpdateSize(IOBufferExt &dst, uint32_t sizeOffset) {
	uint32_t size = GETAVAILABLEBYTESCOUNT(dst) - sizeOffset;
	uint8_t *pBuffer = GETIBPOINTER(dst) + sizeOffset;
	EHTONLP(pBuffer, size);
	
	//DEBUG("Offset: %"PRIu32", Size: %"PRIu32, sizeOffset, size);

	return true;
}

bool Mp4StreamWriter::WriteMVHD(IOBufferExt &dst) {
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

    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D766864LL, true)) //XX XX XX XX 'm' 'v' 'h' 'd'
            || (!dst.ReadFromU32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version and flags
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //modification_time
            || (!dst.ReadFromU32(_mvhdTimescale, true)) //timescale
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) Duration(_mvhdTimescale), true)) : (!dst.ReadFromU64(Duration(_mvhdTimescale), true))) //duration
            || (!dst.ReadFromU32(0x00010000, true)) //rate
            || (!dst.ReadFromU16(0x0100, true)) //volume
            || (!dst.ReadFromU16(0, true)) //reserved
            || (!dst.ReadFromU64(0, true)) //reserved
            || (!dst.ReadFromU64(0x0001000000000000LL, true)) //matrix
            || (!dst.ReadFromU64(0, true)) //matrix
            || (!dst.ReadFromU64(0x0001000000000000LL, true)) //matrix
            || (!dst.ReadFromU64(0, true)) //matrix
            || (!dst.ReadFromU32(0x40000000, true)) //matrix
            || (!dst.ReadFromU64(0, true)) //pre_defined
            || (!dst.ReadFromU64(0, true)) //pre_defined
            || (!dst.ReadFromU64(0, true)) //pre_defined
            || (!dst.ReadFromU32(_nextTrackId, true)) //next_track_ID
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write mvhd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteTRAK(IOBufferExt &dst, Track *track) {
    if (track->_id == 0)
        return true;
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000007472616BLL, true)) //XX XX XX XX 't' 'r' 'a' 'k'
            || (!WriteTKHD(dst, track)) //tkhd
            || (!WriteMDIA(dst, track)) //mdia
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write trak");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteTKHD(IOBufferExt &dst, Track *track) {
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
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x00000000746B6864LL, true)) //XX XX XX XX 't' 'k' 'h' 'd'
            || (!dst.ReadFromU32(_winQtCompat ? 0x0000000f : 0x0100000f, true)) //version 1 and flags 0x0f (enabled,inMovie,inPreview)
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //modification_time
            || (!dst.ReadFromU32(track->_id, true)) //track_ID
            || (!dst.ReadFromU32(0, true)) //reserved
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) Duration(_mvhdTimescale), true)) : (!dst.ReadFromU64(Duration(_mvhdTimescale), true))) //duration
            || (!dst.ReadFromU64(0, true)) //reserved
            || (!dst.ReadFromU16(0, true)) //layer
            || (!dst.ReadFromU16(!track->_isAudio, true)) //alternate_group
            || (!dst.ReadFromU16(track->_isAudio ? 0x0100 : 0, true)) //volume
            || (!dst.ReadFromU16(0, true)) //reserved
            || (!dst.ReadFromU64(0x0001000000000000LL, true)) //matrix
            || (!dst.ReadFromU64(0, true)) //matrix
            || (!dst.ReadFromU64(0x0001000000000000LL, true)) //matrix
            || (!dst.ReadFromU64(0, true)) //matrix
            || (!dst.ReadFromU32(0x40000000, true)) //matrix
            || (!dst.ReadFromU32((track->_width << 16), true)) //width
            || (!dst.ReadFromU32((track->_height << 16), true)) //height
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write tkhd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteMDIA(IOBufferExt &dst, Track *track) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D646961LL, true)) //XX XX XX XX 'm' 'd' 'i' 'a'
            || (!WriteMDHD(dst, track)) //mdhd
            || (!WriteHDLR(dst, track)) //hdlr
            || (!WriteMINF(dst, track)) //minf
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write mdia");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteMDHD(IOBufferExt &dst, Track *track) {
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
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D646864LL, true)) //XX XX XX XX 'm' 'd' 'h' 'd'
            || (!dst.ReadFromU32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version and flags
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //creation_time
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _creationTime, true)) : (!dst.ReadFromU64(_creationTime, true))) //modification_time
            || (!dst.ReadFromU32(track->_timescale, true)) //timescale
            || (_winQtCompat ? (!dst.ReadFromU32((uint32_t) Duration(track->_timescale), true)) : (!dst.ReadFromU64(Duration(track->_timescale), true))) //duration
            || (!dst.ReadFromU16(0x55C4, true)) //language which is 'und'
            || (!dst.ReadFromU16(0, true)) //pre_defined
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write mdhd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteHDLR(IOBufferExt &dst, Track *track) {
    /*
     * 			hdlr(vide)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		soun/vide
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000068646C72LL, true)) //XX XX XX XX 'h' 'd' 'l' 'r'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU32(0, true)) //pre_defined
            || (!dst.ReadFromU32(track->_isAudio ? 0x736F756E : 0x76696465, true)) //handler_type
            || (!dst.ReadFromU64(0, true)) //reserved
            || (!dst.ReadFromU32(0, true)) //reserved
            ) {
        FATAL("Unable to write hdlr");
        return false;
    }

	dst.ReadFromByte(0); //name

    return UpdateSize(dst, sizeOffset);
}

bool Mp4StreamWriter::WriteMINF(IOBufferExt &dst, Track *track) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D696E66LL, true)) //XX XX XX XX 'm' 'i' 'n' 'f'
            || ((track->_isAudio) ? (!WriteSMHD(dst, track)) : (!WriteVMHD(dst, track))) //smhd/vmhd
            || (!WriteDINF(dst, track)) //dinf
            || (!WriteSTBL(dst, track)) //stbl
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write minf");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSMHD(IOBufferExt &dst, Track *track) {
    /*
     * 				smhd (sound media header, overall information (sound track only))
     *					-version			0
     *					-flags				0
     *					-balance			0
     *					-reserved			0
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x00000000736D6864LL, true)) //XX XX XX XX 's' 'm' 'h' 'd'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU16(0, true)) //balance
            || (!dst.ReadFromU16(0, true)) //reserved
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write smhd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteVMHD(IOBufferExt &dst, Track *track) {
    /*
     * 				vmhd
     *					-version			0
     *					-flags				1
     *					-graphicsmode		0
     *					-opcolor			0
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x00000000766D6864LL, true)) //XX XX XX XX 'v' 'm' 'h' 'd'
            || (!dst.ReadFromU32(0x00000001, true)) //version and flags
            || (!dst.ReadFromU64(0, true)) //graphicsmode,opcolor
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write vmhd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteDINF(IOBufferExt &dst, Track *track) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000064696E66LL, true)) //XX XX XX XX 'd' 'i' 'n' 'f'
            || (!WriteDREF(dst, track)) //dref
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write dinf");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTBL(IOBufferExt &dst, Track *track) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000007374626CLL, true)) //XX XX XX XX 's' 't' 'b' 'l'
            || (!WriteSTSD(dst, track)) //stsd
            || (!WriteSTTS(dst, track)) //stts
            || ((track->_isAudio) ? false : (!WriteSTSS(dst, track)))
            || ((track->_isAudio) ? false : (!WriteCTTS(dst, track)))
            || (!WriteSTSC(dst, track))
            || (!WriteSTSZ(dst, track))
            || (!WriteCO64(dst, track))
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stbl");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteDREF(IOBufferExt &dst, Track *track) {
    /*
     * 					dref
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						url
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000064726566LL, true)) //XX XX XX XX 'd' 'r' 'e' 'f'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU32(1, true)) //entry_count;
            || (!WriteURL(dst, track)) //url
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write dref");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteURL(IOBufferExt &dst, Track *track) {
    /*
     * 						url
     *							-version			0
     *							-flags				1
     *							-location			NULL(0)
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000075726C20LL, true)) //XX XX XX XX 'u' 'r' 'l' ' '
            || (!dst.ReadFromU32(0x00000001, true)) //version and flags
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write url");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTSD(IOBufferExt &dst, Track *track) {
    /*
     * 					stsd
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						avc1/mp4a
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000073747364LL, true)) //XX XX XX XX 's' 't' 's' 'd'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU32(1, true)) //entry_count;
            || (track->_isAudio ? (!WriteMP4A(dst, track)) : (!WriteAVC1(dst, track))) //avc1/mp4a
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stsd");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTTS(IOBufferExt &dst, Track *track) {
    /*
     * 					stts (time-to-sample, and DTS must be used, not PTS)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_delta)		DTS delta expressed in mdia:timescale
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000073747473LL, true)) //XX XX XX XX 's' 't' 't' 's'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU32(track->_stts._entryCount, true)) //entry_count;
            || (!dst.ReadFromBuffer(track->_stts.Content(), track->_stts.ContentSize())) //the table
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stts");
        return false;
    }
    return true;
}

bool Mp4StreamWriter::WriteCTTS(IOBufferExt &dst, Track *track) {
    /*
     * 					ctts (compositionTime-to-sample)
     *						-version			0
     *						-flags				0
     *						sample_offset)		CTS expressed in mdia:timescale
     */
    uint32_t contentSize = track->_ctts.ContentSize();
    if (contentSize == 0)
        return true;
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000063747473LL, true)) //XX XX XX XX 'c' 't' 't' 's'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
			|| (!dst.ReadFromU32(track->_ctts._entryCount, true)) //entry_count;
            || (!dst.ReadFromBuffer(track->_ctts.Content(), contentSize)) //the table
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write ctts");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTSS(IOBufferExt &dst, Track *track) {
    /*
     * 					stss (sync sample table (random access points))
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						-sample_number		the ordinal number of the sample which is also a key frame
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000073747373LL, true)) //XX XX XX XX 's' 't' 's' 's'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
			|| (!dst.ReadFromU32(track->_stss._entryCount, true)) //entry_count;
            || (!dst.ReadFromBuffer(track->_stss.Content(), track->_stss.ContentSize())) //the table
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stss");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTSC(IOBufferExt &dst, Track *track) {
    /*
     * 					stsc (sample-to-chunk, partial data-offset information). We are going to have only one chunk per file
     *						-version					0
     *						-flags						0
     *						-entry_count				1
     *						-first_chunk				1
     *						-samples_per_chunk			1
     *						-sample_description_index	1
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000073747363LL, true)) //XX XX XX XX 's' 't' 's' 'c'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromU32(1, true)) //entry_count
            || (!dst.ReadFromU32(1, true)) //first_chunk
            || (!dst.ReadFromU32(1, true)) //samples_per_chunk
            || (!dst.ReadFromU32(1, true)) //sample_description_index
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stsc");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteSTSZ(IOBufferExt &dst, Track *track) {
    /*
     * 					stsz (sample sizes (framing))
     *						-version					0
     *						-flags						0
     *						-sample_size				0 (we ware going to always specify the sample sizes)
     *						-sample_count				total frames count in this track
     *						*entry_size					size of each frame
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000007374737ALL, true)) //XX XX XX XX 's' 't' 's' 'z'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
			|| (!dst.ReadFromU32(0x00000000, true)) //sample_size
			|| (!dst.ReadFromU32(track->_stsz._sampleCount, true))
            || (!dst.ReadFromBuffer(track->_stsz.Content(), track->_stsz.ContentSize())) //the table
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stsz");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteCO64(IOBufferExt &dst, Track *track) {
    /*
     * 					co64 (64-bit chunk offset, we are going to always use 64 bits, not stco)
     *						-version					0
     *						-flags						0
     *						-entry_count				1 (we are going to have only one chunk)
     *						-chunk_offset				where the first frame begins, regardless of the audio/video type
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    uint64_t boxHeader = track->_co64._is64 ? 0x00000000636F3634LL : 0x000000007374636FLL; //XX XX XX XX ('c' 'o' '6' '4' | 's' 't' 'c' 'o')
    if ((!dst.ReadFromU64(boxHeader, true))
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
			|| (!dst.ReadFromU32(track->_co64._sampleCount, true))
            || (!dst.ReadFromBuffer(track->_co64.Content(), track->_co64.ContentSize())) //the table
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write stsz");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteMP4A(IOBufferExt &dst, Track *track) {
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
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D703461LL, true)) //XX XX XX XX 'm' 'p' '4' 'a'
            || (!dst.ReadFromU32(0, true)) //reserved
            || (!dst.ReadFromU32(1, true)) //reserved, data_reference_index
            || (!dst.ReadFromU64(0, true)) //reserved
            || (!dst.ReadFromU16(2, true)) //channelcount
            || (!dst.ReadFromU16(16, true)) //samplesize
            || (!dst.ReadFromU16(0, true)) //pre_defined
            || (!dst.ReadFromU16(0, true)) //reserved
            || (!dst.ReadFromU32((track->_timescale << 16), true)) //samplerate
            || (!WriteESDS(dst, track)) //esds
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write mp4a");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteAVC1(IOBufferExt &dst, Track *track) {
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
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000061766331LL, true)) //XX XX XX XX 'a' 'v' 'c' '1'
            || (!dst.ReadFromU32(0, true)) //reserved
            || (!dst.ReadFromU32(1, true)) //reserved, data_reference_index
            || (!dst.ReadFromU64(0, true)) //pre_defined,reserved,pre_defined
            || (!dst.ReadFromU64(0, true)) //pre_defined,reserved,pre_defined
            || (!dst.ReadFromU16((uint16_t) track->_width, true)) //width
            || (!dst.ReadFromU16((uint16_t) track->_height, true)) //height
            || (!dst.ReadFromU32(0x00480000, true)) //horizresolution
            || (!dst.ReadFromU32(0x00480000, true)) //vertresolution
            || (!dst.ReadFromU32(0, true)) //reserved
            || (!dst.ReadFromU16(1, true)) //frame_count
            || (!dst.ReadFromU64(0, true)) //compressorname
            || (!dst.ReadFromU64(0, true)) //compressorname
            || (!dst.ReadFromU64(0, true)) //compressorname
            || (!dst.ReadFromU64(0, true)) //compressorname
            || (!dst.ReadFromU16(0x18, true)) //depth
            || (!dst.ReadFromU16(0xffff, true)) //pre_defined
            || (!WriteAVCC(dst, track)) //avcC
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write avc1");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteESDS(IOBufferExt &dst, Track *track) {
    /*
     * 							esds
     *								-version			0
     *								-flags				0
     *								-ES_Descriptor
     */
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000065736473LL, true)) //XX XX XX XX 'e' 's' 'd' 's'
            || (!dst.ReadFromU32(0x00000000, true)) //version and flags
            || (!dst.ReadFromBuffer(GETIBPOINTER(track->_esDescriptor), GETAVAILABLEBYTESCOUNT(track->_esDescriptor))) //ES_Descriptor
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write esds");
        return false;
    }

    return true;
}

bool Mp4StreamWriter::WriteAVCC(IOBufferExt &dst, Track *track) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x0000000061766343LL, true)) //XX XX XX XX 'a' 'v' 'c' 'C'
            || (!dst.ReadFromBuffer(GETIBPOINTER(track->_esDescriptor), GETAVAILABLEBYTESCOUNT(track->_esDescriptor))) //ES_Descriptor
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write avcC");
        return false;
    }

    return true;
}
