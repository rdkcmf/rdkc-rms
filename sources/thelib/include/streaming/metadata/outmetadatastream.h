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
/**
 * OutMetadataStream - handle the Video Metafile psuedo-stream
 * ST_OUT_META
 * 
 */
#ifndef __OUTMETADATASTREAM_H__
#define __OUTMETADATASTREAM_H__

#include "streaming/baseoutstream.h"
#include "protocols/baseprotocol.h"


class DLLEXP OutMetadataStream
: public BaseOutStream {
public:
	OutMetadataStream(BaseProtocol *pProtocol, uint64_t type, string name);

	// over-riding this call is the main purpose of this module
	virtual bool SendMetadata(string metadataStr);
	//
	// Over-ride and simply return true for the Data feed calls
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame)
		{return true;};
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts)
		{return true;};
	virtual bool IsCodecSupported(uint64_t codec) {return true;};
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio) {return true;};
	//
	// must override from BaseInStream
	// will these come in handy?
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew) {;}
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew) {;}
	//
	// ToDo: these should be covered, is Eclipse lying to me???
	// gotta implement these???
	virtual bool Play(double dts, double length) {return true;}
	virtual bool Pause() {return true;}
	virtual bool Resume() {return true;}
	virtual bool Seek(double dts) {return true;}
	virtual bool Stop();
	virtual bool SignalPlay(double &dts, double &length) {return true;}
	virtual bool SignalPause() {return true;}
	virtual bool SignalResume() {return true;}
	virtual bool SignalSeek(double &dts) {return true;}
	virtual bool SignalStop();
	virtual void ReadyForSend() {;}
	virtual bool IsCompatibleWithType(uint64_t type) {return true;}

private:
	bool _attached;
	BaseProtocol * _pProtocol;
	IOBuffer _ioBuf;
};


#endif // __OUTMETADATASTREAM_H__
