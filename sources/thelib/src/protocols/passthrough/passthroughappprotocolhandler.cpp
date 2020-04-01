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


#include "protocols/passthrough/passthroughappprotocolhandler.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "application/baseclientapplication.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "streaming/baseinstream.h"
#include "streaming/hls/outnethlsstream.h"

PassThroughAppProtocolHandler::PassThroughAppProtocolHandler(Variant& configuration)
: BaseAppProtocolHandler(configuration) {

}

PassThroughAppProtocolHandler::~PassThroughAppProtocolHandler() {
}

void PassThroughAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void PassThroughAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool PassThroughAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	Variant customParameters = streamConfig;
	customParameters["customParameters"]["externalStreamConfig"] = streamConfig;
	string scheme = uri.scheme();
	if ((scheme == SCHEME_MPEGTSUDP)
			|| (scheme == SCHEME_DEEP_MPEGTSUDP)) {
		return PullMpegtsUdp(uri, customParameters);
	} else if ((scheme == SCHEME_MPEGTSTCP)
			|| (scheme == SCHEME_DEEP_MPEGTSTCP)) {
		return PullMpegtsTcp(uri, customParameters);
	} else {
		FATAL("Invalid scheme name: %s", STR(scheme));
		return false;
	}
}

bool PassThroughAppProtocolHandler::PushLocalStream(Variant streamConfig) {
	//1. get the stream name
	string streamName = (string) streamConfig["localStreamName"];

	//2. Get the streams manager
	StreamsManager *pStreamsManager = GetApplication()->GetStreamsManager();

	//3. Search for all streams named streamName having the type of IN_NET
	map<uint32_t, BaseStream *> streams = pStreamsManager->FindByTypeByName(
			ST_IN_NET, streamName, true, true);
	if (streams.size() == 0) {
		FATAL("Stream %s not found", STR(streamName));
		return false;
	}

	//4. See if inside the returned collection of streams
	//we have something compatible with RTMP
	BaseInStream *pInStream = NULL;

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_PASSTHROUGH) ||
				MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_TS)) {
			pInStream = (BaseInStream *) MAP_VAL(i);
			break;
		}
	}
	if (pInStream == NULL) {
		WARN("Stream %s not found or is incompatible with PassThrough output",
				STR(streamName));
		return false;
	}

	Variant customParameters;
	customParameters["customParameters"]["localStreamConfig"] = streamConfig;
	if (streamConfig["targetUri"]["scheme"] == SCHEME_MPEGTSUDP) {
		return PushMpegtsUdp(pInStream, customParameters);
	} else if (streamConfig["targetUri"]["scheme"] == SCHEME_MPEGTSTCP) {
		return PushMpegtsTcp(pInStream, customParameters);
	} else {
		FATAL("Invalid scheme name: %s", STR(streamConfig["targetUri"]["scheme"]));
		return false;
	}
}

bool PassThroughAppProtocolHandler::PullMpegtsUdp(URI &uri, Variant &streamConfig) {
	//1. get the complete protocol chain
	BaseProtocol *pProtocol = ProtocolFactoryManager::CreateProtocolChain(
			CONF_PROTOCOL_UDP_PASSTHROUGH, streamConfig);
	if (pProtocol == NULL) {
		FATAL("Unable to create the protocol chain");
		return false;
	}

	//2. create the carrier
	UDPCarrier *pCarrier = UDPCarrier::Create(uri.ip(), uri.port(),
			(uint16_t) streamConfig["ttl"],
			(uint16_t) streamConfig["tos"],
			(string) streamConfig["ssmIp"]);
	if (pCarrier == NULL) {
		FATAL("Unable to bind on %s:%hu", STR(uri.ip()), uri.port());
		return false;
	}
	if (!pCarrier->StartAccept()) {
		FATAL("Unable to bind on %s:%hu", STR(uri.ip()), uri.port());
		return false;
	}

	//3. Link the carrier with the protocol
	pProtocol->GetFarEndpoint()->SetIOHandler(pCarrier);
	pCarrier->SetProtocol(pProtocol->GetFarEndpoint());

	//4. Setup the application
	pProtocol->GetNearEndpoint()->SetApplication(GetApplication());

	//5. Done
	return true;
}

bool PassThroughAppProtocolHandler::PullMpegtsTcp(URI &uri, Variant &streamConfig) {
	//1. Create a tcp acceptor having CONF_PROTOCOL_INBOUND_TCP_TS as protocol blueprint
	Variant varAcceptor;
	varAcceptor[CONF_IP] = uri.ip();
	varAcceptor[CONF_PORT] = uri.port();
	varAcceptor[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_TCP_TS;
	varAcceptor["fireOnlyOnce"] = (bool)true;
	varAcceptor["localStreamName"] = streamConfig["localStreamName"];
	varAcceptor["customParameters"]["externalStreamConfig"] = streamConfig;
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(CONF_PROTOCOL_INBOUND_TCP_TS);

	if (chain.size() == 0) {
		FATAL("Unable to get chain "CONF_PROTOCOL_INBOUND_TCP_TS);
		return false;
	}

	map<uint32_t, IOHandler *> activeHandlers = IOHandlerManager::GetActiveHandlers();

	FOR_MAP(activeHandlers, uint32_t, IOHandler *, i) {
		if (MAP_VAL(i)->GetType() != IOHT_ACCEPTOR)
			continue;
		TCPAcceptor *pTemp = (TCPAcceptor *) MAP_VAL(i);
		if ((pTemp->GetParameters()[CONF_IP] == uri.ip().c_str())
				&& ((uint16_t) pTemp->GetParameters()[CONF_PORT] == uri.port())) {
			FATAL("Invalid IP:PORT specified: overlapping values");
			return false;
		}
	}

	TCPAcceptor *pAcceptor = new TCPAcceptor(varAcceptor[CONF_IP],
			varAcceptor[CONF_PORT],
			varAcceptor,
			chain);

	pAcceptor->GetParameters()["id"] = pAcceptor->GetId();
	pAcceptor->SetApplication(GetApplication());

	//2. Bind the acceptor created at step 1
	if (!pAcceptor->Bind()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Unable to bind acceptor");
		return false;
	}
	if (!pAcceptor->StartAccept()) {
		IOHandlerManager::EnqueueForDelete(pAcceptor);
		FATAL("Unable to start acceptor");
		return false;
	}
	return true;
}

bool PassThroughAppProtocolHandler::PushMpegtsUdp(BaseInStream *pInStream, Variant streamConfig) {

	if (pInStream->GetType() == ST_IN_NET_PASSTHROUGH) {
		//1. get the protocol
		PassThroughProtocol *pProtocol = (PassThroughProtocol *) pInStream->GetProtocol();

		//2. create and register the outbound stream
		return pProtocol->RegisterOutboundUDP(streamConfig);
	}
#ifdef HAS_PROTOCOL_HLS
	else {
		//TODO: Ad your generic code here
		string localStreamName = (string) streamConfig["customParameters"]["localStreamConfig"]["localStreamName"];
		OutNetHLSStream *pOutNetHLS = OutNetHLSStream::GetInstance(
				this->GetApplication(),
				localStreamName,
				streamConfig);
		if (pOutNetHLS == NULL) {
			FATAL("Unable to create HLS stream");
			return false;
		}

		//4. Link them together
		if (!pInStream->Link(pOutNetHLS)) {
			pOutNetHLS->EnqueueForDelete();
			FATAL("Source stream %s not compatible with HLS streaming", STR(localStreamName));
			return false;
		}
		pOutNetHLS->Enable(true);
		return true;
	}
#else
	else {
		FATAL("Invalid source stream type");
		return false;
	}
#endif /* HAS_PROTOCOL_HLS */
}

bool PassThroughAppProtocolHandler::PushMpegtsTcp(BaseInStream *pInStream, Variant streamConfig) {
	NYIR;
}
