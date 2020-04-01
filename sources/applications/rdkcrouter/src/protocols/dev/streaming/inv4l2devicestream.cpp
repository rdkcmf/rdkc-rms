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

#ifdef HAS_V4L2

#include <sys/ioctl.h>
#include <sys/mman.h>
#include "protocols/dev/streaming/inv4l2devicestream.h"
#include "protocols/dev/v4l2deviceprotocol.h"
#include "application/baseclientapplication.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamsmanager.h"
#include "streaming/nalutypes.h"

bool feedStarted = false;
bool firstBuffer = true;

using namespace app_rdkcrouter;

#ifdef LATENCY_TEST
bool BufferLatencyTimer::_running = false;
string BufferLatencyTimer::_tag;
vector<struct timeval>  BufferLatencyTimer::_timeStops;
void BufferLatencyTimer::MarkStart(string tag) {
	_timeStops.clear();
	_tag = tag;
	Mark();
}
void BufferLatencyTimer::Mark() {
	struct timeval tv;
	if (!gettimeofday(&tv, NULL)) {
		_timeStops.push_back(tv);
	}
}
void BufferLatencyTimer::PrintTimeEntries() {
	if (_timeStops.size() < 2)
		return;
	string result = _tag == "" ? "Recorded times:" : format("[%s] ", STR(_tag));
	uint64_t prev = _timeStops[0].tv_sec * 1000000 + _timeStops[0].tv_usec;
	FOR_VECTOR(_timeStops, i) {
		if (i == 0)
			continue;
		uint64_t next = _timeStops[i].tv_sec * 1000000 + _timeStops[i].tv_usec;
		result += format(" %"PRIu64, next - prev);
		prev = next;
	}
	WARN("%s", STR(result));
}
#endif	/* LATENCY_TEST */

#define WARN_ERR(desc) \
	do { \
		int err = errno; \
		WARN("%s. Error: (%d) %s", desc, err, strerror(err)); \
	} while(0)

#define FATAL_ERR(desc) \
	do { \
		int err = errno; \
		FATAL("%s. Error: (%d) %s", desc, err, strerror(err)); \
		} while(0)


InV4L2DeviceStream *InV4L2DeviceStream::_pInstance = NULL;

InV4L2DeviceStream *InV4L2DeviceStream::GetInstance(BaseClientApplication *pApp, Variant &streamConfig, StreamsManager *pStreamsManager) {
	string name = streamConfig["localStreamName"];
	if (_pInstance != NULL) {
		if (_pInstance->GetName() != name) {
			WARN("Device stream already present with a different name (%s)", STR(_pInstance->GetName()));
		}
		return _pInstance;
	}
	V4L2DeviceProtocol *pProto = new V4L2DeviceProtocol();
	pProto->SetApplication(pApp);
	Variant dummy;
	if (!pProto->Initialize(dummy)) {
		FATAL("Failed to create stream protocol");
		pProto->EnqueueForDelete();
		return NULL;
	}
	_pInstance = new InV4L2DeviceStream(pProto, name);
	if (!_pInstance->Initialize(dummy)) {
		FATAL("Failed to initialize device stream");
		pProto->EnqueueForDelete();
		delete _pInstance;
		return NULL;
	}
    WARN("Setting Streams Manager for %s", STR(_pInstance->GetName()));
    if (!_pInstance->SetStreamsManager(pStreamsManager)) {
		FATAL("Failed to set Streams Manager");
		pProto->EnqueueForDelete();
		delete _pInstance;
		return NULL;
	}

	Variant &config = pProto->GetCustomParameters();
	config["customParameters"]["externalStreamConfig"] = streamConfig;
    WARN("Starting streams");
    if (!_pInstance->StartStreams()) {
        FATAL("Unable to start streams");
        return NULL;
    }
    WARN("Streams Manager set; attempting to play stream");
	double dts = 0;
	double length = 0;
	if (!_pInstance->Play(dts, length)) {
		FATAL("Unable to play stream");
	}

	return _pInstance;
}

InV4L2DeviceStream::InV4L2DeviceStream(BaseProtocol *pProtocol, string name)
: BaseInDeviceStream(pProtocol, ST_IN_DEVICE_V4L2, name) {
	_videoCfd = -1;
	_audioCfd = -1;
	_videoDevBuffers = NULL;
	_audioDevBuffers = NULL;
	_lastVideoPts = 0;
	_lastAudioPts = 0;
	_bufferCount = 0;
	_currTimestamp = 0;
	_baseTimestamp = -1;
	_audioDevBuffersCount = 0;
	_videoDevBuffersCount = 0;
	_audioSampleRate = 0xFFFFFFFF;
	_audioChannelConfig = 0xFFFFFFFF;
	_nalPrefixLength = 0;
	_videoCapsChanged = false;
	_videoDevState = DEV_STATE_CLOSED;
	_audioDevState = DEV_STATE_CLOSED;	
	if (pProtocol->GetType() == PT_V4L2) {
		V4L2DeviceProtocol *pProto = (V4L2DeviceProtocol *) pProtocol;
		pProto->SetStream(this);
	}
}

InV4L2DeviceStream::~InV4L2DeviceStream() {
	CloseDevs();
//	if (GetProtocol() != NULL) GetProtocol()->EnqueueForDelete();
	_pInstance = NULL;
	INFO("Destroying Stream");
//	GetStreamsManager()->UnRegisterStream(this);
}

bool InV4L2DeviceStream::Initialize(Variant &settings) {
	if (!BaseInDeviceStream::Initialize(settings)) {
		FATAL("Failed to initialize stream");
		return false;
	}
	if (!OpenVideoDev()) {
		FATAL("Unable to initialize video");
		return false;
	}
#if 0
	if (!(_hasAudio = OpenAudioDev())) {
		WARN("Unable to initialize audio");
	}
#endif
	if (!RequestDevBuffers(_videoCfd, _videoDevBuffers, _videoRb, _videoDevBuffersCount)) {
		FATAL("Unable to request device video buffers");
		return false;
	}
	_videoDevState = DEV_STATE_ALLOCATED;
	INFO("_videoDevBuffers allocated at %p", _videoDevBuffers);

	if (_hasAudio) {
		if (!RequestDevBuffers(_audioCfd, _audioDevBuffers, _audioRb, _audioDevBuffersCount)) {
			WARN("Unable to request device audio buffers");
			_hasAudio = false;
		}
		_audioDevState = DEV_STATE_ALLOCATED;
		INFO("_audioDevBuffers allocated at %p", _audioDevBuffers);
	}
#ifdef V4L2_DUMP
	_dumpFile.Initialize("v4l2_dump.txt", FILE_OPEN_MODE_TRUNCATE);
#endif /*  V4L2_DUMP */

	return true;
}

bool InV4L2DeviceStream::RequestDevBuffers(int32_t fd, devbuf_t *&buffer, reqbuff_t &rb, uint32_t &count) {
	V4L2_LOG_FATAL("Requesting Device Buffers for %"PRId32, fd);
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

//	struct v4l2_requestbuffers rb;
	rb.type = fmt.type;
	rb.memory = V4L2_MEMORY_MMAP;
	rb.count = MAX_DEV_BUFF_COUNT;

	if (ioctl(fd, VIDIOC_REQBUFS, &rb) == -1) {
//		int err = errno;
		WARN_ERR("Request buffers failed");
		return false;
	}

	count = rb.count;
	if (count == 0) {
		WARN("Buffer allocation error");
		return false;
	}
	V4L2_LOG_INFO("%"PRIu32" buffers allocated", rb.count);
	buffer = new devbuf_t[count];
	V4L2_LOG_WARN("Buffer allocated on %p", buffer);
	memset(buffer, 0, sizeof(devbuf_t) * count);
	for (uint32_t i = 0; i < count; i++) {
		buffer[i].ptr = NULL;
	}

	bool error = false;
	V4L2_LOG_WARN("Mapping buffers");
	for (uint32_t i = 0; i < count; i++) {
		struct v4l2_buffer buf;
		buf.type = fmt.type;
		buf.index = i;
		buf.memory = rb.memory;
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
			FATAL_ERR("Unable to query for buffers");
			error = true;
			break;
		}

		buffer[i].v4l2 = buf;
		void* ptr = mmap( 0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset ); 
		if (ptr == MAP_FAILED) {
			FATAL("Unable to map buffers");
			error = true;
			break;
		}

		buffer[i].ptr = ptr;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			FATAL_ERR("Unable to queue buffer");
			error = true;
			break;
		}
		V4L2_LOG_WARN("Mapping index=%"PRIu32"\tbuf=%p\tptr=%p", i, &(buffer[i].v4l2), buffer[i].ptr);
	}

	if (error) {
		for (uint32_t i = 0; i < count; i++) {
			if (buffer[i].ptr != NULL) {
				munmap(buffer[i].ptr, buffer[i].v4l2.length);
			}
		}
		delete[] buffer;
		buffer = NULL;
		FATAL("Cannot map device memory");
		return false;
	}

	V4L2_LOG_FINE("Device buffers mapped");
	return true;
}

bool InV4L2DeviceStream::OpenVideoDev() {
	V4L2_LOG_FATAL("Opening video device");
	_videoCfd = open(VIDEO_DEV, O_RDWR);
	if (_videoCfd == -1) {
		WARN_ERR("Failed to open video device");
		return false;
	}
	_videoDevState = DEV_STATE_OPEN;
	V4L2_LOG_DEBUG("Opened video device, fd: %"PRIu32, _videoCfd);
	return true;
}

bool InV4L2DeviceStream::OpenAudioDev() {
	V4L2_LOG_FATAL("Opening audio device");
	_audioCfd = open(AUDIO_DEV, O_RDWR);
	if (_audioCfd == -1) {
		WARN_ERR("Failed to open audio device");
		return false;
	}
	_audioDevState = DEV_STATE_OPEN;
	V4L2_LOG_DEBUG("Opened audio device, fd: %"PRIu32, _audioCfd);
	return true;
}

bool InV4L2DeviceStream::StartDeviceStreamStatus(bool isAudio) {
	V4L2_LOG_FATAL("Starting Device %s Stream", isAudio ? "Audio" : "Video");
	int32_t cfd = isAudio ? _audioCfd : _videoCfd;
	reqbuff_t &rb = isAudio ? _audioRb : _videoRb;
	uint32_t &devState = isAudio ? _audioDevState : _videoDevState;
	if (devState != DEV_STATE_ALLOCATED) {
		FATAL("Invalid %s state", isAudio ? "audio" : "video");
		return false;
	}
	if( ioctl( cfd, VIDIOC_STREAMON, &(rb.type) ) == -1 ) {
		FATAL_ERR(STR(format("%s: VIDIOC_STREAMON ERROR\n", isAudio ? "Audio" : "Video")));
		return false;
	}
	V4L2_LOG_INFO("Device %s Stream started", isAudio ? "Audio" : "Video");
	devState = DEV_STATE_STREAMING;
	return true;
}
bool InV4L2DeviceStream::StopDeviceStreamStatus(bool isAudio) {
	V4L2_LOG_FATAL("Stopping Device %s Stream", isAudio ? "Audio" : "Video");
	int32_t cfd = isAudio ? _audioCfd : _videoCfd;
	reqbuff_t &rb = isAudio ? _audioRb : _videoRb;
	uint32_t &devState = isAudio ? _audioDevState : _videoDevState;
	if (ioctl(cfd, VIDIOC_STREAMOFF, &(rb.type)) == -1) {
		FATAL_ERR(STR(format("%s: VIDIOC_STREAMON ERROR\n", isAudio ? "Audio" : "Video")));
		return false;
	}
	devState = DEV_STATE_ALLOCATED;
	V4L2_LOG_FATAL("Device %s Stream stopped", isAudio ? "Audio" : "Video");
	return true;
}

bool InV4L2DeviceStream::StartStreams() {
    if (!StartDeviceStreamStatus(false)) {
        FATAL("Unable to start video stream");
        return false;
    }
    V4L2_LOG_WARN("Getting video frame rate");
	struct v4l2_streamparm parm;
#ifdef COACH
	parm.type = V4L2_BUF_TYPE_PRIVATE;
#else
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
#endif /* COACH */
	if (ioctl(_videoCfd, VIDIOC_G_PARM, &parm ) == -1 ) {
		FATAL_ERR("Video: VIDIOC_G_PARM ERROR");
	} else {
#ifdef COACH
		uint32_t *paramPtr = (uint32_t *)parm.parm.raw_data;
		uint32_t frameRate = paramPtr[0];
		uint32_t frameRateBase = paramPtr[1];
		double playTimePerFrame = (double) (frameRateBase * 1000) / (double) frameRate;
		V4L2_LOG_WARN("frameRate = %"PRIu32"\tframeRateBase = %"PRIu32, frameRate, frameRateBase);
#else
		v4l2_fract frameRate = parm.parm.capture.timeperframe;
		double playTimePerFrame = (double)(frameRate.numerator * 1000) / (double)frameRate.denominator;
#endif
		_frameDuration = playTimePerFrame;
		INFO("frameDuration=%.3f seconds", _frameDuration);
	}
	SetSocketBlockingStatus(_videoCfd, false);
	if (_hasAudio) {
		if (!StartDeviceStreamStatus(true)) {
			FATAL("Unable to start audio stream");
			return false;
		}
		V4L2_LOG_WARN("Getting audio sample rate");
		if (ioctl(_audioCfd, VIDIOC_G_PARM, &parm ) == -1 ) {
			FATAL_ERR("Audio: VIDIOC_G_PARM ERROR");
		} else {
#ifdef COACH
			uint32_t *paramPtr = (uint32_t *)parm.parm.raw_data;
			_audioSampleRate = paramPtr[0];
			_audioChannelConfig = paramPtr[1];
#else

#endif
		}
		SetSocketBlockingStatus(_audioCfd, false);
	}
    return true;
}

bool InV4L2DeviceStream::Feed() {
//uint64_t run_start, run_end;
//    GETMILLISECONDS( run_start );
    
	// 2. Process buffer
	if (!FeedVideo()) {
		FATAL("Unable to feed video");
		return false;
	}
	if (_hasAudio && !FeedAudio()) {
		FATAL("Unable to feed audio");
		return false;
	}
	//GETMILLISECONDS( run_end );
    //FATAL("IT TOOK THIS MANY MILLISECONDS TO Feed: %05"PRIu64, (run_end-run_start) );
	return true;
}

bool InV4L2DeviceStream::FeedVideo() {
    
	IOBuffer videoBuffer;
	//V4L2_LOG_WARN("Feeding Video"); // V4L2 debug print
	double ts = 0;
	do {
#ifdef LATENCY_TEST
		BufferLatencyTimer::MarkStart("Video");
#endif
		uint32_t functionRet = ProcessBuffer(_videoCfd, _videoDevBuffers, videoBuffer, ts);
#ifdef LATENCY_TEST
		BufferLatencyTimer::Mark();
#endif
		if (functionRet < 0) {
			FATAL("Unable to process video buffer");
			return false;
		}

		if (functionRet == EAGAIN)
			return true;

		if (!feedStarted) {
			feedStarted = true;
	//		V4L2_LOG_INFO("Starting feed...");
		}

		ProcessVideoBuffer(videoBuffer, ts);
	} while (ts < _currTimestamp);
	_currTimestamp = ts;
            
	return true;
}

bool InV4L2DeviceStream::FeedAudio() {
	IOBuffer audioBuffer;
	//V4L2_LOG_WARN("Feeding Audio");
	double ts = 0;
	do {
#ifdef LATENCY_TEST
		BufferLatencyTimer::MarkStart("Audio");
#endif
		uint32_t functionRet = ProcessBuffer(_audioCfd, _audioDevBuffers, audioBuffer, ts);
#ifdef LATENCY_TEST
		BufferLatencyTimer::Mark();
#endif
		if (functionRet < 0) {
			FATAL("Unable to process audio buffer");
			return false;
		}

		if (functionRet == EAGAIN)
			return true;

		// hardcoded AAC codecData
		if (GetCapabilities()->GetAudioCodec() == NULL && 
				(_audioSampleRate != 0xFFFFFFFF && _audioChannelConfig != 0xFFFFFFFF)) {
			uint32_t rates[] = {
				96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000,
				12000, 11025, 8000, 7350
			};
			uint32_t sampleRateIndex = 15;

			for (uint32_t i= 0; i < 13; i++) {
				if (rates[i] == _audioSampleRate) {
					sampleRateIndex = i;
					break;
				}
			}
			// ISO 14496-3 Section 1.6.2.1 (AudioSpecificConfig)
			BitArray ba;
			ba.PutBits<uint8_t>(0x02, 5);					// audioObjectType
			ba.PutBits<uint32_t>(sampleRateIndex, 4);		// sampleRateIndex
			if (sampleRateIndex == 15) {
				ba.PutBits<uint32_t>(_audioSampleRate, 24); // specific frequency
			}
			ba.PutBits<uint32_t>(_audioChannelConfig, 4);	// channel config	(see channelConfiguration on 1.6.3.5)
			ba.PutBits<uint8_t>(0x00, 1);					// frame length flag
			ba.PutBits<uint8_t>(0x00, 1);					// core coder depedence flag
			ba.PutBits<uint8_t>(0x00, 1);					// frame length flag

			V4L2_LOG_DEBUG("Updating audio codec");
			V4L2_LOG_INFO("SamplingRate = %"PRIu32" (index = %"PRIu32"); Channel Config = %"PRIu32, _audioSampleRate, sampleRateIndex, _audioChannelConfig);
			V4L2_LOG_INFO("AudioSpecificConfig: %s", STR(ba.ToString()));
			GetCapabilities()->AddTrackAudioAAC(GETIBPOINTER(ba), GETAVAILABLEBYTESCOUNT(ba), true, this);
			V4L2_LOG_DEBUG("Audio Codec: %s", STR(tagToString(GetCapabilities()->GetAudioCodecType())));
			V4L2_LOG_INFO("Sampling rate set to %"PRIu32, GetCapabilities()->GetAudioCodec<AudioCodecInfoAAC>()->_samplingRate);
			V4L2_LOG_INFO("Channel config set to %"PRIu32, GetCapabilities()->GetAudioCodec<AudioCodecInfoAAC>()->_channelsCount);

		}

		_stats.audio.packetsCount++;
		_stats.audio.bytesCount = GETAVAILABLEBYTESCOUNT(audioBuffer);

		vector<BaseOutStream *> outStreams = GetOutStreams();
#ifdef LATENCY_TEST
		BufferLatencyTimer::Mark();
#endif
		FOR_VECTOR(outStreams, i) {
			outStreams[i]->FeedData(GETIBPOINTER(audioBuffer), 
					GETAVAILABLEBYTESCOUNT(audioBuffer),
					0,
					GETAVAILABLEBYTESCOUNT(audioBuffer),
					ts,
					ts,
					true);
		}
#ifdef LATENCY_TEST
		BufferLatencyTimer::PrintTimeEntries();
#endif
	} while (ts < _currTimestamp);
	_currTimestamp = ts;
	return true;
}

uint32_t InV4L2DeviceStream::ProcessBuffer(int32_t fd, devbuf_t *&devBuffer, IOBuffer &ioBuf, double &ts) {
//	V4L2_LOG_FATAL("Processing buffer...");

//    uint64_t run_start, run_end;
//    GETMILLISECONDS( run_start );
        
	struct v4l2_buffer buff;
	memset(&buff, 0, sizeof(struct v4l2_buffer));
	buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buff.memory = V4L2_MEMORY_MMAP;

	// Dequeue buffer
	int32_t ioctlRet = ioctl(fd, VIDIOC_DQBUF, &buff);
	if (ioctlRet < 0 ) {
		if (errno == -EAGAIN || errno == EAGAIN) {
//			V4L2_LOG_WARN("No frames yet.");
			return EAGAIN;
		} else {
			FATAL_ERR("Error buffer dequeue");
			return errno;
		}
	}

	// Process buffers
	if (buff.bytesused == 0) {
		FATAL("Buffer error");
		return -1;
	}
	ioBuf.IgnoreAll();
	ioBuf.ReadFromBuffer((uint8_t *) devBuffer[buff.index].ptr, buff.bytesused);
	firstBuffer = false;
	double cam_ts = (double) (buff.timestamp.tv_sec * 1000) + ((double) buff.timestamp.tv_usec / (double) 1000);
	if (_baseTimestamp < 0) {
		// set base timestamp
		_baseTimestamp = cam_ts;
	}
#if 0
	_currTimestamp = ts - _baseTimestamp;
#else
	ts = cam_ts - _baseTimestamp;
#endif
	//V4L2_LOG_INFO("Timestamp: %f", ts);
	// Queue back buffer
	ioctlRet = ioctl(fd, VIDIOC_QBUF, &buff);
	if (ioctlRet < 0 ) {
		if (errno != -EAGAIN && errno != EAGAIN) {
			FATAL_ERR("Error buffer queue");
			return -1;
		}
	}
//	V4L2_LOG_DEBUG("Buffers Processed");
//    GETMILLISECONDS( run_end );
 //   if( (run_end-run_start) > 1000 )
 //       FATAL("IT TOOK THIS MANY MILLISECONDS TO RUN: %05"PRIu64, (run_end-run_start) );
	return 0;
}

void InV4L2DeviceStream::CloseDevs() {
	// video
	V4L2_LOG_DEBUG("Closing devices");
	SetSocketBlockingStatus(_videoCfd, true);
	V4L2_LOG_DEBUG("Streaming video off");
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (_videoDevState == DEV_STATE_STREAMING) {
		if (ioctl(_videoCfd, VIDIOC_STREAMOFF, &type) == -1) {
			FATAL_ERR("Unable to turn off video streaming");
		}
	}
	if (_videoDevState == DEV_STATE_ALLOCATED) {
		V4L2_LOG_DEBUG("Unmapping video buffers");
		for (uint32_t i = 0; i < _videoDevBuffersCount; i++) {
			munmap(_videoDevBuffers[i].ptr, _videoDevBuffers[i].v4l2.length);
		}
		V4L2_LOG_DEBUG("Deleting video buffers");
		delete[] _videoDevBuffers;
		_videoDevState--;
	}
	if (_videoDevState == DEV_STATE_OPEN)
		close(_videoCfd);
	// audio
	if (_hasAudio) {
		SetSocketBlockingStatus(_audioCfd, true);
		if (_audioDevState == DEV_STATE_STREAMING) {
			if (ioctl(_audioCfd, VIDIOC_STREAMOFF, &type) == -1) {
				FATAL_ERR("Unable to turn off video streaming");
			}
		}
		if (_audioDevState == DEV_STATE_ALLOCATED) {
			for (uint32_t i = 0; i < _videoDevBuffersCount; i++) {
				munmap(_audioDevBuffers[i].ptr, _audioDevBuffers[i].v4l2.length);
			}
			delete[] _audioDevBuffers;
			_audioDevState--;
		}
		if (_audioDevState == DEV_STATE_OPEN)
			close(_audioCfd);
	}
}

bool InV4L2DeviceStream::SetSocketBlockingStatus(int32_t fd, bool blocking) {
	V4L2_LOG_INFO("Setting fd=%"PRId32" to %sblocking", fd, blocking ? "" : "non-");
	if (fd < 0) return false;
	int32_t flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
	if (fcntl(fd, F_SETFL, flags) != 0){
		V4L2_LOG_INFO("Unable to set blocking status");
		return false;
	}
	V4L2_LOG_INFO("Blocking status set");
	return true;
}

bool InV4L2DeviceStream::ProcessVideoBuffer(IOBuffer &ioBuf, double ts) {
	// 1. Ignore invalid length
	uint8_t *pData = GETIBPOINTER(ioBuf);
	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(ioBuf);
	if (dataLength < 4) {
		FATAL("Incorrect NAL-U size. Got %"PRIu32, GETAVAILABLEBYTESCOUNT(ioBuf));
		return false;
	}
	
	// 2. Determine correct nalu prefix length
	if (_nalPrefixLength == 0) {
		if (pData[0] != 0 || pData[1] != 0) {
			FATAL("Invalid NAL-U prefix");
			return false;
		}
		if (pData[2] == 1)
			_nalPrefixLength = 3;
		else if (pData[2] == 0 && pData[3] == 1)
			_nalPrefixLength = 4;
		else {
			FATAL("Invalid NAL-U prefix");
			return false;
		}
	}

	uint8_t *begin = FindNalu(pData, 0, dataLength);
	bool hasMoreNalu = (begin != NULL);

	while (hasMoreNalu) {
		hasMoreNalu = false;
		begin += _nalPrefixLength;

		uint8_t type = NALU_TYPE(*begin);

		uint8_t *end = FindNalu(begin, 0, dataLength - (begin - pData));
		uint32_t naluLength = 0;
		if (end == NULL) {
			naluLength = dataLength - (begin - pData);
		} else {
			hasMoreNalu = true;
			naluLength = end - begin;
		}

		switch (type) {
			case NALU_TYPE_SPS:
				if (GETAVAILABLEBYTESCOUNT(_sps) == 0) {
					_sps.IgnoreAll();
					_sps.ReadFromBuffer(begin, naluLength);
					_videoCapsChanged = true & (GETAVAILABLEBYTESCOUNT(_pps) > 0);
				}
				break;
			case NALU_TYPE_PPS:
				if (GETAVAILABLEBYTESCOUNT(_pps) == 0) {
					_pps.IgnoreAll();
					_pps.ReadFromBuffer(begin, naluLength);
					_videoCapsChanged = true & (GETAVAILABLEBYTESCOUNT(_sps) > 0);
				}
				break;
			case NALU_TYPE_IDR:
			case NALU_TYPE_SLICE:
			case NALU_TYPE_SEI:
				FeedVideoFrame(begin, naluLength, ts);
				break;
			case NALU_TYPE_PD:
				// Ignore AUD NAL-u
				break;
			default:
				WARN("Unsupported NAL unit: %"PRIu8" (ignored)", type);
				break;
		}
		
		
		if (hasMoreNalu)
			begin = end;
	}
	if (_videoCapsChanged) {
		GetCapabilities()->AddTrackVideoH264(
				GETIBPOINTER(_sps), GETAVAILABLEBYTESCOUNT(_sps),
				GETIBPOINTER(_pps), GETAVAILABLEBYTESCOUNT(_pps),
				0, -1, -1, false, this);
			
		_videoCapsChanged = false;
	}
	return true;
}

void InV4L2DeviceStream::FeedVideoFrame(uint8_t *pData, uint32_t dataLength, double ts) {
	vector<BaseOutStream *> outStreams = GetOutStreams();
	FOR_VECTOR(outStreams, i) {
		outStreams[i]->FeedData(pData, dataLength, 0, dataLength, ts, ts, false);
	}
    // set timestamp here - as this InStream doesn't override FeedData()
	SavePts(ts);
}

uint8_t *InV4L2DeviceStream::FindNalu(uint8_t *pData, uint32_t offset, uint32_t dataLength) {
	uint8_t *ptr = pData + offset;
	uint32_t jump = 0;

	while (ptr <= pData + dataLength - _nalPrefixLength) {
		if (_nalPrefixLength == 4) {
			if (ptr[3] != 1) {
				jump = ptr[3] == 0 ? 1 : _nalPrefixLength;
			} else if (ptr[2] != 0 || ptr[1] != 0 || ptr[0] != 0) {
				jump = _nalPrefixLength;
			} else {
				return ptr;
			}
		} else if (_nalPrefixLength == 3) {
			if (ptr[2] != 1) {
				jump = ptr[2] == 0 ? 1 : _nalPrefixLength;
			} else if (ptr[1] != 0 || ptr[0] != 0) {
				jump = _nalPrefixLength;
			} else {
				return ptr;
			}
		}
		ptr += jump;
	}
	return NULL;

}

#endif /* HAS_V4L2 */
