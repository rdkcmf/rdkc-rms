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



#include "protocols/passthrough/passthroughprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "streaming/codectypes.h"
#include "streaming/baseinstream.h"
#include "mediaformats/writers/mp4initsegment/outfilemp4initsegment.h"
#include "application/baseappprotocolhandler.h"
#include "eventlogger/eventlogger.h"
#include "streaming/streamstypes.h"

#define DEFAULT_TRAK_ID	1

uint8_t OutFileMP4InitSegment::_ftyp[32] = { //This ftyp is used in MPEG-DASH
    0x00, 0x00, 0x00, 0x20, //size: 32
    0x66, 0x74, 0x79, 0x70, //type: ftyp
    0x69, 0x73, 0x6F, 0x6D, //major_brand: isom
    0x00, 0x00, 0x00, 0x00, //minor_version: 0
    0x69, 0x73, 0x6F, 0x6D, //compatible_brands[0]: isom
    0x6D, 0x70, 0x34, 0x32, //compatible_brands[1]: mp42
    0x61, 0x76, 0x63, 0x31, //compatible_brands[2]: avc1
    0x64, 0x61, 0x73, 0x68 //compatible_brands[3]: dash

};

OutFileMP4InitSegment::OutFileMP4InitSegment(BaseProtocol *pProtocol, string name, Variant &settings)
: OutFileMP4Base(pProtocol, name, settings),
_pDummyProtocol(pProtocol),
_pAudio(NULL),
_pVideo(NULL),
_withFtype(true) {
    if (_Mode == MSS) 
        _mvhdTimescale = MSS_MVHD_TIMESCALE;
}

OutFileMP4InitSegment* OutFileMP4InitSegment::GetInstance(BaseClientApplication *pApplication,
        string name, Variant &settings) {
    //1. Create a dummy protocol
    PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

    //2. create the parameters
    Variant parameters;
    parameters["customParameters"]["recordConfig"] = settings;

    //3. Initialize the protocol
    if (!pDummyProtocol->Initialize(parameters)) {
        FATAL("Unable to initialize passthrough protocol");
        pDummyProtocol->EnqueueForDelete();
        return NULL;
    }

    //4. Set the application
    if (pApplication) {
        pDummyProtocol->SetApplication(pApplication);
    }

    //5. Create the MP4 stream
    OutFileMP4InitSegment *pOutFileMP4 = new OutFileMP4InitSegment(pDummyProtocol, name, settings);
    if (!pOutFileMP4->SetStreamsManager(pApplication->GetStreamsManager())) {
        FATAL("Unable to set the streams manager");
        delete pOutFileMP4;
        pOutFileMP4 = NULL;
        return NULL;
    }
    pDummyProtocol->SetDummyStream(pOutFileMP4);

    //6. Done
    return pOutFileMP4;
}

OutFileMP4InitSegment::~OutFileMP4InitSegment() {
    ((PassThroughProtocol*) _pDummyProtocol)->SetDummyStream(NULL);
}

bool OutFileMP4InitSegment::OpenMP4() {
    // Close any existing files
    if (_withFtype && !CloseMP4()) {
        string error = "Could not close MP4 file!";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    _filePath = (string) _settings["computedPathToFile"];

    if (_withFtype) {
        _pMdatFile = new File();
        if (!_pMdatFile->Initialize(_filePath, FILE_OPEN_MODE_TRUNCATE)) {
            string error = "Unable to open file " + _filePath;
            FATAL("%s", STR(error));
			_chunkInfo["errorMsg"] = error;
            GetEventLogger()->LogRecordChunkError(_chunkInfo);
            return false;
        }
    }

    if (_withFtype && !WriteFTYP(_pMdatFile)) {
        FATAL("Unable to write FTYP");
        return false;
    }

    // Reset _mdatDataSize
    _withFtype ? _mdatDataSizeOffset = _pMdatFile->Size() + 8 : _mdatDataSizeOffset = 0;
    _mdatDataSize = 0;

    if (_permanentData._audioId > 0) {
        _pAudio = new Track();
        _pAudio->_id = _permanentData._audioId;
        _pAudio->_width = 0;
        _pAudio->_height = 0;
        if (_Mode == DASH) 
            _pAudio->_timescale = DASH_TIMESCALE/*_permanentData._audioTimescale*/;
        else if (_Mode == MSS)
            _pAudio->_timescale = MSS_MDHD_TIMESCALE;

        _pAudio->_esDescriptor.ReadFromInputBuffer(_permanentData._audioEsDescriptor,
                GETAVAILABLEBYTESCOUNT(_permanentData._audioEsDescriptor));
        _pAudio->_stts._timescale = _pAudio->_timescale;
        _pAudio->_ctts._timescale = _pAudio->_timescale;

        // Initialize the audio track
        if (!_pAudio->Initialize(true)) {
            FATAL("Could not initialize audio track!");
            return false;
        }
    }

    if (_permanentData._videoId > 0) {
        _pVideo = new Track();
        _pVideo->_id = _permanentData._videoId;
        _pVideo->_width = _permanentData._videoWidth;
        _pVideo->_height = _permanentData._videoHeight;
        if (_Mode == DASH)
            _pVideo->_timescale = DASH_TIMESCALE/*_permanentData._videoTimescale*/;
        else if (_Mode == MSS)
            _pVideo->_timescale = MSS_MDHD_TIMESCALE;

        _pVideo->_esDescriptor.ReadFromInputBuffer(_permanentData._videoEsDescriptor,
                GETAVAILABLEBYTESCOUNT(_permanentData._videoEsDescriptor));
        _pVideo->_stts._timescale = _pVideo->_timescale;
        _pVideo->_ctts._timescale = _pVideo->_timescale;

        // Initialize the video track
        if (!_pVideo->Initialize(false)) {
            FATAL("Could not initialize video track!");
            return false;
        }
    }

    return true;
}

bool OutFileMP4InitSegment::CloseMP4() {
    // Nothing to close
    if ((_pAudio == NULL) && (_pVideo == NULL)) return true;

    if (_pMdatFile != NULL) {
        // Nothing to do here if NOT normal operation

        if (_pVideo != NULL) {
            _pVideo->Finalize();
            _pVideo->Unfinalize();
        }
        if (_pAudio != NULL) {
            _pAudio->Finalize();
            _pAudio->Unfinalize();
        }

        _pMdatFile->SeekEnd();
        _pMdatFile->Close();
        delete _pMdatFile;
        _pMdatFile = NULL;
    }

    if (_pAudio != NULL) {
        delete _pAudio;
        _pAudio = NULL;
    }

    if (_pVideo != NULL) {
        delete _pVideo;
        _pVideo = NULL;
    }

    return true;
}

bool OutFileMP4InitSegment::FinishInitialization(bool hasAudio, bool hasVideo) {
    if (!BaseOutStream::FinishInitialization(NULL)) {
        FATAL("Unable to finish output stream initialization");
        return false;
    }

    if (hasVideo) {
         if (_Mode == DASH)
            _permanentData._videoId = DEFAULT_TRAK_ID;
        else if(_Mode == MSS)
             _permanentData._videoId = _nextTrackId;

        _nextTrackId++;
        switch (GetInStream()->GetCapabilities()->GetVideoCodecType()) {
            case CODEC_VIDEO_H264:
            {
                VideoCodecInfoH264 *pTemp = GetInStream()->GetCapabilities()->GetVideoCodec<VideoCodecInfoH264>();
                if ((pTemp == NULL)
                        || (pTemp->_width == 0)
                        || (pTemp->_height == 0)) {
                    FATAL("Invalid video codec");
                    return false;
                }
                _permanentData._videoWidth = pTemp->_width;
                _permanentData._videoHeight = pTemp->_height;
                IOBuffer &raw = pTemp->GetRTMPRepresentation();
                _permanentData._videoEsDescriptor.ReadFromBuffer(GETIBPOINTER(raw) + 5,
                        GETAVAILABLEBYTESCOUNT(raw) - 5);
                _permanentData._videoTimescale = GetInStream()->GetInputVideoTimescale();
                break;
            }
            default:
            {
                FATAL("Video codec not supported");
                return false;
            }
        }
    } else {
        if (hasAudio)
            _onlyAudio = true;
    }

    if (hasAudio) {
        if (_Mode == DASH)
            _permanentData._audioId = DEFAULT_TRAK_ID;
        else if(_Mode == MSS)
            _permanentData._audioId = _nextTrackId;

        _nextTrackId++;
        switch (GetInStream()->GetCapabilities()->GetAudioCodecType()) {
            case CODEC_AUDIO_AAC:
            {
                AudioCodecInfoAAC *pTemp = GetInStream()->GetCapabilities()->GetAudioCodec<AudioCodecInfoAAC>();
                if (pTemp == NULL) {
                    FATAL("Invalid audio codec");
                    return false;
                }
                uint8_t raw[] = {
                    0x03, //ES_Descriptor tag=ES_DescrTag, ISO_IEC_14496-1;2010(E)-Character_PDF_document page 47/158
                    0x80, 0x80, 0x80, 0x22, //length
                    0xFF, 0xFF, //<ES_ID>
                    0x00, //streamDependenceFlag,URL_Flag,OCRstreamFlag,streamPriority
                    0x04, //DecoderConfigDescriptor tag=DecoderConfigDescrTag, ISO_IEC_14496-1;2010(E)-Character_PDF_document page 48/158
                    0x80, 0x80, 0x80, 0x14, //length
                    0x40, //objectTypeIndication, table 5 ISO_IEC_14496-1;2010(E)-Character_PDF_document page 49/158
                    0x15, //streamType=3, upStream=1, reserved=1
                    0x00, 0x00, 0x00, //bufferSizeDB
                    0x00, 0x00, 0x00, 0x00, //maxBitrate
                    0x00, 0x00, 0x00, 0x00, //avgBitrate
                    0x05, //AudioSpecificConfig tag=DecSpecificInfoTag
                    0x80, 0x80, 0x80, 0x02, //length
                    0xFF, 0xFF, //<codecBytes>, ISO-IEC-14496-3_2005_MPEG4_Audio page 49/1172
                    0x06, //SLConfigDescriptor tag=SLConfigDescrTag
                    0x80, 0x80, 0x80, 0x01, //length
                    0x02
                };
                EHTONSP(raw + 5, (uint16_t) _permanentData._audioId);
                memcpy(raw + 31, pTemp->_pCodecBytes, 2);
                _permanentData._audioEsDescriptor.ReadFromBuffer(raw, sizeof (raw));
                _permanentData._audioTimescale = GetInStream()->GetInputAudioTimescale();
                break;
            }
            default:
            {
                FATAL("Audio codec not supported");
                return false;
            }
        }
    }

    OpenMP4();

    /*
     * ftyp
     *	-major_brand			isom
     *	-minor_version			0x200
     *	-compatible_brands[]	isom,iso2,avc1
     * moov
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
     *		-matrix				0x00010000,0,0,0,0x00010000,0,0,0,0x40000000
     *		-pre_defined		0
     *		-next_track_ID		max(trak:track_ID)+1
     * 	iods - this is optional, should be skipped
     * 	trak
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
     * 		edts - this is optional, should be skipped
     * 		mdia
     * 			mdhd
     *				-version			1
     *				-flags				0
     *				-creation_time		mvhd:creation_time
     *				-modification_time	mvhd:creation_time
     *				-timescale			1000 for video and audio rate for audio.
     *				-duration			duration in mdhd:timescale
     *				-language			default it to ENG or to the "unknown" language
     *				-pre_defined		0
     * 			hdlr(vide)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		vide
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     * 			minf
     * 				vmhd
     *					-version			0
     *					-flags				1
     *					-graphicsmode		0
     *					-opcolor			0
     * 				dinf
     * 					dref
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						url
     *							-version			0
     *							-flags				1
     *							-location			NULL(0)
     * 				stbl
     * 					stsd
     *						-version			0
     *						-flags				0
     *						-entry_count		1
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
     * 							avcC
     *								-here we just deposit the RTMP representation for SPS/PPS from stream capabilities
     * 							btrt - this is optional, should be skipped
     * 					stts (time-to-sample, and DTS must be used, not PTS)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_delta)		DTS delta expressed in mdia:timescale
     * 					ctts (compositionTime-to-sample)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_offset)		CTS expressed in mdia:timescale
     * 					stss (sync sample table (random access points))
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						-sample_number		the ordinal number of the sample which is also a key frame
     * 					stsc (sample-to-chunk, partial data-offset information). We are going to have only one chunk per file
     *						-version					0
     *						-flags						0
     *						-entry_count				1
     *						-first_chunk				1
     *						-samples_per_chunk			total frames count in this track
     *						-sample_description_index	1
     * 					stsz (sample sizes (framing))
     *						-version					0
     *						-flags						0
     *						-sample_size				0 (we ware going to always specify the sample sizes)
     *						-sample_count				total frames count in this track
     *						*entry_size					size of each frame
     * 					co64 (64-bit chunk offset, we are going to always use 64 bits, not stco)
     *						-version					0
     *						-flags						0
     *						-entry_count				1 (we are going to have only one chunk)
     *						-chunk_offset				where the first frame begins, regardless of the audio/video type
     * 	trak
     * 		tkhd (solved)
     * 		mdia (solved)
     * 			mdhd (solved)
     * 			hdlr(soun)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		soun
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     * 			minf
     * 				smhd (sound media header, overall information (sound track only))
     *					-version			0
     *					-flags				0
     *					-balance			0
     *					-reserved			0
     * 				dinf
     * 					dref (solved)
     * 						url (solved)
     * 				stbl
     * 					stsd (solved)
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
     *								-version			0
     *								-flags				0
     *								-ES_Descriptor
     * 					stts (solved)
     * 					stsc (solved)
     * 					stsz (solved)
     * 					co64 (solved)
     * mdat (solved)
     * free (solved)
     */

    return true;
}

bool OutFileMP4InitSegment::WriteFTYP(File *pFile) {
    /*
     * ftyp
     *	-major_brand			isom
     *	-minor_version			0x200
     *	-compatible_brands[]	isom,iso2,avc1,mp41
     */
    return pFile->WriteBuffer(_ftyp, sizeof (_ftyp));
}

bool OutFileMP4InitSegment::WriteMVEX(File *pFile, uint8_t trexCount) {
    uint64_t sizeOffset = 0;
    sizeOffset = pFile->Cursor();
    if (!pFile->WriteUI64(0x000000006D766578LL, true)) {//XX XX XX XX 'm' 'v' 'e' 'x'
        FATAL("Unable to write mvex");
        return false;
    }
    for (uint8_t trxId = 1; trxId <= trexCount; ++trxId) {
        if (!WriteTREX(pFile, trxId)) {//trex
            FATAL("Unable to write trex");
            return false;
        }
    }
    if (!UpdateSize(pFile, sizeOffset)) {
        FATAL("Unable to update mvex size");
        return false;
    }
    return true;
}

bool OutFileMP4InitSegment::WriteTREX(File *pFile, uint8_t trackId) {
    uint64_t sizeOffset = 0;
    sizeOffset = pFile->Cursor();
    uint8_t trexStructure[24] = {
        0x00, 0x00, 0x00, 0x00, //flags: 0
        0x00, 0x00, 0x00, 0x01, //track_ID: 1
        0x00, 0x00, 0x00, 0x01, //default_sample_description_index: 1
        0x00, 0x00, 0x00, 0x00, //default_sample_duration: 0
        0x00, 0x00, 0x00, 0x00, //default_sample_size: 0
        0x00, 0x00, 0x00, 0x00 //default_sample_flags: 0
    };
    *(trexStructure + 7) = trackId; //Update the trackId
    if (!(pFile->WriteUI64(0x0000000074726578LL, true)) //XX XX XX XX 't' 'r' 'e' 'x'
            || (!pFile->WriteBuffer(trexStructure, sizeof (trexStructure)))
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write trex");
        return false;
    }

    return true;
}

bool OutFileMP4InitSegment::WriteMOOVAUDIO(File *pFile) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D6F6F76LL, true)) //XX XX XX XX 'm' 'o' 'o' 'v'
            || (!WriteMVHD(pFile)) //mvhd
            || (!WriteTRAK(pFile, _pAudio)) //trak(soun)
            || (!WriteMVEX(pFile, 1)) //mvex
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write moov");
        return false;
    }

    return true;
}

bool OutFileMP4InitSegment::WriteMOOVVIDEO(File *pFile) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D6F6F76LL, true)) //XX XX XX XX 'm' 'o' 'o' 'v'
            || (!WriteMVHD(pFile)) //mvhd
            || (!WriteTRAK(pFile, _pVideo)) //trak(vide)
            || (!WriteMVEX(pFile, 1)) //mvex
            || (!UpdateSize(_pMdatFile, sizeOffset))
            ) {
        FATAL("Unable to write moov");
        return false;
    }

    return true;
}

bool OutFileMP4InitSegment::WriteSTSC(File *pFile, Track *track) {
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
            || (!pFile->WriteUI32(0, true)) //entry_count
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write stsc");
        return false;
    }

    return true;
}

bool OutFileMP4InitSegment::CreateInitSegment(BaseInStream* pInStream, bool isAudio) {
    _Mode = OutFileMP4InitSegment::DASH;
    if (!Link(pInStream, false)){
        FATAL("Unable to link input stream");
        return false;
    }
    bool hasAudio = false, hasVideo = false;
    isAudio ? hasAudio = true : hasVideo = true;

    if (!FinishInitialization(hasAudio, hasVideo)) {
        FATAL("Initialization failed");
        return false;
    }
	if (isAudio) {
        if (!WriteMOOVAUDIO(_pMdatFile)) {
            FATAL("Unable to write MP4 headers");
            return false;
        }
    } else { 
        if (!WriteMOOVVIDEO(_pMdatFile)) {
            FATAL("Unable to write MP4 headers");
            return false;
        }
    }
    if (!UnLink(false)) {
        FATAL("Unable to unlink input stream");
        return false;
    }
    if (!CloseMP4()) {
        FATAL("Unable to close the file fragment");
        return false;
    }

    return true;
}

bool OutFileMP4InitSegment::CreateMoovHeader(File* pFile, BaseInStream* pInStream, bool hasAudio, bool hasVideo, 
    Variant& streamNames, uint16_t& trakCount) {
    _withFtype = false;
    _pMdatFile = pFile;
    _Mode = OutFileMP4InitSegment::MSS;
    uint64_t sizeOffset = _pMdatFile->Cursor();
    // write moov & mvhd headers
    if ((!_pMdatFile->WriteUI64(0x000000006D6F6F76LL, true)) //XX XX XX XX 'm' 'o' 'o' 'v'
            || (!WriteMVHD(_pMdatFile)) //mvhd
            ) {
        FATAL("Unable to write moov & mvhd");
        return false;
    }

    // loop through the stream names and get the stream pointer in stream manager
    FOR_MAP(streamNames, string, Variant, i) {
        string localStreamName = (string)MAP_VAL(i);
        map<uint32_t, BaseStream*> streamHandle = GetStreamsManager()->FindByTypeByName(ST_IN_NET, localStreamName, true, false);
        if (streamHandle.size() > 0) {
            if (!Link((BaseInStream*)MAP_VAL(streamHandle.begin()), false)){
                FATAL("Unable to link input stream");
                return false;
            }

            if (NULL != _pAudio) {
                _pAudio->Finalize();
                _pAudio->Unfinalize();
                delete _pAudio;
                _pAudio = NULL;
            }

            if (NULL != _pVideo) {
                _pVideo->Finalize();
                _pVideo->Unfinalize();
                delete _pVideo;
                _pVideo = NULL;
            }

            _permanentData._videoEsDescriptor.IgnoreAll();
            _permanentData._audioEsDescriptor.IgnoreAll();

            if (!FinishInitialization(hasAudio, hasVideo)) {
                FATAL("Initialization failed");
                return false;
            }

            // write TRAK headers
            if ((!WriteTRAK(_pMdatFile, _pVideo))       //trak(vide)
                || (!WriteTRAK(_pMdatFile, _pAudio))    //trak(soun)
                ) {
                FATAL ("Unable to write trak headers");
                return false;
            }
            if (!UnLink(false)) {
                FATAL("Unable to unlink input stream");
                return false;
            }
        }
    }
    uint8_t trexCount = (uint8_t)streamNames.MapSize();
    if (hasAudio && hasVideo)
        trexCount *= 2;
    // write MVEX headers
    if ((!WriteMVEX(_pMdatFile, trexCount))      //mvex
        || (!UpdateSize(_pMdatFile, sizeOffset))
        ) { 
        FATAL ("Unable to write mvex header");
        return false;
    }

    trakCount = trexCount;

    return true;
}

uint64_t OutFileMP4InitSegment::Duration(uint32_t timescale) {
    uint64_t result = 0;
    if (_Mode == OutFileMP4InitSegment::MSS)
        result = 0xffffffff;
    return result;
}
