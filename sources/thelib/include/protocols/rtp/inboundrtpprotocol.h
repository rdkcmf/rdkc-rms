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
#ifndef _INBOUNDRTPPROTOCOL_H
#define	_INBOUNDRTPPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/rtp/rtpheader.h"

class SocketAddress;
class InNetRTPStream;
class InboundConnectivity;
#ifdef HAS_STREAM_DEBUG
class StreamDebugFile;
#endif /* HAS_STREAM_DEBUG */

//#define RTP_DETECT_ROLLOVER

class DLLEXP InboundRTPProtocol
: public BaseProtocol {
private:
	RTPHeader _rtpHeader;
	uint8_t _spsPpsPeriod;
	InNetRTPStream *_pInStream;
	InboundConnectivity *_pConnectivity;
	uint16_t _lastSeq;
	uint16_t _seqRollOver;
	bool _isAudio;
	uint32_t _packetsCount;
	bool _unsolicited;
	uint64_t _videoCodec;
#ifdef RTP_DETECT_ROLLOVER
	uint64_t _lastTimestamp;
	uint64_t _timestampRollover;
#endif /* RTP_DETECT_ROLLOVER */
#ifdef HAS_STREAM_DEBUG
	StreamDebugFile *_pStreamDebugFile;
#endif /* HAS_STREAM_DEBUG */
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
	string _farIP;
	uint16_t _farPort;
public:
	InboundRTPProtocol();
	virtual ~InboundRTPProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength, SocketAddress *pPeerAddress);
	IOBuffer* GetOutputBuffer();
	void SetOutboundPayload(string const& farIP, uint16_t farPort, string const& payload);
	SocketAddress* GetDestInfo();

	uint32_t GetSSRC();
	uint32_t GetExtendedSeq();

	void SetStream(InNetRTPStream *pInStream, bool isAudio, bool unsolicited);
	void SetInbboundConnectivity(InboundConnectivity *pConnectivity);
	inline void SetVideoCodec(uint64_t codec) { _videoCodec = codec; }
};


#endif	/* _INBOUNDRTPPROTOCOL_H */
#endif /* HAS_PROTOCOL_RTP */
