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



#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include "streaming/streamsmanager.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "protocols/baseprotocol.h"

BaseOutStream::BaseOutStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseStream(pProtocol, type, name) {
	if (!TAG_KIND_OF(type, ST_OUT)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_OUT)), STR(tagToString(type)));
	}
	_hasAudio = true;
	_canCallDetachedFromInStream = true;
	_pInStream = NULL;
    _nalSEIExists = false;
	_origOutStreamName = "";
	_pOrigInStream = NULL;
	Reset();
}

BaseOutStream::~BaseOutStream() {
	_canCallDetachedFromInStream = false;
	UnLink(true);
	Reset();
}

void BaseOutStream::SetAliasName(string &aliasName) {
	_aliasName = aliasName;
}

StreamCapabilities * BaseOutStream::GetCapabilities() {
	if (_pInStream != NULL)
		return _pInStream->GetCapabilities();
	return NULL;
}

void BaseOutStream::ReadyForSend() {
	if (_pInStream != NULL)
		_pInStream->ReadyForSend();
}

bool BaseOutStream::Link(BaseInStream *pInStream, bool reverseLink) {
	if ((!pInStream->IsCompatibleWithType(GetType()))
			|| (!IsCompatibleWithType(pInStream->GetType()))) {
		FATAL("stream type %s not compatible with stream type %s",
				STR(tagToString(GetType())),
				STR(tagToString(pInStream->GetType())));
		return false;
	}
	if (_pInStream != NULL) {
		if (_pInStream->GetUniqueId() == pInStream->GetUniqueId()) {
			WARN("BaseOutStream::Link: This stream is already linked");
			return true;
		}
		FATAL("BaseOutStream::Link: This stream is already linked to stream with unique id %u",
				_pInStream->GetUniqueId());
		return false;
	}
	_pInStream = pInStream;
	if (reverseLink) {
		if (!_pInStream->Link(this, false)) {
			FATAL("BaseOutStream::Link: Unable to reverse link");
			_pInStream = NULL;
			return false;
		}
	}
	SignalAttachedToInStream();
	_canCallDetachedFromInStream = true;
	return true;
}

bool BaseOutStream::UnLink(bool reverseUnLink) {
	_pStreamsManager->SignalUnLinkingStreams(_pInStream, this);
	if (_pInStream == NULL) {
		//WARN("BaseOutStream::UnLink: This stream is not linked");
		return true;
	}
	if (reverseUnLink) {
		if (!_pInStream->UnLink(this, false)) {
			WARN("BaseOutStream::UnLink: Unable to reverse unLink");
		}
	}
	_pInStream = NULL;
	if (_canCallDetachedFromInStream) {
		SignalDetachedFromInStream();
		_canCallDetachedFromInStream = false;
	}
	Reset();
	return true;
}

bool BaseOutStream::IsLinked() {
	return _pInStream != NULL;
}

BaseInStream *BaseOutStream::GetInStream() {
	return _pInStream;
}

void BaseOutStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseStream::GetStats(info, namespaceId);
	if (_pInStream != NULL) {
		info["inStreamUniqueId"] = (((uint64_t) namespaceId) << 32) | _pInStream->GetUniqueId();
	} else {
		info["inStreamUniqueId"] = Variant();
	}
	
	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities != NULL)
		info["bandwidth"] = (uint32_t) (pCapabilities->GetTransferRate() / 1024.0);
	else
		info["bandwidth"] = (uint32_t) 0;
	if (_aliasName != "") {
		info["aliasName"] = _aliasName;
	}

	Variant customParameters = GetProtocol()->GetCustomParameters();
	if (customParameters.HasKeyChain(V_STRING, false, 1, "streamAlias"))
		info["streamAlias"] = customParameters["streamAlias"];
}

bool BaseOutStream::Play(double dts, double length) {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalPlay(dts, length)) {
			FATAL("Unable to signal play");
			return false;
		}
	}
	return SignalPlay(dts, length);
}

bool BaseOutStream::Pause() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalPause()) {
			FATAL("Unable to signal pause");
			return false;
		}
	}
	return SignalPause();
}

bool BaseOutStream::Resume() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalResume()) {
			FATAL("Unable to signal resume");
			return false;
		}
	}
	return SignalResume();
}

bool BaseOutStream::Seek(double dts) {
	if (!SignalSeek(dts)) {
		FATAL("Unable to signal seek");
		return false;
	}
	if (_pInStream != NULL) {
		if (!_pInStream->SignalSeek(dts)) {
			FATAL("Unable to signal seek");
			return false;
		}
	}
	return true;
}

bool BaseOutStream::Stop() {
	if (_pInStream != NULL) {
		if (!_pInStream->SignalStop()) {
			FATAL("Unable to signal stop");
			return false;
		}
	}
	return SignalStop();
}

bool BaseOutStream::GenericProcessData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {

	//Ignore audio if audio is blocked
	if (isAudio && !_hasAudio)
		return true;
	//1. Ignore the unwanted track (codec not supported)
	if ((((isAudio) && (!_genericProcessDataSetup._hasAudio))
			|| ((!isAudio) && (!_genericProcessDataSetup._hasVideo)))
			&& (_inStreamType != 0)) {
		return true;
	}

	//2. validate the pts/dts
	if ((pts < 0) || (dts < 0)) {
		WARN("Uninitialized PTS/DTS");
		return true;
	}

	//3. Normalize the pts/dts to a time base if required
	if ((_genericProcessDataSetup._timeBase >= 0) && (_inStreamType != 0)) {
		if (_zeroTimeBase < 0) {
			_zeroTimeBase = dts;
		}
		pts = pts - _zeroTimeBase + _genericProcessDataSetup._timeBase;
		dts = dts - _zeroTimeBase + _genericProcessDataSetup._timeBase;
		if ((pts < 0) || (dts < 0)) {
			//WARN("Time basing failed. Skipping this frame");
			return true;
		}
	}

	//2. Depending on the stream type, perform packet aggregation. That means
	//wait until a full frames is composed from those multiple packets
	switch (_inStreamType) {
		case 0:
		{
			//3. We don't know the input stream type yet. Try to detect it and
			//also validate the A/V codecs combination
			if (_pInStream != NULL) {
				_inStreamType = _pInStream->GetType();
				// ugly fix but no choice for now.
				if (TAG_KIND_OF(_inStreamType, ST_IN_DEVICE))
					_inStreamType = ST_IN_DEVICE;
			} else {
				_inStreamType = 0;
			}

			if (_inStreamType == 0) {
				WARN("Unable to determine the type of the input stream");
				return true;
			}

			if ((_inStreamType == ST_IN_NET_RTMP)
					|| (_inStreamType == ST_IN_NET_LIVEFLV)) {
				if ((_maxWaitDts < 0) && (dts > 0))
					_maxWaitDts = dts + 10000;
			}

			if ((_inStreamType == ST_IN_NET_RTMP)
					|| (_inStreamType == ST_IN_NET_LIVEFLV)) {
				if (_maxWaitDts < 0) {
					_inStreamType = 0;
					return true;
				}
			}

			if (!ValidateCodecs(dts)) {
				FATAL("Incompatible A/V codecs combination detected on input stream");
				return false;
			}

			if (_inStreamType == 0) {
				//WARN("Unable to determine the type of the input stream. Wait for another %.2f seconds",
				//		(_maxWaitDts - dts) / 1000.0);
				return true;
			}

			bool oldHasAudio = _genericProcessDataSetup._hasAudio;
			bool oldHasVideo = _genericProcessDataSetup._hasVideo;

			//3.1. Perform last stage initialization
			if (!FinishInitialization(&_genericProcessDataSetup)) {
				FATAL("Unable to initialize output stream");
				return false;
			}

			if (((!oldHasAudio) && (_genericProcessDataSetup._hasAudio))
					|| ((!oldHasVideo) && (_genericProcessDataSetup._hasVideo))) {
				FATAL("Audio/Video track can not be forced to appear. Only disabled when detected");
				return false;
			}

			if (!(_genericProcessDataSetup._hasAudio || _genericProcessDataSetup._hasVideo)) {
				FATAL("All A/V tracks were disabled or detected as incompatible");
				return false;
			}

			//3.2. Call GenericProcessData again (recursively)
			return GenericProcessData(pData, dataLength, processedLength,
					totalLength, pts, dts, isAudio);
		}
		case ST_IN_NET_RTMP:
		case ST_IN_NET_LIVEFLV:
        case ST_IN_NET_EXT:
		{
			//4. All this input stream types MAY deliver one complete frame
			//in multiple packets. because of this, we need to wait for all
			//and accumulate them in the corresponding bucket

			//5. Get the destination bucket
			IOBuffer &buffer = isAudio ? _audioBucket : _videoBucket;

			//6. Empty the bucket if this is the first packet from the frame
			if (processedLength == 0 || totalLength == 0)
				buffer.IgnoreAll();

			if (totalLength == 0)
				return true;

			//7. Store the data into the bucket
			buffer.ReadFromBuffer(pData, dataLength);

			//8. Check for anomalies. If detected, drop the bucket and move forward
			if (processedLength + dataLength > totalLength) {
				WARN("Bogus frame length");
				buffer.IgnoreAll();
				return true;
			}

			//9. If we don't have a complete frame, wait for more data
			if (processedLength + dataLength != totalLength) {
				return true;
			}

			//10. Check for anomalies. If detected, drop the bucket and move forward
			if (GETAVAILABLEBYTESCOUNT(buffer) != totalLength) {
				WARN("Incorrect buffer size");
				buffer.IgnoreAll();
				return true;
			}
			if (GETAVAILABLEBYTESCOUNT(buffer) == 0) {
				WARN("Incorrect buffer size");
				buffer.IgnoreAll();
				return true;
			}

			//11. Get the payload buffer and length
			pData = GETIBPOINTER(buffer);
			dataLength = GETAVAILABLEBYTESCOUNT(buffer);
		}
		case ST_IN_FILE_RTMP:
		case ST_IN_FILE_RTSP:
		{
			//12. Call the corresponding process function
			if (isAudio) {
				if (!_genericProcessDataSetup._hasAudio)
					return true;
				switch (_genericProcessDataSetup._audioCodec) {
					case CODEC_AUDIO_AAC:
					{
						return ProcessAACFromRTMP(pData, dataLength, pts, dts);
					}
					case CODEC_AUDIO_MP3:
					{
						return ProcessMP3FromRTMP(pData, dataLength, pts, dts);
					}
					default:
					{
						FATAL("Audio codec %s not supported by stream type %s",
								STR(tagToString(_genericProcessDataSetup._audioCodec)),
								STR(tagToString(_type)));
						return false;
					}
				}
			} else {
				if (!_genericProcessDataSetup._hasVideo)
					return true;
				return ProcessH264FromRTMP(pData, dataLength, pts, dts);
			}
		}
		case ST_IN_NET_RTP:
		case ST_IN_NET_TS:
		case ST_IN_FILE_TS:
		case ST_IN_DEVICE:
		case ST_IN_NET_RAW:
		{
			//13. All this input stream types ALWAYS deliver one complete frame
			//in one packet. No need to buffer. Call the corresponding process
			//function
			if (isAudio) {
				if (!_genericProcessDataSetup._hasAudio)
					return true;
				switch (_genericProcessDataSetup._audioCodec) {
					case CODEC_AUDIO_AAC:
					{
						return ProcessAACFromTS(pData, dataLength, pts, dts);
					}
					case CODEC_AUDIO_MP3:
					{
						return ProcessMP3FromTS(pData, dataLength, pts, dts);
					}
#ifdef HAS_G711
					case CODEC_AUDIO_G711A:
					case CODEC_AUDIO_G711U:
					{
						return ProcessG711FromTS(pData, dataLength, pts, dts);
					}
#endif	/* HAS_G711 */
					default:
					{
						FATAL("Audio codec %s not supported by stream type %s",
								STR(tagToString(_genericProcessDataSetup._audioCodec)),
								STR(tagToString(_type)));
						return false;
					}
				}
			} else {
				if (!_genericProcessDataSetup._hasVideo)
					return true;
				return ProcessH264FromTS(pData, dataLength, pts, dts);
			}
		}
		default:
		{
			//15. Somehow, we ended up here. Fail
			FATAL("Invalid input stream type: %s", STR(tagToString(_inStreamType)));
			return false;
		}
	}
}

void BaseOutStream::GenericSignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GenericStreamCapabilitiesChanged();
}

void BaseOutStream::GenericSignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GenericStreamCapabilitiesChanged();
}

bool BaseOutStream::Flush() {
	if (!_genericProcessDataSetup.video.avc._aggregateNALU)
		return true;
	bool result = PushVideoData(_videoFrame, _lastVideoPts, _lastVideoDts, _isKeyFrame);
	_videoFrame.IgnoreAll();
	_isKeyFrame = false;
	return result;
}

bool BaseOutStream::FinishInitialization(GenericProcessDataSetup *pGenericProcessDataSetup) {
	return true;
}

void BaseOutStream::GenericStreamCapabilitiesChanged() {
	_genericProcessDataSetup._audioCodec = 0;
	_genericProcessDataSetup._videoCodec = 0;
	_genericProcessDataSetup._pStreamCapabilities = GetCapabilities();
	if (_genericProcessDataSetup._pStreamCapabilities == NULL)
		return;
	_genericProcessDataSetup._audioCodec = _genericProcessDataSetup._pStreamCapabilities->GetAudioCodecType();
	_genericProcessDataSetup._videoCodec = _genericProcessDataSetup._pStreamCapabilities->GetVideoCodecType();
	_genericProcessDataSetup._hasAudio = IsCodecSupported(_genericProcessDataSetup._audioCodec);
	//TODO: THIS IS IMPOSED BY THE CURRENT IMPL. NEEDS TO BE REMOVED/APPENDED
	//IN THE FUTURE
	_genericProcessDataSetup._hasAudio &= (
			(_genericProcessDataSetup._audioCodec == CODEC_AUDIO_AAC)
			|| (_genericProcessDataSetup._audioCodec == CODEC_AUDIO_MP3));
	if (!_genericProcessDataSetup._hasAudio) {
		WARN("Audio codec %s not supported by stream type %s",
				STR(tagToString(_genericProcessDataSetup._audioCodec)),
				STR(tagToString(_type)));
	}

	_genericProcessDataSetup._hasVideo = IsCodecSupported(_genericProcessDataSetup._videoCodec);
	//TODO: THIS IS IMPOSED BY THE CURRENT IMPL. NEEDS TO BE REMOVED/APPENDED
	//IN THE FUTURE
	_genericProcessDataSetup._hasVideo &= (_genericProcessDataSetup._videoCodec == CODEC_VIDEO_H264);
	if (!_genericProcessDataSetup._hasVideo) {
		WARN("Video codec %s not supported by stream type %s",
				STR(tagToString(_genericProcessDataSetup._videoCodec)),
				STR(tagToString(_type)));
	}
}

void BaseOutStream::Reset() {
	_inStreamType = 0;
	_audioBucket.IgnoreAll();
	_audioFrame.IgnoreAll();

	memset(_adtsHeader, 0, sizeof (_adtsHeader));
	memset(_adtsHeaderCache, 0, sizeof (_adtsHeaderCache));

	_lastAudioTimestamp = -1;

	//video
	_videoBucket.IgnoreAll();
	_videoFrame.IgnoreAll();

	_isKeyFrame = false;
	_lastVideoPts = -1;
	_lastVideoDts = -1;

	_zeroTimeBase = -1;
	memset(&_genericProcessDataSetup, 0, sizeof (_genericProcessDataSetup));
	_genericProcessDataSetup._timeBase = -1;
	_maxWaitDts = -1;
}

bool BaseOutStream::ValidateCodecs(double dts) {
	if (_genericProcessDataSetup._pStreamCapabilities == NULL) {
		_genericProcessDataSetup._pStreamCapabilities = GetCapabilities();
		if (_genericProcessDataSetup._pStreamCapabilities == NULL) {
			FATAL("Unable to get in stream capabilities");
			return false;
		}
		_genericProcessDataSetup._audioCodec = _genericProcessDataSetup._pStreamCapabilities->GetAudioCodecType();
		_genericProcessDataSetup._videoCodec = _genericProcessDataSetup._pStreamCapabilities->GetVideoCodecType();

		bool unknownVideo = (_genericProcessDataSetup._videoCodec == CODEC_UNKNOWN)
				|| (_genericProcessDataSetup._videoCodec == CODEC_VIDEO_UNKNOWN)
				|| (_genericProcessDataSetup._videoCodec == 0);

		bool unknownAudio = (_genericProcessDataSetup._audioCodec == CODEC_UNKNOWN)
				|| (_genericProcessDataSetup._audioCodec == CODEC_AUDIO_UNKNOWN)
				|| (_genericProcessDataSetup._audioCodec == 0);

//		bool hasAudio = (_genericProcessDataSetup._pStreamCapabilities->GetAudioCodec() != NULL);
		if ((unknownVideo || (_hasAudio && unknownAudio))
				&& (_maxWaitDts > 0)
				&& (_maxWaitDts >= dts)
				) {
			_genericProcessDataSetup._pStreamCapabilities = NULL;
			_inStreamType = 0;
			return true;
		}

		_genericProcessDataSetup._hasAudio = IsCodecSupported(_genericProcessDataSetup._audioCodec);
		//TODO: THIS IS IMPOSED BY THE CURRENT IMPL. NEEDS TO BE REMOVED/APPENDED
		//IN THE FUTURE
		_genericProcessDataSetup._hasAudio &= (
				(_genericProcessDataSetup._audioCodec == CODEC_AUDIO_AAC)
				|| (_genericProcessDataSetup._audioCodec == CODEC_AUDIO_MP3)
#ifdef HAS_G711
				|| ((_genericProcessDataSetup._audioCodec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
#endif	/* HAS_G711 */
				);
		if (!_genericProcessDataSetup._hasAudio) {
			WARN("Audio codec %s not supported by stream type %s",
					STR(tagToString(_genericProcessDataSetup._audioCodec)),
					STR(tagToString(_type)));
		}

		_genericProcessDataSetup._hasVideo = IsCodecSupported(_genericProcessDataSetup._videoCodec);
		//TODO: THIS IS IMPOSED BY THE CURRENT IMPL. NEEDS TO BE REMOVED/APPENDED
		//IN THE FUTURE
		_genericProcessDataSetup._hasVideo &= (_genericProcessDataSetup._videoCodec == CODEC_VIDEO_H264);
		if (!_genericProcessDataSetup._hasVideo) {
			WARN("Video codec %s not supported by stream type %s",
					STR(tagToString(_genericProcessDataSetup._videoCodec)),
					STR(tagToString(_type)));
		}

		if (!(_genericProcessDataSetup._hasAudio || _genericProcessDataSetup._hasVideo)) {
			FATAL("In stream is not transporting H.264/AAC content");
			return false;
		}
	}
#ifdef HAS_RTSP_LAZYPULL
	BaseProtocol *pInProtocol = GetInStream()->GetProtocol();

	if (pInProtocol != NULL) {
		Variant msg = pInProtocol->GetCustomParameters();
		if (msg.HasKeyChain(_V_NUMERIC, false, 2, "_callback", "protocolID")) {
			// TODO send message to application: header:"codecUpdated"; payload:"protocolID"
		}
	}
#endif /* HAS_RTSP_LAZYPULL */
	return true;
}

void BaseOutStream::InsertVideoNALUMarker(uint32_t naluSize) {
	switch (_genericProcessDataSetup.video.avc._naluMarkerType) {
		case NALU_MARKER_TYPE_SIZE:
		{
			_videoFrame.ReadFromRepeat(0, 4);
			EHTONLP(GETIBPOINTER(_videoFrame) + GETAVAILABLEBYTESCOUNT(_videoFrame) - 4, naluSize);
			break;
		}
		case NALU_MARKER_TYPE_0001:
		{
			_videoFrame.ReadFromRepeat(0x00, 3);
			_videoFrame.ReadFromRepeat(0x01, 1);
			break;
		}
		default:
		{
			break;
		}
	}
}

void BaseOutStream::InsertVideoRTMPPayloadHeader(uint32_t cts) {
	if (_genericProcessDataSetup.video.avc._insertRTMPPayloadHeader) {
		_videoFrame.ReadFromByte(0x27);
		_videoFrame.ReadFromRepeat(0, 4); //reserve the space for payload marker and CTS
		EHTONLP(GETIBPOINTER(_videoFrame) + 1, cts); //write CTS
		GETIBPOINTER(_videoFrame)[1] = 0x01; //always 1 because we always transport video data
	}
}

void BaseOutStream::MarkVideoRTMPPayloadHeaderKeyFrame() {
	if (_genericProcessDataSetup.video.avc._insertRTMPPayloadHeader) {
		GETIBPOINTER(_videoFrame)[0] = 0x17;
	}
}

void BaseOutStream::InsertVideoPDNALU() {
	if (_genericProcessDataSetup.video.avc._insertPDNALU) {
		//6. Put the PD NAL
		InsertVideoNALUMarker(2);
		_videoFrame.ReadFromRepeat(0x09, 1);
		_videoFrame.ReadFromRepeat(0xF0, 1);

		if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS)
			_videoParamsSizes["AUDSize"] = 2;
	}
}

void BaseOutStream::InsertVideoSPSPPSBeforeIDR() {
	if (_genericProcessDataSetup.video.avc._insertSPSPPSBeforeIDR) {
		VideoCodecInfoH264 *pInfo = _genericProcessDataSetup._pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264 > ();
		InsertVideoNALUMarker(pInfo->_spsLength);
		_videoFrame.ReadFromBuffer(pInfo->_pSPS, pInfo->_spsLength);
		InsertVideoNALUMarker(pInfo->_ppsLength);
		_videoFrame.ReadFromBuffer(pInfo->_pPPS, pInfo->_ppsLength);
		if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
			_videoParamsSizes["SPSSize"] = pInfo->_spsLength;
			_videoParamsSizes["PPSSize"] = pInfo->_ppsLength;
		}
	}
}

void BaseOutStream::InsertAudioADTSHeader(uint32_t length) {
	if (_genericProcessDataSetup.audio.aac._insertADTSHeader) {
		//5. compute the ADTS header if necessary
		if (_adtsHeaderCache[0] != 0xff) {
			AudioCodecInfoAAC *pInfo = _genericProcessDataSetup._pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC > ();
			pInfo->GetADTSRepresentation(_adtsHeaderCache, length);
		} else {
			AudioCodecInfoAAC::UpdateADTSRepresentation(_adtsHeaderCache, length);
		}
		// delete this afterwards
//		string ADTS;
//		for (uint8_t i = 0; i < 7; i++) {
//			ADTS += format("%02"PRIx8" ", _adtsHeaderCache[i]);
//		}
//		WARN("[Debug] ADTS: %s", STR(ADTS));
		// delete until here
		_audioFrame.ReadFromBuffer(_adtsHeaderCache, 7);
	}
}

void BaseOutStream::InsertAudioRTMPPayloadHeader() {
	if (_genericProcessDataSetup.audio.aac._insertRTMPPayloadHeader) {
		_audioFrame.ReadFromByte(0xaf);
		_audioFrame.ReadFromByte(0x01);
	}
}

bool BaseOutStream::ProcessH264FromRTMP(uint8_t *pBuffer,
		uint32_t length, double pts, double dts) {
	uint32_t cursor = 0;
	if (_inStreamType != ST_IN_FILE_RTSP && _inStreamType != ST_IN_NET_RTP) {
		//FINEST("-------------------------");
		//2. This is a RTMP stream, so we test if this is actual payload, not codec setup
		if (pBuffer[1] != 1) {
			//FINEST("Codec setup ignored");
			return true;
		}

		//4. Initialize the cursor and skip the RTMP header
		cursor = 5;
	}

	//5. Ignore the current nal
	_videoFrame.IgnoreAll();

	InsertVideoRTMPPayloadHeader((uint32_t) (pts - dts));
	InsertVideoPDNALU();

	_isKeyFrame = false;
	_nalSEIExists = false;

	//7. Cycle over the buffer and feed NAL by NAL
	while (cursor < length) {
		//8. Do we have at least 4 bytes to read the NAL length?
		if (cursor + 4 >= length) {
			WARN("Invalid buffer size");
			_videoFrame.IgnoreAll();
			return true;
		}

		//9. read the NAL size
		uint32_t nalSize = ENTOHLP(pBuffer + cursor);
		cursor += 4;

		//10. Do we have data to read the nal?
		if (cursor + nalSize > length) {
			WARN("Invalid buffer size");
			_videoFrame.IgnoreAll();
			return true;
		}

		//11. Is this a 0 bytes nal?
		if (nalSize == 0)
			continue;

		//14. Preserve only IDR, SLICE and SEI NALS
		uint8_t nalType = NALU_TYPE(pBuffer[cursor]);
		if ((nalType != NALU_TYPE_IDR)
				&& (nalType != NALU_TYPE_SLICE)
				&& (nalType != NALU_TYPE_SEI)) {
			cursor += nalSize;
			continue;
		}

        //15. Insert SPS/PPS just before the SEI if present
        if (!_nalSEIExists && nalType == NALU_TYPE_SEI) {
            MarkVideoRTMPPayloadHeaderKeyFrame();
		    InsertVideoSPSPPSBeforeIDR();
			_nalSEIExists = true;
        }

		//15. Insert SPS/PPS just before the IDR if it wasn't inserted before SEI already
		if (nalType == NALU_TYPE_IDR && (!_isKeyFrame)) {
            if (!_nalSEIExists) {
			    MarkVideoRTMPPayloadHeaderKeyFrame();
			    InsertVideoSPSPPSBeforeIDR();
            }
			_isKeyFrame = true;
		}

		//17. Put the actual NAL
		InsertVideoNALUMarker(nalSize);
		_videoFrame.ReadFromBuffer(pBuffer + cursor, nalSize);
		if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
			_videoParamsSizes["VCLSize"] = nalSize;
			_videoParamsSizes["isKeyFrame"] = _isKeyFrame;
		}

		//19. update the cursor
		cursor += nalSize;

		if (!_genericProcessDataSetup.video.avc._aggregateNALU) {
			if (GETAVAILABLEBYTESCOUNT(_videoFrame) <= 6) {
				_videoFrame.IgnoreAll();
				_isKeyFrame = false;
				_nalSEIExists = false;
				_videoParamsSizes["isKeyFrame"] = false;
				continue;
			}

			//FINEST("SEND NAL: %s", STR(NALUToString(nalType)));
			if (!PushVideoData(_videoFrame, pts, dts, _isKeyFrame)) {
				FATAL("Unable to push video data");
				_videoFrame.IgnoreAll();
				return false;
			}

			_videoFrame.IgnoreAll();
			_isKeyFrame = false;
			_nalSEIExists = false;
		}
	}

	if (_genericProcessDataSetup.video.avc._aggregateNALU) {
		//FINEST("SEND aggregate");
		if (GETAVAILABLEBYTESCOUNT(_videoFrame) <= 6) {
			_videoFrame.IgnoreAll();
			return true;
		}

		if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
			if (!PushVideoData(_videoFrame, pts, dts, _videoParamsSizes)) {
				FATAL("Unable to push video data");
				_videoFrame.IgnoreAll();
				return false;
			}
		}
		else {
			//18. Feed it
			if (!PushVideoData(_videoFrame, pts, dts, _isKeyFrame)) {
				FATAL("Unable to push video data");
				_videoFrame.IgnoreAll();
				return false;
			}
		}

		//20. Done
		_videoFrame.IgnoreAll();
	}

	return true;
}

bool BaseOutStream::ProcessH264FromTS(uint8_t *pBuffer, uint32_t length,
		double pts, double dts) {
	//1. Create timestamp reference
	if (_lastVideoPts < 0) {
		_lastVideoPts = pts;
		_lastVideoDts = dts;
	}
	if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
		_videoParamsSizes["isKeyFrame"] = _isKeyFrame;
	}
	//2. Send over the accumulated stuff if this is a new packet from a
	if (_genericProcessDataSetup.video.avc._aggregateNALU) {
		if (_lastVideoPts != pts /*|| pts == 0*/) {
			if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
				if (!PushVideoData(_videoFrame, _lastVideoPts, _lastVideoDts, _videoParamsSizes)) {
					FATAL("Unable to push video data");
					_videoFrame.IgnoreAll();
					_isKeyFrame = false;
					_nalSEIExists = false;
					return false;
				}
			}
			else {
				if (!PushVideoData(_videoFrame, _lastVideoPts, _lastVideoDts, _isKeyFrame)) {
					FATAL("Unable to push video data");
					_videoFrame.IgnoreAll();
					_isKeyFrame = false;
					_nalSEIExists = false;
					return false;
				}
			}
			_videoFrame.IgnoreAll();
			_isKeyFrame = false;
			_nalSEIExists = false;
		}
		_lastVideoPts = pts;
		_lastVideoDts = dts;
	}
	//brand new sequence of packets
//	WARN("[Debug] _aggregateNALU = %s", _genericProcessDataSetup.video.avc._aggregateNALU ? "true" : "false");



	//3. get the nal type
	uint8_t nalType = NALU_TYPE(pBuffer[0]);
//	if (nalType == NALU_TYPE_IDR)
//		WARN("[Debug] Got IDR");

	//FATAL("NALUTYPE %s", STR(NALUToString(nalType)));

	//2. Create the PD entry if the current chunk is empty
	if (GETAVAILABLEBYTESCOUNT(_videoFrame) == 0) {
		InsertVideoRTMPPayloadHeader((uint32_t) (pts - dts));
		InsertVideoPDNALU();
	}

    //3. Insert SPS/PPS just before the SEI if present
    if (!_nalSEIExists && nalType == NALU_TYPE_SEI) {
        MarkVideoRTMPPayloadHeaderKeyFrame();
		InsertVideoSPSPPSBeforeIDR();
		_nalSEIExists = true;
    }

	//3. Insert SPS/PPS just before the IDR if it wasn't inserted before SEI already
	if (nalType == NALU_TYPE_IDR && (!_isKeyFrame)) {
		//WARN("[Debug] Keyframe found");
        if (!_nalSEIExists) {
		    MarkVideoRTMPPayloadHeaderKeyFrame();
		    InsertVideoSPSPPSBeforeIDR();
        }
		_isKeyFrame = true;
	}

	//4. Only preserve IDR, SLICE and SEI
	if ((nalType == NALU_TYPE_IDR)
			|| (nalType == NALU_TYPE_SLICE)
			|| (nalType == NALU_TYPE_SEI)
			) {
		//4. Put the actual data
//		WARN("[Debug] Reading NALU");
		InsertVideoNALUMarker(length);
		_videoFrame.ReadFromBuffer(pBuffer, length);

		if (GetType() == ST_OUT_FILE_HLS || GetType() == ST_OUT_NET_TS) {
			_videoParamsSizes["VCLSize"] = length;
			_videoParamsSizes["isKeyFrame"] = _isKeyFrame;
		}
		//FINEST("%s", STR(NALUToString(nalType)));
	}

	//6. make sure the packet doesn't grow too big
	if (_genericProcessDataSetup._maxFrameSize != 0) {
		if (GETAVAILABLEBYTESCOUNT(_videoFrame) >= _genericProcessDataSetup._maxFrameSize) {
			WARN("Frame bigger than %"PRIu32" bytes. Discard it",
					_genericProcessDataSetup._maxFrameSize);
			_videoFrame.IgnoreAll();
			_isKeyFrame = false;
			_nalSEIExists = false;
			_videoParamsSizes["isKeyFrame"] = false;
			_lastVideoPts = -1;
			_lastVideoDts = -1;
		}
	}

	if (!_genericProcessDataSetup.video.avc._aggregateNALU) {
		if (!PushVideoData(_videoFrame, pts, dts, _isKeyFrame)) {
			FATAL("Unable to push video data");
			_videoFrame.IgnoreAll();
			_isKeyFrame = false;
			_nalSEIExists = false;
			return false;
		}
		_videoFrame.IgnoreAll();
		_isKeyFrame = false;
		_nalSEIExists = false;
	}
	//7. Done
	return true;
}

bool BaseOutStream::ProcessAACFromRTMP(uint8_t *pBuffer,
		uint32_t length, double pts, double dts) {
	//1. empty current ADTS
	_audioFrame.IgnoreAll();

	if (_inStreamType != ST_IN_FILE_RTSP) {
		//2. Is this actual payload?
		if (pBuffer[1] != 1) {
			//FINEST("Codec setup ignored");
			return true;
		}

		//3. Ignore the RTMP header
		pBuffer += 2;
		length -= 2;
	}

	if (length == 0)
		return true;

	//4. Do we have enough data?
	if ((length + 7) >= 0xffff) {
		WARN("Invalid ADTS frame");
		return true;
	}

	InsertAudioRTMPPayloadHeader();
	InsertAudioADTSHeader(length);

	_audioFrame.ReadFromBuffer(pBuffer, length);

	return PushAudioData(_audioFrame, dts, dts);
}

bool BaseOutStream::ProcessMP3FromRTMP(uint8_t *pBuffer, uint32_t length,
		double pts, double dts) {
	NYIR;
}
#ifdef HAS_G711
bool BaseOutStream::ProcessG711FromTS(uint8_t *pBuffer, uint32_t length, double pts, double dts) {
	if (length == 0)
		return true;
	_audioFrame.IgnoreAll();
	_audioFrame.ReadFromBuffer(pBuffer, length);
	return PushAudioData(_audioFrame, dts, dts);
}
#endif	/* HAS_G711 */

bool BaseOutStream::ProcessAACFromTS(uint8_t *pBuffer, uint32_t length,
		double pts, double dts) {
	if (length == 0)
		return true;

	//1. empty current NALs
	_audioFrame.IgnoreAll();

	InsertAudioRTMPPayloadHeader();

	uint32_t skipBytes = 0;
	if (_inStreamType == ST_IN_NET_RTP) {
		if (length <= 2) {
			return true;
		}
		pBuffer += 2;
		length -= 2;
		InsertAudioADTSHeader(length);
	} else if ((_inStreamType == ST_IN_NET_TS) || (_inStreamType == ST_IN_FILE_TS)) {
//		WARN("[Debug] Processing audio");
		if (!_genericProcessDataSetup.audio.aac._insertADTSHeader) {
//			WARN("[Debug] InsertADTSHeader = false");
			if (length <= 7) {
//				WARN("Invalid ADTS payload length");
				return true;
			}
			skipBytes = 7;
			if ((pBuffer[1]&0x01) == 0) {
				WARN("[Debug] skipping bytes");
				skipBytes += 2;
			}
		}
	} else if(TAG_KIND_OF(_inStreamType, ST_IN_DEVICE)){
//		WARN("[Debug] Processing device audio");
		InsertAudioADTSHeader(length);
	} else if(TAG_KIND_OF(_inStreamType, ST_IN_NET_RAW)){
//		WARN("[Debug] Processing device audio");
		InsertAudioADTSHeader(length);
	} else {
		FATAL("Invalid stream type");
		return false;
	}

	if (length <= skipBytes) {
		WARN("Invalid ADTS payload length");
		return true;
	}
	_audioFrame.ReadFromBuffer(pBuffer + skipBytes, length - skipBytes);

	//FINEST("%s", STR(_audioFrame));
	return PushAudioData(_audioFrame, dts, dts);
}

bool BaseOutStream::ProcessMP3FromTS(uint8_t *pBuffer, uint32_t length,
		double pts, double dts) {
	NYIR;
}

bool BaseOutStream::ProcessMetadata(string const& vmfMetadata, int64_t pts) {
	//any metadata pre-processing should be done in this method
	//for now, just let the metadata pass through.
	return PushMetaData(vmfMetadata, pts);
}

bool BaseOutStream::IsVariableSpsPps(VideoCodecInfo *pOld, VideoCodecInfo *pNew) {
	if ((pOld != NULL) && (pNew != NULL) && (pOld->_type == CODEC_VIDEO_H264) 
			&& (pNew->_type == CODEC_VIDEO_H264)) {
		VideoCodecInfoH264 *pH264Old = (VideoCodecInfoH264 *)pOld;
		VideoCodecInfoH264 *pH264New = (VideoCodecInfoH264 *)pNew;
		
		// We know that this is the same codec type (H264), check old SPS/PPS with
		// new one
		return !(pH264Old->Compare(pH264New->_pSPS, pH264New->_spsLength, pH264New->_pPPS, pH264New->_ppsLength));
	}
	
	return false;
}

void BaseOutStream::SetInboundVod(bool inboundVod) {
	return;
}

BaseStream* BaseOutStream::GetOrigInstream() {
	uint32_t origInStreamId = 0;
	BaseStream* pOrigInStream = NULL;
	Variant& streamConfig = GetProtocol()->GetCustomParameters();
	if (streamConfig.HasKeyChain(_V_NUMERIC, false, 1, "_origId")) {
		origInStreamId = (uint32_t)(GetProtocol()->GetCustomParameters()["_origId"]);
		pOrigInStream = GetStreamsManager()->FindByUniqueId(origInStreamId);
	}
	return pOrigInStream;
}

