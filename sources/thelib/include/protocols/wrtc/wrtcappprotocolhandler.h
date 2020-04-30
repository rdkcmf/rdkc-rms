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
* HISTORY:
 * , 6/3/2015 
* 
**/
#ifdef HAS_PROTOCOL_WEBRTC
#ifndef _WRTCAPPPROTOCOLHANDLER_H
#define	_WRTCAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

// RT Msg includes for RMS to communicate to LED Manager
#ifdef RTMSG
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RDKC
#include "sysUtils.h"
#ifdef USE_MFRLIB
#include "mfrApi.h"
#else
#include "sc_model.h"
#endif
#endif //RDKC

#ifdef __cplusplus
}
#endif

struct UserAgent {
	string deviceMAC;
	string versionNo;
	string fwVersion;
	string deviceModel;
	string deviceMake;
};

class WrtcAppProtocolHandler
: public BaseAppProtocolHandler {
public:
	WrtcAppProtocolHandler(Variant &configuration);
	virtual ~WrtcAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
	virtual void SetApplication(BaseClientApplication *pApplication);

	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters);

	bool StartWebRTC(Variant & settings, bool resolveRRS = true);
	bool StopWebRTC(uint32_t tgtConfigId);

	bool SendWebRTCCommand(Variant & settings);

	bool GetUserAgent(struct UserAgent &userAgent);

#ifdef RTMSG
	rtConnection GetRtSendMessageTag();
#endif

private:
	static map<uint32_t, BaseProtocol *> _instances; // list of WrtcSigProtocols

	bool _hasUserAgent;
	UserAgent _userAgent;
	bool SetUserAgent();

#ifdef RTMSG
	rtConnection connectionSend;
#endif
};

#endif	/* _WRTCAPPPROTOCOLHANDLER_H */
#endif // HAS_PROTOCOL_WEBRTC

