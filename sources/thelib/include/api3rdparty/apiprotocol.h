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
#ifdef HAS_PROTOCOL_API
#ifndef _APIPROTOCOL_H
#define	_APIPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "utils/buffering/iobufferext.h"

#ifdef _HAS_XSTREAM_
#include "xStreamerConsumer.h"
#else
#include "RdkCVideoCapturer.h"
#include "RdkCPluginFactory.h"
#endif //_HAS_XSTREAM_

#ifdef NET_EPOLL
#include "netio/epoll/iohandler.h"
#endif /* NET_EPOLL */

/* Enable AAC Support */
//#define AAC_SUPPORT

#ifndef SDK_DISABLED

/* Bit0 - Bit 3 to identify video id, valid range for video is 0~3
   Bit4 - Bit 7 to identify audio id, valid range for audio is 0~1 */
#define AUDIO_AAC		0x10
#define AUDIO_PCM		0x00
#define AUDIO_G711		0x20
#define AUDIO_MASK		0xF0
#define VIDEO_MASK              0x0F
/* Stream id has to be in the range of 0-3 and 16(video + AAC audio), 32+(0-3)(video + PCMU/G.711 audio) */
#define MAX_STREAM_ID		0x23
#define NIBBLE_LENGTH		0x04
#define SAMPLE_RATE_16K		16000
#define SAMPLE_RATE_8K		8000

#ifdef HAS_DOWNSAMPLE
extern "C" {
#include "soxr.h"
}
#endif //HAS_DOWNSAMPLE

#endif /* SDK_DISABLED */
class DLLEXP ApiProtocol : public BaseProtocol {
public:
	ApiProtocol();
	virtual ~ApiProtocol();

	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool Initialize(Variant &config);
	virtual IOHandler *GetIOHandler();
	virtual void SetIOHandler(IOHandler *pIOHandler);

	virtual bool InitializeApi(Variant &config);
	virtual bool UpdateStreamConfig(Variant &config);
	virtual bool Terminate();
	virtual bool FeedData();
	int32_t GetStreamFd();
#ifdef HAS_DOWNSAMPLE
	void InitializeSoXR();
#endif

#ifdef HAS_THREAD
	bool FeedAsync(Variant &params);
    bool DoFeedAsyncTask(Variant &params);
    void CompleteFeedAsyncTask(Variant &params);
#endif  /* HAS_THREAD */

protected:
	bool SetStreamConfig();
	bool SetAudioConfig();

private:
#ifndef SDK_DISABLED
#ifdef _HAS_XSTREAM_
	stream_hal_stream_config _videoConfig;
	stream_hal_audio_config _audioConfig;
#else
	video_stream_config_t *_videoConfig;
	audio_stream_config_t *_audioConfig;
#endif //_HAS_XSTREAM_
#endif /* SDK_DISABLED */
	IOHandler *_pIoHandler;
	IOBufferExt _outputBuffer;
	uint32_t _streamId;
	int32_t _streamFd;
	int av_flag;
	string _streamName;
#ifdef HAS_THREAD
	Variant _threadStatus;
	bool _feeding;
#endif  /* HAS_THREAD */

#ifdef _HAS_XSTREAM_
	XStreamerConsumer objConsumer;
	frameInfoH264 *appFrameInfo;
#else
	static RdkCPluginFactory* plugin_factory; //creating plugin factory instance
	static RdkCVideoCapturer* recorder;
	camera_resource_config_t *conf;
#endif //_HAS_XSTREAM_

#if !defined ( RMS_PLATFORM_RPI )
#ifdef HAS_DOWNSAMPLE
	/* soxr parameters */
	soxr_error_t error;
	soxr_t _soxr;
	bool is_soxr_initialized;
	// io specification
	soxr_io_spec_t io_spec;
	// quality specification
	unsigned long quality;
	soxr_quality_spec_t qt_spec;
	// runtime specification regarding number of allowed threads
	soxr_runtime_spec_t rt_spec;
#endif //HAS_DOWNSAMPLE
#endif

};

#endif	/* _APIPROTOCOL_H */
#endif	/* HAS_PROTOCOL_API */
