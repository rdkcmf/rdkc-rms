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


#ifndef _INV4L2DEVICESTREAM_H

#include <linux/videodev2.h>
#include "streaming/baseindevicestream.h"

#define VIDEO_DEV "/dev/video0"
#define AUDIO_DEV "/dev/video1"

#define ST_IN_DEVICE_V4L2 MAKE_TAG6('I','D','V','4','L','2')
#define MAX_DEV_BUFF_COUNT 1

#define DEV_STATE_CLOSED 0
#define DEV_STATE_OPEN 1
#define DEV_STATE_ALLOCATED 2
#define DEV_STATE_STREAMING 3

namespace app_rdkcrouter {
	typedef struct devbuf_t {
	  struct v4l2_buffer v4l2;
	  void* ptr;
	} devbuf_t;

	typedef struct videoCodecProp {
		uint32_t nalPrefixLength;
		IOBuffer _sps;
		IOBuffer _pps;
		videoCodecProp() {
			nalPrefixLength = 0;
			_sps.IgnoreAll();
			_pps.IgnoreAll();
		}
		bool HasCompleteData() {
			return (GETAVAILABLEBYTESCOUNT(_sps) > 0 
					&& GETAVAILABLEBYTESCOUNT(_pps) > 0);
		}
	} vidCodecData;

	typedef struct v4l2_requestbuffers reqbuff_t;
#ifdef LATENCY_TEST
	class BufferLatencyTimer {
	private:
		static bool _running;
		static string _tag;
		static vector<struct timeval> _timeStops;
	public:
		static void MarkStart(string tag);
		static void Mark();
		static void PrintTimeEntries();
	};
#endif /* TIME_TESTING */
	class InV4L2DeviceStream
	: public BaseInDeviceStream {
	private:
		int32_t _videoCfd;
		int32_t _audioCfd;
		devbuf_t *_videoDevBuffers;
		devbuf_t *_audioDevBuffers;
		uint32_t _videoDevBuffersCount;
		uint32_t _audioDevBuffersCount;
		reqbuff_t _audioRb;
		reqbuff_t _videoRb;
		double _lastVideoPts;
		double _lastAudioPts;
		double _baseTimestamp;
		double _currTimestamp;
		uint32_t _bufferCount;
		uint32_t _audioSampleRate;
		uint32_t _audioChannelConfig;
		static InV4L2DeviceStream *_pInstance;
		uint32_t _nalPrefixLength;
		IOBuffer _sps;
		IOBuffer _pps;
		bool _videoCapsChanged;
		uint32_t _videoDevState;
		uint32_t _audioDevState;
#ifdef V4L2_DUMP
		File _dumpFile;
#endif /*  V4L2_DUMP */
	public:
		static InV4L2DeviceStream *GetInstance(BaseClientApplication *pApp, Variant &streamConfig, 
                        StreamsManager *pStreamsManager);
		virtual ~InV4L2DeviceStream();
		virtual bool Initialize(Variant &settings);
		bool StartStreams();
		virtual bool Feed();
	private:
		InV4L2DeviceStream(BaseProtocol *pProtocol, string name);
		bool SetSocketBlockingStatus(int32_t fd, bool blocking);
		bool RequestDevBuffers(int32_t fd, devbuf_t *&buffer, reqbuff_t &rb, uint32_t &count);
		bool OpenVideoDev();
		bool OpenAudioDev();
		bool StartDeviceStreamStatus(bool isAudio);
		bool StopDeviceStreamStatus(bool isAudio);
		void CloseDevs();
		bool FeedVideo();
		bool FeedAudio();
		uint32_t ProcessBuffer(int32_t fd, devbuf_t *&devBuffer, IOBuffer &ioBuf, double &ts);
		bool ProcessVideoBuffer(IOBuffer &ioBuf, double ts);
		void FeedVideoFrame(uint8_t *pData, uint32_t dataLength, double ts);
		uint8_t *FindNalu(uint8_t *pData, uint32_t offset, uint32_t dataLength);
	};
}
#endif	/* _INV4L2DEVICESTREAM_H */
