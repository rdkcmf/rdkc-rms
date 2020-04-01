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


#ifdef HAS_PROTOCOL_EXTERNAL
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"
#include "protocols/external/externalmoduleapi.h"
#include "protocols/baseprotocol.h"
#include "protocols/external/externalprotocolmodule.h"
#include "protocols/external/externalprotocol.h"
#include "protocols/external/externaljobtimerprotocol.h"
#include "protocols/external/streaming/innetexternalstream.h"
#include "streaming/streamstypes.h"
#include "streaming/baseoutstream.h"
#include "protocols/protocolfactorymanager.h"
#include "application/baseclientapplication.h"
#include "netio/netio.h"

void apiConnect(struct protocol_factory_t *pFactory, const char *pIp,
		uint16_t port, const char *pProtocolChainName, void *pUserData) {
	if (pFactory == NULL)
		return;
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			pProtocolChainName);
	if (chain.size() == 0) {
		pFactory->events.connection.connectFailed(pFactory, NULL, pIp, port, pUserData);
		return;
	}
	Variant params;
	params["pFactory"] = (uint64_t) pFactory;
	params["ip"] = pIp;
	params["port"] = port;
	params["pUserData"] = (uint64_t) pUserData;
	TCPConnector<ExternalModuleAPI>::Connect(pIp, port, chain, params);
}

void apiSendData(struct connection_t *pConnection, const void *pData, uint32_t length) {
	if (pConnection == NULL)
		return;
	if (pConnection->context.pOpaque == NULL)
		return;
	ExternalProtocol *pExternalProtocol = ((ProtocolIC *) pConnection->context.pOpaque)->pProtocol;
	if (!pExternalProtocol->SendData(pData, length)) {
		pExternalProtocol->EnqueueForDelete();
	}
}

void apiCloseConnection(struct connection_t *pConnection, int gracefully) {
	if (pConnection == NULL)
		return;
	if (pConnection->context.pOpaque == NULL)
		return;
	if (((ProtocolIC *) pConnection->context.pOpaque)->pProtocol == NULL)
		return;
	if (gracefully != 0)
		((ProtocolIC *) pConnection->context.pOpaque)->pProtocol->GracefullyEnqueueForDelete();
	else
		((ProtocolIC *) pConnection->context.pOpaque)->pProtocol->EnqueueForDelete();
}

void apiCreateInStream(struct connection_t *pConnection, const char *pUniqueName,
		void *pUserData) {
	if (pConnection == NULL) {
		FATAL("pConnection is null");
		return;
	}
	if (pConnection->context.pOpaque == NULL) {
		FATAL("corrupted pConnection");
		return;
	}
	ProtocolIC *pIC = (ProtocolIC *) pConnection->context.pOpaque;
	if (!pIC->pApp->StreamNameAvailable(pUniqueName, pIC->pProtocol, true)) {
		FATAL("Stream name `%s` not available", pUniqueName);
		pConnection->pFactory->events.stream.inStreamCreateFailed(pConnection, pUniqueName, pUserData);
		return;
	}
	in_stream_t *pInStream = pIC->pProtocol->CreateInStream(pUniqueName, pUserData);
	if (pInStream == NULL) {
		FATAL("Stream creation failed on name %s", pUniqueName);
		pConnection->pFactory->events.stream.inStreamCreateFailed(pConnection, pUniqueName, pUserData);
		return;
	}

	pInStream->pFactory->events.stream.inStreamCreated(pInStream);
}

bool apiSetupInStreamAudioCodecAAC(struct in_stream_t *pInStream, const uint8_t pCodec[2]) {
	if (pInStream == NULL) {
		FATAL("pInStream is NULL");
		return false;
	}
	if (pInStream->context.pOpaque == NULL) {
		FATAL("pInStream has changed context.pOpaque");
		return false;
	}
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;

	return pSIC->pInStream->SetupAudioCodecAAC(pCodec);
}

bool apiSetupInStreamAudioCodecG711(struct in_stream_t *pInStream) {
	if (pInStream == NULL) {
		FATAL("pInStream is NULL");
		return false;
	}
	if (pInStream->context.pOpaque == NULL) {
		FATAL("pInStream has changed context.pOpaque");
		return false;
	}
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;

	return pSIC->pInStream->SetupAudioCodecG711();
}

bool apiSetupInStreamVideoCodecH264(struct in_stream_t *pInStream, const uint8_t *pSPS,
		uint32_t spsLength, const uint8_t *pPPS, uint32_t ppsLength) {
	if (pInStream == NULL) {
		FATAL("pInStream is NULL");
		return false;
	}
	if (pInStream->context.pOpaque == NULL) {
		FATAL("pInStream has changed context.pOpaque");
		return false;
	}
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;

	return pSIC->pInStream->SetupVideoCodecH264(pSPS, spsLength, pPPS, ppsLength);
}

bool apiFeedInStream(struct in_stream_t *pInStream, uint64_t pts, uint64_t dts,
		uint8_t *pData, uint32_t length, int isAudio) {
	if (pInStream == NULL) {
		FATAL("pInStream is NULL");
		return false;
	}
	if (pInStream->context.pOpaque == NULL) {
		FATAL("pInStream has changed context.pOpaque");
		return false;
	}
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;
	if (!pSIC->pInStream->FeedData(pData, length, 0, length, (double) pts,
			(double) dts, isAudio)) {
		FATAL("Unable to A/V data");
		pSIC->pProtocol->EnqueueForDelete();
		return false;
	}
	return true;
}

void apiCloseInStream(struct in_stream_t *pInStream) {
	if (pInStream == NULL) {
		FATAL("pInStream is NULL");
		return;
	}
	if (pInStream->context.pOpaque == NULL) {
		FATAL("pInStream has changed context.pOpaque");
		return;
	}
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;

	pSIC->pProtocol->CloseInStream(pInStream);
}

void apiCreateOutStream(struct connection_t *pConnection, const char *pInStreamName,
		void *pUserData) {
	if (pConnection == NULL) {
		FATAL("pConnection is null");
		return;
	}
	if (pConnection->context.pOpaque == NULL) {
		FATAL("corrupted pConnection");
		return;
	}
	ProtocolIC *pIC = (ProtocolIC *) pConnection->context.pOpaque;
	out_stream_t *pOutStream = pIC->pProtocol->CreateOutStream(pInStreamName, pUserData);
	if (pOutStream == NULL) {
		FATAL("Stream creation failed on name %s", pInStreamName);
		pConnection->pFactory->events.stream.outStreamCreateFailed(pConnection, pInStreamName, pUserData);
		return;
	}
}

void apiCloseOutStream(struct out_stream_t *pOutStream) {
	if (pOutStream == NULL) {
		FATAL("pOutStream is NULL");
		return;
	}
	if (pOutStream->context.pOpaque == NULL) {
		FATAL("pOutStream has changed context.pOpaque");
		return;
	}
	OutStreamIC *pSIC = (OutStreamIC *) pOutStream->context.pOpaque;

	pSIC->pProtocol->CloseOutStream(pOutStream);
}

void apiCreateTimer(struct protocol_factory_t *pFactory, uint32_t period,
		void *pUserData) {
	ExternalJobTimerProtocol *pTimer = new ExternalJobTimerProtocol(pFactory, pUserData);
	pTimer->GetCustomParameters()["pUSerData"] = (uint64_t) pUserData;
	pTimer->SetApplication(((BaseIC *) pFactory->context.pOpaque)->pApp);
	pTimer->EnqueueForTimeEvent(period);
	jobtimer_t *pInterface = pTimer->GetInterface();
	pInterface->pFactory->events.job.timerCreated(pInterface);
}

void apiCloseTimer(struct jobtimer_t *pJobTimer) {
	if (pJobTimer == NULL)
		return;
	if (pJobTimer->context.pOpaque == NULL)
		return;
	if (((TimerIC *) pJobTimer->context.pOpaque)->pTimer == NULL)
		return;
	((TimerIC *) pJobTimer->context.pOpaque)->pTimer->EnqueueForDelete();
}

uint32_t apiRtmpGetProtocolIdByStreamId(struct protocol_factory_t *pFactory, uint32_t uniqueStreamId) {
	if ((pFactory == NULL)
			|| (pFactory->context.pOpaque == NULL)
			|| (((BaseIC *) pFactory->context.pOpaque)->pApp == NULL)
			)
		return 0;
	BaseClientApplication *pApp = ((BaseIC *) pFactory->context.pOpaque)->pApp;
	StreamsManager *pSM = pApp->GetStreamsManager();
	if (pSM == NULL)
		return 0;
	BaseStream *pStream = pSM->FindByUniqueId(uniqueStreamId);
	if (pStream == NULL)
		return 0;
	BaseProtocol *pProtocol = pStream->GetProtocol();
	if (pProtocol == NULL)
		return 0;
	uint64_t protocolType = pProtocol->GetType();
	if ((protocolType != PT_INBOUND_RTMP) && (protocolType != PT_OUTBOUND_RTMP))
		return 0;
	return pProtocol->GetId();
}

bool apiRtmpSendMessage(struct protocol_factory_t *pFactory,
		uint32_t rtmpConnectionId, variant_t *pMessage, bool expectResponse) {
	if ((pFactory == NULL)
			|| (pFactory->context.pOpaque == NULL)
			|| (((BaseIC *) pFactory->context.pOpaque)->pApp == NULL)
			) {
		FATAL("Invalid factory instance");
		return false;
	}
	BaseClientApplication *pApp = ((BaseIC *) pFactory->context.pOpaque)->pApp;
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(rtmpConnectionId, false);
	if (pProtocol == NULL) {
		FATAL("Protocol %"PRIu32" not available anymore", rtmpConnectionId);
		return false;
	}
	uint64_t protocolType = pProtocol->GetType();
	if ((protocolType != PT_INBOUND_RTMP) && (protocolType != PT_OUTBOUND_RTMP)) {
		FATAL("Protocol %"PRIu32" is not a RTMP protocol instance", rtmpConnectionId);
		return false;
	}
	if (pProtocol->GetApplication() == NULL) {
		FATAL("Protocol %"PRIu32" is not bound to any application", rtmpConnectionId);
		return false;
	}
	if (pProtocol->GetApplication()->GetId() != pApp->GetId()) {
		FATAL("Protocol %"PRIu32" is bound to another application", rtmpConnectionId);
		return false;
	}

	BaseRTMPAppProtocolHandler *pRTMPHandler =
			(BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(pProtocol);
	if (pRTMPHandler == NULL) {
		FATAL("Unable to get RTMP protocol handler");
		return false;
	}
	return pRTMPHandler->SendRTMPMessage((BaseRTMPProtocol *) pProtocol,
			*((Variant *) pMessage->pOpaque), expectResponse);
}

bool apiRtmpBroadcastMessage(struct in_stream_t *pInStream, variant_t *pMessage) {
	if ((pInStream == NULL)
			|| (pInStream->pFactory == NULL)
			|| (pInStream->pFactory->context.pOpaque == NULL)
			|| (((BaseIC *) pInStream->pFactory->context.pOpaque)->pApp == NULL)
			) {
		FATAL("Invalid stream instance");
		return false;
	}
	BaseClientApplication *pApp = ((BaseIC *) pInStream->pFactory->context.pOpaque)->pApp;
	StreamsManager *pSM = pApp->GetStreamsManager();
	if (pSM == NULL) {
		FATAL("Unable to get the streams manager");
		return false;
	}
	BaseRTMPAppProtocolHandler *pRTMPHandler =
			(BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(PT_INBOUND_RTMP);
	if (pRTMPHandler == NULL) {
		pRTMPHandler = (BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(PT_OUTBOUND_RTMP);
		if (pRTMPHandler == NULL) {
			FATAL("Unable to get RTMP protocol handler");
			return false;
		}
	}
	BaseStream *pStream = pSM->FindByUniqueId(pInStream->uniqueId);
	if (pStream == NULL) {
		FATAL("Unable to get the in stream");
		return false;
	}
	if (pStream->GetType() != ST_IN_NET_EXT) {
		FATAL("Invalid stream type");
		return false;
	}
	vector<BaseOutStream*> outStreams = ((BaseInStream *) pStream)->GetOutStreams();
	for (uint32_t i = 0; i < outStreams.size(); i++) {
		BaseOutStream *pOutStream = outStreams[i];
		BaseProtocol *pProtocol = pOutStream->GetProtocol();
		if ((pProtocol == NULL)
				|| ((pProtocol->GetType() != PT_INBOUND_RTMP)
				&& (pProtocol->GetType() != PT_OUTBOUND_RTMP))) {
			continue;
		}
		if (!pRTMPHandler->SendRTMPMessage((BaseRTMPProtocol *) pProtocol,
				*((Variant *) pMessage->pOpaque), false)) {
			WARN("RTMP broadcast error on RTMP connection %"PRIu32, pProtocol->GetId());
		}
	}
	return true;
}

#define GETTARGETVARIANT \
va_list arguments; \
va_start(arguments, depth); \
Variant &target = apiGetTargetVariant(pVariant, depth, arguments); \
va_end(arguments);

Variant &apiGetTargetVariant(variant_t *pVariant, int depth, va_list arguments) {
	if (pVariant == NULL) {
		ASSERT("Variant is NULL");
	}
	if (pVariant->pOpaque == NULL) {
		ASSERT("Variant is NULL");
	}
	Variant *pCursor = (Variant *) pVariant->pOpaque;
	for (uint8_t i = 0; i < depth; i++) {
		const char *pPathElement = va_arg(arguments, const char *);
		if (*pCursor == V_NULL) {
			pCursor->IsArray(false);
			if (i < depth - 1) {
				(*pCursor)[pPathElement] = Variant();
			}
		}
		pCursor = &pCursor->GetValue(pPathElement, true);
	}
	return *pCursor;
}

variant_t * apiVariantCreate() {
	variant_t *pResult = new variant_t;
	pResult->pOpaque = new Variant();
	return pResult;
}

variant_t *apiVariantCreateRtmpRequest(const char *pFunctionName) {
	Variant parameters;
	parameters.PushToArray(Variant());
	Variant request = GenericMessageFactory::GetInvoke(3, 0, 0, false, 0, pFunctionName, parameters);
	variant_t *pResult = new variant_t;
	pResult->pOpaque = new Variant();
	*((Variant *) pResult->pOpaque) = request;
	return pResult;
}

void apiVariantRelease(variant_t *pVariant);

bool apiVariantAddRtmpRequestParameter(variant_t *pRequest, variant_t **ppParameter, bool release) {
	if ((pRequest == NULL)
			|| (pRequest->pOpaque == NULL)
			|| (ppParameter == NULL)
			|| ((*ppParameter) == NULL)
			|| ((*ppParameter)->pOpaque == NULL))
		return false;
	Variant &request = *(Variant *) pRequest->pOpaque;
	M_INVOKE_PARAMS(request).PushToArray(*((Variant *) (*ppParameter)->pOpaque));
	if (release) {
		apiVariantRelease(*ppParameter);
		*ppParameter = NULL;
	}
	return true;
}

variant_t *apiVariantCopy(variant_t *pSource) {
	if ((pSource == NULL)
			|| (pSource->pOpaque == NULL))
		return NULL;
	variant_t *pResult = new variant_t;
	Variant *pVariant = new Variant();
	*pVariant = *((Variant *) pSource->pOpaque);
	pResult->pOpaque = pVariant;
	return pResult;
}

void apiVariantRelease(variant_t *pVariant) {
	if (pVariant == NULL)
		return;
	if (pVariant->pOpaque != NULL) {
		delete (Variant *) (pVariant->pOpaque);
		pVariant->pOpaque = NULL;
	}
}

void apiVariantReset(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	target.Reset();
}

char * apiVariantGetXml(variant_t *pVariant, char *pDest, uint32_t destLength, int depth, ...) {
	GETTARGETVARIANT;
	string result = "";
	target.SerializeToXml(result, true);
	uint32_t length = (uint32_t) result.length();
	if (length > destLength)
		length = destLength;
	memset(pDest, 0, destLength);
	memcpy(pDest, result.c_str(), length);
	return pDest;
}

bool apiVariantGetBool(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (bool) target;
}

int8_t apiVariantGetI8(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (int8_t) target;
}

int16_t apiVariantGetI16(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (int16_t) target;
}

int32_t apiVariantGetI32(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (int32_t) target;
}

int64_t apiVariantGetI64(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (int64_t) target;
}

uint8_t apiVariantGetUI8(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (uint8_t) target;
}

uint16_t apiVariantGetUI16(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (uint16_t) target;
}

uint32_t apiVariantGetUI32(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (uint32_t) target;
}

uint64_t apiVariantGetUI64(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (uint64_t) target;
}

double apiVariantGetDouble(variant_t *pVariant, int depth, ...) {
	GETTARGETVARIANT;
	return (double) target;
}

char* apiVariantGetString(variant_t *pVariant, char *pDest, uint32_t destLength, int depth, ...) {
	GETTARGETVARIANT;
	string result = target;
	uint32_t length = (uint32_t) result.length();
	if (length > destLength)
		length = destLength;
	memset(pDest, 0, destLength);
	memcpy(pDest, result.c_str(), length);
	return pDest;
}

void apiVariantSetBool(variant_t *pVariant, bool value, int depth, ...) {
	GETTARGETVARIANT;
	target = (bool) value;
}

void apiVariantSetI8(variant_t *pVariant, int8_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (int8_t) value;
}

void apiVariantSetI16(variant_t *pVariant, int16_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (int16_t) value;
}

void apiVariantSetI32(variant_t *pVariant, int32_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (int32_t) value;
}

void apiVariantSetI64(variant_t *pVariant, int64_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (int64_t) value;
}

void apiVariantSetUI8(variant_t *pVariant, uint8_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (uint8_t) value;
}

void apiVariantSetUI16(variant_t *pVariant, uint16_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (uint16_t) value;
}

void apiVariantSetUI32(variant_t *pVariant, uint32_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (uint32_t) value;
}

void apiVariantSetUI64(variant_t *pVariant, uint64_t value, int depth, ...) {
	GETTARGETVARIANT;
	target = (uint64_t) value;
}

void apiVariantSetDouble(variant_t *pVariant, double value, int depth, ...) {
	GETTARGETVARIANT;
	target = (double) value;
}

void apiVariantSetString(variant_t *pVariant, const char *pValue, int depth, ...) {
	GETTARGETVARIANT;
	target = pValue;
}

void apiVariantPushBool(variant_t *pVariant, bool value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((bool)value);
}

void apiVariantPushI8(variant_t *pVariant, int8_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((int8_t) value);
}

void apiVariantPushI16(variant_t *pVariant, int16_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((int16_t) value);
}

void apiVariantPushI32(variant_t *pVariant, int32_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((int32_t) value);
}

void apiVariantPushI64(variant_t *pVariant, int64_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((int64_t) value);
}

void apiVariantPushUI8(variant_t *pVariant, uint8_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((uint8_t) value);
}

void apiVariantPushUI16(variant_t *pVariant, uint16_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((uint16_t) value);
}

void apiVariantPushUI32(variant_t *pVariant, uint32_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((uint32_t) value);
}

void apiVariantPushUI64(variant_t *pVariant, uint64_t value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((uint64_t) value);
}

void apiVariantPushDouble(variant_t *pVariant, double value, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray((double) value);
}

void apiVariantPushString(variant_t *pVariant, const char *pValue, int depth, ...) {
	GETTARGETVARIANT;
	target.PushToArray(pValue);
}

BaseClientApplication *ExternalModuleAPI::_pApp = NULL;

ExternalModuleAPI::ExternalModuleAPI(BaseClientApplication *pApp) {
	_pApp = pApp;
}

ExternalModuleAPI::~ExternalModuleAPI() {
}

void ExternalModuleAPI::InitAPI(api_t *pAPI) {
	memset(pAPI, 0, sizeof (api_t));
	pAPI->connection.connect = apiConnect;
	pAPI->connection.sendData = apiSendData;
	pAPI->connection.closeConnection = apiCloseConnection;
	pAPI->stream.createInStream = apiCreateInStream;
	pAPI->stream.setupInStreamAudioCodecAAC = apiSetupInStreamAudioCodecAAC;
	pAPI->stream.setupInStreamAudioCodecG711 = apiSetupInStreamAudioCodecG711;
	pAPI->stream.setupInStreamVideoCodecH264 = apiSetupInStreamVideoCodecH264;
	pAPI->stream.feedInStream = apiFeedInStream;
	pAPI->stream.closeInStream = apiCloseInStream;
	pAPI->stream.createOutStream = apiCreateOutStream;
	pAPI->stream.closeOutStream = apiCloseOutStream;
	pAPI->job.createTimer = apiCreateTimer;
	pAPI->job.closeTimer = apiCloseTimer;
	pAPI->variant.create = apiVariantCreate;
	pAPI->variant.copy = apiVariantCopy;
	pAPI->variant.createRtmpRequest = apiVariantCreateRtmpRequest;
	pAPI->variant.addRtmpRequestParameter = apiVariantAddRtmpRequestParameter;
	pAPI->variant.release = apiVariantRelease;
	pAPI->variant.reset = apiVariantReset;
	pAPI->variant.getXml = apiVariantGetXml;
	pAPI->variant.getBool = apiVariantGetBool;
	pAPI->variant.getI8 = apiVariantGetI8;
	pAPI->variant.getI16 = apiVariantGetI16;
	pAPI->variant.getI32 = apiVariantGetI32;
	pAPI->variant.getI64 = apiVariantGetI64;
	pAPI->variant.getUI8 = apiVariantGetUI8;
	pAPI->variant.getUI16 = apiVariantGetUI16;
	pAPI->variant.getUI32 = apiVariantGetUI32;
	pAPI->variant.getUI64 = apiVariantGetUI64;
	pAPI->variant.getDouble = apiVariantGetDouble;
	pAPI->variant.getString = apiVariantGetString;
	pAPI->variant.setBool = apiVariantSetBool;
	pAPI->variant.setI8 = apiVariantSetI8;
	pAPI->variant.setI16 = apiVariantSetI16;
	pAPI->variant.setI32 = apiVariantSetI32;
	pAPI->variant.setI64 = apiVariantSetI64;
	pAPI->variant.setUI8 = apiVariantSetUI8;
	pAPI->variant.setUI16 = apiVariantSetUI16;
	pAPI->variant.setUI32 = apiVariantSetUI32;
	pAPI->variant.setUI64 = apiVariantSetUI64;
	pAPI->variant.setDouble = apiVariantSetDouble;
	pAPI->variant.setString = apiVariantSetString;
	pAPI->variant.pushBool = apiVariantPushBool;
	pAPI->variant.pushI8 = apiVariantPushI8;
	pAPI->variant.pushI16 = apiVariantPushI16;
	pAPI->variant.pushI32 = apiVariantPushI32;
	pAPI->variant.pushI64 = apiVariantPushI64;
	pAPI->variant.pushUI8 = apiVariantPushUI8;
	pAPI->variant.pushUI16 = apiVariantPushUI16;
	pAPI->variant.pushUI32 = apiVariantPushUI32;
	pAPI->variant.pushUI64 = apiVariantPushUI64;
	pAPI->variant.pushDouble = apiVariantPushDouble;
	pAPI->variant.pushString = apiVariantPushString;
	pAPI->rtmp.getProtocolIdByStreamId = apiRtmpGetProtocolIdByStreamId;
	pAPI->rtmp.sendMessage = apiRtmpSendMessage;
	pAPI->rtmp.broadcastMessage = apiRtmpBroadcastMessage;
}

bool ExternalModuleAPI::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant &parameters) {
	if (pProtocol == NULL) {
		protocol_factory_t *pFactory = (protocol_factory_t *) ((uint64_t) parameters["pFactory"]);
		string ip = (string) parameters["ip"];
		uint16_t port = (uint16_t) parameters["port"];
		void *pUserData = (void *) ((uint64_t) parameters["pUserData"]);
		string pullUri = "";
		if (parameters.HasKeyChain(V_STRING, true, 4,
				"customParameters", "externalStreamConfig", "uri",
				"fullUriWithAuth")) {
			pullUri = (string) parameters["customParameters"]["externalStreamConfig"]["uri"]["fullUriWithAuth"];
		}
		pFactory->events.connection.connectFailed(pFactory,
				pullUri != "" ? STR(pullUri) : NULL, STR(ip), port, pUserData);
		return _pApp->OutboundConnectionFailed(parameters);
	}
	pProtocol->GetCustomParameters()["isOutbound"] = (bool)true;
	pProtocol->SetApplication(_pApp);
	return true;
}

#endif /* HAS_PROTOCOL_EXTERNAL */
