/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/


#include "streaming/baseoutstream.h"
#include "streaming/baseoutdevicestream.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolmanager.h"
#include "streaming/streamstypes.h"

#define MMAP_MIN_WINDOW_SIZE (64*1024)
#define MMAP_MAX_WINDOW_SIZE (1024*1024)

//#define __HAS_DEBUG_MESSAGES
#ifdef __HAS_DEBUG_MESSAGES
#define SHOW_FEED_DBG() FINEST("\n" \
					"--------------------\n" \
					"               name: %s [%.2f - %.2f]\n" \
					"        frame index: %"PRId64"/%"PRIu32"\n" \
					"                dts: %.4f/%.4f\n" \
					"        buffer size: %"PRIu32"/%"PRIu32"\n" \
					"Feed session length: %.2f/%.2f\n" \
					"         Fed frames: %"PRIu32"\n" \
					"         Single GOP: %d", \
					STR(GetName()), \
					_playbackContext._playbackStartDts, \
					_playbackContext._playbackEndDts, \
					_playbackContext._playbackCurrentFrameIndex, \
					_seekOffsets.totalframes, \
					_playbackContext._lastFrameDts / 1000.0, \
					_playbackContext._playbackEndDts / 1000.0, \
					currentOutputBufferSize, \
					_maxOutputDataSize, \
					_playbackContext._feedSessionLengthCurrent, \
					_playbackContext._feedSessionLengthMax, \
					fedFrames, \
					_playbackContext._singleGop)
#define SHOW_PLAYBACK_CONTEXT() FINEST("Playback context:\n%s", STR(_playbackContext))
#else /* __HAS_DEBUG_MESSAGES */
#define SHOW_FEED_DBG()
#define SHOW_PLAYBACK_CONTEXT()
#endif /* __HAS_DEBUG_MESSAGES */

BaseOutDeviceStream::OutDeviceStreamTimer::OutDeviceStreamTimer(BaseOutDeviceStream *pOutDeviceStream) {
	_pOutDeviceStream = pOutDeviceStream;
}

BaseOutDeviceStream::OutDeviceStreamTimer::~OutDeviceStreamTimer() {

}

void BaseOutDeviceStream::OutDeviceStreamTimer::ResetStream() {
	_pOutDeviceStream = NULL;
}

bool BaseOutDeviceStream::OutDeviceStreamTimer::TimePeriodElapsed() {
    //uint64_t run_start, run_end;
    //GETMILLISECONDS( run_start );
	if (_pOutDeviceStream != NULL) {
		_pOutDeviceStream->ReadyForSend();
	}
    //GETMILLISECONDS( run_end );
    //FATAL("IT TOOK THIS MANY MILLISECONDS TO TimePeriodElapsed: %05"PRIu64, (run_end-run_start) );
	return true;
}

BaseOutDeviceStream::BaseOutDeviceStream(BaseProtocol *pProtocol, uint64_t type,
		string name)
: BaseOutStream(pProtocol, type, name) {
	_timerId = 0;
	_frameDuration = 33.33;
//	_frameDuration = 30000;
	_maxOutputDataSize = 512 * 1024;
	_canFeed = false;
	_hasAudio = false;
	_playing = false;
}

BaseOutDeviceStream::~BaseOutDeviceStream() {
	DisarmTimer();
}

StreamCapabilities * BaseOutDeviceStream::GetCapabilities() {
	return &_streamCapabilities;
}

void BaseOutDeviceStream::SingleFrame(bool value) {
	_singleframe = value;
}

bool BaseOutDeviceStream::Initialize(Variant &settings) {
/*	_frameRate = (double) settings.GetValue("frameRate", false);
	if (_frameRate == 0) {
		FATAL("Cannot initialize stream. Frame Rate should not be zero");
		return false;
	}
	_frameDuration = 1 / _frameRate;*/
	return true;
}

bool BaseOutDeviceStream::SignalPlay(double &dts, double &length) {
	//1. Setup the bounds of the playback
	V4L2_LOG_FINE("Signalling play");
	//5. Arm the timer
	_canFeed = true;
	ArmTimer(_frameDuration);

	//7. Done
	return true;
}

bool BaseOutDeviceStream::SignalPause() {
	_canFeed = false;
	DisarmTimer();
	return true;
}

bool BaseOutDeviceStream::SignalResume() {
	_canFeed = true;
	ArmTimer(_frameDuration);
	return true;
}

bool BaseOutDeviceStream::SignalSeek(double &dts) {
	return true;
}

bool BaseOutDeviceStream::SignalStop() {
	_canFeed = false;
	DisarmTimer();
	return true;
}

void BaseOutDeviceStream::ReadyForSend() {
	//1. Test if we can call Feed
//	V4L2_LOG_WARN("Function entry");
#if 0
	if (!_canFeed)
		return;

	if (!Feed()) {
		V4L2_LOG_FINE("Error Feeding");
		if (_outStreams.Size() != 0)
			_outStreams.MoveHead()->value->EnqueueForDelete();
	}
#endif
	NYI
}


int64_t BaseOutDeviceStream::TimeToFrame(double dts, MediaFrame &frame) {
	//1. compute the index in the time2frameindex and seek to that value
	uint32_t frameIndex = 0;
	return frameIndex;
}


void BaseOutDeviceStream::ArmTimer(double period) {
	if (_playing)
		return;
	_playing = true;
	_canFeed = true;
	DisarmTimer();
	OutDeviceStreamTimer *pTemp = new OutDeviceStreamTimer(this);
	V4L2_LOG_FINE("Arming Timer for %.6f", period);
	pTemp->EnqueueForHighGranularityTimeEvent((uint32_t) period);
	_timerId = pTemp->GetId();
}

void BaseOutDeviceStream::DisarmTimer() {
	OutDeviceStreamTimer *pTemp = (OutDeviceStreamTimer *) ProtocolManager::GetProtocol(
			_timerId);
	_timerId = 0;
	if (pTemp == NULL)
		return;
	_playing = false;
	pTemp->ResetStream();
	pTemp->EnqueueForDelete();
}

void BaseOutDeviceStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
}

void BaseOutDeviceStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
}

bool BaseOutDeviceStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	NYIR;
}

bool BaseOutDeviceStream::IsCompatibleWithType(uint64_t type) {
	return type == ST_IN_NET_RTMP
			|| type == ST_IN_NET_TS
			|| type == ST_IN_NET_RTP
			|| type == ST_IN_NET_LIVEFLV
			|| type == ST_IN_FILE_RTSP
			|| type == ST_IN_FILE_TS
			|| type == ST_IN_NET_RAW
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| type == ST_IN_NET_EXT
                        ;
	//return TAG_KIND_OF(type, ST_OUT);
}
