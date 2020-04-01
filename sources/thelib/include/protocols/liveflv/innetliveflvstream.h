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



#ifdef HAS_PROTOCOL_LIVEFLV
#ifndef _INNETLIVEFLVSTREAM_H
#define	_INNETLIVEFLVSTREAM_H

#include "streaming/baseinnetstream.h"

class DLLEXP InNetLiveFLVStream
: public BaseInNetStream {
private:
	double _lastVideoPts;
	double _lastVideoDts;

	double _lastAudioTime;

	Variant _lastStreamMessage;

	StreamCapabilities _streamCapabilities;
	bool _audioCapabilitiesInitialized;
	bool _videoCapabilitiesInitialized;
public:
	InNetLiveFLVStream(BaseProtocol *pProtocol, string name);
	virtual ~InNetLiveFLVStream();

	virtual StreamCapabilities * GetCapabilities();
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
	bool SendStreamMessage(Variant &completeMessage, bool persistent);
	bool SendStreamMessage(string functionName, Variant &parameters,
			bool persistent);
};


#endif	/* _INNETLIVEFLVSTREAM_H */
#endif /* HAS_PROTOCOL_LIVEFLV */

