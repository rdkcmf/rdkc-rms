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



#include "common.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolmanager.h"
#include "application/baseappprotocolhandler.h"
#include "protocols/baseprotocol.h"
#include "streaming/basestream.h"
#include "application/clientapplicationmanager.h"
#include "streaming/streamstypes.h"
#include "eventlogger/eventlogger.h"
#include "eventlogger/baseeventsink.h"
#include "mediaformats/readers/streammetadataresolver.h"

uint32_t BaseClientApplication::_idGenerator = 0;

class StreamAliasExpireTimer
: public BaseTimerProtocol {
private:
	map<string, StreamAlias> *_pAliases;
public:

	StreamAliasExpireTimer(map<string, StreamAlias> *pAliases) {
		_pAliases = pAliases;
	}

	virtual ~StreamAliasExpireTimer() {

	}

	void ResetAliases() {
		_pAliases = NULL;
	}

	virtual bool TimePeriodElapsed() {
		if (_pAliases == NULL)
			return true;
		map<string, StreamAlias>::iterator i = _pAliases->begin();
		while (i != _pAliases->end()) {
			if (MAP_VAL(i).Expired())
				_pAliases->erase(i++);
			else
				++i;
		}

		return true;
	}
};

BaseClientApplication::BaseClientApplication(Variant &configuration)
:  _streamsManager(this) {
	_pMetadataManager = new MetadataManager(this);	// init syntax failed on me
	_isOrigin = true;
	_pEventLogger = NULL;
	_id = ++_idGenerator;
	_configuration = configuration;
	_name = (string) configuration[CONF_APPLICATION_NAME];
	if (configuration.HasKeyChain(V_MAP, false, 1, CONF_APPLICATION_ALIASES)) {

		FOR_MAP((configuration[CONF_APPLICATION_ALIASES]), string, Variant, i) {
			ADD_VECTOR_END(_aliases, MAP_VAL(i));
		}
	}
	_isDefault = false;
	if (configuration.HasKeyChain(V_BOOL, false, 1, CONF_APPLICATION_DEFAULT))
		_isDefault = (bool)configuration[CONF_APPLICATION_DEFAULT];
	_hasStreamAliases = false;
	if (configuration.HasKeyChain(V_BOOL, false, 1, CONF_APPLICATION_HAS_STREAM_ALIASES))
		_hasStreamAliases = (bool)configuration[CONF_APPLICATION_HAS_STREAM_ALIASES];

	_hasIngestPoints = false;
	if (configuration.HasKeyChain(V_BOOL, false, 1, CONF_APPLICATION_HAS_INGEST_POINTS))
		_hasIngestPoints = (bool)configuration[CONF_APPLICATION_HAS_INGEST_POINTS];

	_pStreamMetadataResolver = new StreamMetadataResolver();

    _streamAliasTimer = 0;
    if (_hasStreamAliases) {
            StreamAliasExpireTimer *pTemp = new StreamAliasExpireTimer(&_streamAliases);
            _streamAliasTimer = pTemp->GetId();
            pTemp->EnqueueForTimeEvent(1);
    }
    
    _vodRedirectRtmp = false;
    _backupRmsAddress.clear();
}

BaseClientApplication::~BaseClientApplication() {
    StreamAliasExpireTimer *pTemp = (StreamAliasExpireTimer *) ProtocolManager::GetProtocol(_streamAliasTimer);
    if (pTemp != NULL) {
            pTemp->ResetAliases();
            pTemp->EnqueueForDelete();
    }

	if (_pEventLogger != NULL) {
		delete _pEventLogger;
		_pEventLogger = NULL;
	}

	if (_pStreamMetadataResolver != NULL) {
		delete _pStreamMetadataResolver;
		_pStreamMetadataResolver = NULL;
	}
}

uint32_t BaseClientApplication::GetId() {
    return _id;
}

string BaseClientApplication::GetName() {
    return _name;
}

Variant &BaseClientApplication::GetConfiguration() {
    return _configuration;
}

vector<string> BaseClientApplication::GetAliases() {
    return _aliases;
}

bool BaseClientApplication::IsDefault() {
    return _isDefault;
}

bool BaseClientApplication::IsEdge() {
	return !_isOrigin;
}

bool BaseClientApplication::IsOrigin() {
	return _isOrigin;
}

StreamsManager *BaseClientApplication::GetStreamsManager() {
	return &_streamsManager;
}

StreamMetadataResolver *BaseClientApplication::GetStreamMetadataResolver() {
	return _pStreamMetadataResolver;
}

bool BaseClientApplication::Initialize() {
	if (_configuration.HasKeyChain(V_STRING, false, 1, CONF_APPLICATION_MEDIAFOLDER)) {
		WARN(CONF_APPLICATION_MEDIAFOLDER" is obsolete. Please use "CONF_APPLICATION_MEDIASTORAGE);
		if (!_configuration.HasKeyChain(V_MAP, false, 1, CONF_APPLICATION_MEDIASTORAGE)) {
			_configuration[CONF_APPLICATION_MEDIASTORAGE] = Variant();
			_configuration[CONF_APPLICATION_MEDIASTORAGE].IsArray(false);
		}
		_configuration.GetValue(CONF_APPLICATION_MEDIASTORAGE, false)["__obsolete__mediaFolder"][CONF_APPLICATION_MEDIAFOLDER] =
				_configuration.GetValue(CONF_APPLICATION_MEDIAFOLDER, false);
	}
	if (_configuration.HasKeyChain(V_MAP, false, 1, CONF_APPLICATION_MEDIASTORAGE)) {
		if (!_pStreamMetadataResolver->Initialize(_configuration.GetValue(CONF_APPLICATION_MEDIASTORAGE, false))) {
			FATAL("Unable to initialize stream metadata resolver");
			return false;
		}
	}
	_isOrigin = _configuration.HasKeyChain(V_BOOL, false, 1, "isOrigin")
			&& (bool)_configuration.GetValue("isOrigin", false);

	_pEventLogger = new EventLogger();
	if (_configuration.HasKeyChain(V_MAP, false, 1, "eventLogger")) {
		if (!_pEventLogger->Initialize(
				_configuration.GetValue("eventLogger", false), this)) {
			FATAL("Unable to initialize event logger");
			return false;
		}
	}
        
    VodRedirectRtmpEnabling();
    return true;
}

bool BaseClientApplication::ActivateAcceptors(vector<IOHandler *> &acceptors) {
    for (uint32_t i = 0; i < acceptors.size(); i++) {
        if (!ActivateAcceptor(acceptors[i])) {
            FATAL("Unable to activate acceptor");
            return false;
        }
    }
    return true;
}

bool BaseClientApplication::ActivateAcceptor(IOHandler *pIOHandler) {
    switch (pIOHandler->GetType()) {
        case IOHT_ACCEPTOR:
        {
            TCPAcceptor *pAcceptor = (TCPAcceptor *) pIOHandler;
            pAcceptor->SetApplication(this);
            return pAcceptor->StartAccept();
        }
#ifdef HAS_UDS
		case IOHT_UDS_ACCEPTOR:
		{
			UDSAcceptor *pAcceptor = (UDSAcceptor *) pIOHandler;
			pAcceptor->SetApplication(this);
            return pAcceptor->StartAccept();
		}
#endif /* HAS_UDS */
        case IOHT_UDP_CARRIER:
        {
            UDPCarrier *pUDPCarrier = (UDPCarrier *) pIOHandler;
            pUDPCarrier->GetProtocol()->GetNearEndpoint()->SetApplication(this);
            return pUDPCarrier->StartAccept();
        }
        default:
        {
            FATAL("Invalid acceptor type");
            return false;
        }
    }
}

string BaseClientApplication::GetServicesInfo() {
    map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();
    string result = "";

    FOR_MAP(handlers, uint32_t, IOHandler *, i) {
        result += GetServiceInfo(MAP_VAL(i));
    }
    return result;
}

bool BaseClientApplication::AcceptTCPConnection(TCPAcceptor *pTCPAcceptor) {
    return pTCPAcceptor->Accept();
}

void BaseClientApplication::RegisterAppProtocolHandler(uint64_t protocolType,
        BaseAppProtocolHandler *pAppProtocolHandler) {
    if (MAP_HAS1(_protocolsHandlers, protocolType))
        ASSERT("Invalid protocol handler type. Already registered");
    _protocolsHandlers[protocolType] = pAppProtocolHandler;
    pAppProtocolHandler->SetApplication(this);
}

void BaseClientApplication::UnRegisterAppProtocolHandler(uint64_t protocolType) {
	if (MAP_HAS1(_protocolsHandlers, protocolType))
		_protocolsHandlers[protocolType]->SetApplication(NULL);
	_protocolsHandlers.erase(protocolType);
}

bool BaseClientApplication::StreamNameAvailable(string streamName,
		BaseProtocol *pProtocol, bool bypassIngestPoints) {
	if (_hasStreamAliases) {
		if (MAP_HAS1(_streamAliases, streamName))
			return false;
	}

	if (_hasIngestPoints) {
		if (bypassIngestPoints) {
            if ((pProtocol != NULL) && !(pProtocol->GetApplication()->IsEdge())) {
 				if ((MAP_HAS1(_ingestPointsPrivateToPublic, streamName))
						|| (MAP_HAS1(_ingestPointsPublicToPrivate, streamName))) {
					return false;
				}
            }
		} else {
			if (!MAP_HAS1(_ingestPointsPrivateToPublic, streamName))
				return false;
		}
	}

	return _streamsManager.StreamNameAvailable(streamName);
}

BaseAppProtocolHandler *BaseClientApplication::GetProtocolHandler(
        BaseProtocol *pProtocol) {
    if (pProtocol == NULL)
        return NULL;
    return GetProtocolHandler(pProtocol->GetType());
}

BaseAppProtocolHandler *BaseClientApplication::GetProtocolHandler(uint64_t protocolType) {
    if (!MAP_HAS1(_protocolsHandlers, protocolType)) {
        WARN("Protocol handler not activated for protocol type %s in application %s",
                STR(tagToString(protocolType)), STR(_name));
        return NULL;
    }
    return _protocolsHandlers[protocolType];
}

bool BaseClientApplication::HasProtocolHandler(uint64_t protocolType) {
    return MAP_HAS1(_protocolsHandlers, protocolType);
}

BaseAppProtocolHandler *BaseClientApplication::GetProtocolHandler(string &scheme) {
    BaseAppProtocolHandler *pResult = NULL;
    if (false) {

    }
#ifdef HAS_PROTOCOL_RTMP
    else if (scheme.find("rtmp") == 0) {
        pResult = GetProtocolHandler(PT_INBOUND_RTMP);
        if (pResult == NULL)
            pResult = GetProtocolHandler(PT_OUTBOUND_RTMP);
    }
#endif /* HAS_PROTOCOL_RTMP */
#ifdef HAS_PROTOCOL_RTP
    else if (scheme == "rtsp") {
        pResult = GetProtocolHandler(PT_RTSP);
    } else if (scheme == "rtp") {
        pResult = GetProtocolHandler(PT_INBOUND_RTP);
    } else if (scheme == "file") {
        pResult = GetProtocolHandler(PT_SDP);
    }
#endif /* HAS_PROTOCOL_RTP */
#ifdef HAS_PROTOCOL_HTTP2
    else if (scheme == "http") {
        pResult = GetProtocolHandler(PT_HTTP_ADAPTOR);
    } else if (scheme == "msshttp") {
        pResult = GetProtocolHandler(PT_HTTP_MSS_LIVEINGEST);
    }
#endif
#ifdef HAS_PROTOCOL_NMEA
    else if (scheme == "nmea") {
        pResult = GetProtocolHandler(PT_INBOUND_NMEA);
        WARN("NMEA found Protocol Handler: %s", (pResult?"YES":"no"));
    }
#endif
#ifdef HAS_PROTOCOL_METADATA
    else if (scheme == "jsonmeta") {
        pResult = GetProtocolHandler(PT_JSONMETADATA);
        //INFO("$b2$:JsonMeta found Protocol Handler: %s", (pResult?"YES":"no"));
    }
#endif
#ifdef HAS_VMF
    else if (scheme == "vmfschema") {
        pResult = GetProtocolHandler(PT_INBOUND_VMFSCHEMA);
        //INFO("$b2$:VmfSchema found Protocol Handler: %s", (pResult?"YES":"no"));
    }
#endif
    else if (scheme == "outvmf") {
        pResult = GetProtocolHandler(PT_OUTBOUND_VMF);
    }
	else if (scheme == "wrtcsig")  {
		pResult = GetProtocolHandler(PT_WRTC_SIG);
	}
	else if (scheme == "wrtccnx")  {
		pResult = GetProtocolHandler(PT_WRTC_CNX);
	}
#ifdef HAS_PROTOCOL_API
	else if (scheme == "sercom")  {
		pResult = GetProtocolHandler(PT_API_INTEGRATION);
	}
#endif /* HAS_PROTOCOL_API */
	else {
        WARN("scheme %s not recognized", STR(scheme));
    }
    return pResult;
}

bool BaseClientApplication::OutboundConnectionFailed(Variant &customParameters) {
    WARN("You should override BaseRTMPAppProtocolHandler::OutboundConnectionFailed");
    return false;
}

void BaseClientApplication::RegisterProtocol(BaseProtocol *pProtocol) {
    if (!MAP_HAS1(_protocolsHandlers, pProtocol->GetType()))
        ASSERT("Protocol handler not activated for protocol type %s in application %s",
            STR(tagToString(pProtocol->GetType())),
            STR(_name));
    _protocolsHandlers[pProtocol->GetType()]->RegisterProtocol(pProtocol);
    if (pProtocol != NULL && pProtocol->GetType() != PT_TIMER)
        _pEventLogger->LogProtocolRegisteredToApplication(pProtocol);
}

void BaseClientApplication::UnRegisterProtocol(BaseProtocol *pProtocol) {
    if (!MAP_HAS1(_protocolsHandlers, pProtocol->GetType()))
        ASSERT("Protocol handler not activated for protocol type %s in application %s",
            STR(tagToString(pProtocol->GetType())), STR(_name));
    _streamsManager.UnRegisterStreams(pProtocol->GetId());
    _protocolsHandlers[pProtocol->GetType()]->UnRegisterProtocol(pProtocol);
    if (pProtocol != NULL && pProtocol->GetType() != PT_TIMER)
        _pEventLogger->LogProtocolUnregisteredFromApplication(pProtocol);
    FINEST("Protocol %s unregistered from application: %s", STR(*pProtocol), STR(_name));
}

void BaseClientApplication::SignalStreamRegistered(BaseStream *pStream) {
	if (pStream != NULL)
		INFO("Stream %s registered to %s application `%s`", STR(*pStream),
			IsEdge() ? "edge" : "origin", STR(_name));
}

void BaseClientApplication::SignalStreamUnRegistered(BaseStream *pStream) {
	if (pStream != NULL)
		INFO("Stream %s unregistered from %s application `%s`", STR(*pStream),
			IsEdge() ? "edge" : "origin", STR(_name));
}

bool BaseClientApplication::PullExternalStreams() {
	//1. Minimal verifications
	if (_configuration["externalStreams"] == V_NULL) {
		return true;
	}

	if (_configuration["externalStreams"] != V_MAP) {
		FATAL("Invalid rtspStreams node");
		return false;
	}

	//2. Loop over the stream definitions and validate duplicated stream names
	Variant streamConfigs;
	streamConfigs.IsArray(false);

	FOR_MAP(_configuration["externalStreams"], string, Variant, i) {
		Variant &temp = MAP_VAL(i);
		if ((!temp.HasKeyChain(V_STRING, false, 1, "localStreamName"))
				|| (temp.GetValue("localStreamName", false) == "")) {
			WARN("External stream configuration is doesn't have localStreamName property invalid:\n%s",
					STR(temp.ToString()));
			continue;
		}
		string localStreamName = (string) temp.GetValue("localStreamName", false);
		if (streamConfigs.HasKey(localStreamName)) {
			WARN("External stream configuration produces duplicated stream names\n%s",
					STR(temp.ToString()));
			continue;
		}
		streamConfigs[localStreamName] = temp;
	}


	//2. Loop over the stream definitions and spawn the streams

	FOR_MAP(streamConfigs, string, Variant, i) {
		Variant &streamConfig = MAP_VAL(i);
		if (!PullExternalStream(streamConfig)) {
			WARN("External stream configuration is invalid:\n%s",
					STR(streamConfig.ToString()));
		}
	}

	//3. Done
	return true;
}

bool BaseClientApplication::PullExternalStream(Variant &streamConfig) {
	//1. Minimal verification
	if (streamConfig["uri"] != V_STRING) {
		FATAL("Invalid uri");
		return false;
	}

	bool resolveHost = (!streamConfig.HasKeyChain(V_STRING, true, 1, "httpProxy"))
			|| (streamConfig["httpProxy"] == "")
			|| (streamConfig["httpProxy"] == "self");

	//2. Split the URI
	URI uri;
	if (!URI::FromString(streamConfig["uri"], resolveHost, uri)) {
		FATAL("Invalid URI: %s", STR(streamConfig["uri"].ToString()));
		return false;
	}
	streamConfig["uri"] = uri;

	//3. Depending on the scheme name, get the curresponding protocol handler
	///TODO: integrate this into protocol factory manager via protocol factories
	string scheme = uri.scheme();
	BaseAppProtocolHandler *pProtocolHandler = GetProtocolHandler(scheme);
	if (pProtocolHandler == NULL) {
		WARN("Unable to find protocol handler for scheme %s in application %s",
				STR(scheme),
				STR(GetName()));
		return false;
	}

	//4. Initiate the stream pulling sequence
	return pProtocolHandler->PullExternalStream(uri, streamConfig);
}

bool BaseClientApplication::PushLocalStream(Variant &streamConfig) {
    //1. Minimal verification
    if (streamConfig["targetUri"] != V_STRING) {
        FATAL("Invalid uri");
        return false;
    }
    if (streamConfig["localStreamName"] != V_STRING) {
        FATAL("Invalid local stream name");
        return false;
    }
    string streamName = (string) streamConfig["localStreamName"];
    trim(streamName);
    if (streamName == "") {
        FATAL("Invalid local stream name");
        return false;
    }
    streamConfig["localStreamName"] = streamName;

    //2. Split the URI
    URI uri;
    if (!URI::FromString(streamConfig["targetUri"], true, uri)) {
        FATAL("Invalid URI: %s", STR(streamConfig["targetUri"].ToString()));
        return false;
    }
    streamConfig["targetUri"] = uri;

    //3. Depending on the scheme name, get the curresponding protocol handler
    ///TODO: integrate this into protocol factory manager via protocol factories
    string scheme = uri.scheme();
    BaseAppProtocolHandler *pProtocolHandler = GetProtocolHandler(scheme);
    if (pProtocolHandler == NULL) {
        WARN("Unable to find protocol handler for scheme %s in application %s",
                STR(scheme),
                STR(GetName()));
        return false;
    }

    //4. Initiate the stream pulling sequence
    return pProtocolHandler->PushLocalStream(streamConfig);
}

bool BaseClientApplication::ParseAuthentication() {
    //1. Get the authentication configuration node
    if (!_configuration.HasKeyChain(V_MAP, false, 1, CONF_APPLICATION_AUTH)) {
        if (_configuration.HasKey(CONF_APPLICATION_AUTH, false)) {
            WARN("Authentication node is present for application %s but is empty or invalid", STR(_name));
        }
        return true;
    }
    Variant &auth = _configuration[CONF_APPLICATION_AUTH];

    //2. Cycle over all access schemas

    FOR_MAP(auth, string, Variant, i) {
        //3. get the schema
        string scheme = MAP_KEY(i);

        //4. Get the handler
        BaseAppProtocolHandler *pHandler = GetProtocolHandler(scheme);
        if (pHandler == NULL) {
            WARN("Authentication parsing for app name %s failed. No handler registered for schema %s",
                    STR(_name),
                    STR(scheme));
            return true;
        }

        //5. Call the handler
        if (!pHandler->ParseAuthenticationNode(MAP_VAL(i), _authSettings[scheme])) {
            FATAL("Authentication parsing for app name %s failed. scheme was %s",
                    STR(_name),
                    STR(scheme));
            return false;
        }
    }

    return true;
}

void BaseClientApplication::SignalLinkedStreams(BaseInStream *pInStream, BaseOutStream *pOutStream) {

}

void BaseClientApplication::SignalUnLinkingStreams(BaseInStream *pInStream,
        BaseOutStream *pOutStream) {

}

void BaseClientApplication::Shutdown(BaseClientApplication *pApplication) {
    //1. Get the list of all active protocols
    map<uint32_t, BaseProtocol *> protocols = ProtocolManager::GetActiveProtocols();

    //2. enqueue for delete for all protocols bound to pApplication

    FOR_MAP(protocols, uint32_t, BaseProtocol *, i) {
        if ((MAP_VAL(i)->GetApplication() != NULL)
                && (MAP_VAL(i)->GetApplication()->GetId() == pApplication->GetId())) {
            MAP_VAL(i)->SetApplication(NULL);
            MAP_VAL(i)->EnqueueForDelete();
        }
    }

    //1. Get the list of all active IOHandlers and enqueue for delete for all services bound to pApplication
    map<uint32_t, IOHandler *> handlers = IOHandlerManager::GetActiveHandlers();

    FOR_MAP(handlers, uint32_t, IOHandler *, i) {
        BaseProtocol *pProtocol = MAP_VAL(i)->GetProtocol();
        BaseProtocol *pTemp = pProtocol;
        while (pTemp != NULL) {
            if ((pTemp->GetApplication() != NULL)
                    && (pTemp->GetApplication()->GetId() == pApplication->GetId())) {
                IOHandlerManager::EnqueueForDelete(MAP_VAL(i));
                break;
            }
            pTemp = pTemp->GetNearProtocol();
        }
    }

    handlers = IOHandlerManager::GetActiveHandlers();

    FOR_MAP(handlers, uint32_t, IOHandler *, i) {
        if ((MAP_VAL(i)->GetType() == IOHT_ACCEPTOR)
                && (((TCPAcceptor *) MAP_VAL(i))->GetApplication() != NULL)) {
            if (((TCPAcceptor *) MAP_VAL(i))->GetApplication()->GetId() == pApplication->GetId())
                IOHandlerManager::EnqueueForDelete(MAP_VAL(i));
        }
    }

	//4. Unregister it
	ClientApplicationManager::UnRegisterApplication(pApplication);

    //5. Delete it
    delete pApplication;
}

bool BaseClientApplication::HasExpiredAlias(string streamAlias) {
    map<string, StreamAlias>::iterator i = _streamAliases.find(streamAlias);

    if (i != _streamAliases.end()) {
        return MAP_VAL(i).Expired();
    } else {
        return false;
    }
}

string BaseClientApplication::GetStreamNameByAlias(const string &alias, bool remove) {

    map<string, StreamAlias>::iterator i = _streamAliases.find(alias);

	if (i != _streamAliases.end()) {
        StreamAlias &streamAlias = MAP_VAL(i);
        if (streamAlias.Expired()) {
            _streamAliases.erase(i);
            return "";
        }

        if (streamAlias.oneShot()) {
		if (remove) {
                string temp = streamAlias.localStreamName();
			_streamAliases.erase(i);
                return temp;
            } else {
                return streamAlias.localStreamName();
		}
	} else {
            return streamAlias.localStreamName();
		}
    } else {
        if (_hasStreamAliases)
            return "";
        else
            return alias;
	}
	}

bool BaseClientApplication::SetStreamAlias(string &localStreamName,
		string &streamAlias, int64_t expirePeriod, StreamAlias *pValue) {
	if (!_hasStreamAliases) {
		FATAL(CONF_APPLICATION_HAS_STREAM_ALIASES" property was not set up inside the configuration file");
		return false;
	}
    _streamAliases[streamAlias].localStreamName(localStreamName);
    _streamAliases[streamAlias].aliasName(streamAlias);
    _streamAliases[streamAlias].creationTime(time(NULL));
    _streamAliases[streamAlias].expirePeriod(llabs(expirePeriod));
    _streamAliases[streamAlias].permanent(expirePeriod == 0);
    _streamAliases[streamAlias].oneShot(expirePeriod < 0);
    if (pValue != NULL)
        *pValue = _streamAliases[streamAlias];
    
	return true;
}

bool BaseClientApplication::RemoveStreamAlias(const string &streamAlias) {
	if (!_hasStreamAliases)
		return false;
	_streamAliases.erase(streamAlias);
	return true;
}

map<string, StreamAlias> & BaseClientApplication::GetAllStreamAliases() {
	return _streamAliases;
}

static const char *gStrSettings[] = {
	"",
	"pullSettings",
	"pushSettings",
	"hlsSettings",
	"hdsSettings",
	"mssSettings",
    "dashSettings",
	"recordSettings",
	"processSettings"
};

OperationType BaseClientApplication::GetOperationType(BaseProtocol *pProtocol, Variant &streamConfig) {
	streamConfig.Reset();
	if (pProtocol == NULL)
		return OPERATION_TYPE_STANDARD;
	//2. Get connection type
	return GetOperationType(pProtocol->GetCustomParameters(), streamConfig);
}

OperationType BaseClientApplication::GetOperationType(Variant &allParameters, Variant &streamConfig) {
	//1. Reset the streamconfig
	streamConfig.Reset();

	//2. Check the parameters and see if they are present
	if (allParameters != V_MAP)
		return OPERATION_TYPE_STANDARD;
	if (!allParameters.HasKey("customParameters")) {
		// 2-1. Standard operation, now check if WebRTC
		if ( (allParameters.HasKey("operationType")) &&
			 (uint8_t(allParameters["operationType"]) == OPERATION_TYPE_WEBRTC) &&
			 (bool(allParameters["keepAlive"])) ) {
					streamConfig = allParameters;
					return OPERATION_TYPE_WEBRTC;
		} else {
			return OPERATION_TYPE_STANDARD;
		}
	} 
	Variant customParameters = allParameters["customParameters"];
	if (customParameters != V_MAP)
		return OPERATION_TYPE_STANDARD;

	//3. Is this a pull?
	if (customParameters.HasKey("externalStreamConfig")) {
		streamConfig = customParameters["externalStreamConfig"];
		string uri;
		if (streamConfig.HasKeyChain(V_STRING, true, 2, "uri", "fullUriWithAuth")) {
			uri = (string)streamConfig["uri"]["fullUriWithAuth"];
		} else if (streamConfig.HasKeyChain(V_STRING, true, 2, "uri", "fullDocumentPath")) {
			// Check if scheme is sercom
			if ((streamConfig.HasKeyChain(V_STRING, true, 2, "uri", "scheme"))
					&& ((string)streamConfig["uri"]["scheme"] == "sercom")) {
				uri = "sercom://" + ((string)streamConfig["uri"]["fullDocumentPath"]);
			} else {
				uri = "file://" + ((string)streamConfig["uri"]["fullDocumentPath"]);
			}
		}
		streamConfig["uri"] = uri;
		return OPERATION_TYPE_PULL;
	}

	//4. Is this a push?
	if (customParameters.HasKey("localStreamConfig")) {
		streamConfig = customParameters["localStreamConfig"];
		string uri = streamConfig["targetUri"]["fullUriWithAuth"];
		streamConfig["targetUri"] = uri;
		return OPERATION_TYPE_PUSH;
	}

	//5. Is this an HLS?
	if (customParameters.HasKey("hlsConfig")) {
		streamConfig = customParameters["hlsConfig"];
		return OPERATION_TYPE_HLS;
	}

	//6. Is this an HDS?
	if (customParameters.HasKey("hdsConfig")) {
		streamConfig = customParameters["hdsConfig"];
		return OPERATION_TYPE_HDS;
	}

	//7. Is this an MSS?
	if (customParameters.HasKey("mssConfig")) {
		streamConfig = customParameters["mssConfig"];
		return OPERATION_TYPE_MSS;
	}

    //8. Is this a DASH?
    if (customParameters.HasKey("dashConfig")) {
        streamConfig = customParameters["dashConfig"];
        return OPERATION_TYPE_DASH;
    }

    //9. Is this a record?
	if (customParameters.HasKey("recordConfig")) {
		streamConfig = customParameters["recordConfig"];
		return OPERATION_TYPE_RECORD;
	}

    //10. Is this a process?
	if (customParameters.HasKey("processConfig")) {
		streamConfig = customParameters["processConfig"];
		return OPERATION_TYPE_LAUNCHPROCESS;
	}

    //11. This is something else
	return OPERATION_TYPE_STANDARD;
}

void BaseClientApplication::StoreConnectionType(Variant &dest, BaseProtocol *pProtocol) {
	Variant streamConfig;
	OperationType operationType = GetOperationType(pProtocol, streamConfig);
	if ((operationType >= OPERATION_TYPE_PULL) && (operationType <= OPERATION_TYPE_LAUNCHPROCESS)) {
		dest[gStrSettings[operationType]] = streamConfig;
	}
	dest["connectionType"] = (uint8_t) operationType;
}

Variant &BaseClientApplication::GetStreamSettings(Variant &src) {
	OperationType operationType;
	if ((src.HasKeyChain(_V_NUMERIC, true, 1, "connectionType"))
			&& ((operationType = (OperationType) ((uint8_t) src["connectionType"])) >= OPERATION_TYPE_PULL)
			&& (operationType <= OPERATION_TYPE_METADATA)
			&& (src.HasKeyChain(V_MAP, true, 1, gStrSettings[operationType]))
			) {
		return src[gStrSettings[operationType]];
	} else {
		return _dummy;
	}
}

string BaseClientApplication::GetServiceInfo(IOHandler *pIOHandler) {
    if ((pIOHandler->GetType() != IOHT_ACCEPTOR)
            && (pIOHandler->GetType() != IOHT_UDP_CARRIER))
        return "";
    if (pIOHandler->GetType() == IOHT_ACCEPTOR) {
        if ((((TCPAcceptor *) pIOHandler)->GetApplication() == NULL)
                || (((TCPAcceptor *) pIOHandler)->GetApplication()->GetId() != GetId())) {
            return "";
        }
    } else {
        if ((((UDPCarrier *) pIOHandler)->GetProtocol() == NULL)
                || (((UDPCarrier *) pIOHandler)->GetProtocol()->GetNearEndpoint()->GetApplication() == NULL)
                || (((UDPCarrier *) pIOHandler)->GetProtocol()->GetNearEndpoint()->GetApplication()->GetId() != GetId())) {
            return "";
        }
    }
    Variant &params = pIOHandler->GetType() == IOHT_ACCEPTOR ?
            ((TCPAcceptor *) pIOHandler)->GetParameters()
            : ((UDPCarrier *) pIOHandler)->GetParameters();
    if (params != V_MAP)
        return "";
    stringstream ss;
    ss << "+---+---------------+-----+-------------------------+-------------------------+" << endl;
    ss << "|";
    ss.width(3);
    ss << (pIOHandler->GetType() == IOHT_ACCEPTOR ? "tcp" : "udp");
    ss << "|";

    ss.width(3 * 4 + 3);
    ss << (string) params[CONF_IP];
    ss << "|";

    ss.width(5);
    ss << (uint16_t) params[CONF_PORT];
    ss << "|";

    ss.width(25);
    ss << (string) params[CONF_PROTOCOL];
    ss << "|";

    ss.width(25);
    ss << GetName();
    ss << "|";

    ss << endl;

    return ss.str();
}

string BaseClientApplication::GetIngestPointPublicName(string &privateStreamName) {
    if (!_hasIngestPoints)
        return privateStreamName;
    map<string, string>::iterator i = _ingestPointsPrivateToPublic.find(privateStreamName);
    if (i == _ingestPointsPrivateToPublic.end())
        return "";
    return MAP_VAL(i);
}

string BaseClientApplication::GetIngestPointPrivateName(string &publicStreamName) {
	if (!_hasIngestPoints)
		return publicStreamName;
	map<string, string>::iterator i = _ingestPointsPublicToPrivate.find(publicStreamName);
	if (i == _ingestPointsPublicToPrivate.end())
		return "";
	return MAP_VAL(i);
}

bool BaseClientApplication::CreateIngestPoint(string &privateStreamName, string &publicStreamName) {
    if (!_hasIngestPoints) {
        FATAL("ingest points not activated inside the configuration file. Set hasIngestPoints=true inside the configuration file");
        return false;
    }

    trim(privateStreamName);
    trim(publicStreamName);
    if ((privateStreamName == "")
            || (publicStreamName == "")
            || (publicStreamName == privateStreamName)) {
        FATAL("Invalid ingest point public/private stream name");
        return false;
    }

    if ((!StreamNameAvailable(privateStreamName, NULL, true))
            || (!StreamNameAvailable(publicStreamName, NULL, true))) {
        FATAL("Both private and public ingest point stream names must be available");
        return false;
    }

    _ingestPointsPrivateToPublic[privateStreamName] = publicStreamName;
    _ingestPointsPublicToPrivate[publicStreamName] = privateStreamName;

    //GetEventLogger()->LogIngestPointCreated(this, publicStreamName, privateStreamName);

    return true;
}

void BaseClientApplication::RemoveIngestPoint(string &privateStreamName, string &publicStreamName) {
    publicStreamName = _ingestPointsPrivateToPublic[privateStreamName];
    _ingestPointsPrivateToPublic.erase(privateStreamName);
    _ingestPointsPublicToPrivate.erase(publicStreamName);
}

map<string, string> & BaseClientApplication::ListIngestPoints() {
    return _ingestPointsPrivateToPublic;
}

EventLogger * BaseClientApplication::GetEventLogger() {
    return _pEventLogger;
}

void BaseClientApplication::SignalEventSinkCreated(BaseEventSink *pSink) {

}

void BaseClientApplication::EventLogEntry(BaseEventSink *pEventSink, Variant &event) {
    if ((event != V_MAP) && (event != V_TYPED_MAP))
        return;
    pEventSink->Log(event);
}

void BaseClientApplication::SignalApplicationMessage(Variant &message) {

}

void BaseClientApplication::SignalServerBootstrapped() {
    //TODO: add here anything general as needed
}

map<string, RecordingSession>& BaseClientApplication::GetRecordSession() {
    return _dummyRet;
}

void BaseClientApplication::VodRedirectRtmpEnabling() {

    if (_configuration.HasKeyChain(V_STRING, false, 1, "vodRedirectRtmpIp")) {
        _backupRmsAddress = (string)_configuration["vodRedirectRtmpIp"];
        _vodRedirectRtmp = _backupRmsAddress.empty() ? false : true;
    }
}

bool BaseClientApplication::IsVodRedirectRtmpEnabled() {
    return _vodRedirectRtmp;
}

string BaseClientApplication::GetBackupRmsAddress() {

    return _backupRmsAddress;
}

void BaseClientApplication::InsertRtspStreamWithScale(string localStreamName, double rtspScale) {
	_rtspStreamsWithScale[localStreamName] = rtspScale;
}

map<string, double> BaseClientApplication::GetRtspStreamsWithScale(){
	return _rtspStreamsWithScale;
}
