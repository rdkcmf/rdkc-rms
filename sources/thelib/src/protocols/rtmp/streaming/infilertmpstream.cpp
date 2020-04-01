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
#include "protocols/rtmp/streaming/infilertmpstream.h"
#include "streaming/streamstypes.h"
#include "streaming/codectypes.h"
#include "eventlogger/eventlogger.h"
#include "streaming/baseoutstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "streaming/nalutypes.h"

InFileRTMPStream::BaseBuilder::BaseBuilder() {

}

InFileRTMPStream::BaseBuilder::~BaseBuilder() {

}

InFileRTMPStream::AVCBuilder::AVCBuilder() {
	_videoCodecHeaderInit[0] = 0x17;
	_videoCodecHeaderInit[1] = 0;
	_videoCodecHeaderInit[2] = 0;
	_videoCodecHeaderInit[3] = 0;
	_videoCodecHeaderInit[4] = 0;

	_videoCodecHeaderKeyFrame[0] = 0x17;
	_videoCodecHeaderKeyFrame[1] = 1;

	_videoCodecHeader[0] = 0x27;
	_videoCodecHeader[1] = 1;
}

InFileRTMPStream::AVCBuilder::~AVCBuilder() {

}

bool InFileRTMPStream::AVCBuilder::BuildFrame(MediaFile* pFile, MediaFrame& mediaFrame, IOBuffer& buffer) {
	if (mediaFrame.isBinaryHeader) {
		buffer.ReadFromBuffer(_videoCodecHeaderInit, sizeof (_videoCodecHeaderInit));
	} else {
		if (mediaFrame.isKeyFrame) {
			// video key frame
			buffer.ReadFromBuffer(_videoCodecHeaderKeyFrame, sizeof (_videoCodecHeaderKeyFrame));
		} else {
			//video normal frame
			buffer.ReadFromBuffer(_videoCodecHeader, sizeof (_videoCodecHeader));
		}
		uint32_t cts = (EHTONL(((uint32_t) mediaFrame.cts) & 0x00ffffff)) >> 8;
		buffer.ReadFromBuffer((uint8_t *) & cts, 3);
	}

	if (!pFile->SeekTo(mediaFrame.start)) {
		FATAL("Unable to seek to position %"PRIu64, mediaFrame.start);
		return false;
	}

	if (!buffer.ReadFromFs(*pFile, (uint32_t) mediaFrame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, mediaFrame.length, mediaFrame.start);
		return false;
	}

	return true;
}

InFileRTMPStream::AACBuilder::AACBuilder() {
	_audioCodecHeaderInit[0] = 0xaf;
	_audioCodecHeaderInit[1] = 0;
	_audioCodecHeader[0] = 0xaf;
	_audioCodecHeader[1] = 0x01;
}

InFileRTMPStream::AACBuilder::~AACBuilder() {

}

bool InFileRTMPStream::AACBuilder::BuildFrame(MediaFile* pFile, MediaFrame& mediaFrame, IOBuffer& buffer) {
	//1. add the binary header
	if (mediaFrame.isBinaryHeader) {
		buffer.ReadFromBuffer(_audioCodecHeaderInit, sizeof (_audioCodecHeaderInit));
	} else {
		buffer.ReadFromBuffer(_audioCodecHeader, sizeof (_audioCodecHeader));
	}

	//2. Seek into the data file at the correct position
	if (!pFile->SeekTo(mediaFrame.start)) {
		FATAL("Unable to seek to position %"PRIu64, mediaFrame.start);
		return false;
	}

	//3. Read the data
	if (!buffer.ReadFromFs(*pFile, (uint32_t) mediaFrame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, mediaFrame.length, mediaFrame.start);
		return false;
	}

	return true;
}

InFileRTMPStream::MP3Builder::MP3Builder() {

}

InFileRTMPStream::MP3Builder::~MP3Builder() {

}

bool InFileRTMPStream::MP3Builder::BuildFrame(MediaFile *pFile,
		MediaFrame &mediaFrame, IOBuffer &buffer) {
	buffer.ReadFromRepeat(0x2f, 1);

	//2. Seek into the data file at the correct position
	if (!pFile->SeekTo(mediaFrame.start)) {
		FATAL("Unable to seek to position %"PRIu64, mediaFrame.start);
		return false;
	}

	//3. Read the data
	if (!buffer.ReadFromFs(*pFile, (uint32_t) mediaFrame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, mediaFrame.length, mediaFrame.start);
		return false;
	}

	return true;
}

InFileRTMPStream::PassThroughBuilder::PassThroughBuilder() {

}

InFileRTMPStream::PassThroughBuilder::~PassThroughBuilder() {

}

bool InFileRTMPStream::PassThroughBuilder::BuildFrame(MediaFile *pFile,
		MediaFrame &mediaFrame, IOBuffer &buffer) {
	//1. Seek into the data file at the correct position
	if (!pFile->SeekTo(mediaFrame.start)) {
		FATAL("Unable to seek to position %"PRIu64, mediaFrame.start);
		return false;
	}

	//2. Read the data
	if (!buffer.ReadFromFs(*pFile, (uint32_t) mediaFrame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, mediaFrame.length, mediaFrame.start);
		return false;
	}

	//3. Done
	return true;
}

InFileRTMPStream::InFileRTMPStream(BaseProtocol *pProtocol, string name)
: BaseInFileStream(pProtocol, ST_IN_FILE_RTMP, name) {
	_pAudioBuilder = NULL;
	_pVideoBuilder = NULL;
}

InFileRTMPStream::~InFileRTMPStream() {
	if (_pAudioBuilder != NULL) {
		delete _pAudioBuilder;
		_pAudioBuilder = NULL;
	}
	if (_pVideoBuilder != NULL) {
		delete _pVideoBuilder;
		_pVideoBuilder = NULL;
	}
	GetEventLogger()->LogStreamClosed(this);
}

InFileRTMPStream *InFileRTMPStream::GetInstance(BaseRTMPProtocol *pRTMPProtocol,
		StreamsManager *pStreamsManager, Metadata &metadata) {
	InFileRTMPStream *pResult = NULL;

	string type = metadata.type();
	if (type == MEDIA_TYPE_FLV
			|| type == MEDIA_TYPE_MP3
			|| type == MEDIA_TYPE_MP4) {
		pResult = new InFileRTMPStream((BaseProtocol *) pRTMPProtocol,
				metadata.mediaFullPath());
	} else {
		FATAL("File type not supported yet. Metadata:\n%s",
				STR(metadata.ToString()));
	}

	if (pResult != NULL) {
		if (!pResult->SetStreamsManager(pStreamsManager)) {
			FATAL("Unable to set the streams manager");
			delete pResult;
			pResult = NULL;
			return NULL;
		}
	}

	return pResult;
}

bool InFileRTMPStream::Initialize(Metadata &metadata) {
	if (!BaseInFileStream::Initialize(metadata)) {
		return false;
	}

	//2. Get stream capabilities
	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities == NULL) {
		FATAL("Invalid stream capabilities");
		return false;
	}

	//3. Create the video builder
	uint64_t videoCodec = pCapabilities->GetVideoCodecType();
	if ((videoCodec != 0)
			&& (videoCodec != CODEC_VIDEO_UNKNOWN)
			&& (videoCodec != CODEC_VIDEO_H264)
			&& (videoCodec != CODEC_VIDEO_PASS_THROUGH)) {
		FATAL("Invalid video stream capabilities %s detected on %s",
				STR(tagToString(videoCodec)), STR(*this));
		return false;
	}
	if (videoCodec == CODEC_VIDEO_H264) {
		_pVideoBuilder = new AVCBuilder();
	} else if (videoCodec == CODEC_VIDEO_PASS_THROUGH) {
		_pVideoBuilder = new PassThroughBuilder();
	}

	//4. Create the audio builder
	uint64_t audioCodec = pCapabilities->GetAudioCodecType();
	if ((audioCodec != 0)
			&& (audioCodec != CODEC_AUDIO_UNKNOWN)
			&& (audioCodec != CODEC_AUDIO_AAC)
			&& (audioCodec != CODEC_AUDIO_MP3)
			&& (audioCodec != CODEC_AUDIO_PASS_THROUGH)) {
		FATAL("Invalid audio stream capabilities %s detected on %s",
				STR(tagToString(audioCodec)), STR(*this));
		return false;
	}
	if (audioCodec == CODEC_AUDIO_AAC) {
		_pAudioBuilder = new AACBuilder();
	} else if (audioCodec == CODEC_AUDIO_MP3) {
		_pAudioBuilder = new MP3Builder();
	} else if (audioCodec == CODEC_AUDIO_PASS_THROUGH) {
		_pAudioBuilder = new PassThroughBuilder();
	}
	return true;
}

bool InFileRTMPStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HLS);
}

bool InFileRTMPStream::SignalSeek(double &dts) {
	NYIR;
}

bool InFileRTMPStream::SignalStop() {
	NYIR;
}

void InFileRTMPStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
	//2. Set a big chunk size on the corresponding connection
	if (TAG_KIND_OF(pOutStream->GetType(), ST_OUT_NET_RTMP)) {
		((BaseRTMPProtocol *) pOutStream->GetProtocol())->TrySetOutboundChunkSize(4 * 1024 * 1024);
		((OutNetRTMPStream *) pOutStream)->SetFeederChunkSize(4 * 1024 * 1024);
	}
}

void InFileRTMPStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
	//NYI;
}

uint32_t InFileRTMPStream::GetChunkSize() {
	return 4 * 1024 * 1024;
}

bool InFileRTMPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!isAudio) SavePts(pts); // stash timestamp for MetadataManager
	NYIR;
}

bool InFileRTMPStream::StreamCompleted() {
	NYIR;
}

bool InFileRTMPStream::FeedFrame(MediaFrame &frame, bool dtsAdjusted) {
	//1. Do we have an outbound stream attached?
	if (_outStreams.Size() == 0) {
		FATAL("No outbound stream attached");
		return false;
	}

	//2. based on the media frame type, we do A/V/M feeding
	switch (frame.type) {
		case MEDIAFRAME_TYPE_AUDIO:
		{
			if (_pAudioBuilder == NULL)
				return true;
			_audioBuffer.IgnoreAll();
			if ((!_pAudioBuilder->BuildFrame(_pMediaFile, frame, _audioBuffer))
					|| (!_outStreams.MoveHead()->value->FeedData(
					GETIBPOINTER(_audioBuffer), //pData
					GETAVAILABLEBYTESCOUNT(_audioBuffer), //dataLength
					0, //processedLength
					GETAVAILABLEBYTESCOUNT(_audioBuffer), //totalLength
					frame.pts, //pts
					frame.dts, //dts
					true //isAudio
					)))
				return false;
			return true;
		}
		case MEDIAFRAME_TYPE_VIDEO:
		{
			if (_pVideoBuilder == NULL)
				return true;
			_videoBuffer.IgnoreAll();
			// if NALU is a KEYFRAME
			// then cache it
			if (frame.isKeyFrame) {
				_cachedKeyFrame.IgnoreAll();
				_cachedKeyFrame.ReadFromBuffer(GETIBPOINTER(_videoBuffer), GETAVAILABLEBYTESCOUNT(_videoBuffer));
				_cachedDTS = frame.dts;
				_cachedPTS = frame.pts;
				_cachedProcLen = 0;
				_cachedTotLen = GETAVAILABLEBYTESCOUNT(_videoBuffer);
				//INFO("Keyframe Cache has been armed! SIZE=%d PTS=%lf", GETAVAILABLEBYTESCOUNT(_videoBuffer), _cachedPTS);
			}
			if ((!_pVideoBuilder->BuildFrame(_pMediaFile, frame, _videoBuffer))
					|| (!_outStreams.MoveHead()->value->FeedData(
					GETIBPOINTER(_videoBuffer), //pData
					GETAVAILABLEBYTESCOUNT(_videoBuffer), //dataLength
					0, //processedLength
					GETAVAILABLEBYTESCOUNT(_videoBuffer), //totalLength
					frame.pts, //pts
					frame.dts, //dts
					false //isVideo
					)))
				return false;
			return true;
		}
		case MEDIAFRAME_TYPE_DATA:
		{
			return true;
		}
		default:
		{
			FATAL("Invalid frame type encountered");
			return false;
		}
	}
}

#endif	/* HAS_PROTOCOL_RTMP */
