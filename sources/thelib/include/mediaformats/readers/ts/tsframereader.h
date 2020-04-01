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
#ifndef _TSFRAMEREADER_H
#define	_TSFRAMEREADER_H

#include "mediaformats/readers/ts/tsparser.h"
#include "mediaformats/readers/ts/tsparsereventsink.h"
#include "mediaformats/readers/mediafile.h"
#include "mediaformats/readers/mediaframe.h"
#include "streaming/streamcapabilities.h"

class TSFrameReaderInterface;
class BaseInStream;

class TSFrameReader
: public TSParser, TSParserEventsSink {
private:
	MediaFile *_pFile;
	bool _freeFile;
	uint8_t _chunkSizeDetectionCount;
	uint8_t _chunkSize;
	uint32_t _defaultBlockSize;
	bool _chunkSizeErrors;
	IOBuffer _chunkBuffer;
	bool _frameAvailable;
	StreamCapabilities _streamCapabilities;
	bool _eof;
	TSFrameReaderInterface *_pInterface;
public:
	TSFrameReader(TSFrameReaderInterface *pInterface);
	virtual ~TSFrameReader();

	bool SetFile(string filePath);
	bool IsEOF();
	bool Seek(uint64_t offset);
	bool ReadFrame();

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
	void FreeFile();
	bool DetermineChunkSize();
	bool TestChunkSize(uint8_t chunkSize);
	bool GetByteAt(uint64_t offset, uint8_t &byte);
};


#endif	/* _TSFRAMEREADER_H */
#endif	/* HAS_MEDIA_TS */

