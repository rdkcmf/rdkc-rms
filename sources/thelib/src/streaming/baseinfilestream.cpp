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
#include "streaming/baseinfilestream.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolmanager.h"

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

BaseInFileStream::InFileStreamTimer::InFileStreamTimer(BaseInFileStream *pInFileStream) {
	_pInFileStream = pInFileStream;
}

BaseInFileStream::InFileStreamTimer::~InFileStreamTimer() {

}

void BaseInFileStream::InFileStreamTimer::ResetStream() {
	_pInFileStream = NULL;
}

bool BaseInFileStream::InFileStreamTimer::TimePeriodElapsed() {
	if (_pInFileStream != NULL) {
		if (_pInFileStream->_playbackContext._feedSessionLengthCurrent >= _pInFileStream->_playbackContext._feedSessionLengthMax) {
			_pInFileStream->_playbackContext._feedSessionLengthCurrent = 0;
			_pInFileStream->_playbackContext._feedSessionLengthMax = _pInFileStream->_clientSideBuffer;
		}
		_pInFileStream->ReadyForSend();
	}
	return true;
}

BaseInFileStream::BaseInFileStream(BaseProtocol *pProtocol, uint64_t type,
		string name)
: BaseInStream(pProtocol, type, name) {
	_timerId = 0;
	_pSeekFile = NULL;
	_pMediaFile = NULL;
	_seekOffsets.Reset();
	_realTime = false;
	_collapseBackwardsTimestamps = true;
	_clientSideBuffer = 1000;
	_startupTime = -1;
	_playbackContext.Reset(_clientSideBuffer);
	_maxOutputDataSize = 512 * 1024;
	_canFeed = false;
}

BaseInFileStream::~BaseInFileStream() {
	DisarmTimer();
	ReleaseFile(_pSeekFile);
	_pSeekFile = NULL;
	ReleaseFile(_pMediaFile);
	_pMediaFile = NULL;
	_seekOffsets.Reset();
}

void BaseInFileStream::SetClientSideBuffer(double clientSideBuffer, bool realTime) {
	_realTime = realTime;
	_clientSideBuffer = clientSideBuffer <= 0 ? 1000 : clientSideBuffer;
	_playbackContext._feedSessionLengthMax = 2 * _clientSideBuffer;
	_playbackContext._feedSessionLengthCurrent = 0;
	ArmTimer(_clientSideBuffer);
	if (!_playbackContext._greedyMode)
		ReadyForSend();
}

StreamCapabilities * BaseInFileStream::GetCapabilities() {
	return &_streamCapabilities;
}

Metadata &BaseInFileStream::GetCompleteMetadata() {
	return _metadata;
}

const MediaFrame & BaseInFileStream::GetCurrentFrame() {
	return _playbackContext._currentFrame;
}

void BaseInFileStream::SingleGop(bool value) {
	_playbackContext._singleGop = value;
}

bool BaseInFileStream::Initialize(Metadata &metadata) {
	_metadata = metadata;
	//1. Check to see if we have an universal seeking file
	string seekFilePath = _metadata.seekFileFullPath();
	if (!fileExists(seekFilePath)) {
		FATAL("Seek file not found: %s", STR(seekFilePath));
		return false;
	}

	//2. Open the seek file
	_pSeekFile = GetFile(seekFilePath, 128 * 1024);
	if (_pSeekFile == NULL) {
		FATAL("Unable to open seeking file: %s", STR(seekFilePath));
		return false;
	}

	//3. read stream capabilities
	uint32_t streamCapabilitiesSize = 0;
	IOBuffer raw;
	if (!_pSeekFile->ReadUI32(&streamCapabilitiesSize)) {
		FATAL("Unable to read stream Capabilities Size");
		return false;
	}
	if (streamCapabilitiesSize > 0x01000000) {
		FATAL("Unable to deserialize stream capabilities");
		return false;
	}

	if (!raw.ReadFromFs(*_pSeekFile, streamCapabilitiesSize)) {
		FATAL("Unable to read stream capabilities for stream %s", STR(*this));
		return false;
	}

	if (!_streamCapabilities.Deserialize(raw, this)) {
		FATAL("Unable to deserialize stream Capabilities. Please delete %s and %s files so they can be regenerated",
				STR(GetName() + "."MEDIA_TYPE_SEEK),
				STR(GetName() + "."MEDIA_TYPE_META));
		return false;
	}

	//4. compute part of the offsets collection
	_seekOffsets.base = _pSeekFile->Cursor();
	_seekOffsets.frames = _seekOffsets.base + 4;


	//5. Compute the optimal window size by reading the biggest frame size
	//from the seek file.
	if (!_pSeekFile->SeekTo(_pSeekFile->Size() - 8)) {
		FATAL("Unable to seek to %"PRIu64" position", _pSeekFile->Cursor() - 8);
		return false;
	}
	uint64_t maxFrameSize = 0;
	if (!_pSeekFile->ReadUI64(&maxFrameSize)) {
		FATAL("Unable to read max frame size");
		return false;
	}
	if (!_pSeekFile->SeekBegin()) {
		FATAL("Unable to seek to beginning of the file");
		return false;
	}

	//6. Compute the window size for the media file
	uint32_t windowSize = (uint32_t) maxFrameSize * 16;
	windowSize = windowSize == 0 ? MMAP_MAX_WINDOW_SIZE : windowSize;
	windowSize = windowSize < MMAP_MIN_WINDOW_SIZE ? MMAP_MIN_WINDOW_SIZE : windowSize;
	windowSize = (windowSize > MMAP_MAX_WINDOW_SIZE) ? (windowSize / 2) : windowSize;

	//7. Open the media file
	_pMediaFile = GetFile(GetName(), windowSize);
	if (_pMediaFile == NULL) {
		FATAL("Unable to open media file %s for stream %s", STR(GetName()), STR(*this));
		return false;
	}

	//8. Read the frames count from the file
	if (!_pSeekFile->SeekTo(_seekOffsets.base)) {
		FATAL("Unable to seek to _seekBaseOffset: %"PRIu64, _seekOffsets.base);
		return false;
	}
	if (!_pSeekFile->ReadUI32(&_seekOffsets.totalframes)) {
		FATAL("Unable to read the frames count");
		return false;
	}

	//9. Compute the offset of the timeToIndex table
	_seekOffsets.time2index = _seekOffsets.frames
			+ _seekOffsets.totalframes * sizeof (MediaFrame);

	//10. Read the seek file sampling rate and adjust _timeToIndexOffset
	if (!_pSeekFile->SeekTo(_seekOffsets.time2index)) {
		FATAL("Failed to seek to ms->FrameIndex table");
		return false;
	}

	if (!_pSeekFile->ReadUI32(&_seekOffsets.samplintRate)) {
		FATAL("Unable to read the frames per second");
		return false;
	}
	_seekOffsets.time2index += 4;

	//12. Read the endDts
	MediaFrame frame;
	if ((!_pSeekFile->SeekTo(_seekOffsets.frames + (_seekOffsets.totalframes - 1) * sizeof (MediaFrame)))
			|| (!_pSeekFile->ReadBuffer((uint8_t *) & frame, sizeof (MediaFrame)))) {
		FATAL("Unable to read the end dts");
		return false;
	}
	_seekOffsets.endDts = frame.dts;

	//11. Update the stats
	if ((GetProtocol() != NULL)&&(GetProtocol()->GetApplication() != NULL))
		GetProtocol()->GetApplication()->GetStreamMetadataResolver()->UpdateStats(
			_metadata.mediaFullPath(),
			_metadata.statsFileFullPath(),
			STATS_OPERATION_INCREMENT_OPEN_COUNT, 1);

	//13. Done
	return true;
}

bool BaseInFileStream::SignalPlay(double &dts, double &length) {
	//1. Setup the bounds of the playback
	SetupPlaybackBounds(dts, length);

	//2. Enable feeding
	_canFeed = true;

	//3. Setup the feed session length
	_playbackContext._feedSessionLengthMax = 2 * _clientSideBuffer;
	_playbackContext._feedSessionLengthCurrent = 0;
	_playbackContext._lastFrameDts = -1;
	SHOW_PLAYBACK_CONTEXT();

	//4. reset the startup time
	GETCLOCKS(_startupTime, double);
	_startupTime = _startupTime / (double) CLOCKS_PER_SECOND * 1000.0;

	//5. Arm the timer
	ArmTimer(_clientSideBuffer);

	//7. Done
	return true;
}

bool BaseInFileStream::SignalPause() {
	_canFeed = false;
	DisarmTimer();
	return true;
}

bool BaseInFileStream::SignalResume() {
	_canFeed = true;
	ArmTimer(_clientSideBuffer);
	return true;
}

void BaseInFileStream::ReadyForSend() {
	//1. Test if we can call Feed
	if (!_canFeed)
		return;

	//2. Call feed
	if (_realTime) {
		if (!FeedRealTime()) {
			if (_outStreams.Size() != 0)
				_outStreams.MoveHead()->value->EnqueueForDelete();
		}
	} else {
		if (!FeedBursts()) {
			if (_outStreams.Size() != 0)
				_outStreams.MoveHead()->value->EnqueueForDelete();
		}
	}
}

bool BaseInFileStream::FeedAllFile() {
	_playbackContext._greedyMode = true;
	double start = 0;
	double length = _seekOffsets.endDts * 1000;
	SetClientSideBuffer(length, false);
	return SignalPlay(start, length)
			&& FeedBursts();
}

void BaseInFileStream::SetupPlaybackBounds(double &startDts, double &duration) {
	//1. Normalize the input vales and setup the playback session
	_playbackContext._playbackStartDts = startDts < 0 ? 0 : startDts;
	_playbackContext._playbackDuration = duration < 0 ? -1 : duration;
	_playbackContext._playbackEndDts = _playbackContext._playbackDuration < 0
			? _seekOffsets.endDts
			: _playbackContext._playbackStartDts + _playbackContext._playbackDuration;

	//2. Transform the start/stop timestamps into frame indexes
	MediaFrame startFrame;
	_playbackContext._playbackStartFrameIndex = TimeToFrame(
			_playbackContext._playbackStartDts, startFrame);
	_playbackContext._playbackCurrentFrameIndex = _playbackContext._playbackStartFrameIndex;
}

int64_t BaseInFileStream::TimeToFrame(double dts, MediaFrame &frame) {
	//1. compute the index in the time2frameindex and seek to that value
	uint32_t frameIndex = 0;
	double temp = dts / (double) _seekOffsets.samplintRate;
	uint32_t tableIndex = 0;
	if ((temp - (uint32_t) temp) != 0)
		tableIndex = (uint32_t) temp + 1;
	else
		tableIndex = (uint32_t) temp;

	if ((_seekOffsets.time2index + tableIndex * 4)>(_pSeekFile->Size() - 12)) {
		if (!_pSeekFile->SeekTo(_pSeekFile->Size() - 12))
			return -1;
	} else {
		if (!_pSeekFile->SeekTo(_seekOffsets.time2index + tableIndex * 4))
			return -1;
	}

	if (!_pSeekFile->ReadUI32(&frameIndex))
		return -1;

	//3. Position the seek file to that particular frame
	if (!_pSeekFile->SeekTo(_seekOffsets.frames + frameIndex * sizeof (MediaFrame)))
		return -1;

	//4. Read the frame
	if (!_pSeekFile->ReadBuffer((uint8_t *) & frame, sizeof (MediaFrame)))
		return -1;

	return frameIndex;
}

bool BaseInFileStream::FeedBursts() {
	//1. Get the current output buffer size
	uint32_t currentOutputBufferSize = 0;
	BaseProtocol *pProtocol = NULL;
	IOBuffer *pBuffer = NULL;
	bool dtsAdjusted = false;
	if (((pProtocol = GetProtocol()) != NULL)
			&&((pBuffer = pProtocol->GetOutputBuffer()) != NULL))
		currentOutputBufferSize = GETAVAILABLEBYTESCOUNT(*pBuffer);

	//2. Feed frame by frame as long as the following conditions are met:
	// - we have a start and current frame index
	// - current frame index is less than total frames
	// - the output buffer size is less than _maxOutputDataSize
	// - we must feed at most _feedSessionLength (in seconds) worth of data
#ifdef __HAS_DEBUG_MESSAGES
	uint32_t fedFrames = 0;
#endif /* __HAS_DEBUG_MESSAGES */
	while ((_canFeed) //feeding is enabled
			&&(_playbackContext._playbackStartFrameIndex >= 0) //we have a start point
			&&(_playbackContext._playbackCurrentFrameIndex >= 0) //current frame index is set
			&&(_playbackContext._playbackCurrentFrameIndex < _seekOffsets.totalframes) //current frame index is valid
			&&(currentOutputBufferSize < _maxOutputDataSize) //we have enough free space on output pipe
			&&(_playbackContext._feedSessionLengthCurrent < _playbackContext._feedSessionLengthMax) //we are respecting the client side buffer
			&&(_playbackContext._lastFrameDts < _playbackContext._playbackEndDts) //we feed exactly how much is required
			) {
		//4. Read the frame metadata
		if ((!_pSeekFile->SeekTo(_seekOffsets.frames + _playbackContext._playbackCurrentFrameIndex * sizeof (MediaFrame)))
				|| (!_pSeekFile->ReadBuffer((uint8_t *) & _playbackContext._currentFrame, sizeof (MediaFrame)))) {
			FATAL("Unable to load the media frame");
			return false;
		}

		//5. Adjust the timestamps and make sure they are at least the wanted value
		dtsAdjusted = false;
		if (_collapseBackwardsTimestamps
				&& ((_playbackContext._currentFrame.pts < _playbackContext._playbackStartDts)
				|| (_playbackContext._currentFrame.dts < _playbackContext._playbackStartDts))) {
			_playbackContext._currentFrame.pts = _playbackContext._playbackStartDts;
			_playbackContext._currentFrame.dts = _playbackContext._playbackStartDts;
			_playbackContext._currentFrame.cts = 0;
			if (_playbackContext._currentFrame.type != MEDIAFRAME_TYPE_VIDEO) {
				_playbackContext._currentFrame.length = 0;
			}
			dtsAdjusted = true;
		}

		//6. Pass the frame to be read and delivered by the real class implementation,
		//skipping audio if the timestamps are not ok
		if (_playbackContext._currentFrame.length != 0) {
			if (!FeedFrame(_playbackContext._currentFrame, dtsAdjusted)) {
				FATAL("Unable to feed frame");
				return false;
			}
		}

		//7. Adjust the counters and states
		_playbackContext._playbackCurrentFrameIndex++;
		currentOutputBufferSize += _playbackContext._greedyMode ? 0 : (uint32_t) _playbackContext._currentFrame.length;
		if (_playbackContext._lastFrameDts > 0) {
			_playbackContext._feedSessionLengthCurrent += (_playbackContext._currentFrame.dts - _playbackContext._lastFrameDts);
		}
		_playbackContext._lastFrameDts = _playbackContext._currentFrame.dts;
#ifdef __HAS_DEBUG_MESSAGES
		fedFrames++;
#endif /* __HAS_DEBUG_MESSAGES */
		if ((_playbackContext._currentFrame.length != 0)
				&&(_playbackContext._singleGop)
				&&(_playbackContext._currentFrame.dts > _playbackContext._playbackStartDts)
				) {
			_playbackContext._singleGop = false;
			_canFeed = false;
			DisarmTimer();
			_outStreams.MoveHead()->value->SignalStreamCompleted();
			break;
		}
	}
	SHOW_FEED_DBG();
	//8. Signal finish
	if ((_playbackContext._lastFrameDts >= _playbackContext._playbackEndDts)
			|| (_playbackContext._playbackCurrentFrameIndex >= _seekOffsets.totalframes)) {
		_canFeed = false;
		_playbackContext.Reset(_clientSideBuffer);
		DisarmTimer();
		_outStreams.MoveHead()->value->SignalStreamCompleted();
		//This MUST be the last thing this function does, because
		//SignalStreamCompleted may delete this class
		return true;
	}

	//10. Done
	return true;
}

bool BaseInFileStream::FeedRealTime() {
	//2. Feed frame by frame as long as the following conditions are met:
	// - we have a start and current frame index
	// - current frame index is less than total frames
	// - the output buffer size is less than _maxOutputDataSize
	// - we must feed at most _feedSessionLength (in seconds) worth of data

	bool dtsAdjusted = false;
	double currentTime = 0;
	GETCLOCKS(currentTime, double);
	currentTime = currentTime / (double) CLOCKS_PER_SECOND * 1000.0;

	double elapsed = currentTime - _startupTime;
	double fed = _playbackContext._lastFrameDts < 0 ? 0 : _playbackContext._lastFrameDts - _playbackContext._playbackStartDts;

	while ((_canFeed) //feeding is enabled
			&&(_playbackContext._playbackStartFrameIndex >= 0) //we have a start point
			&&(_playbackContext._playbackCurrentFrameIndex >= 0) //current frame index is set
			&&(_playbackContext._playbackCurrentFrameIndex < _seekOffsets.totalframes) //current frame index is valid
			&&(_playbackContext._lastFrameDts < _playbackContext._playbackEndDts) //we feed exactly how much is required
			&&(fed < (elapsed + _clientSideBuffer))) {
		//4. Read the frame metadata
		if ((!_pSeekFile->SeekTo(_seekOffsets.frames + _playbackContext._playbackCurrentFrameIndex * sizeof (MediaFrame)))
				|| (!_pSeekFile->ReadBuffer((uint8_t *) & _playbackContext._currentFrame, sizeof (MediaFrame)))) {
			FATAL("Unable to load the media frame");
			return false;
		}

		//5. Adjust the timestamps and make sure they are at least the wanted value
		dtsAdjusted = false;
		if ((_playbackContext._currentFrame.pts < _playbackContext._playbackStartDts)
				|| (_playbackContext._currentFrame.dts < _playbackContext._playbackStartDts)) {
			_playbackContext._currentFrame.pts = _playbackContext._playbackStartDts;
			_playbackContext._currentFrame.dts = _playbackContext._playbackStartDts;
			_playbackContext._currentFrame.cts = 0;
			if (_playbackContext._currentFrame.type != MEDIAFRAME_TYPE_VIDEO) {
				_playbackContext._currentFrame.length = 0;
			}
			dtsAdjusted = true;
		}

		//6. Pass the frame to be read and delivered by the real class implementation,
		//skipping audio if the timestamps are not ok
		if (_playbackContext._currentFrame.length != 0) {
			if (!FeedFrame(_playbackContext._currentFrame, dtsAdjusted)) {
				FATAL("Unable to feed frame");
				return false;
			}
		}

		//7. Adjust the counters and states
		_playbackContext._playbackCurrentFrameIndex++;
		_playbackContext._lastFrameDts = _playbackContext._currentFrame.dts;
		fed = _playbackContext._lastFrameDts - _playbackContext._playbackStartDts;
		//FINEST("%.2f/%.2f", fed, elapsed);
	}

	//8. Signal finish
	if ((_playbackContext._lastFrameDts >= _playbackContext._playbackEndDts)
			|| (_playbackContext._playbackCurrentFrameIndex >= _seekOffsets.totalframes)) {
		_canFeed = false;
		_playbackContext.Reset(_clientSideBuffer);
		DisarmTimer();
		_outStreams.MoveHead()->value->SignalStreamCompleted();
		//This MUST be the last thing this function does, because
		//SignalStreamCompleted may delete this class
		return true;
	}

	//10. Done
	return true;
}

void BaseInFileStream::ArmTimer(double period) {
	if (_playbackContext._greedyMode)
		return;
	if (_realTime)
		period = 50;
	DisarmTimer();
	InFileStreamTimer *pTemp = new InFileStreamTimer(this);
	pTemp->EnqueueForHighGranularityTimeEvent((uint32_t) period);
	_timerId = pTemp->GetId();
}

void BaseInFileStream::DisarmTimer() {
	if (_playbackContext._greedyMode)
		return;
	InFileStreamTimer *pTemp = (InFileStreamTimer *) ProtocolManager::GetProtocol(
			_timerId);
	_timerId = 0;
	if (pTemp == NULL)
		return;
	pTemp->ResetStream();
	pTemp->EnqueueForDelete();
}
