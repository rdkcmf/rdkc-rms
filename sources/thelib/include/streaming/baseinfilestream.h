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


#ifndef _BASEINFILESTREAM_H
#define	_BASEINFILESTREAM_H

#include "streaming/baseinstream.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "mediaformats/readers/mediaframe.h"
#include "mediaformats/readers/mediafile.h"

class BaseInFileStream
: public BaseInStream {
private:

	class InFileStreamTimer
	: public BaseTimerProtocol {
	private:
		BaseInFileStream *_pInFileStream;
	public:
		InFileStreamTimer(BaseInFileStream *pInFileStream);
		virtual ~InFileStreamTimer();
		void ResetStream();
		virtual bool TimePeriodElapsed();
	};
	friend class InFileStreamTimer;

	//InFileStreamTimer *_pTimer;
	uint32_t _timerId;
protected:
	Metadata _metadata;
	MediaFile *_pSeekFile;
	MediaFile *_pMediaFile;
	StreamCapabilities _streamCapabilities;

	struct {
		uint64_t base;
		uint64_t frames;
		uint64_t time2index;
		uint32_t totalframes;
		double endDts;
		uint32_t samplintRate;

		void Reset() {
			base = 0;
			frames = 0;
			time2index = 0;
			totalframes = 0;
			endDts = 0;
			samplintRate = 0;
		}
	} _seekOffsets;

	bool _realTime;
	bool _collapseBackwardsTimestamps;
	double _clientSideBuffer; //milliseconds
	double _startupTime; //milliseconds

	struct {
		double _playbackStartDts;
		double _playbackDuration;
		double _playbackEndDts;

		int64_t _playbackStartFrameIndex;
		int64_t _playbackCurrentFrameIndex;

		double _feedSessionLengthMax; //milliseconds
		double _feedSessionLengthCurrent; //milliseconds
		double _lastFrameDts;

		MediaFrame _currentFrame;
		bool _singleGop;
		bool _greedyMode;

		void Reset(double clientSideBuffer) {
			_playbackStartDts = 0;
			_playbackDuration = -1;
			_playbackEndDts = -1;
			_playbackStartFrameIndex = -1;
			_playbackCurrentFrameIndex = -1;
			_feedSessionLengthMax = 2 * clientSideBuffer;
			_feedSessionLengthCurrent = 0;
			_lastFrameDts = -1;
			_singleGop = false;
			_greedyMode = false;
			memset(&_currentFrame, 0, sizeof (_currentFrame));
		}

		operator string() {
			return format(
					"         _playbackStartDts: %.2f\n"
					"         _playbackDuration: %.2f\n"
					"           _playbackEndDts: %.2f\n"
					"  _playbackStartFrameIndex: %"PRId64"\n"
					"_playbackCurrentFrameIndex: %"PRId64"\n"
					"     _feedSessionLengthMax: %.2f\n"
					" _feedSessionLengthCurrent: %.2f\n"
					"             _lastFrameDts: %.2f\n"
					"                _singleGop: %d\n",
					"               _greedyMode: %d",
					_playbackStartDts,
					_playbackDuration,
					_playbackEndDts,
					_playbackStartFrameIndex,
					_playbackCurrentFrameIndex,
					_feedSessionLengthMax,
					_feedSessionLengthCurrent,
					_lastFrameDts,
					_singleGop,
					_greedyMode
					);
		}
	} _playbackContext;

	uint32_t _maxOutputDataSize;
	bool _canFeed;
public:
	BaseInFileStream(BaseProtocol *pProtocol, uint64_t type, string name);
	virtual ~BaseInFileStream();

	virtual StreamCapabilities * GetCapabilities();
	Metadata &GetCompleteMetadata();

	const MediaFrame & GetCurrentFrame();

	/*!
	 * @brief Tells the stream to stop the feeding immediatly after the first
	 * served frame
	 */
	void SingleGop(bool value);

	/*!
	 * @brief Called by the framework to initialize the stream
	 *
	 * @param metadata - The metadata object used to initialize the stream
	 *
	 * @return true on success, false otherwise
	 */
	bool Initialize(Metadata &metadata);

	/*!
	 * @brief Called by the framework to set the client side buffer
	 *
	 * @param clientSideBuffer - The client side buffer in milliseconds
	 *
	 * @discussion Defaulted to 1000 in constructor. The value is at least 1
	 */
	void SetClientSideBuffer(double clientSideBuffer, bool realTime);

	/*!
	 * @brief Called by the framework when play should start
	 *
	 * @param dts - dts expressed in milliseconds
	 *
	 * @param length - total length if milliseconds of the play
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool SignalPlay(double &dts, double &length);

	/*!
	 * @brief Called by the framework when play should be paused
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool SignalPause();

	/*!
	 * @brief Called by the framework when play should be resumed
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool SignalResume();

	/*!
	 * @brief called by the framework when the pipe is clear and we can safely
	 * send data
	 */
	virtual void ReadyForSend();

	/*!
	 * @brief feeds the entire file in one tight loop
	 *
	 * @return true on success, false otherwise
	 */
	bool FeedAllFile();

protected:
	/*!
	 * @brief called by the framework for each individual frame that needs to
	 * be delivered
	 *
	 * @param frame - the metadata of the frame
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool FeedFrame(MediaFrame &frame, bool dtsAdjusted) = 0;

private:
	/*!
	 * @brief This functions adjusts the limits of the playback in terms of
	 * startup and length.
	 *
	 * @param startDts - dts expressed in milliseconds
	 *
	 * @param duration - total length if milliseconds of the play
	 */
	void SetupPlaybackBounds(double &startDts, double &duration);

	/*!
	 * @brief converts a time into a frame index
	 *
	 * @param time - the time we are searching expressed in milliseconds
	 */
	int64_t TimeToFrame(double dts, MediaFrame &frame);

	/*!
	 * @brief The main function which feeds the data. It will feed it in bursts
	 */
	bool FeedBursts();

	/*!
	 * @brief The main function which feeds the data. It will feed it in real-time
	 * with high granularity timer
	 */
	bool FeedRealTime();

	/*!
	 * @brief Arms the feed timer
	 *
	 * @param period - the period of the timer in milliseconds
	 */
	void ArmTimer(double period);

	/*!
	 * @brief disarms the current timer (if any)
	 */
	void DisarmTimer();
};

#endif	/* _BASEINFILESTREAM_H */
