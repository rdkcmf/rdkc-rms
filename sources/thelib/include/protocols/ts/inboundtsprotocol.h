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



#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
#ifndef _INBOUNDTSPROTOCOL_H
#define	_INBOUNDTSPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "mediaformats/readers/ts/pidtypes.h"
#include "mediaformats/readers/ts/tspacketheader.h"
#include "mediaformats/readers/ts/piddescriptor.h"
#include "mediaformats/readers/ts/tsparsereventsink.h"

#define TS_CHUNK_188 188
#define TS_CHUNK_204 204
#define TS_CHUNK_208 208

class TSParser;
class BaseTSAppProtocolHandler;
class InNetTSStream;

class DLLEXP InboundTSProtocol
: public BaseProtocol, TSParserEventsSink {
private:
	uint32_t _chunkSizeDetectionCount;
	uint32_t _chunkSize;
	BaseTSAppProtocolHandler *_pProtocolHandler;
	TSParser *_pParser;
	InNetTSStream *_pInStream;
#ifdef HAS_MULTIPROGRAM_TS
	Variant _filters;
#endif	/* HAS_MULTIPROGRAM_TS */
	
public:
	InboundTSProtocol();
	virtual ~InboundTSProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer);

	virtual void SetApplication(BaseClientApplication *pApplication);
	BaseTSAppProtocolHandler *GetProtocolHandler();
	uint32_t GetChunkSize();

	virtual BaseInStream *GetInStream();
	virtual void SignalResetChunkSize();
	virtual void SignalPAT(TSPacketPAT *pPAT);
	virtual void SignalPMT(TSPacketPMT *pPMT);
	virtual void SignalPMTComplete();
	virtual bool SignalStreamsPIDSChanged(map<uint16_t, TSStreamInfo> &streams);
	virtual bool SignalStreamPIDDetected(TSStreamInfo &streamInfo,
			BaseAVContext *pContext, PIDType type, bool &ignore);
	virtual void SignalUnknownPIDDetected(TSStreamInfo &streamInfo);
	virtual bool FeedData(BaseAVContext *pContext, uint8_t *pData,
			uint32_t dataLength, double pts, double dts, bool isAudio);
private:
	bool DetermineChunkSize(IOBuffer &buffer);
};


#endif	/* _INBOUNDTSPROTOCOL_H */
#endif	/* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */

