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
#ifndef _AVCONTEXT_H
#define	_AVCONTEXT_H

#include "common.h"

class StreamCapabilities;
class TSParserEventsSink;
class BaseInStream;

class BaseAVContext {
public:

	struct {
		double time;
		uint64_t lastRaw;
		uint32_t rollOverCount;
	} _pts, _dts;
	int8_t _sequenceNumber;
	uint64_t _droppedPacketsCount;
	uint64_t _droppedBytesCount;
	uint64_t _packetsCount;
	uint64_t _bytesCount;
	IOBuffer _bucket;
	StreamCapabilities *_pStreamCapabilities;
	TSParserEventsSink *_pEventsSink;
public:
	BaseAVContext();
	virtual ~BaseAVContext();
	virtual void Reset();
	void DropPacket();
	virtual bool HandleData() = 0;
	bool FeedData(uint8_t *pData, uint32_t dataLength, double pts, double dts,
			bool isAudio);
	BaseInStream *GetInStream();
private:
	void InternalReset();
};

class H264AVContext
: public BaseAVContext {
private:
	IOBuffer _SPS;
	IOBuffer _PPS;
	vector<IOBuffer *> _backBuffers;
	vector<IOBuffer *> _backBuffersCache;
	double _backBuffersPts;
	double _backBuffersDts;
	bool _pairedSpsPps;
public:
	H264AVContext();
	virtual ~H264AVContext();
	virtual void Reset();
	virtual bool HandleData();
private:
	void EmptyBackBuffers();
	void DiscardBackBuffers();
	void InsertBackBuffer(uint8_t *pBuffer, int32_t length);
	bool ProcessNal(uint8_t *pBuffer, int32_t length, double pts, double dts);
	void InitializeCapabilities(uint8_t *pData, uint32_t length);
	void InternalReset();
};

class AACAVContext
: public BaseAVContext {
private:
	double _lastSentTimestamp;
	double _samplingRate;
	bool _initialMarkerFound;
	uint32_t _markerRetryCount;
public:
	AACAVContext();
	virtual ~AACAVContext();
	virtual void Reset();
	virtual bool HandleData();
private:
	void InitializeCapabilities(uint8_t *pData, uint32_t length);
	void InternalReset();
};
#endif	/* _AVCONTEXT_H */
#endif	/* HAS_MEDIA_TS */
