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


#ifndef _INNETPASSTHROUGHSTREAM_H
#define	_INNETPASSTHROUGHSTREAM_H

#include "streaming/baseinnetstream.h"

class BaseClientApplication;

class InNetPassThroughStream
: public BaseInNetStream {
public:
	InNetPassThroughStream(BaseProtocol *pProtocol, string name);
	virtual ~InNetPassThroughStream();

	virtual bool IsCompatibleWithType(uint64_t type);
	virtual StreamCapabilities * GetCapabilities();
	virtual void ReadyForSend();
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);

	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	bool RegisterOutboundUDP(BaseClientApplication *pApplication,
			Variant streamConfig);
};
#endif	/* _INNETPASSTHROUGHSTREAM_H */

