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



#ifdef HAS_PROTOCOL_VAR
#ifndef _BASEVARIANTAPPPROTOCOLHANDLER_H
#define	_BASEVARIANTAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

class BaseVariantProtocol;

typedef enum {
	VariantSerializer_BIN,
	VariantSerializer_XML,
	VariantSerializer_JSON
} VariantSerializer;

class DLLEXP BaseVariantAppProtocolHandler
: public BaseAppProtocolHandler {
private:
	Variant _urlCache;
	vector<uint64_t> _outboundHttpBinVariant;
	vector<uint64_t> _outboundHttpXmlVariant;
	vector<uint64_t> _outboundHttpJsonVariant;
	vector<uint64_t> _outboundHttpsBinVariant;
	vector<uint64_t> _outboundHttpsXmlVariant;
	vector<uint64_t> _outboundHttpsJsonVariant;
	vector<uint64_t> _outboundBinVariant;
	vector<uint64_t> _outboundXmlVariant;
	vector<uint64_t> _outboundJsonVariant;
#ifdef HAS_THREAD
//        map<string, uint32_t> _sendMutexes;
//        uint32_t _sendTaskId;
//        uint32_t _sendMutexId;
//        string CreateSendMutexKey(string url, bool hasCert, bool boolHasServerCert);
#endif  /* HAS_THREAD */
public:
	BaseVariantAppProtocolHandler(Variant &configuration);
	virtual ~BaseVariantAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	//opens an OUTBOUNDXMLVARIANT or an OUTBOUNDBINVARIANT
	//and sends the variant
	bool Send(string ip, uint16_t port, Variant &variant,
			VariantSerializer serializer = VariantSerializer_XML);

	//opens an OUTBOUNDHTTPXMLVARIANT or an OUTBOUNDHTTPBINVARIANT
	//and sends the variant (with optional client certificate setting)
	bool Send(string url, Variant &variant,
			VariantSerializer serializer = VariantSerializer_XML,
			string serverCertificate = "", string clientCertificate = "",
			string clientCertificateKey = "");
#ifdef HAS_THREAD
	//opens an OUTBOUNDHTTPXMLVARIANT or an OUTBOUNDHTTPBINVARIANT
	//and sends the variant (with optional client certificate setting)
	bool SendAsync(string url, Variant &variant,
			VariantSerializer serializer = VariantSerializer_XML,
			string serverCertificate = "", string clientCertificate = "",
			string clientCertificateKey = "");
        virtual bool DoSendAsyncTask(Variant &params);
        virtual void CompleteSendAsyncTask(Variant &params);
#endif  /* HAS_THREAD */
	//used internally
	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters);
	virtual void ConnectionFailed(Variant &parameters);

	//this is called whenever a message is received
	virtual bool ProcessMessage(BaseVariantProtocol *pProtocol,
			Variant &lastSent, Variant &lastReceived);

protected:
	virtual Variant &GetScaffold(string &uriString);
	virtual vector<uint64_t> &GetTransport(VariantSerializer serializerType, bool isHttp, bool isSsl);
};


#endif	/* _BASEVARIANTAPPPROTOCOLHANDLER_H */
#endif	/* HAS_PROTOCOL_VAR */
