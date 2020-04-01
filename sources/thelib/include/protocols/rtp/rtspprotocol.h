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


#ifdef HAS_PROTOCOL_RTP
#ifndef _RTSPPROTOCOL_H
#define	_RTSPPROTOCOL_H

#include "protocols/rtp/basertspprotocol.h"
#include "protocols/timer/basetimerprotocol.h"
#include "protocols/rtp/sdp.h"
#include "streaming/baseoutnetrtpudpstream.h"
#include "streaming/baseinfilestream.h"

class BaseRTSPAppProtocolHandler;
class BaseInNetStream;
class BaseOutNetRTPUDPStream;
class InboundRTPProtocol;
class InNetRTPStream;
class OutboundConnectivity;
class InboundConnectivity;
class BaseOutStream;
class InFileRTSPStream;
class PassThroughProtocol;
struct RTPClient;

//#define RTSP_DUMP_TRAFFIC

class DLLEXP RTSPProtocol
: public BaseRTSPProtocol {
private:

	class RTSPKeepAliveTimer
	: public BaseTimerProtocol {
	private:
		uint32_t _protocolId;
	public:
		RTSPKeepAliveTimer(uint32_t protocolId);
		virtual ~RTSPKeepAliveTimer();
		virtual bool TimePeriodElapsed();
	};
#ifdef HAS_RTSP_LAZYPULL
	bool _waiting;
	string _pendingInputStreamName;
#endif /* HAS_RTSP_LAZYPULL */
#ifdef RTSP_DUMP_TRAFFIC
	string _debugInputData;
#endif /* RTSP_DUMP_TRAFFIC */
	BaseVM *_pCallbacksVM;
	uint32_t _requestedRtpBlocksize;
protected:
	uint32_t _state;
	bool _rtpData;
	uint32_t _rtpDataLength;
	uint32_t _rtpDataChanel;
	Variant _inboundHeaders;
	string _inboundContent;
	uint32_t _contentLength;
	SDP _inboundSDP;
	SDP _completeSDP; // this is needed to be sent as metadata (avigilon)
	Variant _playResponse; // this is needed to be sent as metadata (avigilon)
	Variant _playNotifyReq; // this is needed to be sent as metadata (avigilon)
	BaseRTSPAppProtocolHandler *_pProtocolHandler;
	IOBuffer _outputBuffer;

	Variant _responseHeaders;
	string _responseContent;

	Variant _requestHeaders;
	string _requestContent;
	uint32_t _requestSequence;
	map<uint32_t, Variant> _pendingRequestHeaders;
	map<uint32_t, string> _pendingRequestContent;

	OutboundConnectivity *_pOutboundConnectivity;
	InboundConnectivity *_pInboundConnectivity;
	InFileRTSPStream *_pVODStream;
	double _duration;
	double _startPos;

	Variant _authentication;

	uint32_t _keepAliveTimerId;
	BaseOutStream *_pOutStream;

	string _keepAliveURI;
	string _keepAliveMethod;

	string _sessionId;
	bool _enableTearDown;

	IOBuffer _internalBuffer;
	uint32_t _maxBufferSize;

	bool _isHTTPTunneled;
	string _xSessionCookie;
	string _httpTunnelUri;
	string _httpTunnelHostPort;
	uint32_t _passThroughProtocolId;
	bool _sendRenewStream;
public:
	RTSPProtocol();
	virtual ~RTSPProtocol();

	virtual void SetWitnessFile(string path);

	virtual IOBuffer * GetOutputBuffer();
	virtual bool Initialize(Variant &parameters);
	virtual void SetApplication(BaseClientApplication *pApplication);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	bool FeedRTSPRequest(string &data);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	virtual void EnqueueForDelete();

	void SignalOutStreamAttached();

	void IsHTTPTunneled(bool value);
	bool IsHTTPTunneled();
	bool OpenHTTPTunnel();
	string GetSessionId();
	string GenerateSessionId();
	bool SetSessionId(string sessionId);

	bool SetAuthentication(string authenticateHeader, string userName,
			string password, bool isRtsp);
	void EnableSendRenewStream();
	bool EnableKeepAlive(uint32_t period, string keepAliveURI);
	void EnableTearDown();
	void SetKeepAliveMethod(string keepAliveMethod);
	bool SendKeepAlive();
	bool HasConnectivity();

	SDP &GetInboundSDP();

	//all are avigilon requests
	SDP &GetCompleteSDP();
	Variant GetPlayResponse();
	void SetPlayResponse(Variant playResp);
	Variant& GetPlayNotifyReq();
	void SetPlayNotifyReq(Variant playNotifyReq);
	//avigilon features

	BaseVM *GetCallbackVM();

	//void ClearRequestMessage();
	void PushRequestFirstLine(string method, string url, string version);
	void PushRequestHeader(string name, string value);
	void PushRequestContent(string outboundContent, bool append);
	bool SendRequestMessage();
	uint32_t LastRequestSequence();
	bool GetRequest(uint32_t seqId, Variant &result, string &content);

	void ClearResponseMessage();
	void PushResponseFirstLine(string version, uint32_t code, string reason);
	void PushResponseHeader(string name, string value);
	void PushResponseContent(string outboundContent, bool append);
	bool SendResponseMessage();

	OutboundConnectivity * GetOutboundConnectivity(BaseInNetStream *pInNetStream,
			bool forceTcp);
	void CloseOutboundConnectivity();

	InboundConnectivity *GetInboundConnectivity(string sdpStreamName,
			uint32_t bandwidthHint, uint8_t rtcpDetectionInterval,
			int16_t a, int16_t b);
	InboundConnectivity *GetInboundConnectivity();
	//	InboundConnectivity *GetInboundConnectivity1(Variant &videoTrack,
	//			Variant &audioTrack, string sdpStreamName, uint32_t bandwidthHint);

	void CloseInboundConnectivity();

	bool SendRaw(uint8_t *pBuffer, uint32_t length, bool allowDrop);
	bool SendRawUplink(uint8_t *pBuffer, uint32_t length, bool allowDrop);
	bool SendRaw(MSGHDR *pMessage, uint16_t length, RTPClient *pClient,
			bool isAudio, bool isData, bool allowDrop);
	
	bool SendPauseReq();
	bool SendPlayResume();

	void SetOutStream(BaseOutStream *pOutStream);

	BaseInStream *GetVODStream(string streamName);
	double GetVODStreamStartTimestamp();
	double GetVODStreamEndTimestamp();
	double GetVODStreamDuration();
	double GetVODPosition();
	void PauseVOD();
	void PlayVOD(double startPos, double endPos, double clientSideBuffer, bool realTime);
#ifdef HAS_RTSP_LAZYPULL
	bool IsWaiting();
	void IsWaiting(bool state);
	string PendingStreamName();
	bool IsWaitingForStream(string streamName);
	uint32_t GetRequestedRTPBlockSize();
	void SetRequestedRTPBlockSize(uint32_t blockSize);
#endif /* HAS_RTSP_LAZYPULL */
	virtual void ReadyForSend();
	static bool SignalProtocolCreated(BaseProtocol *pProtocol,
			Variant &parameters);
private:
	bool SignalPassThroughProtocolCreated(PassThroughProtocol *pProtocol,
			Variant &parameters);
	bool SendMessage(string &firstLine, Variant &headers, string &content);
	bool ParseHeaders(IOBuffer &buffer);
	bool ParseInterleavedHeaders(IOBuffer &buffer);
	bool ParseNormalHeaders(IOBuffer &buffer);
	bool ParseFirstLine(string &line);
	bool HandleRTSPMessage(IOBuffer &buffer);
};

#endif	/* _RTSPPROTOCOL_H */
#endif /* HAS_PROTOCOL_RTP */
