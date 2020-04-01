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
#ifndef _OUTNETHLSTSSTREAM_H
#define	_OUTNETHLSTSSTREAM_H

#include  "streaming/hls/baseouthlsstream.h"
#include  "streaming/hls/nettssegment.h"

class NetTSSegment;
class BaseOutHLSStream;
class UDPSenderProtocol;

class DLLEXP OutNetHLSStream
: public BaseOutHLSStream {
private:
	double _lastPatPmtSpsPpsTimestamp;
	UDPSenderProtocol *_pSender;
	NetTSSegment *_pCurrentSegment;
	IOBuffer _segmentVideoBuffer;
	IOBuffer _segmentAudioBuffer;
	IOBuffer _segmentPatPmtAndCountersBuffer;
	bool _enabled;
public:
	OutNetHLSStream(BaseProtocol *pProtocol, string name, Variant &settings);
	static OutNetHLSStream* GetInstance(BaseClientApplication *pApplication,
			string name, Variant &settings);
	virtual ~OutNetHLSStream();

	void Enable(bool value);
	bool InitUdpSender(Variant &config);
	uint16_t GetUdpBindPort();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
	virtual bool SendMetadata(string metadataStr) {return false;};
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			Variant& videoParamsSizes);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) { return false; }
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
private:
	bool WritePatPmt(double ts);
};

#endif	/* _OUTNETHLSTSSTREAM_H */
#endif /* HAS_PROTOCOL_HLS */
