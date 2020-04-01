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

#ifndef _OUTNETFMP4STREAM_H
#define	_OUTNETFMP4STREAM_H

#include "streaming/baseoutnetstream.h"
#include "streaming/fmp4/fmp4writer.h"
#include "protocols/wrtc/wrtcconnection.h"

class BaseClientApplication;
class WrtcConnection;

class OutNetFMP4Stream
: public BaseOutNetStream, public FMP4Writer {
private:
	IOBufferExt _completedMoov;
	IOBufferExt _inProgressMoov;
	IOBufferExt _completedFragment;
	IOBufferExt _inProgressFragment;
	IOBufferExt _progressiveBuffer;
	map<uint32_t, void *> _signaledProtocols;
	uint32_t _emptyRunsCount;
	bool _progressive;
	bool _useHttp;
	vector<uint32_t> _pendigRemovals;
	bool _delayedRemoval;
	bool _dtsNeedsUpdate;
	double _pauseDts;
	double _resumeDts;
	bool _wrtcChannelReady;
	uint32_t _wrtcProtocolId;
	bool _firstFrameSent;
	bool _isOutStreamStackEstablished; // indicates when the UDP<->IDTLS<->SCTP<->WRTCCNX(20) is linked to the outstream
public:
	OutNetFMP4Stream(BaseProtocol *pProtocol, string name, bool progressive, bool useHttp = false);
	virtual ~OutNetFMP4Stream();

	static OutNetFMP4Stream *GetInstance(BaseClientApplication *pApplication,
			string name, bool progressive, bool useHttp = false);
	void RegisterOutputProtocol(uint32_t protocolId, void *pUserData);
	void UnRegisterOutputProtocol(uint32_t protocolId);
	bool IsProgressive() const;
	virtual bool Flush();
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalPing();
	void UpdatePauseStatus(bool isNowPaused);
	void SetWrtcConnection(WrtcConnection* pWrtc);
	void SetOutStreamStackEstablished();
	virtual bool PushMetaData(string const& vmfMetadata, int64_t pts);
	virtual void SetInboundVod(bool inboundVod);
	virtual bool GenericProcessData(uint8_t * pData, uint32_t dataLength, uint32_t processedLength, uint32_t totalLength, double pts, double dts, bool isAudio);
protected:
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
	virtual bool BeginWriteMOOV();
	virtual bool EndWriteMOOV(uint32_t durationOffset, bool durationIs64Bits);
	virtual bool BeginWriteFragment();
	virtual bool EndWriteFragment(double moovStart, double moovDuration,
			double moofStart);
	virtual bool WriteProgressiveBuffer(FMP4ProgressiveBufferType type,
			const uint8_t *pBuffer, size_t size);
	virtual bool WriteBuffer(FMP4WriteState state, const uint8_t *pBuffer, size_t size);
	bool SignalProtocols(const IOBuffer &buffer, bool isMetadata = false, void *metadata = NULL);
	virtual bool SendMetadata(string const& vmfMetadata, int64_t pts);
	virtual bool SendMetadata(Variant sdpMetadata, int64_t pts);
private:
	WrtcConnection *GetWrtcConnection();
};

#endif	/* _OUTNETFMP4STREAM_H */
