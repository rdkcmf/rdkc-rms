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



#ifdef HAS_PROTOCOL_RTMP
#include "protocols/rtmp/streaming/outfilertmpflvstream.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/streamstypes.h"
#include "protocols/baseprotocol.h"

OutFileRTMPFLVStream::OutFileRTMPFLVStream(BaseProtocol *pProtocol, string name,
		string filename)
: BaseOutFileStream(pProtocol, ST_OUT_FILE_RTMP_FLV, name) {
	_timeBase = -1;
	_prevTagSize = 0;
	_filename = filename;
}

OutFileRTMPFLVStream::~OutFileRTMPFLVStream() {
	if (_file.IsOpen()) {
		_file.Close();
	}
}

void OutFileRTMPFLVStream::Initialize() {
	if (!_file.Initialize(_filename, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to initialize file %s", STR(_filename));
		_pProtocol->EnqueueForDelete();
	}

	//REFERENCE: video_file_format_spec_v10.pdf page 8/48

	//2. Write FLV header
	string flv = "FLV";
	if (!_file.WriteString(flv)) {
		FATAL("Unable to write FLV signature");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//3. Write FLV version
	if (!_file.WriteUI8(1)) {
		FATAL("Unable to write FLV version");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//4. Write FLV flags
	if (!_file.WriteUI8(5)) {
		FATAL("Unable to write flags");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//5. Write FLV offset
	if (!_file.WriteUI32(9)) {
		FATAL("Unable to write data offset");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//6. Write first dummy audio
	if (!FeedData(NULL, 0, 0, 0, 0, 0, true)) {
		FATAL("Unable to write dummy audio packet");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//7. Write first dummy video
	if (!FeedData(NULL, 0, 0, 0, 0, 0, false)) {
		FATAL("Unable to write dummy audio packet");
		_pProtocol->EnqueueForDelete();
		return;
	}

	//8. Set the timebase to unknown value
	_timeBase = -1;
	return;
}

bool OutFileRTMPFLVStream::SignalPlay(double &dts, double &length) {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalPause() {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalResume() {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalSeek(double &dts) {
	NYIR;
}

bool OutFileRTMPFLVStream::SignalStop() {
	NYIR;
}

bool OutFileRTMPFLVStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!_file.IsOpen()) {
		Initialize();
	}

	if (_timeBase < 0)
		_timeBase = dts;

	IOBuffer &buffer = isAudio ? _audioBuffer : _videoBuffer;

	if (!buffer.ReadFromBuffer(pData, dataLength)) {
		FATAL("Unable to save data");
		return false;
	}

	if (GETAVAILABLEBYTESCOUNT(buffer) > totalLength) {
		FATAL("Invalid video input");
		return false;
	}

	if (GETAVAILABLEBYTESCOUNT(buffer) < totalLength) {
		return true;
	}

	if (!_file.WriteUI32(_prevTagSize)) {
		FATAL("Unable to write prev tag size");
		return false;
	}

	if (!_file.WriteUI8(isAudio ? 8 : 9)) {
		FATAL("Unable to write marker");
		return false;
	}

	if (!_file.WriteUI24(totalLength)) {
		FATAL("Unable to write data size");
		return false;
	}

	if (!_file.WriteSUI32((uint32_t) dts - (uint32_t) _timeBase)) {
		FATAL("Unable to timestamp");
		return false;
	}

	if (!_file.WriteUI24(0)) {
		FATAL("Unable to write streamId");
		return false;
	}

	if (!_file.WriteBuffer(GETIBPOINTER(buffer),
			GETAVAILABLEBYTESCOUNT(buffer))) {
		FATAL("Unable to write packet data");
		return false;
	}

	_prevTagSize = GETAVAILABLEBYTESCOUNT(buffer) + 11;

	buffer.IgnoreAll();

	return true;
}

bool OutFileRTMPFLVStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP);
}

void OutFileRTMPFLVStream::SignalAttachedToInStream() {

}

void OutFileRTMPFLVStream::SignalDetachedFromInStream() {
	_file.Close();
}

void OutFileRTMPFLVStream::SignalStreamCompleted() {
}

void OutFileRTMPFLVStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if ((pOld != NULL)&&(pNew != NULL))
		EnqueueForDelete();
}

void OutFileRTMPFLVStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	if ((pOld != NULL)&&(pNew != NULL))
		EnqueueForDelete();
}

bool OutFileRTMPFLVStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	ASSERT("Operation not supported");
	return false;
}

bool OutFileRTMPFLVStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool OutFileRTMPFLVStream::IsCodecSupported(uint64_t codec) {
	ASSERT("Operation not supported");
	return false;
}

#endif /* HAS_PROTOCOL_RTMP */

