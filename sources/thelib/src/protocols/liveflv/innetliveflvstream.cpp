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


#ifdef HAS_PROTOCOL_LIVEFLV
#include "protocols/liveflv/innetliveflvstream.h"
#include "protocols/baseprotocol.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "protocols/rtmp/messagefactories/streammessagefactory.h"
#include "protocols/rtmp/streaming/innetrtmpstream.h"
#include "eventlogger/eventlogger.h"

InNetLiveFLVStream::InNetLiveFLVStream(BaseProtocol *pProtocol, string name)
: BaseInNetStream(pProtocol, ST_IN_NET_LIVEFLV, name) {
	_lastVideoPts = 0;
	_lastVideoDts = 0;

	_lastAudioTime = 0;

	_audioCapabilitiesInitialized = false;
	_videoCapabilitiesInitialized = false;
}

InNetLiveFLVStream::~InNetLiveFLVStream() {
    GetEventLogger()->LogStreamClosed(this);
}

StreamCapabilities * InNetLiveFLVStream::GetCapabilities() {
    return &_streamCapabilities;
}

bool InNetLiveFLVStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (isAudio) {
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += dataLength;
		if ((!_audioCapabilitiesInitialized) && (processedLength == 0)) {
			if (!InNetRTMPStream::InitializeAudioCapabilities(this,
					_streamCapabilities, _audioCapabilitiesInitialized, pData,
					dataLength)) {
				FATAL("Unable to initialize audio capabilities");
				return false;
			}
			_streamCapabilities.GetRTMPMetadata(_publicMetadata);
		}
		_lastAudioTime = pts;
	} else {
		SavePts(pts);
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
		if ((!_videoCapabilitiesInitialized) && (processedLength == 0)) {
			if (!InNetRTMPStream::InitializeVideoCapabilities(this,
					_streamCapabilities, _videoCapabilitiesInitialized, pData,
					dataLength)) {
				FATAL("Unable to initialize audio capabilities");
				return false;
			}
			_streamCapabilities.GetRTMPMetadata(_publicMetadata);
		}
		_lastVideoPts = pts;
		_lastVideoDts = dts;
	}

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->FeedData(pData, dataLength, processedLength, totalLength,
				pts, dts, isAudio)) {
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

void InNetLiveFLVStream::ReadyForSend() {

}

bool InNetLiveFLVStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
			|| TAG_KIND_OF(type, ST_OUT_NET_RTP)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HLS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_HDS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MSS)
            || TAG_KIND_OF(type, ST_OUT_FILE_DASH)
			|| TAG_KIND_OF(type, ST_OUT_FILE_TS)
			|| TAG_KIND_OF(type, ST_OUT_FILE_MP4)
			|| TAG_KIND_OF(type, ST_OUT_FILE_RTMP_FLV)
			|| TAG_KIND_OF(type, ST_OUT_NET_TS)
			|| TAG_KIND_OF(type, ST_OUT_VMF)
			|| TAG_KIND_OF(type, ST_OUT_NET_FMP4)
			|| TAG_KIND_OF(type, ST_OUT_NET_MP4)
			;
}

void InNetLiveFLVStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
	if (_lastStreamMessage != V_NULL) {
		if (TAG_KIND_OF(pOutStream->GetType(), ST_OUT_NET_RTMP)) {
			if (!((OutNetRTMPStream *) pOutStream)->SendStreamMessage(
					_lastStreamMessage)) {
				FATAL("Unable to send notify on stream. The connection will go down");
				pOutStream->EnqueueForDelete();
			}
		}
	}
}

void InNetLiveFLVStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {

}

bool InNetLiveFLVStream::SignalPlay(double &dts, double &length) {
    return true;
}

bool InNetLiveFLVStream::SignalPause() {
    return true;
}

bool InNetLiveFLVStream::SignalResume() {
    return true;
}

bool InNetLiveFLVStream::SignalSeek(double &dts) {
    return true;
}

bool InNetLiveFLVStream::SignalStop() {
    return true;
}

bool InNetLiveFLVStream::SendStreamMessage(Variant &completeMessage, bool persistent) {
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

	//4. Save the message for future use if necessary
	if (persistent)
		_lastStreamMessage = completeMessage;

	//5. Done
	return true;
}

bool InNetLiveFLVStream::SendStreamMessage(string functionName, Variant &parameters,
		bool persistent) {

	//1. Prepare the message
	Variant message;

	if (persistent && (lowerCase(functionName) == "onmetadata")) {

		FOR_MAP(parameters, string, Variant, i) {
			if (MAP_VAL(i) == V_MAP) {
				_publicMetadata = MAP_VAL(i);
				MAP_VAL(i)[HTTP_HEADERS_SERVER] = BRANDING_BANNER;
			}
		}
		_streamCapabilities.GetRTMPMetadata(_publicMetadata);
		message = StreamMessageFactory::GetFlexStreamSend(0, 0, 0, false,
				functionName, _publicMetadata);
	} else {
		message = StreamMessageFactory::GetFlexStreamSend(0, 0, 0, false,
				functionName, parameters);
	}

	return SendStreamMessage(message, persistent);
}

#endif /* HAS_PROTOCOL_LIVEFLV */
