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



#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/inboundrtpprotocol.h"
#include "protocols/rtp/rtspprotocol.h"
#include "protocols/rtp/streaming/innetrtpstream.h"
#include "protocols/rtp/connectivity/inboundconnectivity.h"
#include "streaming/streamdebug.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "streaming/codectypes.h"
#include "utils/misc/socketaddress.h"

InboundRTPProtocol::InboundRTPProtocol()
: BaseProtocol(PT_INBOUND_RTP) {
	_spsPpsPeriod = 0;
	_pInStream = NULL;
	_pConnectivity = NULL;
	memset(&_rtpHeader, 0, sizeof (_rtpHeader));
	_lastSeq = 0;
	_seqRollOver = 0;
	_isAudio = false;
	_packetsCount = 0;
	_unsolicited = false;
	_videoCodec = CODEC_UNKNOWN;
#ifdef RTP_DETECT_ROLLOVER
	_lastTimestamp = 0;
	_timestampRollover = 0;
#endif /* RTP_DETECT_ROLLOVER */
#ifdef HAS_STREAM_DEBUG
	_pStreamDebugFile = new StreamDebugFile(format("./rtp_%"PRIu32".bin", GetId()),
			sizeof (StreamDebugRTP));
#endif /* HAS_STREAM_DEBUG */
}

InboundRTPProtocol::~InboundRTPProtocol() {
	// Free up _pInstream if this is unsolicited RTP
	if (_unsolicited && (_pInStream != NULL)) {
		delete _pInStream;
		_pInStream = NULL;
	}
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL) {
		delete _pStreamDebugFile;
		_pStreamDebugFile = NULL;
	}
#endif /* HAS_STREAM_DEBUG */
}

bool InboundRTPProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool InboundRTPProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_RTSP
			|| type == PT_UDP;
}

bool InboundRTPProtocol::AllowNearProtocol(uint64_t type) {
	return type == PT_INBOUND_TS;
}

bool InboundRTPProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}

bool InboundRTPProtocol::SignalInputData(IOBuffer &buffer) {
	NYIR;
}

IOBuffer* InboundRTPProtocol::GetOutputBuffer() {
	return &_outputBuffer;
}

SocketAddress* InboundRTPProtocol::GetDestInfo() {
	SocketAddress* sa = new SocketAddress();
	sa->setIPAddress(_farIP, _farPort);


	return sa;
}


void InboundRTPProtocol::SetOutboundPayload(string const& farIP, uint16_t farPort, string const& payload) {
	_outputBuffer.IgnoreAll();
	_outputBuffer.ReadFromString(payload);
	_farIP = farIP;
	_farPort = farPort;
}

bool InboundRTPProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	NYIR;
}


bool InboundRTPProtocol::SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength,
		SocketAddress *pPeerAddress) {
	//1. Get the raw buffer and its length
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
#ifdef HAS_STREAM_DEBUG
	if (_pStreamDebugFile != NULL)
		_pStreamDebugFile->PushRTP(pBuffer, length);
#endif /* HAS_STREAM_DEBUG */

	//2. Do we have enough data?
	if (length < 12) {
		buffer.IgnoreAll();
		return true;
	}

	//3. Get the RTP header parts that we are interested in
	_rtpHeader._flags = ENTOHLP(pBuffer);
	_rtpHeader._timestamp = ENTOHLP(pBuffer + 4);
	_rtpHeader._ssrc = ENTOHLP(pBuffer + 8);

	//4. Advance the sequence roll-over counter if necessary
	if (GET_RTP_SEQ(_rtpHeader) < _lastSeq) {
		if ((_lastSeq - GET_RTP_SEQ(_rtpHeader)) > (0xffff >> 2)) {
			_seqRollOver++;
			_lastSeq = GET_RTP_SEQ(_rtpHeader);
		} else {
			buffer.IgnoreAll();
			return true;
		}
	} else {
		_lastSeq = GET_RTP_SEQ(_rtpHeader);
	}

	//5. Do we have enough data?
	if (length < ((uint32_t) 12 + GET_RTP_CC(_rtpHeader)*4 + 1)) {
		buffer.IgnoreAll();
		return true;
	}

	//6. Skip the RTP header
	pBuffer += 12 + GET_RTP_CC(_rtpHeader)*4;
	length -= 12 + GET_RTP_CC(_rtpHeader)*4;
	if (GET_RTP_P(_rtpHeader)) {
		uint32_t rtpPLength = (uint32_t)pBuffer[length - 1];
		if (length < rtpPLength)
			return false;
		length -= rtpPLength;
	}

	if (GET_RTP_X(_rtpHeader)) {
		if (length < 4)
			return false;
		uint32_t extLen = (((uint32_t) ENTOHSP(pBuffer + 2)) + 1)*4;
		if (length < extLen)
			return false;
		pBuffer += extLen;
		length -= extLen;
	}

	if (length == 0) {
		FATAL("RTP payload size is 0 bytes");
		return false;
	}

	//7. Detect rollover and adjust the timestamp
#ifdef RTP_DETECT_ROLLOVER
	if (_rtpHeader._timestamp < _lastTimestamp) {
		if ((((_rtpHeader._timestamp & 0x80000000) >> 31) == 0)
				&& (((_lastTimestamp & 0x80000000) >> 31) == 1)) {
			_timestampRollover++;
			_lastTimestamp = _rtpHeader._timestamp;
		}
	} else {
		_lastTimestamp = _rtpHeader._timestamp;
	}
	_rtpHeader._timestamp = (_timestampRollover << 32) | _rtpHeader._timestamp;
#endif
	// if codec is mp2t, send the the data to inboundts protocol instead
	if (_videoCodec == CONTAINER_AUDIO_VIDEO_MP2T) {
		if (NULL == _pNearProtocol) {
			InboundTSProtocol* pInboundTs = new InboundTSProtocol();
			if (!pInboundTs->Initialize(GetCustomParameters())) {
				FATAL("Unable to initialize InboundTSProtocol");
				delete pInboundTs;
				return false;
			}
			SetNearProtocol(pInboundTs);
			_pNearProtocol->SetFarProtocol(this);
			_pNearProtocol->SetApplication(GetApplication());
		}
		_inputBuffer.ReadFromBuffer(pBuffer, length);
		if (NULL != _pNearProtocol && !_pNearProtocol->SignalInputData(_inputBuffer)) {
			FATAL("Unable to send payload to InboundTSProtocol");
			return false;
		}
	}

	//5. Feed the data to the stream
	if (_pInStream != NULL && _videoCodec != CONTAINER_AUDIO_VIDEO_MP2T) {
		if (_isAudio) {
			if (!_pInStream->FeedAudioData(pBuffer, length, _rtpHeader)) {
				FATAL("Unable to stream data");
				if (_pConnectivity != NULL) {
					_pConnectivity->EnqueueForDelete();
					_pConnectivity = NULL;
				}
				return false;
			}
		} else {
			if (!_pInStream->FeedVideoData(pBuffer, length, _rtpHeader)) {
				FATAL("Unable to stream data");
				if (_pConnectivity != NULL) {
					_pConnectivity->EnqueueForDelete();
					_pConnectivity = NULL;
				}
				return false;
			}
		}
	}

	//6. Ignore the data
	buffer.IgnoreAll();

	//7. Increment the packets count
	_packetsCount++;

	//8. Send the RR if necesary
	if ((_packetsCount % 300) == 0) {

		if (_pConnectivity != NULL) {
			if (!_pConnectivity->SendRR(_isAudio)) {
				FATAL("Unable to send RR");
				_pConnectivity->EnqueueForDelete();
				_pConnectivity = NULL;
				return false;
			}
		}
	}

	//7. Done
	return true;
}

uint32_t InboundRTPProtocol::GetSSRC() {
	return _rtpHeader._ssrc;
}

uint32_t InboundRTPProtocol::GetExtendedSeq() {
	return (((uint32_t) _seqRollOver) << 16) | _lastSeq;
}

void InboundRTPProtocol::SetStream(InNetRTPStream *pInStream, bool isAudio,
		bool unsolicited) {
	_pInStream = pInStream;
	_isAudio = isAudio;
	_unsolicited = unsolicited;
}

void InboundRTPProtocol::SetInbboundConnectivity(InboundConnectivity *pConnectivity) {
	_pConnectivity = pConnectivity;
}
#endif /* HAS_PROTOCOL_RTP */
