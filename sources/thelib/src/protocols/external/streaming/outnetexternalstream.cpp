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

#include "protocols/external/streaming/outnetexternalstream.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include "protocols/external/externalmoduleapi.h"
#include "protocols/baseprotocol.h"

OutNetExternalStream::OutNetExternalStream(BaseProtocol *pProtocol, string name,
		void *pUserData, connection_t *pConnection)
: BaseOutNetStream(pProtocol, ST_OUT_NET_EXT, name) {
	memset(&_streamInterface, 0, sizeof (out_stream_t));

	ProtocolIC *pPIC = (ProtocolIC *) pConnection->context.pOpaque;
	OutStreamIC *pSIC = new OutStreamIC();
	pSIC->pAPI = pPIC->pAPI;
	pSIC->pApp = pPIC->pApp;
	pSIC->pHandler = pPIC->pHandler;
	pSIC->pOutStream = this;
	pSIC->pModule = pPIC->pModule;
	pSIC->pProtocol = pPIC->pProtocol;

	_streamInterface.context.pOpaque = pSIC;
	_streamInterface.context.pUserData = pUserData;
	_streamInterface.pConnection = pConnection;
	_streamInterface.pFactory = pConnection->pFactory;
	_streamInterface.pUniqueName = new char[name.size() + 1];
	memcpy(_streamInterface.pUniqueName, STR(name), name.size());
	_streamInterface.pUniqueName[name.size()] = 0;
	_streamInterface.uniqueId = GetUniqueId();
	AddStreamToConnectionInterface();

	_audioPacketsCount = 0;
	_audioBytesCount = 0;
	_videoPacketsCount = 0;
	_videoBytesCount = 0;
}

OutNetExternalStream::~OutNetExternalStream() {
	RemoveStreamFromConnectionInterface();
	_streamInterface.pFactory->events.stream.outStreamClosed(&_streamInterface);
	delete (OutStreamIC *) _streamInterface.context.pOpaque;
	_streamInterface.context.pOpaque = NULL;
	if (_streamInterface.pUniqueName != NULL) {
		delete[] _streamInterface.pUniqueName;
		_streamInterface.pUniqueName = NULL;
	}
}

out_stream_t *OutNetExternalStream::GetInterface() {
	return &_streamInterface;
}

bool OutNetExternalStream::IsCompatibleWithType(uint64_t type) {
	return (type == ST_IN_NET_RTP)
			|| (type == ST_IN_NET_TS)
			|| (type == ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			;
}

void OutNetExternalStream::SignalAttachedToInStream() {
	uint32_t inStreamId = 0;
	uint32_t inStreamProtocolId = 0;
	if (_pInStream != NULL) {
		inStreamId = _pInStream->GetUniqueId();
		if (_pInStream->GetProtocol() != NULL) {
			inStreamProtocolId = _pInStream->GetProtocol()->GetId();
		}
	}
	_streamInterface.pFactory->events.stream.outStreamAttached(&_streamInterface, inStreamId, inStreamProtocolId);
}

void OutNetExternalStream::SignalDetachedFromInStream() {
	uint32_t inStreamId = 0;
	uint32_t inStreamProtocolId = 0;
	if (_pInStream != NULL) {
		inStreamId = _pInStream->GetUniqueId();
		if (_pInStream->GetProtocol() != NULL) {
			inStreamProtocolId = _pInStream->GetProtocol()->GetId();
		}
	}
	_streamInterface.pFactory->events.stream.outStreamDetached(&_streamInterface, inStreamId, inStreamProtocolId);
}

void OutNetExternalStream::SignalStreamCompleted() {
	NYIA;
}

void OutNetExternalStream::SignalAudioStreamCapabilitiesChanged(
            StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
            AudioCodecInfo *pNew){
    GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
    
}

void OutNetExternalStream::SignalVideoStreamCapabilitiesChanged(
            StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
            VideoCodecInfo *pNew) {
    GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
}


bool OutNetExternalStream::PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetExternalStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	ASSERT("Operation not supported");
	return false;
}

bool OutNetExternalStream::IsCodecSupported(uint64_t codec) {
	ASSERT("Operation not supported");
	return false;
}

void OutNetExternalStream::AddStreamToConnectionInterface() {
	_streamInterface.pConnection->outStreamsCount++;
	out_stream_t **ppTemp = new out_stream_t *[_streamInterface.pConnection->outStreamsCount];
	for (uint32_t i = 0; i < _streamInterface.pConnection->outStreamsCount - 1; i++) {
		ppTemp[i] = _streamInterface.pConnection->ppOutStreams[i];
	}
	ppTemp[_streamInterface.pConnection->outStreamsCount - 1] = &_streamInterface;
	if (_streamInterface.pConnection->ppOutStreams != NULL) {
		delete[] _streamInterface.pConnection->ppOutStreams;
		_streamInterface.pConnection->ppOutStreams = NULL;
	}
	_streamInterface.pConnection->ppOutStreams = ppTemp;
}

void OutNetExternalStream::RemoveStreamFromConnectionInterface() {
	if (_streamInterface.pConnection->outStreamsCount == 0)
		return;
	if (_streamInterface.pConnection->ppOutStreams == NULL)
		return;
	bool found = false;
	for (uint32_t i = 0; i < _streamInterface.pConnection->outStreamsCount; i++) {
		if (_streamInterface.pConnection->ppOutStreams[i] == &_streamInterface) {
			found = true;
			break;
		}
	}
	if (!found)
		return;
	out_stream_t **ppTemp = NULL;
	if ((_streamInterface.pConnection->outStreamsCount - 1) > 0) {
		ppTemp = new out_stream_t *[_streamInterface.pConnection->outStreamsCount - 1];
	}
	uint32_t cursor = 0;
	for (uint32_t i = 0; i < _streamInterface.pConnection->outStreamsCount; i++) {
		if (_streamInterface.pConnection->ppOutStreams[i] == &_streamInterface)
			continue;
		ppTemp[cursor++] = _streamInterface.pConnection->ppOutStreams[i];
	}
	if (_streamInterface.pConnection->ppOutStreams != NULL) {
		delete[] _streamInterface.pConnection->ppOutStreams;
		_streamInterface.pConnection->ppOutStreams = NULL;
	}
	_streamInterface.pConnection->ppOutStreams = ppTemp;
	_streamInterface.pConnection->outStreamsCount--;
}

bool OutNetExternalStream::SignalPlay(double &dts, double &length) {
	NYIR;
}

bool OutNetExternalStream::SignalPause() {
	NYIR;
}

bool OutNetExternalStream::SignalResume() {
	NYIR;
}

bool OutNetExternalStream::SignalSeek(double &dts) {
	NYIR;
}

bool OutNetExternalStream::SignalStop() {
	NYIR;
}

bool OutNetExternalStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	_streamInterface.pFactory->events.stream.outStreamData(
			&_streamInterface,
			(uint64_t) pts,
			(uint64_t) dts,
			pData,
			dataLength,
			isAudio
			);
	return true;
}


#endif /* HAS_PROTOCOL_EXTERNAL */
