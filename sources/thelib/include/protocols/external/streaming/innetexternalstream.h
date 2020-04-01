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


#ifdef HAS_PROTOCOL_EXTERNAL
#ifndef _INNETEXTERNALSTREAM_H
#define	_INNETEXTERNALSTREAM_H

#include "streaming/baseinnetstream.h"
#include "protocols/external/rmsprotocol.h"
#include "streaming/streamcapabilities.h"

class InNetExternalStream
: public BaseInNetStream {
private:
	in_stream_t _streamInterface;
	StreamCapabilities _streamCapabilities;
public:
	InNetExternalStream(BaseProtocol *pProtocol, string name, void *pUserData,
			connection_t *pConnection);
	virtual ~InNetExternalStream();

	in_stream_t *GetInterface();
	virtual StreamCapabilities * GetCapabilities();
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void ReadyForSend();
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);

	bool SetupAudioCodecAAC(const uint8_t pCodec[2]);
	bool SetupAudioCodecG711();
	bool SetupVideoCodecH264(const uint8_t *pSPS, uint32_t spsLength,
			const uint8_t *pPPS, uint32_t ppsLength);
private:
	void AddStreamToConnectionInterface();
	void RemoveStreamFromConnectionInterface();
	void SignalAttachedOrDetached(BaseOutStream *pOutStream, bool attached);
};

#endif	/* _INNETEXTERNALSTREAM_H */
#endif /* HAS_PROTOCOL_EXTERNAL */
