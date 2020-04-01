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
#ifndef _INNETTSSTREAM_H
#define	_INNETTSSTREAM_H

#include "streaming/baseinnetstream.h"
#include "streaming/streamcapabilities.h"

class DLLEXP InNetTSStream
: public BaseInNetStream {
private:
	//audio section
	bool _hasAudio;

	//video section
	bool _hasVideo;

	StreamCapabilities _streamCapabilities;
	bool _enabled;
public:
	InNetTSStream(BaseProtocol *pProtocol, string name, uint32_t bandwidthHint);
	virtual ~InNetTSStream();
	virtual StreamCapabilities * GetCapabilities();

	bool HasAudio();
	void HasAudio(bool value);
	bool HasVideo();
	void HasVideo(bool value);
	void Enable(bool value);

	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void ReadyForSend();
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
};


#endif	/* _INNETTSSTREAM_H */
#endif	/* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */

