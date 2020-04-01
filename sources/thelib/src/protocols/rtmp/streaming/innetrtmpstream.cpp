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
#include "protocols/rtmp/streaming/innetrtmpstream.h"
#include "streaming/baseoutstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/messagefactories/streammessagefactory.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "protocols/rtmp/streaming/outfilertmpflvstream.h"
#include "streaming/streamstypes.h"
#include "eventlogger/eventlogger.h"
#include "application/baseclientapplication.h"
#include "recording/flv/outfileflv.h"
#include "protocols/rtmp/rtmpmessagefactory.h"
#include "streaming/nalutypes.h"
#define CODEC_UPDATE_WAIT_TIME 1000

InNetRTMPStream::InNetRTMPStream(BaseProtocol *pProtocol, string name,
		uint32_t rtmpStreamId, uint32_t chunkSize, uint32_t channelId,
		string privateName)
: BaseInNetStream(pProtocol, ST_IN_NET_RTMP, name) {
	_rtmpStreamId = rtmpStreamId;
	_chunkSize = chunkSize;
	_channelId = channelId;
	_clientId = ((BaseRTMPProtocol *) pProtocol)->GetClientId();
	_videoCts = 0;

	_dummy = false;
	_lastAudioCodec = 0;
	_lastVideoCodec = 0;

	_privateName = privateName;
}

InNetRTMPStream::~InNetRTMPStream() {
	GetEventLogger()->LogStreamClosed(this);


	BaseRTMPProtocol *pProtocol = GetRTMPProtocol();
	if ((pProtocol == NULL) || pProtocol->IsEnqueueForDelete())
		return;
	if (!GetRTMPProtocol()->SendMessage(
			RTMPMessageFactory::GetInvokeOnStatusNetStreamUnpublishSuccess(
			GetRTMPStreamId(), 0, false,
			pProtocol->GetClientId(), _privateName)))
		return;

	if (!GetRTMPProtocol()->SendMessage(
			RTMPMessageFactory::GetInvokeOnFCUnpublishNetStreamUnpublishSuccess(
			GetRTMPStreamId(), 0, false,
			pProtocol->GetClientId(), _privateName)))
		return;
}

StreamCapabilities * InNetRTMPStream::GetCapabilities() {
    return &_streamCapabilities;
}

void InNetRTMPStream::ReadyForSend() {
    ASSERT("Operation not supported");
}

bool InNetRTMPStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
			|| TAG_KIND_OF(type, ST_OUT_FILE_RTMP)
			|| TAG_KIND_OF(type, ST_OUT_NET_RTP)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HLS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HDS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MSS)
            || TAG_KIND_OF(type, ST_OUT_FILE_DASH)
			|| TAG_KIND_OF(type, ST_OUT_FILE_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MP4)
			|| TAG_KIND_OF(type, ST_OUT_NET_EXT)
			|| TAG_KIND_OF(type, ST_OUT_FILE_RTMP_FLV)
			|| TAG_KIND_OF(type, ST_OUT_VMF)
			|| TAG_KIND_OF(type, ST_OUT_NET_FMP4)
			|| TAG_KIND_OF(type, ST_OUT_NET_MP4)
			;

}

uint32_t InNetRTMPStream::GetRTMPStreamId() {
    return _rtmpStreamId;
}

uint32_t InNetRTMPStream::GetChunkSize() {
    return _chunkSize;
}

void InNetRTMPStream::SetChunkSize(uint32_t chunkSize) {
	_chunkSize = chunkSize;
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if ((!pCurrent->value->IsEnqueueForDelete())&&(TAG_KIND_OF(pCurrent->value->GetType(), ST_OUT_NET_RTMP)))
			((BaseRTMPProtocol *) pCurrent->value->GetProtocol())->TrySetOutboundChunkSize(chunkSize);
		pCurrent = _outStreams.MoveNext();
	}
}

bool InNetRTMPStream::SendStreamMessage(Variant &completeMessage) {
	AnalyzeMetadata(completeMessage);

	//2. Loop on the subscribed streams and send the message
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if ((pCurrent->value->IsEnqueueForDelete()) || (!TAG_KIND_OF(pCurrent->value->GetType(), ST_OUT_NET_RTMP))) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!((OutNetRTMPStream *) pCurrent->value)->SendStreamMessage(completeMessage)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	//3. Test to see if we are still alive. One of the target streams might
	//be on the same RTMP connection as this stream is and our connection
	//here might be enque for delete
	if (IsEnqueueForDelete())
		return false;

	//5. Done
	return true;
}

bool InNetRTMPStream::SendOnStatusMessages() {
	Variant &message = RTMPMessageFactory::GetInvokeOnStatusNetStreamPublishStart(
			_rtmpStreamId, 0, false, _clientId, GetName());
	if (!GetRTMPProtocol()->SendMessage(message)) {
		FATAL("Unable to send message");
		return false;
	}

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if ((pCurrent->value->IsEnqueueForDelete()) || (!TAG_KIND_OF(pCurrent->value->GetType(), ST_OUT_NET_RTMP))) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!((OutNetRTMPStream *) pCurrent->value)->SignalStreamPublished(GetName())) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}
	return true;
}

bool InNetRTMPStream::RecordFLV(Metadata &meta, bool append) {
	//1. Get the target file name
	string fileName = GetRecordedFileName(meta);
	if (fileName == "") {
		WARN("Unable to record stream %s", STR(*this));
		return true;
	}

	//2. create the settings
	Variant settings;
	settings["waitForIDR"] = (bool)false;
	settings["chunkLength"] = (uint32_t) 0;
	settings["computedPathToFile"] = fileName;

	//2. Create the stream
	OutFileFLV *pOutStream = new OutFileFLV(_pProtocol, fileName, settings);
	if (!pOutStream->SetStreamsManager(
			GetProtocol()->GetApplication()->GetStreamsManager())) {
		WARN("Unable to record stream %s", STR(*this));
		delete pOutStream;
		pOutStream = NULL;
		return false;
	}

	if (!Link(pOutStream)) {
		WARN("Unable to record stream %s", STR(*this));
		delete pOutStream;
		pOutStream = NULL;
		return false;
	}

	return true;
}

bool InNetRTMPStream::RecordMP4(Metadata &meta) {
	//	BaseProtocol *pProtocol = NULL;
	//	BaseClientApplication *pApplication = NULL;
	//	StreamMetadataResolver *pSM = NULL;
	//	string recordedStreamsStorage = "";
	//	if (((pProtocol = GetProtocol()) == NULL)
	//			|| ((pApplication = pProtocol->GetApplication()) == NULL)
	//			|| ((pSM = pApplication->GetStreamMetadataResolver()) == NULL)
	//			|| ((recordedStreamsStorage = pSM->GetRecordedStreamsStorage()) == "")
	//			) {
	//		FATAL("Unable to get the recorded streams storage");
	//		return false;
	//	}
	//	ASSERT("\n%s", STR(meta.ToString()));
	NYIR;
}

void InNetRTMPStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
}

void InNetRTMPStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
}

bool InNetRTMPStream::SignalPlay(double &dts, double &length) {
    return true;
}

bool InNetRTMPStream::SignalPause() {
    return true;
}

bool InNetRTMPStream::SignalResume() {
    return true;
}

bool InNetRTMPStream::SignalSeek(double &dts) {
    return true;
}

bool InNetRTMPStream::SignalStop() {
    return true;
}

//#define RTMP_DUMP_PTSDTS
#ifdef RTMP_DUMP_PTSDTS
double __lastRtmpPts = 0;
double __lastRtmpDts = 0;
#endif /* RTMP_DUMP_PTSDTS */

bool InNetRTMPStream::FeedDataAggregate(uint8_t *pData, uint32_t dataLength,
        uint32_t processedLength, uint32_t totalLength,
        double pts, double dts, bool isAudio) {
    if (processedLength != GETAVAILABLEBYTESCOUNT(_aggregate)) {
        _aggregate.IgnoreAll();
        return true;
    }

    if (dataLength + processedLength > totalLength) {
        _aggregate.IgnoreAll();
        return true;
    }

    _aggregate.ReadFromBuffer(pData, dataLength);

    uint8_t *pBuffer = GETIBPOINTER(_aggregate);
    uint32_t length = GETAVAILABLEBYTESCOUNT(_aggregate);

    if ((length != totalLength) || (length == 0)) {
        return true;
    }

    uint32_t frameLength = 0;
    uint32_t timestamp = 0;
    while (length >= 15) {
        frameLength = (ENTOHLP(pBuffer)) &0x00ffffff;
        timestamp = ENTOHAP(pBuffer + 4);
        if (length < frameLength + 15) {
            _aggregate.IgnoreAll();
            return true;
        }
        if ((pBuffer[0] != 8) && (pBuffer[0] != 9)) {
            length -= (frameLength + 15);
            pBuffer += (frameLength + 15);
            continue;
        }

        if (!FeedData(pBuffer + 11, frameLength, 0, frameLength, timestamp,
                timestamp, (pBuffer[0] == 8))) {
            FATAL("Unable to feed data");
            return false;
        }
        length -= (frameLength + 15);
        pBuffer += (frameLength + 15);
    }
    _aggregate.IgnoreAll();
    return true;
}

bool InNetRTMPStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	//	if ((!isAudio)&&(processedLength == 0)) {
	//		printf("%s\n", STR(hex(pData, dataLength >= 64 ? 64 : dataLength)));
	//	}
	//	if (processedLength == 0) {
	//		printf("%c %.2f: %s\n",
	//				isAudio ? 'A' : 'V',
	//				dts,
	//				STR(hex(pData, dataLength >= 64 ? 64 : dataLength)));
	//	}
	if (isAudio) {
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += dataLength;
		if (
				(processedLength == 0) //start of the packet
				&&(dataLength >= 2) //enough data
				&&((_lastAudioCodec != (pData[0] >> 4)) || ((pData[1] == 0)&&((pData[0] >> 4) == 0x0a))) //different codec or this is a new AAC codec
				) {
			if (!InitializeAudioCapabilities(this, _streamCapabilities,
					_dummy, pData, dataLength)) {
				FATAL("Unable to initialize audio capabilities");
				return false;
			}
			_lastAudioCodec = (pData[0] >> 4);
		}

		StreamCapabilities* pCapabs = &_streamCapabilities;
		if (dts > CODEC_UPDATE_WAIT_TIME &&
			pCapabs != NULL &&
			pCapabs->GetVideoCodec() == NULL) {
				pCapabs->FalseVideoCodecUpdate(this);
		}
	} else {
		SavePts(pts); // stash timestamp for MetadataManager
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
		if (
				(processedLength == 0) //start of the packet
				&&(dataLength >= 2) //enough data
				&&((_lastVideoCodec != (pData[0]&0x0f)) || ((pData[1] == 0)&&(pData[0] == 0x17))) //different codec or this is a new h264 codec
				) {
			if (!InitializeVideoCapabilities(this, _streamCapabilities,
					_dummy, pData, dataLength)) {
				FATAL("Unable to initialize video capabilities");
				return false;
			}
			_lastVideoCodec = (pData[0]&0x0f);
		}

		StreamCapabilities* pCapabs = &_streamCapabilities;
		if (dts > CODEC_UPDATE_WAIT_TIME &&
			pCapabs != NULL &&
			pCapabs->GetAudioCodec() == NULL) {
				pCapabs->FalseAudioCodecUpdate(this);
		}

		if ((processedLength == 0) && ((pData[0]&0x0f) == 7) && (dataLength >= 6)) {
			_videoCts = (ENTOHLP(pData + 2) >> 8);
			_videoCts = ((_videoCts & 0x00800000) == 0x00800000) ? (_videoCts | 0xff000000) : _videoCts;
		}
		pts = dts + _videoCts;
	}

#ifdef RTMP_DUMP_PTSDTS
    if (!isAudio) {
        FINEST("pts: %8.2f\tdts: %8.2f\tcts: %8.2f\tptsd: %+6.2f\tdtsd: %+6.2f\t%s",
                pts,
                dts,
                pts - dts,
                pts - __lastRtmpPts,
                dts - __lastRtmpDts,
                pts == dts ? "" : "DTS Present");
        if (dataLength + processedLength == totalLength) {
            __lastRtmpPts = pts;
            __lastRtmpDts = dts;
            FINEST("--------------");
        }
    }
#endif /* RTSP_DUMP_PTSDTS */
	// if NALU is a KEYFRAME
	// then cache it
	if ((NALU_TYPE(pData[0]) == NALU_TYPE_IDR) &&
		!isAudio) {
		_cachedKeyFrame.IgnoreAll();
		_cachedKeyFrame.ReadFromBuffer(pData, dataLength);
		_cachedDTS = dts;
		_cachedPTS = pts;
		_cachedProcLen = processedLength;
		_cachedTotLen = totalLength;
		//INFO("Keyframe Cache has been armed! SIZE=%d PTS=%lf", dataLength, pts);
	}

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->FeedData(pData, dataLength, processedLength,
				totalLength, pts, dts, isAudio)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	return true;
}

uint32_t InNetRTMPStream::GetInputVideoTimescale() {
	return 1000;
}

uint32_t InNetRTMPStream::GetInputAudioTimescale() {
	return 1000;
}

void InNetRTMPStream::AnalyzeMetadata(Variant &completeMessage) {
	if ((uint32_t) VH_MT(completeMessage) != RM_HEADER_MESSAGETYPE_NOTIFY)
		return;

	Variant &params = M_NOTIFY_PARAMS(completeMessage);
	if (params != V_MAP)
		return;

	bool isMetadata = false;

	FOR_MAP(params, string, Variant, i) {
		isMetadata |= ((MAP_VAL(i) == V_STRING)&&(lowerCase((string) MAP_VAL(i)) == "onmetadata"));
		if (!isMetadata)
			continue;
		if (MAP_VAL(i) == V_MAP) {
			_publicMetadata = MAP_VAL(i);
			_publicMetadata.RemoveKey("duration", false);
			break;
		}
	}

	if ((!isMetadata) || (_publicMetadata != V_MAP))
		return;

	if (_publicMetadata.HasKeyChain(_V_NUMERIC, false, 1, "bandwidth")) {
		_streamCapabilities.SetTransferRate((double) _publicMetadata.GetValue("bandwidth", false)*1024.0);
	} else {
		double transferRate = -1;
		if (_publicMetadata.HasKeyChain(_V_NUMERIC, false, 1, "audiodatarate")) {
			transferRate += (double) _publicMetadata.GetValue("audiodatarate", false)*1024.0;
		}
		if (_publicMetadata.HasKeyChain(_V_NUMERIC, false, 1, "videodatarate")) {
			transferRate += (double) _publicMetadata.GetValue("videodatarate", false)*1024.0;
		}
		if (transferRate >= 0) {
			transferRate += 1;
			_streamCapabilities.SetTransferRate(transferRate);
		}
	}

	completeMessage = RTMPMessageFactory::GetNotifyOnMetaData(_rtmpStreamId, 0,
			false, GetPublicMetadata());
}

string InNetRTMPStream::GetRecordedFileName(Metadata &meta) {
	BaseProtocol *pProtocol = NULL;
	BaseClientApplication *pApplication = NULL;
	StreamMetadataResolver *pSM = NULL;
	string recordedStreamsStorage = "";
	if ((meta.computedCompleteFileName() == "")
			|| ((pProtocol = GetProtocol()) == NULL)
			|| ((pApplication = pProtocol->GetApplication()) == NULL)
			|| ((pSM = pApplication->GetStreamMetadataResolver()) == NULL)
			|| ((recordedStreamsStorage = pSM->GetRecordedStreamsStorage()) == "")
			) {
		FATAL("Unable to compute the destination file path");
		return "";
	}
	return recordedStreamsStorage + meta.computedCompleteFileName();
}

BaseRTMPProtocol *InNetRTMPStream::GetRTMPProtocol() {
    return (BaseRTMPProtocol *) _pProtocol;
}

bool InNetRTMPStream::InitializeAudioCapabilities(
		BaseInStream *pStream, StreamCapabilities &streamCapabilities,
		bool &capabilitiesInitialized, uint8_t *pData, uint32_t length) {
	if (length == 0) {
		capabilitiesInitialized = false;
		return true;
	}
	CodecInfo *pCodecInfo = NULL;
	switch (pData[0] >> 4) {
		case 0: //Linear PCM, platform endian
		case 1: //ADPCM
		case 3: //Linear PCM, little endian
		case 7: //G.711 A-law logarithmic PCM
		case 8: //G.711 mu-law logarithmic PCM
		case 11: //Speex
		case 14: //MP3 8-Khz
		case 15: //Device-specific sound
		{
			WARN("RTMP input audio codec %"PRIu8" defaulted to pass through", (uint8_t) (pData[0] >> 4));
			pCodecInfo = streamCapabilities.AddTrackAudioPassThrough(pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse pass-through codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 2: //MP3
		{
			uint32_t ssRate = 0;
			switch ((pData[0] >> 2)&0x03) {
				case 0:
					ssRate = 5512;
					break;
				case 1:
					ssRate = 11025;
					break;
				case 2:
					ssRate = 22050;
					break;
				case 3:
					ssRate = 44100;
					break;
			}
			pCodecInfo = streamCapabilities.AddTrackAudioMP3(
					ssRate,
					(pData[0] & 0x01) == 0 ? 1 : 2,
					((pData[0] >> 1)&0x01) == 0 ? 8 : 16,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse MP3 codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 4: //Nellymoser 16-kHz mono
		{
			pCodecInfo = streamCapabilities.AddTrackAudioNellymoser(
					16000,
					1,
					((pData[0] >> 1)&0x01) == 0 ? 8 : 16,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse Nellymoser 16-kHz mono codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 5: //Nellymoser 8-kHz mono
		{
			pCodecInfo = streamCapabilities.AddTrackAudioNellymoser(
					8000,
					1,
					((pData[0] >> 1)&0x01) == 0 ? 8 : 16,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse Nellymoser 8-kHz mono codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 6: //Nellymoser
		{
			uint32_t ssRate = 0;
			switch ((pData[0] >> 2)&0x03) {
				case 0:
					ssRate = 5512;
					break;
				case 1:
					ssRate = 11025;
					break;
				case 2:
					ssRate = 22050;
					break;
				case 3:
					ssRate = 44100;
					break;
			}
			pCodecInfo = streamCapabilities.AddTrackAudioNellymoser(
					ssRate,
					(pData[0] & 0x01) == 0 ? 1 : 2,
					((pData[0] >> 1)&0x01) == 0 ? 8 : 16,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse Nellymoser codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 10: //AAC
		{
			if (length < 4) {
				FATAL("Invalid length");
				return false;
			}
			if (pData[1] != 0) {
				WARN("stream: %s; this is not an AAC codec setup request. Ignore it: %02"PRIx8"%02"PRIx8,
						(pStream != NULL) ? (STR(pStream->GetName())) : "",
						pData[0], pData[1]);
				return true;
			}
			pCodecInfo = streamCapabilities.AddTrackAudioAAC(
					pData + 2, (uint8_t) (length - 2), true,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse AAC codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 9: //reserved
		default:
		{
			FATAL("Invalid audio codec ID %"PRIu32" detected on an input RTMP stream",
					pData[0] >> 4);
			return false;
		}
	}

	capabilitiesInitialized = true;

	return true;
}

bool InNetRTMPStream::InitializeVideoCapabilities(
		BaseInStream *pStream, StreamCapabilities &streamCapabilities,
		bool &capabilitiesInitialized, uint8_t *pData, uint32_t length) {
	if ((length == 0) || ((pData[0] >> 4) == 5)) {
		capabilitiesInitialized = false;
		return true;
	}
	CodecInfo *pCodecInfo = NULL;
	switch (pData[0]&0x0f) {
		case 1: //JPEG (currently unused)
		case 3: //Screen video
		case 5: //On2 VP6 with alpha channel
		case 6: //Screen video version 2
		{
			WARN("RTMP input video codec %"PRIu8" defaulted to pass through", (uint8_t) (pData[0]&0x0f));
			pCodecInfo = streamCapabilities.AddTrackVideoPassThrough(pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse pass-through codec setup bytes for input RTMP stream");
				return false;
			}
			break;
		}
		case 2: //Sorenson H.263
		{
			//17 - marker
			//5 - format1
			//8 - picture number/timestamp
			//3 - format2
			//2*8 or 2*16 - width/height depending on the format2
			if (length < 11) {
				FATAL("Not enough data to initialize Sorenson H.263 for an input RTMP stream. Wanted: %"PRIu32"; Got: %"PRIu32,
						(uint32_t) (11), length);
				return false;
			}
			pCodecInfo = streamCapabilities.AddTrackVideoSorensonH263(pData + 1,
					10,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse Sorenson H.263 headers for input RTMP stream");
				return false;
			}
			break;
		}
		case 4: //On2 VP6
		{
			if (length < 7) {
				FATAL("Not enough data to initialize On2 VP6 codec for an input RTMP stream. Wanted: %"PRIu32"; Got: %"PRIu32,
						(uint32_t) (7), length);
				return false;
			}
			pCodecInfo = streamCapabilities.AddTrackVideoVP6(pData + 1, 6,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse On2 VP6 codec for input RTMP stream");
				return false;
			}
			break;
		}
		case 7: //AVC
		{
			/*
			 * map:
			 * iiiiiiiiiii ss <sps> i pp <pps>
			 *
			 * i - ignored
			 * ss - sps length
			 * <sps> - sps data
			 * pp - pps length
			 * <pps> - pps data
			 */
			if (length < 11 + 2) {
				FATAL("Not enough data to initialize AVC codec for an input RTMP stream. Wanted: %"PRIu32"; Got: %"PRIu32,
						(uint32_t) (11 + 2), length);
				return false;
			}
			if (((pData[0] >> 4) != 1) || (pData[1] != 0)) {
				WARN("stream: %s; this is not a key frame or not a H264 codec setup request. Ignore it: %02"PRIx8"%02"PRIx8,
						(pStream != NULL) ? (STR(pStream->GetName())) : "",
						pData[0], pData[1]);
				return true;
			}
			uint32_t spsLength = ENTOHSP(pData + 11);
			if (length < 11 + 2 + spsLength + 1 + 2) {
				FATAL("Not enough data to initialize AVC codec for an input RTMP stream. Wanted: %"PRIu32"; Got: %"PRIu32,
						11 + 2 + spsLength + 1 + 2, length);
				return false;
			}
			uint32_t ppsLength = ENTOHSP(pData + 11 + 2 + spsLength + 1);
			if (length < 11 + 2 + spsLength + 1 + 2 + ppsLength) {
				FATAL("Invalid AVC codec packet length for an input RTMP stream. Wanted: %"PRIu32"; Got: %"PRIu32,
						11 + 2 + spsLength + 1 + 2 + ppsLength, length);
				return false;
			}
			uint8_t *pSPS = pData + 11 + 2;
			uint8_t *pPPS = pData + 11 + 2 + spsLength + 1 + 2;
			pCodecInfo = streamCapabilities.AddTrackVideoH264(
					pSPS, spsLength, pPPS, ppsLength, 90000, -1, -1, false,
					pStream);
			if (pCodecInfo == NULL) {
				FATAL("Unable to parse SPS/PPS for input RTMP stream");
				return false;
			}
			break;
		}
		default:
		{
			FATAL("Invalid audio codec ID %"PRIu32" detected on an input RTMP stream:",
					pData[0]&0x0f);
			return false;
		}
	}

	capabilitiesInitialized = true;

	return true;
}

void InNetRTMPStream::GetStats(Variant &info, uint32_t namespaceId) {
    BaseInStream::GetStats(info, namespaceId);
    Variant &protoConfig = _pProtocol->GetCustomParameters();
    if (protoConfig.HasKeyChain(V_STRING, false, 1, "tcUrl"))
        info["tcUrl"] = protoConfig["tcUrl"];
    if (protoConfig.HasKeyChain(V_STRING, false, 1, "swfUrl"))
        info["swfUrl"] = protoConfig["swfUrl"];
    if (protoConfig.HasKeyChain(V_STRING, false, 1, "pageUrl"))
        info["pageUrl"] = protoConfig["pageUrl"];
}
#endif /* HAS_PROTOCOL_RTMP */

