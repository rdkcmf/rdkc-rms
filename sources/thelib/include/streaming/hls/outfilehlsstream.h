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
#ifndef _OUTFILEHLSTSSTREAM_H
#define	_OUTFILEHLSTSSTREAM_H

#include  "streaming/hls/baseouthlsstream.h"


class HLSPlaylist;
class BaseOutHLSStream;

class DLLEXP OutFileHLSStream
: public BaseOutHLSStream {
private:
	HLSPlaylist *_pPlaylist;
private:
	OutFileHLSStream(BaseProtocol *pProtocol, string name, Variant &settings);
public:
	static OutFileHLSStream* GetInstance(BaseClientApplication *pApplication,
			string name, Variant &settings);
	virtual ~OutFileHLSStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, Variant& videoParamsSizes);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) { return false; }
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool PushMetaData(string const& vmfMetadata, int64_t pts);
private:
	bool ClosePlaylist();
};

#endif	/* _OUTFILEHLSTSSTREAM_H */
#endif /* HAS_PROTOCOL_HLS */
