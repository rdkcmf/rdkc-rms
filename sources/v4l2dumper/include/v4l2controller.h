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
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
//#include <types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "common.h"

#define VIDEO_DEV "/dev/video0"
#define AUDIO_DEV "/dev/video1"

#define MAX_DEV_BUFF_COUNT 1

typedef struct deviceBuffers {
	struct v4l2_buffer v4l2;
	void* ptr;
} devbuf_t;

typedef struct v4l2_requestbuffers reqbuff_t;
/*
struct buf {
	uint8_t *ptr;
	size_t length;
	buf() {
		ptr = NULL;
		length = 0;
	}
	void Copy(uint8_t *src, size_t length) {
		if (ptr != NULL)
			Clear();
		ptr = new uint8_t[length +1];
		length = 0;
	}
	void Clear() {
		if (ptr != NULL) {
			delete[] ptr;
			ptr = NULL;
		}
		length = 0;
	}
};
typedef struct buf buffer_t;
*/
class V4L2Controller {
private:
	IOBuffer _buffer;
	uint8_t _test;
	int32_t _videoCfd;
	int32_t _audioCfd;
	devbuf_t *_videoDevBuffers;
	devbuf_t *_audioDevBuffers;
	uint32_t _videoDevBuffersCount;
	uint32_t _audioDevBuffersCount;
	reqbuff_t _audioRb;
	reqbuff_t _videoRb;
	bool OpenVideoDev();
	bool OpenAudioDev();
	bool RequestDevBuffers(int32_t fd, devbuf_t *&buffer, reqbuff_t &rb, uint32_t &count);
	int32_t GetBuffer(int32_t fd, devbuf_t *&devBuffer);
public:
	bool _hasAudio;
	V4L2Controller();
	~V4L2Controller();
	bool Initialize();
	bool StartDeviceStreamStatus(bool isAudio);
	IOBuffer *GetVideoBuffer();
	IOBuffer *GetAudioBuffer();
};

