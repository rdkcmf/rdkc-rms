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
#ifndef _BASERTSPAPPPROTOCOLHANDLER_H
#define	_BASERTSPAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"
#include "streaming/streamcapabilities.h"

#include <map>

#ifdef HAS_RTSP_LAZYPULL

#define PENDING_STREAM_REGISTERED_FLAG 0x1
#define PENDING_STREAM_VCODEC_UPDATED_FLAG 0x2
#define PENDING_STREAM_ACODEC_UPDATED_FLAG 0x4
#define PENDING_STREAM_FAIL_FLAG 0x1

#define RTSP_LP_SESSION_PARAMS "rtspSessionParams"
#define RTSP_LP_STREAMNAME "rtspProtocolId"
#define RTSP_LP_INSTREAM_FLAGS "statusFlags"

#endif /* HAS_RTSP_LAZYPULL */

class RTSPProtocol;
class BaseInNetStream;
class OutboundConnectivity;
class BaseStream;

class DLLEXP BaseRTSPAppProtocolHandler
: public BaseAppProtocolHandler {
protected:
	Variant _realms;
	string _usersFile;
	bool _authenticatePlay;
	double _lastUsersFileUpdate;
	map<string, uint32_t> _httpSessions;
#ifdef HAS_RTSP_LAZYPULL
	map<uint32_t, Variant> _pendingInboundStreams;
#endif /* HAS_RTSP_LAZYPULL */
public:
	BaseRTSPAppProtocolHandler(Variant &configuration);
	virtual ~BaseRTSPAppProtocolHandler();

	void CallVM(const string &functionName, RTSPProtocol *pFrom,
			Variant &headers, string &content);

	virtual bool ParseAuthenticationNode(Variant &node, Variant &result);

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	virtual bool PullExternalStream(URI uri, Variant streamConfig);
	virtual bool PushLocalStream(Variant streamConfig);
	static bool SignalProtocolCreated(BaseProtocol *pProtocol,
			Variant &parameters);
#ifdef HAS_RTSP_LAZYPULL
	void UpdatePendingStream(uint32_t protocolID, BaseStream *pStream);
#endif /* HAS_RTSP_LAZYPULL */
	virtual bool HandleHTTPRequest(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent);
	virtual bool HandleRTSPRequest(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent);
	virtual bool HandleRTSPResponse(RTSPProtocol *pFrom, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleHTTPResponse(RTSPProtocol *pFrom, Variant &responseHeaders,
			string &responseContent);
protected:
	//handle requests routines
	virtual bool HandleRTSPRequestOptions(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestDescribe(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestSetup(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestSetupOutbound(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestSetupInbound(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestTearDown(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestAnnounce(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPause(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPlayOrRecord(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPlay(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPlayNotify(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestRecord(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestSetParameter(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestGetParameter(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);

	//handle response routines
	virtual bool HandleHTTPResponse(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleHTTPResponse200(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleHTTPResponse401(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleHTTPResponse200Get(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleHTTPResponse401Get(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse401(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse404(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Options(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Describe(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Setup(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Play(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Announce(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Record(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse404Play(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse404Describe(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);
	virtual bool HandleRTSPResponse200Pause(RTSPProtocol *pFrom, Variant &requestHeaders,
			string &requestContent, Variant &responseHeaders,
			string &responseContent);

	//operations
	virtual bool TriggerPlayOrAnnounce(RTSPProtocol *pFrom);
protected:
	virtual bool NeedAuthentication(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual string GetAuthenticationRealm(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
private:
	virtual bool HandleRTSPRequestSetupOutboundTs(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestSetupOutboundNormal(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPlayTs(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	virtual bool HandleRTSPRequestPlayNormal(RTSPProtocol *pFrom,
			Variant &requestHeaders, string &requestContent);
	void ComputeRTPInfoHeader(RTSPProtocol *pFrom,
			OutboundConnectivity *pOutboundConnectivity, double start);
	void ParseRange(string raw, double &start, double &end);
	double ParseNPT(string raw);
	bool AnalyzeUri(RTSPProtocol *pFrom, string rawUri);
	bool IsVod(RTSPProtocol *pFrom);
	bool IsTs(RTSPProtocol *pFrom);
	bool IsRawTs(RTSPProtocol *pFrom);
	string GetStreamName(RTSPProtocol *pFrom);
	OutboundConnectivity *GetOutboundConnectivity(RTSPProtocol *pFrom,
			bool forceTcp);
	BaseInStream *GetInboundStream(string streamName, RTSPProtocol *pFrom);
	StreamCapabilities *GetInboundStreamCapabilities(string streamName, RTSPProtocol *pFrom);
	string GetAudioTrack(RTSPProtocol *pFrom,
			StreamCapabilities *pCapabilities);
	string GetVideoTrack(RTSPProtocol *pFrom,
			StreamCapabilities *pCapabilities);
	bool SendSetupTrackMessages(RTSPProtocol *pFrom);
	bool ParseUsersFile();
	bool SendAuthenticationChallenge(RTSPProtocol *pFrom, Variant &realm);
	string ComputeSDP(RTSPProtocol *pFrom, string localStreamName,
			string targetStreamName, bool isAnnounce);
	void EnableDisableOutput(RTSPProtocol *pFrom, bool value);
	void PushScaleParam(RTSPProtocol* pFrom);
	void ParseCustHeaderKeyValuePair(std::map<std::string, std::string>& out, const std::string& in);
	void ParseCustHeaderKeyValueString(std::map<std::string, std::string>& out, const std::string& in);
};


#endif	/* _BASERTSPAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_RTP */
