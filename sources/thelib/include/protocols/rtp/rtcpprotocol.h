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
#ifndef _RTCPPROTOCOL_H
#define	_RTCPPROTOCOL_H

#include "protocols/baseprotocol.h"

class InboundConnectivity;
class SocketAddress;
#ifdef HAS_STREAM_DEBUG
class StreamDebugFile;
#endif /* HAS_STREAM_DEBUG */

class DLLEXP RTCPProtocol
: public BaseProtocol {
private:
	InboundConnectivity *_pConnectivity;
	uint32_t _lsr;
	uint8_t _buff[32];
	SocketAddress _lastAddress;
	bool _isAudio;
	uint32_t _ssrc;
	bool _validLastAddress;
#ifdef HAS_STREAM_DEBUG
	StreamDebugFile *_pStreamDebugFile;
#endif /* HAS_STREAM_DEBUG */
	IOBuffer _outputBuffer;
	string _farIP;
	uint16_t _farPort;
public:
	RTCPProtocol();
	virtual ~RTCPProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer);
	IOBuffer* GetOutputBuffer();
	void SetOutboundPayload(string const& farIP, uint16_t farPort, string const& payload);
	SocketAddress* GetDestInfo();

	uint32_t GetLastSenderReport();
	SocketAddress *GetLastAddress();
	uint32_t GetSSRC();

	void SetInbboundConnectivity(InboundConnectivity *pConnectivity, bool isAudio);
};


#endif	/* _RTCPPROTOCOL_H */
#endif /* HAS_PROTOCOL_RTP */

