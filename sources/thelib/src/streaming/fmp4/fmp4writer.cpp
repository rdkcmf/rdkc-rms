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

#include "streaming/fmp4/fmp4writer.h"
#include "streaming/codectypes.h"
#include "streaming/streamcapabilities.h"
#include <math.h>

#define SF_SAMPLE_NON_KEY_FRAME 0x01010000
#define SF_SAMPLE_KEY_FRAME 0x02000000
#define DEFAULT_SAMPLES_PER_FRAGMENT 64

#define TFDT_64BIT
#ifdef TFDT_64BIT
#define TFDT_TYPE uint64_t
#define TFDT_FLAGS 0x01000000
#define TFDT_ReadFrom ReadFromU64
#define TFDT_HTON EHTONLLP
#else /* TFDT_64BIT */
#define TFDT_TYPE uint32_t
#define TFDT_FLAGS 0x00000000
#define TFDT_ReadFrom ReadFromU32
#define TFDT_HTON EHTONLP
#endif /* TFDT_64BIT */

#define MAX_SAMPLE_SIZE 0xfff0
#define MAX_DTSDELTAS_SIZE 200
#define MIN_DTSDELTAS_SIZE 10
#define MAX_DTSFLUCTUATION 2

const char * FMP4ProgressiveBufferTypeToString(FMP4ProgressiveBufferType type) {
	static char unknown[5];
	switch (type) {
		case PBT_BEGIN:
			return "BEGIN";
		case PBT_HEADER_MOOV:
			return "MOOV";
		case PBT_HEADER_MOOF:
			return "MOOF";
		case PBT_VIDEO_BUFFER:
			return "VIDEO_BUFFER";
		case PBT_AUDIO_BUFFER:
			return "AUDIO_BUFFER";
		case PBT_HEADER_MDAT:
			return "HEADER_MDAT";
		case PBT_END:
			return "END";
		default:
		{
			sprintf(unknown, "0x%02"PRIx8, (uint8_t) type);
			return unknown;
		}
	}
}

FMP4Writer::TrackContext::TrackContext() {
	Init();
}

// Do a HARD RESET of the track context.  We do this when we are about to end 
// the current FMP4 stream and start a new one.  HARD RESET brings back 
// TrackContext to a fresh state.
void FMP4Writer::TrackContext::Init() {
	_id = 0;
	_isAudio = false;
	//_trunEntrySize = 8; // adjusted (only size and composition time offset)
	//_trunEntrySize = 0; // 2nd iteration: TRUN sample has no actual data
	_trunEntrySize = 4; // 3rd iteration: TRUN only composition time offset
	_width = 0;
	_height = 0;
	_samplingRate = 0;
	_defaultSampleDuration = 0;
	_samplesCount = 0;
	_maxSamplesCount = 0;
	_trafSize = 0;
	_trunSize = 0;
	_fragmentStartTs = -1;
	_lastDuration = 0;
	_totalDuration = 0;
	_sampleSize = 0;

	_timescale = 1000;
	_ignoreLastSample = false;

	_sync._syncPoint = -1;
	_sync._samplesCount = 0;
	_sync._enabled = false;
	memset(&_offset, 0, sizeof(_offset));

	_codecBytes.IgnoreAll();
	_trafHeader.IgnoreAll();
	_trunBuffer.IgnoreAll();
	_mdat.IgnoreAll();
}

// Prepare the track context for the new MOOF.  NOT to be confused with
// TrackContext::Init()
void FMP4Writer::TrackContext::Reset() {
	_fragmentStartTs = -1;
	_samplesCount = 0;
	_mdat.IgnoreAll();
	_trunBuffer.IgnoreAll();
	_lastDuration = 0;
	_totalDuration = 0;
	_ignoreLastSample = false;
	_sampleSize = 0;
}


FMP4Writer::ResetContext::ResetContext() {
	isEnqueued = false;		// set to TRUE if a reset has been requested.
	enableAudio = false;
	enableVideo = false;
	progressive = false;
	pCap = NULL;
}

FMP4Writer::FMP4Writer() {
	_initMembers();
}

FMP4Writer::~FMP4Writer() {
	while (_bufferedAudio.size() != 0) {
		delete _bufferedAudio[0];
		_bufferedAudio.erase(_bufferedAudio.begin());
	}
}

void FMP4Writer::_initMembers() {
	_now = 0;
	_fragmentSequenceNumber = 0;
	_offsetFragmentSequenceNumber = 0;
	_offsetDuration = 0;
	_mdat = 0;
	_initialSegmentOpened = false;
	_moovStartPts = 0;
	memset(&_progressive, 0, sizeof(_progressive));
	_lastKnownAudioPts = -1;
	_lastKnownAudioDts = -1;
	_lastKnownVideoPts = -1;
	_lastKnownVideoDts = -1;
	_vidRcv = _audRcv = 0;

	_winQtCompat = true;
	_mvhdTimescale = 1000;
	_startAudioTimeTicks = 0;
	_startVideoTimeTicks = 0;
	_isPlayerPaused = false;
	_stopOutbound = false;
	_dtsOffset = 0;
	_inboundVod = false;
	_aveVideoDtsDelta = -1;
	_aveAudioDtsDelta = -1;
}

// Do a hard reset of the FMP4Writer object.  This brings back the object to a 
// fresh state, as if 'new' has been called.
void FMP4Writer::_resetFMP4Writer() {
	_initMembers();
	_audio.Init();
	_video.Init();
}



// This is called when we receive a StreamCapabilityChange notification.
// This will require the writer to send a new FTYP + MOOV.
bool FMP4Writer::EnqueueResetStream(bool enableAudio, bool enableVideo, 
									StreamCapabilities *pCapabilities, bool progressive) {
	_resetter.enableAudio = enableAudio;
	_resetter.enableVideo = enableVideo;
	_resetter.progressive = progressive;
	_resetter.pCap        = pCapabilities;
	_resetter.isEnqueued  = true;

	return true;
}



// (1) Create the MOOV
// (2) Create blank TRAF atoms for use inside the MOOF
bool FMP4Writer::Initialize(bool enableAudio, bool enableVideo,
		StreamCapabilities *pCapabilities, bool progressive) {
	_progressive._enabled = progressive;
	_now = (uint64_t) time(NULL) + (uint64_t) 0x7C25B080; // 1970 based -> 1904 based
	_moov.IgnoreAll();

#ifdef FMP4_DEBUG
	FINE("Now: %"PRIu32, _now);
#endif

	if ((!enableAudio)&&(!enableVideo)) {
		FATAL("FMP4 writer needs at least an audio or video track");
		return false;
	}

	// Create the TRAF atoms.  
	// NOTE: Vid tracks always come first, if available.  Hence if VID/AUD, then vid will be track1
	//       aud will be track2.  
	uint32_t trackId = 1;
	if ((enableVideo)&&(pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264)) {
		VideoCodecInfoH264 *pCodec = pCapabilities->GetVideoCodec<VideoCodecInfoH264>();

		_video._isAudio = false;
		_video._id = trackId++;
		IOBuffer &raw = pCodec->GetRTMPRepresentation();
		_video._codecBytes.ReadFromBuffer(GETIBPOINTER(raw) + 5, GETAVAILABLEBYTESCOUNT(raw) - 5);
		_video._width = pCodec->_width;
		_video._height = pCodec->_height;
		// Err... value of sampling rate shouldn't be hard coded.
		_video._samplingRate = pCodec->_samplingRate;
		//_video._samplingRate = 1890;
	}
	if ((enableAudio)&&(pCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC)) {
		AudioCodecInfoAAC *pCodec = pCapabilities->GetAudioCodec<AudioCodecInfoAAC>();

		_audio._isAudio = true;
		_audio._id = trackId++;
		_audio._codecBytes.ReadFromBuffer(pCodec->_pCodecBytes, 2);
		_audio._samplingRate = pCodec->_samplingRate;
		//_video._samplingRate = 1000;
		if (_video._id == 0) {
			_audio._maxSamplesCount = DEFAULT_SAMPLES_PER_FRAGMENT;
		}
	}

	return BuildAtomTRAF(_audio) && BuildAtomTRAF(_video);
}

bool FMP4Writer::WriteVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	if (_video._id == 0)
		return true;

	if ((!_initialSegmentOpened) && (!isKeyFrame)) {
		_lastKnownVideoDts = dts;
		return true;
	}

	return WriteData(_video, buffer, pts, dts, isKeyFrame);
}

bool FMP4Writer::WriteAudioData(IOBuffer &buffer, double pts, double dts) {
	if (_audio._id == 0)
		return true;

	if ((_video._id != 0) && (!_initialSegmentOpened)) {
		_lastKnownAudioDts = dts;
		return true;
	}

	// Initially store the audio sample
	BufferedAudio *pBa = new BufferedAudio();
	pBa->buffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
	pBa->pts = pts;
	pBa->dts = dts;
	_bufferedAudio.push_back(pBa);

	// Send it once we have an existing sample
	bool result = true;
	if (_bufferedAudio.size() > 1) {
		pBa = _bufferedAudio[0];
		_bufferedAudio.erase(_bufferedAudio.begin());
		result = WriteData(_audio, pBa->buffer, pBa->pts, pBa->dts, true);
		delete pBa;
	}

	return result;
}

bool FMP4Writer::Flush() {
	return CloseFragment();
}

#define WRITE_ATOM_HEADER(dst,x) (!(dst).ReadFromU32(0, true)) || (!(dst).ReadFromU32((x), true))

// BuildAtomFTYP : Create the FTYP atom.  
// NOTE: The intention is to have the FTYE and MOOV atoms share the same
//       IOBuffer.  
//
// IOBufferExt& dst : This is where the FTYP atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomFTYP(IOBufferExt &dst) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'FTYP'                                            |
	// +---------------+---------------+---------------+---------------+
	// |      major_brand                                              |
	// +---------------+---------------+---------------+---------------+
	// |      minor_version                                            |
	// +---------------+---------------+---------------+---------------+
	// |      compatible_brands[]                                      |
	// +---------------+---------------+---------------+---------------+
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x66747970)
			|| (!dst.ReadFromU32(0x69736F35, true)) //major_brand: `iso5`
			|| (!dst.ReadFromU32(0x0200, true)) //minor_version
			|| (!dst.ReadFromU32(0x69736F36, true)) //compatible_brands: `iso6`
			|| (!dst.ReadFromU32(0x6D703431, true)) //compatible_brands: `mp41`
			) {
		FATAL("Unable to write atom FTYP");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	_moofOffset = GETAVAILABLEBYTESCOUNT(dst) - start;
	return true;
}


// Create the MOOV (Movie) atom.  
// NOTE: The intention is to have the FTYE and MOOV atoms share the same
//       IOBuffer.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomMOOV(IOBufferExt &dst) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'moov'                                            |
	// +---------------+---------------+---------------+---------------+
	// |      > MVHD                                                   |
	// +---------------+---------------+---------------+---------------+
	// |      > TRAK (video, if available)                             |
	// +---------------+---------------+---------------+---------------+
	// |      > TRAK (audio, if available)                             |
	// +---------------+---------------+---------------+---------------+
	// |      > MVEX                                                   |
	// +---------------+---------------+---------------+---------------+
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D6F6F76)
			|| (!BuildAtomMVHD(dst))
			|| (!BuildAtomTRAK(dst, _video))
			|| (!BuildAtomTRAK(dst, _audio))
			|| (!BuildAtomMVEX(dst))
			) {
		FATAL("Unable to write atom MOOV");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	_moofOffset += (GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

// Create the MVHD (Movie Header) atom.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomMVHD(IOBufferExt &dst) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'mvhd'                                            |
	// +---------------+---------------+---------------+---------------+
	// | version = 0   |  flags = 0                                    |
	// +---------------+---------------+---------------+---------------+
	// |      creation_time (32bits since version = 0)                 |
	// +---------------+---------------+---------------+---------------+
	// |      modification_time (32bits since version = 0)             |
	// +---------------+---------------+---------------+---------------+
	// |      timescale (32bits since version = 0)                     |
	// +---------------+---------------+---------------+---------------+
	// |      duration (32bits since versoin = 0)                      |
	// +---------------+---------------+---------------+---------------+
	// |      rate = 0x00010000                                        |
	// +---------------+---------------+---------------+---------------+
	// |      volume = 0x0100          |  reserved = 0x0000            |
	// +---------------+---------------+---------------+---------------+
	// |                                                               |
	// +----- reserved = 0x00 -----------------------------------------+
	// |                                                               |
	// +---------------+---------------+---------------+---------------+
	// |      matrix [36]                                              |
	// |                                                               |
	// |                                                               |
	// +---------------+---------------+---------------+---------------+
	// |      pre_defined [24] = 0x00                                  |
	// |                                                               |
	// +---------------+---------------+---------------+---------------+
	// |      next_track_id                                            |
	// +---------------+---------------+---------------+---------------+
	static uint8_t matrix[] = {
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x00, 0x00, 0x00
	};
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D766864)
			|| (!dst.ReadFromU32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version/flags
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //creation_time
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //modification_time
			|| (!dst.ReadFromU32(_mvhdTimescale, true)) //timescale
			|| (_winQtCompat ? (!dst.ReadFromU32(0, true)) : (!dst.ReadFromU64(0, true))) //duration
			|| (!dst.ReadFromU32(0x00010000, true)) //rate
			|| (!dst.ReadFromU16(0x0100, true)) //volume
			|| (!dst.ReadFromU16(0, true)) //reserved
			|| (!dst.ReadFromRepeat(0, 8)) //reserved
			|| (!dst.ReadFromBuffer(matrix, sizeof (matrix))) //matrix
			|| (!dst.ReadFromRepeat(0, 24)) //pre_defined
			|| (!dst.ReadFromU32(GetTracksCount() + 1, true)) //next_track_ID
			) {
		FATAL("Unable to write atom MVHD");
		return false;
	}

	// Placeholder
	_offsetDuration = start + 24;
	if (_winQtCompat) {
		_offsetDuration += 8;
	}

	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);

	return true;
}

// Create the MVEX (Movie Extends) atom.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomMVEX(IOBufferExt &dst) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'mvex'                                            |
	// +---------------+---------------+---------------+---------------+
	// |      > TREX (for video, if available                          |
	// +---------------+---------------+---------------+---------------+
	// |      > TREX (for audio, if available                          |
	// +---------------+---------------+---------------+---------------+
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D766578)
			|| (!BuildAtomTREX(dst, _video))
			|| (!BuildAtomTREX(dst, _audio))
			) {
		FATAL("Unable to write atom MVEX");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

// Create the TREX (Track Extends) atom.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomTREX(IOBufferExt &dst, TrackContext &track) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'trex'                                            |
	// +---------------+---------------+---------------+---------------+
	// | version = 0   |  flags = 0                                    |
	// +---------------+---------------+---------------+---------------+
	// |      track_id = track._id                                     |
	// +---------------+---------------+---------------+---------------+
	// |      default_sample_description_index = 0x0001                |
	// +---------------+---------------+---------------+---------------+
	// |      default_sample_duration = 0x0000                         |
	// +---------------+---------------+---------------+---------------+
	// |      default_sample_size = 0x0000                             |
	// +---------------+---------------+---------------+---------------+
	// |      default_sample_flags = 0x0000                            |
	// +---------------+---------------+---------------+---------------+
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x74726578)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(track._id, true)) //track_ID
			|| (!dst.ReadFromU32(1, true)) //default_sample_description_index
			|| (!dst.ReadFromU32(0, true)) //default_sample_duration
			|| (!dst.ReadFromU32(0, true)) //default_sample_size
			|| (!dst.ReadFromU32(0, true)) //default_sample_flags
			) {
		FATAL("Unable to write atom TREX");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

// Create the TRAK (Track) atom.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomTRAK(IOBufferExt &dst, TrackContext &track) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'trak'                                            |
	// +---------------+---------------+---------------+---------------+
	// |      > TKHD                                                   |
	// +---------------+---------------+---------------+---------------+
	// |      > MDIA                                                   |
	// +---------------+---------------+---------------+---------------+
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x7472616B)
			|| (!BuildAtomTKHD(dst, track))
			//|| (!BuildAtomEDTS(dst))
			|| (!BuildAtomMDIA(dst, track))
			) {
		FATAL("Unable to write atom TRAK");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

// Create the TKHD (Track Header) atom.  
//
// IOBufferExt& dst : This is where the atom shall be APPENDED to.
//
bool FMP4Writer::BuildAtomTKHD(IOBufferExt &dst, TrackContext &track) {
	//                    1                   2                   3
	//  1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
	// +---------------+---------------+---------------+---------------+
	// |      size                                                     |
	// +---------------+---------------+---------------+---------------+
	// |      type = 'tkhd'                                            |
	// +---------------+---------------+---------------+---------------+
	// | version = 0   |  flags = 0                                    |
	// +---------------+---------------+---------------+---------------+
	// |      creation_time (32bits since version = 0)                 |
	// +---------------+---------------+---------------+---------------+
	// |      modification_time (32bits since version = 0)             |
	// +---------------+---------------+---------------+---------------+
	// |      track_ID = track._id                                     |
	// +---------------+---------------+---------------+---------------+
	// |      reserved = 0x00000000                                    |
	// +---------------+---------------+---------------+---------------+
	// |      duration (32bits since version = 0)                      |
	// +---------------+---------------+---------------+---------------+
	// |      reserved = 0x00000000                                    | 
	// +---------------+---------------+---------------+---------------+
	// |                 0x00000000                                    |
	// +---------------+---------------+---------------+---------------+
	// |      layer = 0x0000           |   alternate_group = (1|0)     |
	// +---------------+---------------+---------------+---------------+
	// |      volume = (0x0100|0x0000) |   reserved = 0x0000           |
	// +---------------+---------------+---------------+---------------+
	// |      matrix = [36 bytes]                                      |
	// |                                                               |
	// +---------------+---------------+---------------+---------------+
	// |      width = videowidth << 16                                 |
	// +---------------+---------------+---------------+---------------+
	// |      height = videoheight << 16                               |
	// +---------------+---------------+---------------+---------------+
	if (track._id == 0)
		return true;
	static uint8_t matrix[] = {
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x00, 0x00, 0x00
	};
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);

	//TKHD flags:
	//0x000001 // Track_enabled
	//0x000002 // Track_in_movie
	//0x000004 // Track_in_preview
	//0x000008 // Track_size_is_aspect_ratio
	
	if (WRITE_ATOM_HEADER(dst, 0x746B6864)
			|| (!dst.ReadFromU32(_winQtCompat ? 0x00000003 : 0x01000003, true)) //version/flags
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //creation_time
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //modification_time
			|| (!dst.ReadFromU32(track._id, true)) //track_ID
			|| (!dst.ReadFromU32(0, true)) //reserved
			|| (_winQtCompat ? (!dst.ReadFromU32(0, true)) : (!dst.ReadFromU64(0, true))) //duration
			|| (!dst.ReadFromRepeat(0, 8)) //reserved
			|| (!dst.ReadFromU16(0, true)) //layer
			|| (!dst.ReadFromU16(track._isAudio ? 1 : 0, true)) //alternate_group
			|| (!dst.ReadFromU16(track._isAudio ? 0x0100 : 0, true)) //volume
			|| (!dst.ReadFromU16(0, true)) //reserved
			|| (!dst.ReadFromBuffer(matrix, sizeof (matrix))) //matrix
			|| (!dst.ReadFromU32(track._width << 16, true)) //width
			|| (!dst.ReadFromU32(track._height << 16, true)) //height
			) {
		FATAL("Unable to write atom TKHD");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomEDTS(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x65647473)
			|| (!BuildAtomELST(dst))
			) {
		FATAL("Unable to write atom EDTS");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomELST(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	uint32_t versionAndFlags = 0x00000000; //version 0, short 32 bits fields,
	if (WRITE_ATOM_HEADER(dst, 0x656C7374)
			|| (!dst.ReadFromU32(versionAndFlags, true)) //version/flags
			|| (!dst.ReadFromU32(1, true)) //entry_count
			|| (!dst.ReadFromU32(0, true)) //segment_duration
			|| (!dst.ReadFromU32(0, true)) //media_time
			|| (!dst.ReadFromU16(1, true)) //media_rate_integer
			|| (!dst.ReadFromU16(0, true)) //media_rate_fraction
			) {
		FATAL("Unable to write atom ELST");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMDIA(IOBufferExt &dst, TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D646961)
			|| (!BuildAtomMDHD(dst, track))
			|| (!BuildAtomHDLR(dst, track))
			|| (!BuildAtomMINF(dst, track))
			) {
		FATAL("Unable to write atom MDIA");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMDHD(IOBufferExt &dst, TrackContext &track) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);

	if (WRITE_ATOM_HEADER(dst, 0x6D646864)
			|| (!dst.ReadFromU32(_winQtCompat ? 0x00000000 : 0x01000000, true)) //version/flags
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //creation_time
			|| (_winQtCompat ? (!dst.ReadFromU32((uint32_t) _now, true)) : (!dst.ReadFromU64(_now, true))) //modification_time
			|| (!dst.ReadFromU32(track._timescale, true)) //timescale
			|| (_winQtCompat ? (!dst.ReadFromU32(0, true)) : (!dst.ReadFromU64(0, true))) //duration
			|| (!dst.ReadFromU16(0x55C4, true)) //language which is 'und'
			|| (!dst.ReadFromU16(0, true)) //pre_defined
			) {
		FATAL("Unable to write atom MDHD");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomHDLR(IOBufferExt &dst, TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x68646C72)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //pre_defined
			|| (!dst.ReadFromU32(track._isAudio ? 0x736F756E : 0x76696465, true))
			|| (!dst.ReadFromRepeat(0, 12)) //reserved
			|| (!dst.ReadFromRepeat(0, 1)) //the \0 for the name
			) {
		FATAL("Unable to write atom HDLR");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMINF(IOBufferExt &dst, TrackContext &track) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D696E66)
			|| (!(track._isAudio ? BuildAtomSMHD(dst) : BuildAtomVMHD(dst)))
			|| (!BuildAtomDINF(dst))
			|| (!BuildAtomSTBL(dst, track))
			) {
		FATAL("Unable to write atom MINF");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSMHD(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x736D6864)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU16(0, true)) //balance
			|| (!dst.ReadFromU16(0, true)) //reserved
			) {
		FATAL("Unable to write atom SMHD");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomVMHD(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x766D6864)
			|| (!dst.ReadFromU32(1, true)) //version/flags
			|| (!dst.ReadFromU16(0, true)) //graphicsmode
			|| (!dst.ReadFromRepeat(0, 6)) //opcolor
			) {
		FATAL("Unable to write atom VMHD");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomDINF(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x64696E66)
			|| (!BuildAtomDREF(dst))
			) {
		FATAL("Unable to write atom DINF");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomDREF(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x64726566)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(1, true)) //entry_count
			|| (!BuildAtomURL(dst))
			) {
		FATAL("Unable to write atom DREF");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomURL(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x75726C20)
			|| (!dst.ReadFromU32(1, true)) //version/flags
			) {
		FATAL("Unable to write atom URL");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}
/*OK But do we neeed to include STSS and CTTS?*/
bool FMP4Writer::BuildAtomSTBL(IOBufferExt &dst, TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x7374626C)
			|| (!BuildAtomSTSD(dst, track))
			|| (!BuildAtomSTTS(dst))
			|| (!BuildAtomSTSC(dst))
			|| (!BuildAtomSTSZ(dst))
			|| (!BuildAtomSTCO(dst))
			) {
		FATAL("Unable to write atom STBL");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSTSD(IOBufferExt &dst, TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x73747364)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(1, true)) //entry_count
			|| (!(track._isAudio ? BuildAtomMP4A(dst) : BuildAtomAVC1(dst)))
			) {
		FATAL("Unable to write atom STSD");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSTTS(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x73747473)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //entry_count
			) {
		FATAL("Unable to write atom STTS");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSTSC(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x73747363)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //entry_count
			) {
		FATAL("Unable to write atom STSC");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSTSZ(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x7374737A)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //sample_size
			|| (!dst.ReadFromU32(0, true)) //sample_count
			) {
		FATAL("Unable to write atom STSZ");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomSTCO(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x7374636F)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //entry_count
			) {
		FATAL("Unable to write atom STCO");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMP4A(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D703461)
			|| (!dst.ReadFromRepeat(0, 6)) //reserved
			|| (!dst.ReadFromU16(1, true)) //data_reference_index
			|| (!dst.ReadFromRepeat(0, 8)) //reserved
			|| (!dst.ReadFromU16(2, true)) //channelcount
			|| (!dst.ReadFromU16(16, true)) //samplesize
			|| (!dst.ReadFromU16(0, true)) //pre_defined
			|| (!dst.ReadFromU16(0, true)) //reserved
			|| (!dst.ReadFromU32(_audio._samplingRate << 16, true)) //samplerate
			|| (!BuildAtomESDS(dst))
			) {
		FATAL("Unable to write atom MP4A");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomAVC1(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x61766331)
			|| (!dst.ReadFromRepeat(0, 6)) //reserved
			|| (!dst.ReadFromU16(1, true)) //data_reference_index
			|| (!dst.ReadFromU16(0, true)) //pre_defined
			|| (!dst.ReadFromU16(0, true)) //reserved
			|| (!dst.ReadFromRepeat(0, 12)) //pre_defined
			|| (!dst.ReadFromU16((uint16_t) _video._width, true)) //width
			|| (!dst.ReadFromU16((uint16_t) _video._height, true)) //height
			|| (!dst.ReadFromU32(0x00480000, true)) //horizresolution
			|| (!dst.ReadFromU32(0x00480000, true)) //vertresolution
			|| (!dst.ReadFromU32(0, true)) //reserved
			|| (!dst.ReadFromU16(1, true)) //frame_count
			|| (!dst.ReadFromRepeat(22, 1)) //compressorname___1, a total of 32
			|| (!dst.ReadFromString("RDKC Media Server")) //compressorname___2, a total of 32
			|| (!dst.ReadFromRepeat(0, 9)) //compressorname___3, a total of 32
			|| (!dst.ReadFromU16(0x0018, true)) //depth
			|| (!dst.ReadFromU16(0xffff, true)) //pre_defined
			|| (!BuildAtomAVCC(dst))
			) {
		FATAL("Unable to write atom AVC1");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomESDS(IOBufferExt &dst) {
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
	EHTONSP(raw + 5, (uint16_t) _audio._id);
	memcpy(raw + 31, GETIBPOINTER(_audio._codecBytes), 2);

	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x65736473)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromBuffer(raw, sizeof (raw)))
			) {
		FATAL("Unable to write atom ESDS");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomAVCC(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x61766343)
			|| (!dst.ReadFromBuffer(GETIBPOINTER(_video._codecBytes), GETAVAILABLEBYTESCOUNT(_video._codecBytes)))
			) {
		FATAL("Unable to write atom AVCC");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMOOF(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D6F6F66)
			|| (!BuildAtomMFHD(dst))
			) {
		FATAL("Unable to write atom MOOF");
		return false;
	}
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomMFHD(IOBufferExt &dst) {
	uint32_t start = GETAVAILABLEBYTESCOUNT(dst);
	if (WRITE_ATOM_HEADER(dst, 0x6D666864)
			|| (!dst.ReadFromU32(0, true)) //version/flags
			|| (!dst.ReadFromU32(0, true)) //sequence_number
			) {
		FATAL("Unable to write atom MFHD");
		return false;
	}
	_offsetFragmentSequenceNumber = GETAVAILABLEBYTESCOUNT(dst) - 4;
	EHTONLP(GETIBPOINTER(dst) + start, GETAVAILABLEBYTESCOUNT(dst) - start);
	return true;
}

bool FMP4Writer::BuildAtomTRAF(TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(track._trafHeader);
	if (WRITE_ATOM_HEADER(track._trafHeader, 0x74726166)
			|| (!BuildAtomTFHD(track))
			|| (!BuildAtomTFDT(track))
			|| (!BuildAtomTRUN(track))
			) {
		FATAL("Unable to write atom TRAF");
		return false;
	}
	EHTONLP(GETIBPOINTER(track._trafHeader) + start, GETAVAILABLEBYTESCOUNT(track._trafHeader) - start);
	return true;
}

bool FMP4Writer::BuildAtomTFHD(TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(track._trafHeader);
	uint32_t versionAndFlags = 0x00000000 //version 0
			//| 0x000001 //base‐data‐offset-present // 2nd iteration don't have this
			//| 0x000002 //sample‐description‐index-present
			| 0x000008 //default‐sample‐duration-present
			| 0x000010 //default‐sample‐size-present // 2nd iteration has this because there would be a single TRUN entry
			| 0x000020 //default-sample-flags-present
			//| 0x010000 //duration‐is‐empty
			| 0x020000 //default-base-is-moof // 2nd iteration has this
			;

	if (WRITE_ATOM_HEADER(track._trafHeader, 0x74666864)
			|| (!track._trafHeader.ReadFromU32(versionAndFlags, true)) //version/flags
			|| (!track._trafHeader.ReadFromU32(track._id, true)) //track_ID
			//|| (!track._trafHeader.ReadFromU32(1, true)) //sample‐description‐index
			//|| (!track._trafHeader.ReadFromU64(0, true)) //base‐data‐offset
			|| (!track._trafHeader.ReadFromU32(0, true)) //default‐sample‐duration
			|| (!track._trafHeader.ReadFromU32(0, true)) //default‐sample‐size
			|| (!track._trafHeader.ReadFromU32(SF_SAMPLE_NON_KEY_FRAME, true)) //default-sample-flags
			) {
		FATAL("Unable to write atom TFHD");
		return false;
	}
	
	track._offset._tfhd = start;
	EHTONLP(GETIBPOINTER(track._trafHeader) + start, GETAVAILABLEBYTESCOUNT(track._trafHeader) - start);
	return true;
}

bool FMP4Writer::BuildAtomTFDT(TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(track._trafHeader);
	if (WRITE_ATOM_HEADER(track._trafHeader, 0x74666474)
			|| (!track._trafHeader.ReadFromU32(TFDT_FLAGS, true)) //version/flags
			|| (!track._trafHeader.TFDT_ReadFrom(0, true)) //baseMediaDecodeTime
			) {
		FATAL("Unable to write atom TFDT");
		return false;
	}

	track._offset._tfdtBaseMediaDecodeTime = GETAVAILABLEBYTESCOUNT(track._trafHeader) - sizeof (TFDT_TYPE);
	EHTONLP(GETIBPOINTER(track._trafHeader) + start, GETAVAILABLEBYTESCOUNT(track._trafHeader) - start);
	return true;
}

bool FMP4Writer::BuildAtomTRUN(TrackContext &track) {
	if (track._id == 0)
		return true;
	uint32_t start = GETAVAILABLEBYTESCOUNT(track._trafHeader);
	uint32_t versionAndFlags = 0x00000000 //version 0
			| 0x000001 //data-offset-present
			| 0x000004 //first‐sample‐flags-present
			//| 0x000100 //sample‐duration-present
			//| 0x000200 //sample‐size-present // not included in 2nd iteration: single sample per TRUN
			//| 0x000400 //sample‐flags-present
			| 0x000800 //sample‐composition‐time‐offsets‐present 
					   // not included in 2nd iteration: single sample per TRUN
					   // brought back for 3rd iteration.
			;

	if (WRITE_ATOM_HEADER(track._trafHeader, 0x7472756E)
			|| (!track._trafHeader.ReadFromU32(versionAndFlags, true)) //version/tr_flags
			|| (!track._trafHeader.ReadFromU32(0, true)) //sample_count
			|| (!track._trafHeader.ReadFromU32(0, true)) //data_offset
			|| (!track._trafHeader.ReadFromU32(SF_SAMPLE_KEY_FRAME, true)) //first_sample_flags
			) {
		FATAL("Unable to write atom TRUN");
		return false;
	}
	track._offset._trun = start;
	EHTONLP(GETIBPOINTER(track._trafHeader) + start, GETAVAILABLEBYTESCOUNT(track._trafHeader) - start);
	return true;
}


bool FMP4Writer::WriteData(TrackContext &track, IOBuffer &buffer, double pts,
		double dts, bool isKeyFrame) {
	if (track._id == 0)
		return true;

	double origPTS = pts;

	// Check for ACC (avigilon) issue
	double lastTs = (track._isAudio ? _lastKnownAudioDts : _lastKnownVideoDts);
	if (dts < lastTs) {
		DEBUG("Back timestamp. Current: %f, Previous: %f", dts, lastTs);
		// If keyframe, preserve sample, otherwise discard
		if (isKeyFrame) {
			dts = lastTs;
			pts = dts;
		} else {
			if (!track._isAudio) {
				_lastKnownVideoDts = dts;
				_lastKnownVideoPts = pts;
			} else {
				_lastKnownAudioDts = dts;
				_lastKnownAudioPts = pts;
			}
			return true;
		}
	}

	// This checks for RSTP-VOD issues (same timestamp)
	double duration = dts - lastTs;
	if (duration < 5) {
		//track._ignoreLastSample = true; //TODO: disable for now
	} else if (duration > 10000) { 
		// PTS/DTS spike workaround.  The 10000 value is arbitrary... but I think this is 
		// safe.  I doubt there'll be a valid real-world frame duration of 10000ms.
		lastTs = dts - track._lastDuration;
	}

	if (track._isAudio) {
		_audRcv++;
		_lastKnownAudioPts = pts;
		_lastKnownAudioDts = dts;
	} else {
		_vidRcv++;
		_lastKnownVideoPts = pts;
		_lastKnownVideoDts = dts;
	}

	if (!_initialSegmentOpened) {
		_moovStartPts = pts;
		if (!OpenSegment()) {
			FATAL("Unable to open segment");
			return false;
		}
		_initialSegmentOpened = true;
	}

	// 2nd iteration: new fragment each frame
	// We cannot just send out a new MOOV when it appears.  Instead we must wait until the in-progress
	// fragment has finished and been sent out.
	if (track._fragmentStartTs > 0) {
		// Okay -- so now we are ready to close the fragment.  IF there is a need to reset and resend
		// a MOOV, this is the time to do it!
		if (_resetter.isEnqueued) {
			// Reset, resend a MOOV, then continue as normal!
			CloseFragment();
			_resetter.isEnqueued = false;
			_resetFMP4Writer();
			Initialize(true, true, _resetter.pCap, _resetter.progressive);

			OpenSegment();
			_initialSegmentOpened = true;
			NewFragment();
		} else {
			// Standard operation... just continue as normal.
			if (!NewFragment()) {
				FATAL("Unable to create a new fragment");
				return false;
			}
		}
	}

	// Set the start of fragment timestamp if a fragment has been created already
	if (track._fragmentStartTs <= 0) {
//		track._fragmentStartTs = dts;
		track._fragmentStartTs = (dts>0)?dts:0.1;
		pts = dts;
	}

	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);

	//============ debug log purposes ================

	//if (!track._isAudio) {
	//	uint8_t tmp[4] = { 0 };
	//	memcpy(tmp, GETIBPOINTER(buffer), 4);
	//	uint32_t computedLength = ENTOHLP(tmp) + 4;
	//	if ((length != computedLength) && ((tmp[0] & 0x1F) == 1))
	//		DEBUG("[DEBUG] length=%"PRIu32"\tcomputedLength=%"PRIu32, length, computedLength);
	//}

	//================================================
	if (_progressive._enabled) {
		if (!WriteProgressiveBuffer(track._isAudio ? PBT_AUDIO_BUFFER : PBT_VIDEO_BUFFER,
				GETIBPOINTER(buffer), length)) {
			FATAL("Unable to write progressive buffer");
			return false;
		}

		(track._isAudio ? _progressive._mdatAudioSize : _progressive._mdatVideoSize) += length;
	} else {
		track._mdat.ReadFromBuffer(GETIBPOINTER(buffer), length);
	}

	// Get the longest duration as default
	if (track._lastDuration > track._defaultSampleDuration) {
		track._defaultSampleDuration = track._lastDuration;
	}
	
	track._sampleSize = length; // 2nd iteration

	track._offset._lastSampleDuration = GETAVAILABLEBYTESCOUNT(track._trunBuffer);
	//track._trunBuffer.ReadFromU32(track._lastDuration, true);
	//track._trunBuffer.ReadFromU32(length, true); // 2nd iteration
	//track._trunBuffer.ReadFromU32(isKeyFrame ? SF_SAMPLE_KEY_FRAME : SF_SAMPLE_NON_KEY_FRAME, true);
	track._trunBuffer.ReadFromU32((uint32_t) (origPTS - dts), true); // 2nd iteration

	//if (isKeyFrame)
		//_dtsOffset = 0;
	track._lastDuration = (uint32_t)(dts - lastTs);
	//FATAL("Duration: %u, dts: %lf, lastdts: %lf, offset: %lf", track._lastDuration, dts, lastTs, _dtsOffset);

	//Handling of bloated dts's during resume in inbound vod streams
	if (track._isAudio) {
		RemoveAudioDtsSpikes(dts, lastTs, track);
	} else {
		RemoveVideoDtsSpikes(dts, lastTs, track);
	}
	/*if (_dtsOffset != 0) {
		//INFO("DTS Offset caused by pause/resume: %lf", _dtsOffset);
		_dtsOffset = 0;
	}*/

	track._totalDuration += track._lastDuration;
	track._samplesCount++;
	return true;
}


// OpenSegment : This is the start of all things!  Creates the FTYP and MOOV atoms
//               (1) Create the FTYP atom
//               (2) Create the MOOV atom
bool FMP4Writer::OpenSegment() {
	// Indicate presence of video and audio on the moov buffer
	if (_progressive._enabled) {
		uint8_t capab = 0xF0;

		if (_video._id > 0) {
			capab |= 0x01;
		}

		if (_audio._id > 0) {
			capab |= 0x02;
		}

		_moov.ReadFromByte(capab);

		if (_video._id > 0) {
			// Get the video codec bytes and pass to player
			uint8_t *pCodec = GETIBPOINTER(_video._codecBytes);
			_moov.ReadFromByte(pCodec[1]);
			_moov.ReadFromByte(pCodec[2]);
			_moov.ReadFromByte(pCodec[3]);
		} else {
			_moov.ReadFromRepeat(0, 3);
		}
	}

	return BuildAtomFTYP(_moov)
			&& BuildAtomMOOV(_moov)
			&& (_progressive._enabled ? true : BeginWriteMOOV())
			&& (_progressive._enabled ? WriteProgressiveBuffer(PBT_HEADER_MOOV, GETIBPOINTER(_moov), GETAVAILABLEBYTESCOUNT(_moov)) : WriteBuffer(WS_MOOV, GETIBPOINTER(_moov), GETAVAILABLEBYTESCOUNT(_moov)))
			&& (_progressive._enabled ? true : EndWriteMOOV(_offsetDuration, false))
			&& OpenFragment()
			;
}

uint32_t FMP4Writer::GetTracksCount() {
	uint32_t result = 0;
	if (_audio._id != 0)
		result++;
	if (_video._id != 0)
		result++;
	return result;
}

bool FMP4Writer::OpenFragment() {
	//FINEST(" Open fragment");
	_fragmentSequenceNumber++;
	if (GETAVAILABLEBYTESCOUNT(_moof) == 0) {
		if (!BuildAtomMOOF(_moof)) {
			FATAL("Writing MOOF atom failed");
			return false;
		}
	}
	//	FINEST("%s", STR(hex(GETIBPOINTER(_moof), GETAVAILABLEBYTESCOUNT(_moof))));
	//	FINEST("_moof:\n%s", STR(_moof));
	return _progressive._enabled ? WriteProgressiveBuffer(PBT_BEGIN, NULL, 0) : true;
}

bool FMP4Writer::CloseFragment() {
	uint32_t tempVidId = _video._id;
	uint32_t tempAudId = _audio._id;
	//if (_vidRcv == 0) {
	////if (_video._fragmentStartTs < 0 ) {
	//	_video._id = 0;
	//}
	//if (_audRcv == 0) {
	////if (_audio._fragmentStartTs < 0 ) {
	//	_audio._id = 0;
	//}

	if (_video._fragmentStartTs <= 0) {
		_video._id = 0;
	}else {
		--_vidRcv;
	}
	if (_audio._fragmentStartTs <= 0) {
		_audio._id = 0;
	}else {
		--_audRcv;
	}

	//FINEST("Close fragment");
	uint8_t *pMoof = GETIBPOINTER(_moof);
	uint32_t moofLength = GETAVAILABLEBYTESCOUNT(_moof);

	//fragment sequence number
	EHTONLP(pMoof + _offsetFragmentSequenceNumber, _fragmentSequenceNumber);

	if ((!ComputeSize(_video))
			|| (!ComputeSize(_audio))) {
		FATAL("Unable to compute tracks sizes");
		return false;
	}

	//moof
	uint32_t moofSize = _audio._trafSize + _video._trafSize + moofLength;
	EHTONLP(pMoof, moofSize);
	
	uint32_t currentMoofOffset = moofSize;

	//mdat header
	_mdat = (_progressive._enabled ? (_progressive._mdatVideoSize + _progressive._mdatAudioSize) : (GETAVAILABLEBYTESCOUNT(_video._mdat) + GETAVAILABLEBYTESCOUNT(_audio._mdat))) + 8;
	currentMoofOffset += uint32_t(_mdat);
	_mdat = EHTONLL((_mdat << 32) | 0x6D646174);

	if ((!CloseFragmentTrack(_video))
			|| (!CloseFragmentTrack(_audio))) {
		FATAL("Unable to close tracks");
		return false;
	}
	
	// Update moofOffset
	_moofOffset += currentMoofOffset;

	if (_progressive._enabled) {
		if ((!WriteProgressiveBuffer(PBT_HEADER_MOOF, pMoof, moofLength))
				|| (!WriteTrackBuffers(_video))
				|| (!WriteTrackBuffers(_audio))
				|| (!WriteProgressiveBuffer(PBT_HEADER_MDAT, (uint8_t *) & _mdat, 8))
				|| (!WriteProgressiveBuffer(PBT_END, NULL, 0))
				) {
			FATAL("Unable to close tracks");
			return false;
		}
		_progressive._mdatAudioSize = 0;
		_progressive._mdatVideoSize = 0;
	} else {
		double moofStartPts = -1;
		if (_video._id > 0) {
			moofStartPts = moofStartPts < 0 ? _video._fragmentStartTs : moofStartPts;
			moofStartPts = moofStartPts > _video._fragmentStartTs ? _video._fragmentStartTs : moofStartPts;
		}
		if (_audio._id > 0) {
			moofStartPts = moofStartPts < 0 ? _audio._fragmentStartTs : moofStartPts;
			moofStartPts = moofStartPts > _audio._fragmentStartTs ? _audio._fragmentStartTs : moofStartPts;
		}

		double lastTs = (_lastKnownVideoDts > _lastKnownAudioDts) ? _lastKnownVideoDts : _lastKnownAudioDts;
		if ((!BeginWriteFragment())
				|| (!WriteBuffer(WS_MOOF, pMoof, moofLength))
				|| (!WriteTrackBuffers(_video))
				|| (!WriteTrackBuffers(_audio))
				|| (!WriteBuffer(WS_MOOF, (uint8_t *) & _mdat, 8))
				|| (!WriteBuffer(WS_MOOF, GETIBPOINTER(_video._mdat), GETAVAILABLEBYTESCOUNT(_video._mdat)))
				|| (!WriteBuffer(WS_MOOF, GETIBPOINTER(_audio._mdat), GETAVAILABLEBYTESCOUNT(_audio._mdat)))
				|| (!EndWriteFragment(_moovStartPts, lastTs - _moovStartPts, moofStartPts))
				) {
			FATAL("Unable to close tracks");
			return false;
		}
	}
	//	FINEST("**** A: %"PRIu32"; V: %"PRIu32, _audio._samplesCount, _video._samplesCount);
	//	FINEST("%s", STR(hex(GETIBPOINTER(_moof), GETAVAILABLEBYTESCOUNT(_moof))));
	//	FINEST("_moof:\n%s", STR(_moof));

	_audio.Reset();
	_video.Reset();
	_vidRcv = _audRcv = 0;
	_video._id = tempVidId;
	_audio._id = tempAudId;

	if (_isPlayerPaused) {
		_stopOutbound = true;
	}
	return true;
}

bool FMP4Writer::ComputeSize(TrackContext &track) {
	if (track._id == 0) {
		track._trunSize = 0;
		track._trafSize = 0;
		return true;
	}
	// 24 is the TRUN header size
	track._trunSize = track._samplesCount * track._trunEntrySize + 24;
	track._trafSize = track._offset._trun + track._trunSize;
	return true;
}

bool FMP4Writer::CloseFragmentTrack(TrackContext &track) {
	if (track._id == 0)
		return true;

	if (track._fragmentStartTs < 0) {
		FATAL("Fragment is empty");
		return false;
	}

	uint8_t *pTrafHeader = GETIBPOINTER(track._trafHeader);

	TFDT_TYPE tfdtBaseMediaDecodeTime = 0;

	// TFHD
	//EHTONLLP(pTrafHeader + track._offset._tfhd + 16, _moofOffset); // base data offset (MOOF location)
	//EHTONLP(pTrafHeader + track._offset._tfhd + 24, track._defaultSampleDuration); // default sample duration
	// 2nd iteration
	EHTONLP(pTrafHeader + track._offset._tfhd + 16, u_long(track._totalDuration)); // default sample duration
	//INFO("total Duration: %u", u_long(track._totalDuration));
	EHTONLP(pTrafHeader + track._offset._tfhd + 20, track._sampleSize); // default sample size

	// TFDT
	double& decodeTs = track._isAudio ? _startAudioTimeTicks : _startVideoTimeTicks;
	tfdtBaseMediaDecodeTime = (TFDT_TYPE) decodeTs;
	TFDT_HTON(pTrafHeader + track._offset._tfdtBaseMediaDecodeTime, tfdtBaseMediaDecodeTime);

	// TRUN
	uint32_t dataOffset = _audio._trafSize + _video._trafSize + GETAVAILABLEBYTESCOUNT(_moof) + 8;
	if (track._isAudio)
		dataOffset += (uint32_t) (_progressive._enabled ? _progressive._mdatVideoSize : GETAVAILABLEBYTESCOUNT(_video._mdat));

	EHTONLP(pTrafHeader + track._offset._trun, track._trunSize); //the size of TRUN atom
	if (track._ignoreLastSample) {
		track._samplesCount--;
	}
	EHTONLP(pTrafHeader + track._offset._trun + 12, track._samplesCount); //sample_count
	EHTONLP(pTrafHeader + track._offset._trun + 16, dataOffset); //data_offset

	if (track._ignoreLastSample) {
		track._lastDuration = 0;
	}

	// Adjust the last sample duration to match the next fragment timestamp and 
	// prevent time gaps. This happens normally on RTSP sources.
	//TODO: disabled for now since we no longer have duration per sample
	//uint8_t *pTrun = GETIBPOINTER(track._trunBuffer);
	//EHTONLP(pTrun + track._offset._lastSampleDuration, track._lastDuration);

	// TRAF
	EHTONLP(pTrafHeader, track._trafSize);

	decodeTs += track._totalDuration;

	return true;
}

bool FMP4Writer::WriteTrackBuffers(TrackContext &track) {
	if (track._id == 0)
		return true;
	if (_progressive._enabled) {
		return WriteProgressiveBuffer(PBT_HEADER_MOOF, GETIBPOINTER(track._trafHeader), GETAVAILABLEBYTESCOUNT(track._trafHeader))
				&& WriteProgressiveBuffer(PBT_HEADER_MOOF, GETIBPOINTER(track._trunBuffer), GETAVAILABLEBYTESCOUNT(track._trunBuffer))
				;
	} else {
		return WriteBuffer(WS_MOOF, GETIBPOINTER(track._trafHeader), GETAVAILABLEBYTESCOUNT(track._trafHeader))
				&& WriteBuffer(WS_MOOF, GETIBPOINTER(track._trunBuffer), GETAVAILABLEBYTESCOUNT(track._trunBuffer))
				;
	}
}

bool FMP4Writer::NewFragment() {
	return CloseFragment() && OpenFragment();
}

void FMP4Writer::RemoveVideoDtsSpikes(double dts, double lastTs, TrackContext &track) {
	double dtsDelta = dts - lastTs;
	//don't store dts if it is too large or too small (outlier)
	//store dts whatever its value if the number of dts delta samples is still too small
	if (_aveVideoDtsDelta == -1
		|| _videoDtsDeltas.size() < MIN_DTSDELTAS_SIZE ||
		(dtsDelta > (0.5 * _aveVideoDtsDelta) && dtsDelta < (MAX_DTSFLUCTUATION + _aveVideoDtsDelta))) {

		_videoDtsDeltas.push_front(dtsDelta);
		if (_videoDtsDeltas.size() > MAX_DTSDELTAS_SIZE)
			_videoDtsDeltas.pop_front();

		//compute for the new average dts delta
		deque<double> dtsDeltaTemp = _videoDtsDeltas;
		double sum = 0;
		while (!dtsDeltaTemp.empty()) {
			sum += dtsDeltaTemp.front();
			dtsDeltaTemp.pop_front();
		}
		_aveVideoDtsDelta = (double)(sum / (double)_videoDtsDeltas.size());
	} else if (dtsDelta >= MAX_DTSFLUCTUATION + _aveVideoDtsDelta) {
		//if the delta dts is greater than 2x of ave delta dts
		if (_inboundVod) {
			_dtsOffset = dts - _aveVideoDtsDelta;
			track._lastDuration = (uint32_t)(dts - _dtsOffset);
		} else {
			track._lastDuration = (uint32_t) _aveVideoDtsDelta;
			//_dtsOffset = 0;
		}
	}
}

void FMP4Writer::RemoveAudioDtsSpikes(double dts, double lastTs, TrackContext &track) {
	double dtsDelta = dts - lastTs;
	//don't store dts if it is too large or too small (outlier)
	//store dts whatever its value if the number of dts delta samples is still too small
	if (_aveAudioDtsDelta == -1
		|| _audioDtsDeltas.size() < MIN_DTSDELTAS_SIZE ||
		(dtsDelta > (0.5 * _aveVideoDtsDelta) && dtsDelta < (MAX_DTSFLUCTUATION + _aveAudioDtsDelta))) {

		_audioDtsDeltas.push_front(dtsDelta);
		if (_audioDtsDeltas.size() > MAX_DTSDELTAS_SIZE)
			_audioDtsDeltas.pop_front();

		//compute for the new average dts delta
		deque<double> dtsDeltaTemp = _audioDtsDeltas;
		double sum = 0;
		while (!dtsDeltaTemp.empty()) {
			sum += dtsDeltaTemp.front();
			dtsDeltaTemp.pop_front();
		}
		_aveAudioDtsDelta = (double)(sum / (double)_audioDtsDeltas.size());
	} else if (dtsDelta >= MAX_DTSFLUCTUATION + _aveAudioDtsDelta) {
		//if the delta dts is greater than 2x of ave delta dts
		if (_inboundVod) {
			_dtsOffset = dts - _aveAudioDtsDelta;
			track._lastDuration = (uint32_t)(dts - _dtsOffset);
		}
		else {
			track._lastDuration = (uint32_t) _aveAudioDtsDelta;
			//_dtsOffset = 0;
		}
	}
}

void FMP4Writer::ResetDtsValues() {
	_videoDtsDeltas.clear();
	_audioDtsDeltas.clear();
	_aveVideoDtsDelta = -1;
	_aveAudioDtsDelta = -1;
}
