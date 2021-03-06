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
#include "protocols/variant/basevariantappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolfactorymanager.h"
#include "application/baseclientapplication.h"
#include "netio/netio.h"
#include "application/clientapplicationmanager.h"
#include "protocols/variant/basevariantprotocol.h"
#include "threading/threading.h"

#ifdef HAS_THREAD
void SendTask(Variant &params, void *requestingObject) {
    BaseVariantAppProtocolHandler *pHandler = (BaseVariantAppProtocolHandler *) requestingObject;
    TASK_STATUS(params) = pHandler->DoSendAsyncTask(params);
   
}

void SendCompletionTask(Variant &params, void *requestingObject) {
    BaseVariantAppProtocolHandler *pHandler = (BaseVariantAppProtocolHandler *) requestingObject;
    pHandler->CompleteSendAsyncTask(params);
}

bool BaseVariantAppProtocolHandler::DoSendAsyncTask(Variant &params) {
    string url = (string)(TASK_PARAM(params)["url"]);
 	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		Variant temp;
		temp["payload"] = TASK_PARAM(params)["payload"];
		temp["serverCert"] = TASK_PARAM(params)["serverCert"];
		temp[CONF_SSL_KEY] = TASK_PARAM(params)[CONF_SSL_KEY];
		temp[CONF_SSL_CERT] = TASK_PARAM(params)[CONF_SSL_CERT];
        TASK_RESULT(params) = temp;
        return false;
	} else {
        TASK_RESULT(params) = parameters;
        TASK_RESULT(params)["payload"] = TASK_PARAM(params)["payload"];
        TASK_RESULT(params)["serverCert"] = TASK_PARAM(params)["serverCert"];
        TASK_RESULT(params)[CONF_SSL_KEY] = TASK_PARAM(params)[CONF_SSL_KEY];
        TASK_RESULT(params)[CONF_SSL_CERT] = TASK_PARAM(params)[CONF_SSL_CERT];
        TASK_RESULT(params)["serializer"] = TASK_PARAM(params)["serializer"];
        return true;
    }
}
void BaseVariantAppProtocolHandler::CompleteSendAsyncTask(Variant &params) {
    if ((bool)TASK_STATUS(params)) {
        VariantSerializer serializer = VariantSerializer_XML;
        uint8_t serializerInt = (uint8_t)(TASK_RESULT(params)["serializer"]);
        if (serializerInt == 0)
            serializer = VariantSerializer_BIN;
        else if (serializerInt == 1)
            serializer = VariantSerializer_JSON;
        else 
            serializer = VariantSerializer_XML;
        if (!TCPConnector<BaseVariantAppProtocolHandler>::Connect(TASK_RESULT(params)["ip"],
                TASK_RESULT(params)["port"],
                GetTransport(serializer, true, TASK_RESULT(params)["isSsl"]),
                TASK_RESULT(params))) {
            FATAL("Unable to open connection");
        }
    } else {
        ConnectionFailed(TASK_RESULT(params));
		FATAL("Unable to get parameters scaffold");
    }    
}
#endif /* HAS_THREAD */

BaseVariantAppProtocolHandler::BaseVariantAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {
	_urlCache["dummy"] = Variant();
#ifdef HAS_PROTOCOL_HTTP
	_outboundHttpBinVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTP_BIN_VARIANT);
	_outboundHttpXmlVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTP_XML_VARIANT);
	_outboundHttpJsonVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTP_JSON_VARIANT);
	_outboundHttpsBinVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTPS_BIN_VARIANT);
	_outboundHttpsXmlVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTPS_XML_VARIANT);
	_outboundHttpsJsonVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_HTTPS_JSON_VARIANT);
	_outboundBinVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_BIN_VARIANT);
	_outboundXmlVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_XML_VARIANT);
	_outboundJsonVariant = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_OUTBOUND_JSON_VARIANT);
	if (_outboundHttpBinVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTP_BIN_VARIANT);
	}
	if (_outboundHttpXmlVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTP_XML_VARIANT);
	}
	if (_outboundHttpJsonVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTP_JSON_VARIANT);
	}
	if (_outboundHttpsBinVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTPS_BIN_VARIANT);
	}
	if (_outboundHttpsXmlVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTPS_XML_VARIANT);
	}
	if (_outboundHttpsJsonVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_HTTPS_JSON_VARIANT);
	}

	if (_outboundBinVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_BIN_VARIANT);
	}
	if (_outboundXmlVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_XML_VARIANT);
	}
	if (_outboundJsonVariant.size() == 0) {
		ASSERT("Unable to resolve protocol stack %s",
				CONF_PROTOCOL_OUTBOUND_JSON_VARIANT);
	}
#else
	FATAL("HTTP protocol not supported");
#endif /* HAS_PROTOCOL_HTTP */
#ifdef HAS_THREAD
//    _sendMutexId = ThreadManager::RequestLockId();
#endif
}

BaseVariantAppProtocolHandler::~BaseVariantAppProtocolHandler() {
//    ThreadManager::UnregisterLockId(_sendMutexId);
}

void BaseVariantAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {
}

void BaseVariantAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}

bool BaseVariantAppProtocolHandler::Send(string ip, uint16_t port, Variant &variant,
		VariantSerializer serializer) {
	//1. Build the parameters
	Variant parameters;
	parameters["ip"] = ip;
	parameters["port"] = (uint16_t) port;
	parameters["applicationName"] = GetApplication()->GetName();
	parameters["payload"] = variant;

	//2. Start the HTTP request
	if (!TCPConnector<BaseVariantAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(serializer, false, false),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool BaseVariantAppProtocolHandler::Send(string url, Variant &variant,
		VariantSerializer serializer, string serverCertificate,
		string clientCertificate, string clientCertificateKey) {
	//1. Build the parameters
	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		Variant temp;
		temp["payload"] = variant;
		temp["serverCert"] = serverCertificate;
		temp[CONF_SSL_KEY] = clientCertificateKey;
		temp[CONF_SSL_CERT] = clientCertificate;
		ConnectionFailed(temp);
		FATAL("Unable to get parameters scaffold");
		return false;
	}
	parameters["payload"] = variant;
	parameters["serverCert"] = serverCertificate;
	parameters[CONF_SSL_KEY] = clientCertificateKey;
	parameters[CONF_SSL_CERT] = clientCertificate;

	//2. Start the HTTP request
	if (!TCPConnector<BaseVariantAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(serializer, true, parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

#ifdef HAS_THREAD
bool BaseVariantAppProtocolHandler::SendAsync(string url, Variant &variant,
    VariantSerializer serializer, string serverCertificate,
    string clientCertificate, string clientCertificateKey) {
    Variant parameters;
    uint8_t tmp = 0;

    if (serializer == VariantSerializer_BIN) {
        tmp = 0;
    } else if (serializer == VariantSerializer_JSON) {
        tmp = 1;
    } else {
        tmp = 2;
    }
    TASK_PARAM(parameters)["serializer"] = tmp;
    TASK_PARAM(parameters)["url"] = url;
	TASK_PARAM(parameters)["payload"] = Variant(variant);
	TASK_PARAM(parameters)["serverCert"] = serverCertificate;
	TASK_PARAM(parameters)[CONF_SSL_KEY] = clientCertificateKey;
	TASK_PARAM(parameters)[CONF_SSL_CERT] = clientCertificate;
    return ThreadManager::QueueAsyncTask(&SendTask, &SendCompletionTask, parameters, (void *)this);
}
/*
string BaseVariantAppProtocolHandler::CreateSendMutexKey(
        string url, bool hasCert, bool hasServerCert) {
    return url + (hasCert ? "|C" : "") + (hasServerCert ? "|S" : "");
}
*/
#endif
bool BaseVariantAppProtocolHandler::SignalProtocolCreated(BaseProtocol *pProtocol, Variant &parameters) {
	//1. Get the application
	BaseClientApplication *pApplication = ClientApplicationManager::FindAppByName(
			parameters["applicationName"]);
	if (pApplication == NULL) {
		FATAL("Unable to find application %s",
				STR(parameters["applicationName"]));
		return false;
	}

	//2. get the protocol handler
	BaseAppProtocolHandler *pHandler = NULL;
	if (pApplication->HasProtocolHandler(PT_JSON_VAR)) {
		pHandler = pApplication->GetProtocolHandler(PT_JSON_VAR);
	} else if (pApplication->HasProtocolHandler(PT_XML_VAR)) {
		pHandler = pApplication->GetProtocolHandler(PT_XML_VAR);
	} else if (pApplication->HasProtocolHandler(PT_BIN_VAR)) {
		pHandler = pApplication->GetProtocolHandler(PT_BIN_VAR);
	}
	if (pHandler == NULL) {
		WARN("Unable to get protocol handler for variant protocol");
	}

	//3. Is the connection up
	if (pProtocol == NULL) {
		if (pHandler != NULL) {
			((BaseVariantAppProtocolHandler *) pHandler)->ConnectionFailed(parameters);
		} else {
			WARN("Connection failed:\n%s", STR(parameters.ToString()));
		}
		return false;
	}

	//1. Validate the protocol
	if ((pProtocol->GetType() != PT_BIN_VAR)
			&& (pProtocol->GetType() != PT_XML_VAR)
			&& (pProtocol->GetType() != PT_JSON_VAR)) {
		FATAL("Invalid protocol type. Wanted: %s, %s or %s; Got: %s",
				STR(tagToString(PT_BIN_VAR)),
				STR(tagToString(PT_XML_VAR)),
				STR(tagToString(PT_JSON_VAR)),
				STR(tagToString(pProtocol->GetType())));
		return false;
	}

	//3. Register the protocol to it
	pProtocol->SetApplication(pApplication);

	if (pProtocol->GetFarProtocol() == NULL) {
		FATAL("Invalid far protocol");
		return false;
	}

	//4. Do the actual request
	if (pProtocol->GetFarProtocol()->GetType() == PT_TCP) {
		return ((BaseVariantProtocol *) pProtocol)->Send(parameters["payload"]);
	} else {
		return ((BaseVariantProtocol *) pProtocol)->Send(parameters);
	}
}

void BaseVariantAppProtocolHandler::ConnectionFailed(Variant &parameters) {
	WARN("Connection failed:\n%s", STR(parameters.ToString()));
}

bool BaseVariantAppProtocolHandler::ProcessMessage(BaseVariantProtocol *pProtocol,
		Variant &lastSent, Variant &lastReceived) {
	FINEST("lastSent:\n%s", STR(lastSent.ToString()));
	FINEST("lastReceived:\n%s", STR(lastReceived.ToString()));
	return true;
}

Variant &BaseVariantAppProtocolHandler::GetScaffold(string &uriString) {
	//1. Search in the cache first
	if (_urlCache.HasKey(uriString)) {
		return _urlCache[uriString];
	}

	//2. Build it
	Variant result;

	//3. Split the URL into components
	URI uri;
	if (!URI::FromString(uriString, true, uri)) {
		FATAL("Invalid url: %s", STR(uriString));
		return _urlCache["dummy"];
	}

	//6. build the end result
	result["username"] = uri.userName();
	result["password"] = uri.password();
	result["host"] = uri.host();
	result["ip"] = uri.ip();
	result["port"] = uri.port();
	result["document"] = uri.fullDocumentPathWithParameters();
	result["isSsl"] = (bool)(uri.scheme() == "https");
	result["applicationName"] = GetApplication()->GetName();

	//7. Save it in the cache
	_urlCache[uriString] = result;

	//8. Done
	return _urlCache[uriString];
}

vector<uint64_t> &BaseVariantAppProtocolHandler::GetTransport(
		VariantSerializer serializerType, bool isHttp, bool isSsl) {
	switch (serializerType) {
		case VariantSerializer_BIN:
		{
			if (isHttp) {
				if (isSsl) {
					return _outboundHttpsBinVariant;
				} else {
					return _outboundHttpBinVariant;
				}
			} else {
				return _outboundBinVariant;
			}
		}
		case VariantSerializer_XML:
		{
			if (isHttp) {
				if (isSsl) {
					return _outboundHttpsXmlVariant;
				} else {
					return _outboundHttpXmlVariant;
				}
			} else {
				return _outboundXmlVariant;
			}
		}
		case VariantSerializer_JSON:
		default:
		{
			if (isHttp) {
				if (isSsl) {
					return _outboundHttpsJsonVariant;
				} else {
					return _outboundHttpJsonVariant;
				}
			} else {
				return _outboundJsonVariant;
			}
		}
	}
}
#endif	/* HAS_PROTOCOL_VAR */

