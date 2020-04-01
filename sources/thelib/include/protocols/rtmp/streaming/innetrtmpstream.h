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
#ifndef _INNETRTMPSTREAM_H
#define	_INNETRTMPSTREAM_H

#include "streaming/baseinnetstream.h"
#include "streaming/streamcapabilities.h"
#include "mediaformats/readers/streammetadataresolver.h"

class BaseRTMPProtocol;
class BaseOutStream;

class DLLEXP InNetRTMPStream
: public BaseInNetStream {
private:
	uint32_t _rtmpStreamId;
	uint32_t _chunkSize;
	uint32_t _channelId;
	string _clientId;
	int32_t _videoCts;

	bool _dummy;
	uint8_t _lastAudioCodec;
	uint8_t _lastVideoCodec;
	StreamCapabilities _streamCapabilities;

	IOBuffer _aggregate;

	string _privateName;
public:
	InNetRTMPStream(BaseProtocol *pProtocol, string name, uint32_t rtmpStreamId,
			uint32_t chunkSize, uint32_t channelId, string privateName);
	virtual ~InNetRTMPStream();
	virtual StreamCapabilities * GetCapabilities();

	virtual void ReadyForSend();
	virtual bool IsCompatibleWithType(uint64_t type);

	uint32_t GetRTMPStreamId();
	uint32_t GetChunkSize();
	void SetChunkSize(uint32_t chunkSize);

	bool SendStreamMessage(Variant &message);
	bool SendOnStatusMessages();
	bool RecordFLV(Metadata &meta, bool append);
	bool RecordMP4(Metadata &meta);

	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool FeedDataAggregate(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);

	static bool InitializeAudioCapabilities(BaseInStream *pStream,
			StreamCapabilities &streamCapabilities,
			bool &capabilitiesInitialized, uint8_t *pData, uint32_t length);
	static bool InitializeVideoCapabilities(BaseInStream *pStream,
			StreamCapabilities &streamCapabilities,
			bool &capabilitiesInitialized, uint8_t *pData, uint32_t length);
	virtual uint32_t GetInputVideoTimescale();
	virtual uint32_t GetInputAudioTimescale();
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
private:
	void AnalyzeMetadata(Variant &completeMessage);
	string GetRecordedFileName(Metadata &meta);
	BaseRTMPProtocol *GetRTMPProtocol();
};


#endif	/* _INNETRTMPSTREAM_H */

#endif /* HAS_PROTOCOL_RTMP */

