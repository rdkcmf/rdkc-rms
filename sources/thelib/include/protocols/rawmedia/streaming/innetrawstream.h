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


#ifndef _INNETRAWSTREAM_H
#define _INNETRAWSTREAM_H

#include "streaming/baseinnetstream.h"

class BaseProtocol;

class InNetRawStream
	: public BaseInNetStream {
private:
	vector<uint8_t> _nalPrefix;
	IOBuffer _sps;
	IOBuffer _pps;
	bool _videoCapsChanged;
	StreamCapabilities _streamCaps;
public:
	InNetRawStream(BaseProtocol *pProtocol, string name);
	virtual ~InNetRawStream();
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void ReadyForSend();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual StreamCapabilities *GetCapabilities();
	void SetAudioConfig(uint64_t type, uint8_t *pConfigData, uint32_t dataLength);
	void SetVideoConfig(uint64_t type, uint8_t *pConfigData, uint32_t dataLength);
	bool FeedData(uint8_t *pData, uint32_t dataLength, double ts, bool isAudio);
private:
	// returns pointer of the next nalPrefix occurence, NULL if not found
	bool FeedVideoData(uint8_t *pData, uint32_t dataLength, double ts);
	bool FeedAudioData(uint8_t *pData, uint32_t dataLength, double ts);
	void FeedVideoFrame(uint8_t *pData, uint32_t dataLength, double ts);
	uint8_t *FindNalu(uint8_t *pData, uint32_t offset, uint32_t dataLength);
};

#endif /* _INNETRAWSTREAM_H */
