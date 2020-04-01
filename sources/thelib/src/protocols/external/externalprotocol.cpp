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

#include "protocols/external/externalprotocol.h"
#include "protocols/external/externalmoduleapi.h"
#include "application/baseclientapplication.h"
#include "protocols/external/externalappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "protocols/external/streaming/innetexternalstream.h"
#include "protocols/external/streaming/outnetexternalstream.h"
#include "streaming/streamstypes.h"

ExternalProtocol::ExternalProtocol(uint64_t type, protocol_factory_t *pFactory)
: BaseProtocol(type) {
	memset(&_connectionInterface, 0, sizeof (_connectionInterface));
	ProtocolIC *pProtocolIC = new ProtocolIC();
	BaseIC *pFactoryIC = (BaseIC *) pFactory->context.pOpaque;
	pProtocolIC->pAPI = pFactoryIC->pAPI;
	pProtocolIC->pModule = pFactoryIC->pModule;
	pProtocolIC->pApp = pFactoryIC->pApp;
	pProtocolIC->pProtocol = this;
	_connectionInterface.context.pOpaque = pProtocolIC;
	_connectionInterface.type = type;
	_connectionInterface.uniqueId = GetId();
	_connectionInterface.pFactory = pFactory;
	_connectionInterface.pPullInStreamUri = NULL;
	_connectionInterface.pPushOutStreamUri = NULL;
	_connectionInterface.pLocalStreamName = NULL;
	_connectionInterface.ppInStreams = NULL;
	_connectionInterface.inStreamsCount = 0;
	_connectionInterface.ppOutStreams = NULL;
	_connectionInterface.outStreamsCount = 0;
}

ExternalProtocol::~ExternalProtocol() {

	FOR_MAP(_inStreams, uint32_t, InNetExternalStream *, i) {
		delete MAP_VAL(i);
	}
	_inStreams.clear();

	FOR_MAP(_outStreams, uint32_t, OutNetExternalStream *, i) {
		delete MAP_VAL(i);
	}
	_outStreams.clear();

	_connectionInterface.pFactory->events.connection.connectionClosed(&_connectionInterface);
	delete (ProtocolIC *) _connectionInterface.context.pOpaque;
	if (_connectionInterface.pPullInStreamUri != NULL) {
		delete[] _connectionInterface.pPullInStreamUri;
		_connectionInterface.pPullInStreamUri = NULL;
	}
	if (_connectionInterface.pPushOutStreamUri != NULL) {
		delete[] _connectionInterface.pPushOutStreamUri;
		_connectionInterface.pPushOutStreamUri = NULL;
	}
	if (_connectionInterface.pLocalStreamName != NULL) {
		delete[] _connectionInterface.pLocalStreamName;
		_connectionInterface.pLocalStreamName = NULL;
	}
	_connectionInterface.context.pOpaque = NULL;
}

bool ExternalProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	string pullUri = "";
	string pushUri = "";
	string localStreamName = "";
	if ((parameters.HasKeyChain(V_STRING, true, 4, "customParameters", "externalStreamConfig", "uri", "fullUriWithAuth"))
			&& (parameters.HasKeyChain(V_STRING, true, 3, "customParameters", "externalStreamConfig", "localStreamName"))) {
		pullUri = (string) parameters["customParameters"]["externalStreamConfig"]["uri"]["fullUriWithAuth"];
		localStreamName = (string) parameters["customParameters"]["externalStreamConfig"]["localStreamName"];
	}
	if ((parameters.HasKeyChain(V_STRING, true, 4, "customParameters", "localStreamConfig", "targetUri", "fullUriWithAuth"))
			&& (parameters.HasKeyChain(V_STRING, true, 3, "customParameters", "localStreamConfig", "localStreamName"))) {
		pushUri = (string) parameters["customParameters"]["localStreamConfig"]["targetUri"]["fullUriWithAuth"];
		localStreamName = (string) parameters["customParameters"]["localStreamConfig"]["localStreamName"];
	}
	if (pullUri != "") {
		char *pTemp = new char[pullUri.size() + 1];
		memcpy(pTemp, STR(pullUri), pullUri.size());
		pTemp[pullUri.size()] = 0;
		_connectionInterface.pPullInStreamUri = pTemp;
	} else if (pushUri != "") {
		char *pTemp = new char[pushUri.size() + 1];
		memcpy(pTemp, STR(pushUri), pushUri.size());
		pTemp[pushUri.size()] = 0;
		_connectionInterface.pPushOutStreamUri = pTemp;
	}
	if (localStreamName != "") {
		char *pTemp = new char[localStreamName.size() + 1];
		memcpy(pTemp, STR(localStreamName), localStreamName.size());
		pTemp[localStreamName.size()] = 0;
		_connectionInterface.pLocalStreamName = pTemp;
	}

	return true;
}

IOBuffer * ExternalProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;
	return NULL;
}

bool ExternalProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_TCP;
}

void ExternalProtocol::ReadyForSend() {
	BaseProtocol::ReadyForSend();
	_connectionInterface.pFactory->events.connection.outputBufferCompletelySent(&_connectionInterface);
}

void ExternalProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication == NULL)
		return;
	ProtocolIC *pIC = (ProtocolIC *) _connectionInterface.context.pOpaque;
	pIC->pHandler = (ExternalAppProtocolHandler *) pApplication->GetProtocolHandler(this);
	bool isOutbound = false;
	Variant &params = GetCustomParameters();
	if (params.HasKeyChain(V_BOOL, false, 1, "isOutbound"))
		isOutbound = (bool)params.GetValue("isOutbound", false);
	if (isOutbound)
		_connectionInterface.pFactory->events.connection.connectSucceeded(&_connectionInterface);
	else
		_connectionInterface.pFactory->events.connection.connectionAccepted(&_connectionInterface);
}

bool ExternalProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool ExternalProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("This should not be called");
	return false;
}

bool ExternalProtocol::SignalInputData(IOBuffer &buffer) {
	//typedef uint32_t(*eventDataAvailable_f)(struct connection_t *pProtocol, void *pData, uint32_t length);
	uint32_t ignoredSize = _connectionInterface.pFactory->events.connection.dataAvailable(
			&_connectionInterface,
			GETIBPOINTER(buffer),
			GETAVAILABLEBYTESCOUNT(buffer));
	if (ignoredSize > GETAVAILABLEBYTESCOUNT(buffer)) {
		FATAL("Invalid Ignore size: %"PRIu32"; %"PRIu32, ignoredSize, GETAVAILABLEBYTESCOUNT(buffer));
		return false;
	}
	if (ignoredSize > 0)
		buffer.Ignore(ignoredSize);
	return true;
}

connection_t *ExternalProtocol::GetInterface() {
	return &_connectionInterface;
}

bool ExternalProtocol::SendData(const void *pData, uint32_t length) {
	_outputBuffer.ReadFromBuffer((uint8_t *) pData, length);
	return EnqueueForOutbound();
}

in_stream_t *ExternalProtocol::CreateInStream(string name, void *pUserData) {
	if (!GetApplication()->StreamNameAvailable(name, this, true)) {
		FATAL("Stream %s not available", STR(name));
		return NULL;
	}

	InNetExternalStream *pResult = new InNetExternalStream(
			this,
			name,
			pUserData,
			&_connectionInterface);
	if (!pResult->SetStreamsManager(GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pResult;
		pResult = NULL;
		return NULL;
	}
	_inStreams[pResult->GetUniqueId()] = pResult;

	return pResult->GetInterface();
}

void ExternalProtocol::CloseInStream(in_stream_t *pInStream) {
	InStreamIC *pSIC = (InStreamIC *) pInStream->context.pOpaque;
	if (!MAP_HAS1(_inStreams, pSIC->pInStream->GetUniqueId()))
		return;
	uint32_t uniqueId = pSIC->pInStream->GetUniqueId();
	delete _inStreams[uniqueId];
	_inStreams.erase(uniqueId);
}

out_stream_t *ExternalProtocol::CreateOutStream(string inStreamName, void *pUserData) {
	map<uint32_t, BaseStream *> possibleStreams =
			GetApplication()->GetStreamsManager()->FindByTypeByName(
			ST_IN_NET, inStreamName, true, false);
	BaseInNetStream *pInStream = NULL;

	FOR_MAP(possibleStreams, uint32_t, BaseStream *, i) {
		if (((BaseInStream *) MAP_VAL(i))->IsCompatibleWithType(ST_OUT_NET_EXT)) {
			pInStream = (BaseInNetStream *) MAP_VAL(i);
			break;
		}
	}
	if (pInStream == NULL) {
		FATAL("No compatible inbound stream found for name %s", STR(inStreamName));
		return NULL;
	}

	OutNetExternalStream *pResult = new OutNetExternalStream(
			this,
			inStreamName,
			pUserData,
			&_connectionInterface);
	if (!pResult->SetStreamsManager(GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pResult;
		pResult = NULL;
		return NULL;
	}
	_outStreams[pResult->GetUniqueId()] = pResult;

	pResult->GetInterface()->pFactory->events.stream.outStreamCreated(pResult->GetInterface());

	if (!pInStream->Link(pResult)) {
		FATAL("No compatible inbound stream found for name %s", STR(inStreamName));
		_outStreams[pResult->GetUniqueId()] = NULL;
		delete pResult;
		return NULL;
	}

	return pResult->GetInterface();
}

void ExternalProtocol::CloseOutStream(out_stream_t *pOutStream) {
	OutStreamIC *pSIC = (OutStreamIC *) pOutStream->context.pOpaque;
	if (!MAP_HAS1(_outStreams, pSIC->pOutStream->GetUniqueId()))
		return;
	uint32_t uniqueId = pSIC->pOutStream->GetUniqueId();
	delete _outStreams[uniqueId];
	_outStreams.erase(uniqueId);
}

#endif /* HAS_PROTOCOL_EXTERNAL */
