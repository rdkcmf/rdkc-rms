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


#ifndef _INFILERTSPSTREAM_H
#define	_INFILERTSPSTREAM_H

#include "streaming/baseinfilestream.h"
#include "mediaformats/readers/streammetadataresolver.h"

class RTSPProtocol;
class StreamsManager;

class InFileRTSPStream
: public BaseInFileStream {
public:
	InFileRTSPStream(BaseProtocol *pProtocol, string name);
	virtual ~InFileRTSPStream();

	static InFileRTSPStream *GetInstance(RTSPProtocol *pRTSPProtocol,
			StreamsManager *pStreamsManager, Metadata &metadata);

	bool Initialize(Metadata &metadata);
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual bool FeedFrame(MediaFrame &frame, bool dtsAdjusted);
private:
	virtual bool FeedAudioFrame(MediaFrame &frame);
	virtual bool FeedVideoFrame(MediaFrame &frame);
};


#endif	/* _INFILERTSPSTREAM_H */
