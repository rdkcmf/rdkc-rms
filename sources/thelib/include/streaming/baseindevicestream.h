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


#ifndef _BASEINDEVICESTREAM_H
#define	_BASEINDEVICESTREAM_H

#include "streaming/baseinstream.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "mediaformats/readers/mediaframe.h"
#include "mediaformats/readers/mediafile.h"

#ifdef V4L2_DEBUG
#define V4L2_LOG_FATAL(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	FATAL("[V4L2]: %s",STR(__msg__)); \
} while(0)
#define V4L2_LOG_DEBUG(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	DEBUG("[V4L2]: %s",STR(__msg__)); \
} while(0)
#define V4L2_LOG_WARN(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	WARN("[V4L2]: %s",STR(__msg__)); \
} while(0)
#define V4L2_LOG_INFO(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	INFO("[V4L2]: %s",STR(__msg__)); \
} while(0)
#define V4L2_LOG_FINE(...) \
do { \
	string __msg__=format(__VA_ARGS__); \
	FINE("[V4L2]: %s",STR(__msg__)); \
} while(0)

#else
#define V4L2_LOG_DEBUG(...)
#define V4L2_LOG_WARN(...)
#define V4L2_LOG_INFO(...)
#define V4L2_LOG_FINE(...)
#define V4L2_LOG_FATAL(...)
#endif /* V4L2_DEBUG */
class BaseInDeviceStream
: public BaseInStream {
private:

	class InDeviceStreamTimer
	: public BaseTimerProtocol {
	private:
		BaseInDeviceStream *_pInDeviceStream;
	public:
		InDeviceStreamTimer(BaseInDeviceStream *pInDeviceStream);
		virtual ~InDeviceStreamTimer();
		void ResetStream();
		virtual bool TimePeriodElapsed();
	};
	friend class InDeviceStreamTimer;

	//InDeviceStreamTimer *_pTimer;
	uint32_t _timerId;
	bool _singleframe;
	bool _playing;
protected:
	StreamCapabilities _streamCapabilities;
	double _frameDuration; //milliseconds
	double _frameRate;
	uint32_t _maxOutputDataSize;
	bool _canFeed;
	bool _hasAudio;
public:
	BaseInDeviceStream(BaseProtocol *pProtocol, uint64_t type, string name);
	virtual ~BaseInDeviceStream();

	virtual StreamCapabilities * GetCapabilities();
//	Metadata &GetCompleteMetadata();

//	const MediaFrame & GetCurrentFrame();

	/*!
	 * @brief Tells the stream to stop the feeding immediatly after the first
	 * served frame
	 */
	void SingleFrame(bool value);

	/*!
	 * @brief Called by the framework to initialize the stream
	 *
	 * @param metadata - The metadata object used to initialize the stream
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool Initialize(Variant &settings);

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
		@brief Called when a seek command was issued
		@param dts
	 */
	virtual bool SignalSeek(double &dts);

	/*!
		@brief Called when a stop command was issued
	 */
	virtual bool SignalStop();

	/*!
	 * @brief called by the framework when the pipe is clear and we can safely
	 * send data
	 */
	virtual void ReadyForSend();

	/*!
	 * @brief The main function which feeds the data. It will feed it in real-time
	 * with high granularity timer
	 */
	virtual bool Feed() = 0;

	/*!
		@brief Called after the link is complete
		@param pOutStream - the newly added stream
	 */
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);

	/*!
		@brief Called after the link is broken
		@param pOutStream - the removed stream
		*/
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);

	/*!
		@brief This is called to ensure that the linking process can be done
		@param type - the target type to which this strem must be linked against
	 */
	virtual bool IsCompatibleWithType(uint64_t type);

	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
protected:
	/*!
	 * @brief called by the framework for each individual frame that needs to
	 * be delivered
	 *
	 * @param frame - the metadata of the frame
	 *
	 * @return true on success, false otherwise
	 */
//	virtual bool FeedFrame(MediaFrame &frame, bool dtsAdjusted) = 0;

private:
	/*!
	 * @brief converts a time into a frame index
	 *
	 * @param time - the time we are searching expressed in milliseconds
	 */
	int64_t TimeToFrame(double dts, MediaFrame &frame);


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

#endif	/* _BASEINDEVICESTREAM_H */
