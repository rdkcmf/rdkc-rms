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


#ifdef HAS_MEDIA_TS
#ifndef _TSDOCUMENT_H
#define	_TSDOCUMENT_H

#include "common.h"
#include "mediaformats/readers/basemediadocument.h"
#include "mediaformats/readers/ts/tsparsereventsink.h"

class TSParser;
class BaseInStream;

class TSDocument
: public BaseMediaDocument, TSParserEventsSink {
private:
	uint8_t _chunkSizeDetectionCount;
	uint8_t _chunkSize;
	TSParser *_pParser;
	bool _chunkSizeErrors;
	uint64_t _lastOffset;
public:
	TSDocument(Metadata &metadata);
	virtual ~TSDocument();

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
protected:
	virtual bool ParseDocument();
	virtual bool BuildFrames();
	virtual Variant GetPublicMeta();
private:
	void AddFrame(double pts, double dts, uint8_t frameType);
	bool DetermineChunkSize();
	bool GetByteAt(uint64_t offset, uint8_t &byte);
	bool TestChunkSize(uint8_t chunkSize);
};


#endif	/* _TSDOCUMENT_H */
#endif /* HAS_MEDIA_TS */
