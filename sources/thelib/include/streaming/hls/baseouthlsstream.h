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


#ifdef HAS_PROTOCOL_HLS
#ifndef _BASEOUTHLSTSSTREAM_H
#define	_BASEOUTHLSTSSTREAM_H

#include "streaming/baseoutstream.h"

class DLLEXP BaseOutHLSStream
: public BaseOutStream {
protected:
	Variant _settings;
private:
	double _timeBase;
public:
	BaseOutHLSStream(BaseProtocol *pProtocol, uint64_t type, string name,
			Variant &settings, double timeBase);
	virtual ~BaseOutHLSStream();
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual bool IsCodecSupported(uint64_t codec);
	virtual bool SendMetadata(string const& metadataStr, int64_t pts);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
};
#endif	/* _BASEOUTHLSTSSTREAM_H */
#endif /* HAS_PROTOCOL_HLS */
