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


#include "protocols/rtp/streaming/infilertspstream.h"
#include "protocols/rtp/rtspprotocol.h"
#include "streaming/streamstypes.h"
#include "streaming/codectypes.h"

InFileRTSPStream::InFileRTSPStream(BaseProtocol *pProtocol, string name)
: BaseInFileStream(pProtocol, ST_IN_FILE_RTSP, name) {
	_collapseBackwardsTimestamps = true;
}

InFileRTSPStream::~InFileRTSPStream() {
}

InFileRTSPStream *InFileRTSPStream::GetInstance(RTSPProtocol *pRTSPProtocol,
		StreamsManager *pStreamsManager, Metadata &metadata) {
	InFileRTSPStream *pResult = NULL;

	string type = metadata.type();
	if (type == MEDIA_TYPE_MP4) {
		pResult = new InFileRTSPStream(pRTSPProtocol, metadata.mediaFullPath());
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

bool InFileRTSPStream::Initialize(Metadata &metadata) {
	if (!BaseInFileStream::Initialize(metadata))
		return false;

	//2. Get stream capabilities
	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities == NULL) {
		FATAL("Invalid stream capabilities");
		return false;
	}

	uint64_t videoCodec = pCapabilities->GetVideoCodecType();
	uint64_t audioCodec = pCapabilities->GetAudioCodecType();

	videoCodec = videoCodec != CODEC_VIDEO_H264 ? 0 : CODEC_VIDEO_H264;
	audioCodec = audioCodec != CODEC_AUDIO_AAC ? 0 : CODEC_AUDIO_AAC;
	if ((videoCodec == 0)&&(audioCodec == 0)) {
		FATAL("Incompatible audio/video codecs detected");
		return false;
	}

	return true;
}

bool InFileRTSPStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTP)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS);
}

bool InFileRTSPStream::SignalSeek(double &dts) {
	NYIR;
}

bool InFileRTSPStream::SignalStop() {
	NYIR;
}

void InFileRTSPStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
}

void InFileRTSPStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
}

bool InFileRTSPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!isAudio) SavePts(pts); // stash timestamp for MetadataManager
	NYIR;
}

bool InFileRTSPStream::FeedFrame(MediaFrame &frame, bool dtsAdjusted) {
	//1. Do we have an outbound stream attached?
	if (_outStreams.Size() == 0) {
		FATAL("No outbound stream attached");
		return false;
	}

	//	if (dtsAdjusted)
	//		return true;

	//FINEST("%s", STR(frame));

	//2. based on the media frame type, we do A/V/M feeding
	switch (frame.type) {
		case MEDIAFRAME_TYPE_AUDIO:
		{
			return FeedAudioFrame(frame);
		}
		case MEDIAFRAME_TYPE_VIDEO:
		{
			return FeedVideoFrame(frame);
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

bool InFileRTSPStream::FeedAudioFrame(MediaFrame &frame) {
	IOBuffer _audioBuffer;
	_audioBuffer.IgnoreAll();
	//1. add the binary header
	if (frame.isBinaryHeader)
		return true;

	//2. Seek into the data file at the correct position
	if (!_pMediaFile->SeekTo(frame.start)) {
		FATAL("Unable to seek to position %"PRIu64, frame.start);
		return false;
	}

	//3. Read the data
	if (!_audioBuffer.ReadFromFs(*_pMediaFile, (uint32_t) frame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, frame.length, frame.start);
		return false;
	}

	return _outStreams.MoveHead()->value->FeedData(
			GETIBPOINTER(_audioBuffer), //pData
			GETAVAILABLEBYTESCOUNT(_audioBuffer), //dataLength
			0, //processedLength
			GETAVAILABLEBYTESCOUNT(_audioBuffer), //totalLength
			frame.pts, //pts
			frame.dts, //dts
			true //isAudio
			);
}

bool InFileRTSPStream::FeedVideoFrame(MediaFrame &frame) {
	IOBuffer _videoBuffer;
	_videoBuffer.IgnoreAll();
	if (frame.isBinaryHeader)
		return true;


	if (!_pMediaFile->SeekTo(frame.start)) {
		FATAL("Unable to seek to position %"PRIu64, frame.start);
		return false;
	}

	if (!_videoBuffer.ReadFromFs(*_pMediaFile, (uint32_t) frame.length)) {
		FATAL("Unable to read %"PRIu64" bytes from offset %"PRIu64, frame.length, frame.start);
		return false;
	}

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

	return _outStreams.MoveHead()->value->FeedData(
			GETIBPOINTER(_videoBuffer), //pData
			GETAVAILABLEBYTESCOUNT(_videoBuffer), //dataLength
			0, //processedLength
			GETAVAILABLEBYTESCOUNT(_videoBuffer), //totalLength
			frame.pts, //pts
			frame.dts, //dts
			false //isVideo
			);
}
