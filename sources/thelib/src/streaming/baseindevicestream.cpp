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


#include "streaming/baseoutstream.h"
#include "streaming/baseindevicestream.h"
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

BaseInDeviceStream::InDeviceStreamTimer::InDeviceStreamTimer(BaseInDeviceStream *pInDeviceStream) {
	_pInDeviceStream = pInDeviceStream;
}

BaseInDeviceStream::InDeviceStreamTimer::~InDeviceStreamTimer() {

}

void BaseInDeviceStream::InDeviceStreamTimer::ResetStream() {
	_pInDeviceStream = NULL;
}

bool BaseInDeviceStream::InDeviceStreamTimer::TimePeriodElapsed() {
    //uint64_t run_start, run_end;
    //GETMILLISECONDS( run_start );
	if (_pInDeviceStream != NULL) {
		_pInDeviceStream->ReadyForSend();
	}
    //GETMILLISECONDS( run_end );
    //FATAL("IT TOOK THIS MANY MILLISECONDS TO TimePeriodElapsed: %05"PRIu64, (run_end-run_start) );
	return true;
}

BaseInDeviceStream::BaseInDeviceStream(BaseProtocol *pProtocol, uint64_t type,
		string name)
: BaseInStream(pProtocol, type, name) {
	_timerId = 0;
	_frameDuration = 33.33;
//	_frameDuration = 30000;
	_maxOutputDataSize = 512 * 1024;
	_canFeed = false;
	_hasAudio = false;
	_playing = false;
}

BaseInDeviceStream::~BaseInDeviceStream() {
	DisarmTimer();
}

StreamCapabilities * BaseInDeviceStream::GetCapabilities() {
	return &_streamCapabilities;
}

void BaseInDeviceStream::SingleFrame(bool value) {
	_singleframe = value;
}

bool BaseInDeviceStream::Initialize(Variant &settings) {
/*	_frameRate = (double) settings.GetValue("frameRate", false);
	if (_frameRate == 0) {
		FATAL("Cannot initialize stream. Frame Rate should not be zero");
		return false;
	}
	_frameDuration = 1 / _frameRate;*/
	return true;
}

bool BaseInDeviceStream::SignalPlay(double &dts, double &length) {
	//1. Setup the bounds of the playback
	V4L2_LOG_FINE("Signalling play");
	//5. Arm the timer
	_canFeed = true;
	ArmTimer(_frameDuration);

	//7. Done
	return true;
}

bool BaseInDeviceStream::SignalPause() {
	_canFeed = false;
	DisarmTimer();
	return true;
}

bool BaseInDeviceStream::SignalResume() {
	_canFeed = true;
	ArmTimer(_frameDuration);
	return true;
}

bool BaseInDeviceStream::SignalSeek(double &dts) {
	return true;
}

bool BaseInDeviceStream::SignalStop() {
	_canFeed = false;
	DisarmTimer();
	return true;
}

void BaseInDeviceStream::ReadyForSend() {
	//1. Test if we can call Feed
//	V4L2_LOG_WARN("Function entry");
	if (!_canFeed)
		return;

	if (!Feed()) {
		V4L2_LOG_FINE("Error Feeding");
		if (_outStreams.Size() != 0)
			_outStreams.MoveHead()->value->EnqueueForDelete();
	}
}


int64_t BaseInDeviceStream::TimeToFrame(double dts, MediaFrame &frame) {
	//1. compute the index in the time2frameindex and seek to that value
	uint32_t frameIndex = 0;
	return frameIndex;
}


void BaseInDeviceStream::ArmTimer(double period) {
	if (_playing)
		return;
	_playing = true;
	_canFeed = true;
	DisarmTimer();
	InDeviceStreamTimer *pTemp = new InDeviceStreamTimer(this);
	V4L2_LOG_FINE("Arming Timer for %.6f", period);
	pTemp->EnqueueForHighGranularityTimeEvent((uint32_t) period);
	_timerId = pTemp->GetId();
}

void BaseInDeviceStream::DisarmTimer() {
	InDeviceStreamTimer *pTemp = (InDeviceStreamTimer *) ProtocolManager::GetProtocol(
			_timerId);
	_timerId = 0;
	if (pTemp == NULL)
		return;
	_playing = false;
	pTemp->ResetStream();
	pTemp->EnqueueForDelete();
}

void BaseInDeviceStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
}

void BaseInDeviceStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
}

bool BaseInDeviceStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	NYIR;
}

bool BaseInDeviceStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT);
}
