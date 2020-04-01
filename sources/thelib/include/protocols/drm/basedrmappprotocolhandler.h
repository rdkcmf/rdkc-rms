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


#ifdef HAS_PROTOCOL_DRM
#ifndef _BASEDRMAPPPROTOCOLHANDLER_H
#define	_BASEDRMAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"
#include "protocols/drm/drmkeys.h"

class BaseDRMProtocol;

typedef enum {
	DRMSerializer_None = 0,
	DRMSerializer_Verimatrix = 1
} DRMSerializer;

class DLLEXP BaseDRMAppProtocolHandler
: public BaseAppProtocolHandler {
private:
	Variant _urlCache;
	vector<uint64_t> _outboundHttpDrm;
	vector<uint64_t> _outboundHttpsDrm;
	DRMKeys *_pDRMQueue;
	uint32_t _lastSentId;
public:
	BaseDRMAppProtocolHandler(Variant &configuration);
	virtual ~BaseDRMAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	//opens an OUTBOUNDXMLVARIANT or an OUTBOUNDBINVARIANT
	//and sends the variant
	bool Send(string ip, uint16_t port, Variant &variant,
			DRMSerializer serializer = DRMSerializer_Verimatrix);

	//opens an OUTBOUNDHTTPXMLVARIANT or an OUTBOUNDHTTPBINVARIANT
	//and sends the variant (with optional client certificate setting)
	bool Send(string url, Variant &variant,
			DRMSerializer serializer = DRMSerializer_Verimatrix,
			string serverCertificate = "", string clientCertificate = "",
			string clientCertificateKey = "");

	//used internally
	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters);
	virtual void ConnectionFailed(Variant &parameters);

	//this is called whenever a message is received
	virtual bool ProcessMessage(BaseDRMProtocol *pProtocol,
			Variant &lastSent, Variant &lastReceived);

	virtual DRMKeys *GetDRMQueue();

private:
	Variant &GetScaffold(string &uriString);
	vector<uint64_t> &GetTransport(bool isSsl);
};

#endif	/* _BASEDRMAPPPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_DRM */
