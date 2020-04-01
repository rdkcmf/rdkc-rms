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
#include "v4l2controller.h"
#include "v4l2logger.h"
#include <string.h>

V4L2Controller::V4L2Controller() {
	_videoCfd = 0;
	_audioCfd = 0;
	_videoDevBuffers = NULL;
	_audioDevBuffers = NULL;
	_videoDevBuffersCount = 0;
	_audioDevBuffersCount = 0;
	_hasAudio = false;
}
V4L2Controller::~V4L2Controller() {
}
bool V4L2Controller::OpenVideoDev() {
	V4L2Logger::Log(ALL, "Opening video device");
	_videoCfd = open(VIDEO_DEV, O_RDWR);
	if (_videoCfd == -1) {
		V4L2Logger::Log(ALL, "Failed to open video device");
		return false;
	}
	return true;
}

bool V4L2Controller::OpenAudioDev() {
	V4L2Logger::Log(ALL, "Opening audio device");
	_audioCfd = open(AUDIO_DEV, O_RDWR);
	if (_audioCfd == -1) {
		V4L2Logger::Log(ALL, "Failed to open audio device");
		return false;
	}
	return true;
}

bool V4L2Controller::RequestDevBuffers(int32_t fd, devbuf_t *&buffer, reqbuff_t &rb, uint32_t &count) {
	V4L2Logger::Log(ALL, "Requesting Device Buffers for %"PRId32, fd);
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

//	struct v4l2_requestbuffers rb;
	rb.type = fmt.type;
	rb.memory = V4L2_MEMORY_MMAP;
	rb.count = MAX_DEV_BUFF_COUNT;

	if (ioctl(fd, VIDIOC_REQBUFS, &rb) == -1) {
		V4L2Logger::Log(ALL, "Request buffers failed");
		return false;
	}

	count = rb.count;
	if (count == 0) {
		V4L2Logger::Log(ALL, "Buffer allocation error");
		return false;
	}
	V4L2Logger::Log(ALL, "%"PRIu32" buffers allocated", rb.count);
	buffer = new devbuf_t[count];
	V4L2Logger::Log(ALL, "Buffer allocated on %p", buffer);
	memset(buffer, 0, sizeof(devbuf_t) * count);
	for (uint32_t i = 0; i < count; i++) {
		buffer[i].ptr = NULL;
	}

	bool error = false;
	V4L2Logger::Log(ALL, "Mapping buffers");
	for (uint32_t i = 0; i < count; i++) {
		struct v4l2_buffer buf;
		buf.type = fmt.type;
		buf.index = i;
		buf.memory = rb.memory;
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
			V4L2Logger::Log(ALL, "Unable to query for buffers");
			error = true;
			break;
		}

		buffer[i].v4l2 = buf;
		void* ptr = mmap( 0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset ); 
		if (ptr == MAP_FAILED) {
			V4L2Logger::Log(ALL, "Unable to map buffers");
			error = true;
			break;
		}

		buffer[i].ptr = ptr;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			error = true;
			break;
		}
//		WARN("Mapping index=%"PRIu32"\tbuf=%p\tptr=%p", i, &(buffer[i].v4l2), buffer[i].ptr);
	}

	if (error) {
		for (uint32_t i = 0; i < count; i++) {
			if (buffer[i].ptr != NULL) {
				munmap(buffer[i].ptr, buffer[i].v4l2.length);
			}
		}
		delete[] buffer;
		buffer = NULL;
		V4L2Logger::Log(ALL, "Cannot map device memory");
		return false;
	}

	V4L2Logger::Log(ALL, "Device buffers mapped");
	return true;
}

bool V4L2Controller::StartDeviceStreamStatus(bool isAudio) {
	V4L2Logger::Log(ALL, "Starting %s Device Stream", isAudio ? "Audio" : "Video");
	int32_t cfd = isAudio ? _audioCfd : _videoCfd;
	reqbuff_t &rb = isAudio ? _audioRb : _videoRb;
	if( ioctl( cfd, VIDIOC_STREAMON, &(rb.type) ) == -1 ) {
		V4L2Logger::Log(ALL, "%s: VIDIOC_STREAMON ERROR\n", isAudio ? "Audio" : "Video");
		return false;
	}
	return true;
}

bool V4L2Controller::Initialize() {
	V4L2Logger::Log(ALL, "Initializing video device");
	if (!OpenVideoDev()) {
		V4L2Logger::Log(ALL, "Failed to open video");
		return false;
	}

	if (!(_hasAudio = OpenAudioDev())) {
		V4L2Logger::Log(ALL, "Failed to open audio");
	}

	if (!RequestDevBuffers(_videoCfd, _videoDevBuffers, _videoRb, _videoDevBuffersCount)) {
		V4L2Logger::Log(ALL, "Failed to request video buffers");
		return false;
	}

	if (_hasAudio) {
		if (!RequestDevBuffers(_audioCfd, _audioDevBuffers, _audioRb, _audioDevBuffersCount))
			V4L2Logger::Log(ALL, "Failed to request audio buffers");
	}

	return true;
}

int32_t V4L2Controller::GetBuffer(int32_t fd, devbuf_t *&devBuffer) {
	struct v4l2_buffer buff;
	memset(&buff, 0, sizeof(struct v4l2_buffer));
	buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buff.memory = V4L2_MEMORY_MMAP;

	// Dequeue buffer
	int32_t ioctlRet = ioctl(fd, VIDIOC_DQBUF, &buff);
	if (ioctlRet < 0 ) {
		if (errno == -EAGAIN || errno == EAGAIN) {
			V4L2Logger::Log(ALL, "No frames yet.");
			return EAGAIN;
		} else {
			V4L2Logger::Log(ALL, "Error buffer dequeue");
			return errno;
		}
	}

	// Process buffers
	if (buff.bytesused == 0) {
		V4L2Logger::Log(ALL, "Buffer error");
		return -1;
	}
	//buffer.Clear();
	//buffer.Copy((uint8_t *) devBuffer[buff.index].ptr, buff.bytesused);
	_buffer.IgnoreAll();
	_buffer.ReadFromBuffer((uint8_t *) devBuffer[buff.index].ptr, buff.bytesused);
	printf("Buffer request: %d bytes\n", buff.bytesused);
	// Queue back buffer
	ioctlRet = ioctl(fd, VIDIOC_QBUF, &buff);
	if (ioctlRet < 0 ) {
		if (errno != -EAGAIN && errno != EAGAIN) {
			FATAL("Error buffer queue");
			return -1;
		}
	}
	return 0;
}

IOBuffer *V4L2Controller::GetVideoBuffer() {
	if (GetBuffer(_videoCfd, _videoDevBuffers) != 0) {
		return NULL;
	}
	return &_buffer;
}

IOBuffer *V4L2Controller::GetAudioBuffer() {
	if (GetBuffer(_audioCfd, _audioDevBuffers) != 0) {
		return NULL;
	}
	return &_buffer;
}
