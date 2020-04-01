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
#include "streaming/mp4/outnetmp4stream.h"
#include "application/baseappprotocolhandler.h"
#include "protocols/protocolmanager.h"
#include "eventlogger/eventlogger.h"
#include "streaming/fmp4/fmp4writer.h"

uint8_t OutNetMP4Stream::_ftyp[FTYP_SIZE] = {
    0x00, 0x00, 0x00, 0x20, //size: 0x20
    0x66, 0x74, 0x79, 0x70, //type: ftyp
    0x69, 0x73, 0x6F, 0x6D, //major_brand: isom
    0x00, 0x00, 0x02, 0x00, //minor_version
    0x69, 0x73, 0x6F, 0x6D, //compatible_brands[0]: isom
    0x69, 0x73, 0x6F, 0x32, //compatible_brands[1]: iso2
    0x61, 0x76, 0x63, 0x31, //compatible_brands[2]: avc1
    0x6D, 0x70, 0x34, 0x31, //compatible_brands[3]: mp41
};

OutNetMP4Stream::OutNetMP4Stream(BaseProtocol *pProtocol, string name)
: BaseOutNetStream(pProtocol, ST_OUT_NET_MP4, name),
_hasInitialkeyFrame(false),
_waitForIDR(false),
_ptsDtsDelta(0),
_chunkCount(0),
_frameCount(0),
_pAudio(NULL),
_pVideo(NULL),
_emptyRunsCount(0), 
_delayedRemoval(false) {
	WriteFTYP(_ftypBuffer);
}

OutNetMP4Stream* OutNetMP4Stream::GetInstance(BaseClientApplication *pApplication,
        string name) {
	//1. Create a dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//2. create the parameters
	Variant parameters;

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(parameters)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

    //4. Set the application
    pDummyProtocol->SetApplication(pApplication);

    //5. Create the MP4 stream
    OutNetMP4Stream *pOutNetMP4Stream = new OutNetMP4Stream(pDummyProtocol, name);
    if (!pOutNetMP4Stream->SetStreamsManager(pApplication->GetStreamsManager())) {
        FATAL("Unable to set the streams manager");
        delete pOutNetMP4Stream;
        pOutNetMP4Stream = NULL;
        return NULL;
    }
    pDummyProtocol->SetDummyStream(pOutNetMP4Stream);

    //6. Done
    return pOutNetMP4Stream;
}

void OutNetMP4Stream::RegisterOutputProtocol(uint32_t protocolId, void *pUserData) {
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(protocolId);
	if (pProtocol == NULL)
		return;
	INFO("%s protocol linked to stream %s", STR(*pProtocol), STR(*this));

	_signaledProtocols[protocolId] = pUserData;
	_emptyRunsCount = 0;
}

void OutNetMP4Stream::UnRegisterOutputProtocol(uint32_t protocolId) {
	if (!MAP_HAS1(_signaledProtocols, protocolId))
		return;
	if (_delayedRemoval) {
		_pendigRemovals.push_back(protocolId);
	} else {
		_signaledProtocols.erase(protocolId);
		if (_signaledProtocols.size() == 0)
			EnqueueForDelete();
	}
}

OutNetMP4Stream::~OutNetMP4Stream() {
	FOR_MAP(_signaledProtocols, uint32_t, void *, i) {
		BaseProtocol *pProtocol = ProtocolManager::GetProtocol(MAP_KEY(i));
		if (pProtocol != NULL)
			pProtocol->SignalOutOfBandDataEnd(MAP_VAL(i));
	}
	GetEventLogger()->LogStreamClosed(this);
}

bool OutNetMP4Stream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_FILE)
            || TAG_KIND_OF(type, ST_IN_NET_EXT);
}
#ifdef DEBUG_MP4_STREAM
bool OutNetMP4Stream::AssembleFromBuffer() {
	string _filePath = "test.mp4";
	File mp4File;

	if (!mp4File.Initialize(_filePath, FILE_OPEN_MODE_TRUNCATE)) {
		string error = "Unable to open file " + _filePath;
		FATAL("%s", STR(error));

		return false;
	}

	// Update size of mdat buffer
	if (!FinalizeMdat())
		return false;

	//Append the ftyp atom to the mp4 file
	if (!mp4File.WriteBuffer(GETIBPOINTER(_ftypBuffer), GETAVAILABLEBYTESCOUNT(_ftypBuffer))) {
		FATAL("Unable to write ftyp to mp4");
		return false;
	}

	//Append the mdat atom to the mp4 file
	if (!mp4File.WriteBuffer(GETIBPOINTER(_mdatBuffer), GETAVAILABLEBYTESCOUNT(_mdatBuffer))) {
		FATAL("Unable to write mdat to mp4");
		return false;
	}

	//Append the moov atom to the mp4 file
	if (!mp4File.WriteBuffer(GETIBPOINTER(_moovBuffer), GETAVAILABLEBYTESCOUNT(_moovBuffer))) {
		FATAL("Unable to write moov to mp4");
		return false;
	}

	mp4File.Close();

	ASSERT("File %s created!", STR(_filePath));
	return true;
}
#endif
bool OutNetMP4Stream::OpenMP4() {
	// Send start header as well as FTYP box
	WriteProgressiveBuffer(M_PBT_BEGIN, GETIBPOINTER(_ftypBuffer), GETAVAILABLEBYTESCOUNT(_ftypBuffer));

    // Close any existing files
    if (!CloseMP4()) {
        string error = "Could not close MP4 file!";
        FATAL("%s", STR(error));

        return false;
    }
#ifdef DEBUG_MP4_STREAM
    _mdatBuffer.IgnoreAll();
    _mdatDataSizeOffset = 8;
    _mdatDataSize = 0;

    //00 00 00 01 m d a t XX XX XX XX XX XX XX XX
    if ((!_mdatBuffer.ReadFromU64(0x000000016D646174LL, true))
            || (!_mdatBuffer.ReadFromU64(0, true))) {
        FATAL("Unable to write the mdat header");
        return false;
    }
#else
	_mdatDataSize = 8;
#endif
    if (_permanentData._audioId > 0) {
        _pAudio = new Track();
        _pAudio->_id = _permanentData._audioId;
        _pAudio->_width = 0;
        _pAudio->_height = 0;
        _pAudio->_timescale = _permanentData._audioTimescale;
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
        _pVideo->_timescale = _permanentData._videoTimescale;
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

bool OutNetMP4Stream::CloseMP4() {
	// Nothing to close
	if ((_pAudio == NULL) && (_pVideo == NULL)) return true;

	// Nothing to do here if NOT normal operation
	if (_pVideo != NULL) _pVideo->_stts.Finish();
	if (_pAudio != NULL) _pAudio->_stts.Finish();

	if (!Finalize()) {
		WARN("Recording failed!");
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

bool OutNetMP4Stream::FinishInitialization(
        GenericProcessDataSetup *pGenericProcessDataSetup) {
    if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
        FATAL("Unable to finish output stream initialization");
        return false;
    }

    _winQtCompat = true;
    _waitForIDR = true;
	// Chunk length in milliseconds
    _chunkLength = 200;

    //video setup
    pGenericProcessDataSetup->video.avc.Init(
            NALU_MARKER_TYPE_SIZE, //naluMarkerType,
            false, //insertPDNALU,
            false, //insertRTMPPayloadHeader,
            true, //insertSPSPPSBeforeIDR,
            true //aggregateNALU
            );

    //audio setup
    pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
    pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;

    //misc setup
    pGenericProcessDataSetup->_timeBase = 0;
    pGenericProcessDataSetup->_maxFrameSize = 16 * 1024 * 1024;

    if (pGenericProcessDataSetup->_hasAudio) {
        _permanentData._audioId = _nextTrackId;
        _nextTrackId++;
        switch (pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodecType()) {
            case CODEC_AUDIO_AAC:
            {
                AudioCodecInfoAAC *pTemp = pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC>();
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

    if (pGenericProcessDataSetup->_hasVideo) {
        _permanentData._videoId = _nextTrackId;
        _nextTrackId++;
        switch (pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodecType()) {
            case CODEC_VIDEO_H264:
            {
                VideoCodecInfoH264 *pTemp = pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264>();
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
        _onlyAudio = true;
    }

    if (_waitForIDR) {
        _hasInitialkeyFrame = !pGenericProcessDataSetup->_hasVideo;
    } else {
        _hasInitialkeyFrame = true;
    }

    if (_winQtCompat) {
        if (_creationTime >= 0x100000000LL)
            _creationTime = 0;
    }

    OpenMP4();

  return true;
}

bool OutNetMP4Stream::PushVideoData(IOBuffer &buffer, double pts, double dts,
        bool isKeyFrame) {

    if (!_hasInitialkeyFrame) {
        if (!isKeyFrame) {
            return true;
        }
        _hasInitialkeyFrame = true;
        _ptsDtsDelta = dts;
        //		DEBUG("_ptsDtsDelta: %f", _ptsDtsDelta);
    }
    _frameCount++;

    if (PushData(
            _pVideo,
            GETIBPOINTER(buffer),
            GETAVAILABLEBYTESCOUNT(buffer),
            pts,
            dts,
            isKeyFrame)) {
        return true;
    } else {
        return false;
    }
}

bool OutNetMP4Stream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
    if (!_hasInitialkeyFrame)
        return true;

    if (PushData(
            _pAudio,
            GETIBPOINTER(buffer),
            GETAVAILABLEBYTESCOUNT(buffer),
            pts,
            dts,
            false)) {
        return true;
    } else {
        return false;
    }
}

bool OutNetMP4Stream::PushData(Track *track, const uint8_t *pData, uint32_t dataLength,
        double pts, double dts, bool isKeyFrame) {
    // Adjust PTS/DTS
    pts = pts < _ptsDtsDelta ? 0 : pts - _ptsDtsDelta;
    dts = dts < _ptsDtsDelta ? 0 : dts - _ptsDtsDelta;
	bool isAudio = track->_isAudio;

    // Check if we need to create a new chunk
    if ((_chunkLength > 0)
            && ((dts > _chunkLength) && (track->_stsz._sampleCount > 0))
            && ((!_waitForIDR) || (_waitForIDR && isKeyFrame) || _onlyAudio)) {
        // Open a new MP4 file
        OpenMP4();

        if (isAudio) {
            track = _pAudio;
        } else {
            track = _pVideo;
        }

        // Increment PTS/DTS delta, then adjust PTS/DTS
        _ptsDtsDelta += dts;
        pts = 0;
        dts = 0;
    }

    //STTS
    track->_stts.PushDTS(dts);

    //STSZ
    track->_stsz.PushSize(dataLength);

    if (!isAudio) {
        //STSS
        if (isKeyFrame)
            track->_stss.PushKeyFrameIndex(track->_stsz._sampleCount);

        //CTTS
        track->_ctts.PushCTS(pts - dts);
    }

#ifdef DEBUG_MP4_STREAM
    //CO64
    track->_co64.PushOffset(GETAVAILABLEBYTESCOUNT(_mdatBuffer));

    if (!StoreDataToFile(pData, dataLength)) {
        FATAL("Unable to store data!");
        return false;
    }
#else
	//CO64
    track->_co64.PushOffset(_mdatDataSize);
	_mdatDataSize += dataLength;
#endif

    return WriteProgressiveBuffer((isAudio ? M_PBT_AUDIO_BUFFER : M_PBT_VIDEO_BUFFER), pData, dataLength);
}

bool OutNetMP4Stream::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264) || (codec == CODEC_AUDIO_AAC);
}
#ifdef DEBUG_MP4_STREAM
bool OutNetMP4Stream::StoreDataToFile(const uint8_t *pData, uint32_t dataLength) {
    if (!_mdatBuffer.ReadFromBuffer(pData, dataLength)) {
        FATAL("Unable to write data to file");
        return false;
    }
    _mdatDataSize += dataLength;
    return true;
}
#endif
uint64_t OutNetMP4Stream::Duration(uint32_t timescale) {
/*	FATAL("Duration");*/
    uint64_t audio = (_pAudio ? _pAudio->Duration(timescale) : 0);
    //DEBUG("audio: %"PRIu64, audio);
    uint64_t video = (_pVideo ? _pVideo->Duration(timescale) : 0);
    //DEBUG("video: %"PRIu64, video);
    uint64_t result = audio < video ? video : audio;
    //DEBUG("result: %"PRIu64, result);
    if (_winQtCompat) {
        if (result >= 0x100000000LL)
            result = 0xffffffff;
    }
    //DEBUG("result: %"PRIu64, result);
    return result;
}

bool OutNetMP4Stream::Finalize() {
	// Form the MDAT header
	_mdatHeader = EHTONLL((_mdatDataSize << 32) | 0x6D646174);

	// Form the MOOV box
	_moovBuffer.IgnoreAll();
	if (!WriteMOOV(_moovBuffer)) {
		FATAL("Unable to write moov");
	}

#ifdef DEBUG_MP4_STREAM
	AssembleFromBuffer();
#endif
    return WriteProgressiveBuffer(M_PBT_HEADER_MDAT, (uint8_t *) & _mdatHeader, 8)
			&& WriteProgressiveBuffer(M_PBT_HEADER_MOOV, GETIBPOINTER(_moovBuffer), GETAVAILABLEBYTESCOUNT(_moovBuffer))
			&& WriteProgressiveBuffer(M_PBT_END, NULL, 0)
			;
}

bool OutNetMP4Stream::WriteFTYP(IOBufferExt &dst) {
    /*
     * ftyp
     *	-major_brand			isom
     *	-minor_version			0x200
     *	-compatible_brands[]	isom,iso2,avc1,mp41
     */
    return dst.ReadFromBuffer(_ftyp, sizeof (_ftyp));
}

bool OutNetMP4Stream::WriteMOOV(IOBufferExt &dst) {
    uint32_t sizeOffset = GETAVAILABLEBYTESCOUNT(dst);
    if ((!dst.ReadFromU64(0x000000006D6F6F76LL, true)) //XX XX XX XX 'm' 'o' 'o' 'v'
            || (!WriteMVHD(dst)) //mvhd
            || (_pVideo ? (!WriteTRAK(dst, _pVideo)) : false) //trak(vide)
            || (_pAudio ? (!WriteTRAK(dst, _pAudio)) : false) //trak(soun)
            || (!UpdateSize(dst, sizeOffset))
            ) {
        FATAL("Unable to write moov");
        return false;
    }

    return true;
}
#ifdef DEBUG_MP4_STREAM
bool OutNetMP4Stream::FinalizeMdat() {
	uint8_t *pBuffer = GETIBPOINTER(_mdatBuffer) + _mdatDataSizeOffset;
	EHTONLLP(pBuffer, (_mdatDataSize + 16));
	
	return true;
}
#endif
bool OutNetMP4Stream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutNetMP4Stream::SignalPause() {
	return true;
}

bool OutNetMP4Stream::SignalResume() {
	return true;
}

bool OutNetMP4Stream::SignalSeek(double &dts) {
	return true;
}

bool OutNetMP4Stream::SignalStop() {
	return true;
}

void OutNetMP4Stream::SignalAttachedToInStream() {
}

void OutNetMP4Stream::SignalDetachedFromInStream() {
	if (_pProtocol != NULL) {
		_pProtocol->EnqueueForDelete();
	}
}

void OutNetMP4Stream::SignalStreamCompleted() {
	if (_pProtocol != NULL) {
		_pProtocol->EnqueueForDelete();
	}
}

bool OutNetMP4Stream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
	uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
	packetsCount++;
	bytesCount += dataLength;
	return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
}

void OutNetMP4Stream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if ((pOld == NULL)&&(pNew != NULL))
		return;
	WARN("Codecs changed and the recordings does not support it. Closing recording");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
}

void OutNetMP4Stream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	if ((pOld == NULL)&&(pNew != NULL))
		return;

	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}

	WARN("Codecs changed and the recordings does not support it. Closing recording");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
}

bool OutNetMP4Stream::WriteProgressiveBuffer(MP4ProgressiveBufferType type,
		const uint8_t *pBuffer, size_t size) {
	if (size > 0x00ffffff) {
		FATAL("Frame too big. Maximum allowed size is 0x00ffffff bytes");
		return false;
	}

	return _progressiveBuffer.IgnoreAll()
			&& _progressiveBuffer.ReadFromU32((uint32_t) size | (type << 24), true)
			&& _progressiveBuffer.ReadFromBuffer(pBuffer, (uint32_t) size)
			&& SignalProtocols(_progressiveBuffer);
}

bool OutNetMP4Stream::SignalProtocols(const IOBuffer &buffer) {
	map<uint32_t, void *>::iterator i = _signaledProtocols.begin();
	_delayedRemoval = true;

	while (i != _signaledProtocols.end()) {
		BaseProtocol *pProtocol = ProtocolManager::GetProtocol(i->first);
		if (pProtocol == NULL) {
			_signaledProtocols.erase(i++);
			continue;
		}
		if (!pProtocol->SendOutOfBandData(buffer, i->second)) {
			pProtocol->SignalOutOfBandDataEnd(i->second);
			_signaledProtocols.erase(i++);
			continue;
		}
		i++;
	}
	_delayedRemoval = false;
	while (_pendigRemovals.size() != 0) {
		UnRegisterOutputProtocol(_pendigRemovals[0]);
		_pendigRemovals.erase(_pendigRemovals.begin());
	}

	if (_signaledProtocols.size() == 0)
		_emptyRunsCount++;
	else
		_emptyRunsCount = 0;
	if (_emptyRunsCount >= 10)
		EnqueueForDelete();

	return true;
}
