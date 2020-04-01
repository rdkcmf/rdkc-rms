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

#include "protocols/external/streaming/innetexternalstream.h"
#include "streaming/streamstypes.h"
#include "protocols/external/externalmoduleapi.h"
#include "protocols/external/externalprotocol.h"
#include "streaming/baseoutstream.h"

InNetExternalStream::InNetExternalStream(BaseProtocol *pProtocol, string name,
		void *pUserData, connection_t *pConnection)
: BaseInNetStream(pProtocol, ST_IN_NET_EXT, name) {
	memset(&_streamInterface, 0, sizeof (in_stream_t));

	ProtocolIC *pPIC = (ProtocolIC *) pConnection->context.pOpaque;
	InStreamIC *pSIC = new InStreamIC();
	pSIC->pAPI = pPIC->pAPI;
	pSIC->pApp = pPIC->pApp;
	pSIC->pHandler = pPIC->pHandler;
	pSIC->pInStream = this;
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
}

InNetExternalStream::~InNetExternalStream() {
	RemoveStreamFromConnectionInterface();
	_streamInterface.pFactory->events.stream.inStreamClosed(&_streamInterface);
	delete (InStreamIC *) _streamInterface.context.pOpaque;
	_streamInterface.context.pOpaque = NULL;
	if (_streamInterface.pUniqueName != NULL) {
		delete[] _streamInterface.pUniqueName;
		_streamInterface.pUniqueName = NULL;
	}
}

in_stream_t *InNetExternalStream::GetInterface() {
	return &_streamInterface;
}

StreamCapabilities * InNetExternalStream::GetCapabilities() {
	return &_streamCapabilities;
}

bool InNetExternalStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool InNetExternalStream::SignalPause() {
	return true;
}

bool InNetExternalStream::SignalResume() {
	return true;
}

bool InNetExternalStream::SignalSeek(double &dts) {
	return true;
}

bool InNetExternalStream::SignalStop() {
	return true;
}

bool InNetExternalStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (isAudio) {
		_stats.audio.packetsCount++;
		_stats.audio.bytesCount += dataLength;
	} else {
		SavePts(pts);
		_stats.video.packetsCount++;
		_stats.video.bytesCount += dataLength;
	}

    LinkedListNode<uint32_t, BaseOutStream *> *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()){
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->FeedData(pData, dataLength, 0, dataLength,
				pts, dts, isAudio)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			}
		}
		pCurrent = _outStreams.MoveNext();
	}
	return true;
}

void InNetExternalStream::ReadyForSend() {
	NYI;
}

bool InNetExternalStream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_OUT_NET_RTMP)
            || TAG_KIND_OF(type, ST_OUT_FILE_RTMP)
            || TAG_KIND_OF(type, ST_OUT_NET_RTP)
            || TAG_KIND_OF(type, ST_OUT_NET_TS)
            || TAG_KIND_OF(type, ST_OUT_FILE_HLS)
            || TAG_KIND_OF(type, ST_OUT_FILE_HDS)
            || TAG_KIND_OF(type, ST_OUT_FILE_MSS)
            || TAG_KIND_OF(type, ST_OUT_FILE_DASH)
            || TAG_KIND_OF(type, ST_OUT_FILE_TS)
            || TAG_KIND_OF(type, ST_OUT_FILE_MP4)
            || TAG_KIND_OF(type, ST_OUT_NET_EXT)
            || TAG_KIND_OF(type, ST_OUT_FILE_RTMP_FLV)
			|| TAG_KIND_OF(type, ST_OUT_NET_FMP4)
			|| TAG_KIND_OF(type, ST_OUT_NET_MP4)
			;
}

void InNetExternalStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {
	SignalAttachedOrDetached(pOutStream, true);
}

void InNetExternalStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {
	SignalAttachedOrDetached(pOutStream, false);
}

bool InNetExternalStream::SetupAudioCodecAAC(const uint8_t pCodec[2]) {
	return _streamCapabilities.AddTrackAudioAAC((uint8_t*)pCodec, 2, true, (BaseInStream*)this);
}

bool InNetExternalStream::SetupAudioCodecG711() {
	//return _streamCapabilities.InitAudioG711();
	NYIR;
}

bool InNetExternalStream::SetupVideoCodecH264(const uint8_t *pSPS, uint32_t spsLength,
		const uint8_t *pPPS, uint32_t ppsLength) {
	return _streamCapabilities.AddTrackVideoH264((uint8_t*)pSPS, spsLength, (uint8_t*)pPPS, ppsLength, 90000, -1, -1, false, (BaseInStream*)this);

}

void InNetExternalStream::AddStreamToConnectionInterface() {
	_streamInterface.pConnection->inStreamsCount++;
	in_stream_t **ppTemp = new in_stream_t *[_streamInterface.pConnection->inStreamsCount];
	for (uint32_t i = 0; i < _streamInterface.pConnection->inStreamsCount - 1; i++) {
		ppTemp[i] = _streamInterface.pConnection->ppInStreams[i];
	}
	ppTemp[_streamInterface.pConnection->inStreamsCount - 1] = &_streamInterface;
	if (_streamInterface.pConnection->ppInStreams != NULL) {
		delete[] _streamInterface.pConnection->ppInStreams;
		_streamInterface.pConnection->ppInStreams = NULL;
	}
	_streamInterface.pConnection->ppInStreams = ppTemp;
}

void InNetExternalStream::RemoveStreamFromConnectionInterface() {
	if (_streamInterface.pConnection->inStreamsCount == 0)
		return;
	if (_streamInterface.pConnection->ppInStreams == NULL)
		return;
	bool found = false;
	for (uint32_t i = 0; i < _streamInterface.pConnection->inStreamsCount; i++) {
		if (_streamInterface.pConnection->ppInStreams[i] == &_streamInterface) {
			found = true;
			break;
		}
	}
	if (!found)
		return;
	in_stream_t **ppTemp = NULL;
	if ((_streamInterface.pConnection->inStreamsCount - 1) > 0) {
		ppTemp = new in_stream_t *[_streamInterface.pConnection->inStreamsCount - 1];
	}
	uint32_t cursor = 0;
	for (uint32_t i = 0; i < _streamInterface.pConnection->inStreamsCount; i++) {
		if (_streamInterface.pConnection->ppInStreams[i] == &_streamInterface)
			continue;
		ppTemp[cursor++] = _streamInterface.pConnection->ppInStreams[i];
	}
	if (_streamInterface.pConnection->ppInStreams != NULL) {
		delete[] _streamInterface.pConnection->ppInStreams;
		_streamInterface.pConnection->ppInStreams = NULL;
	}
	_streamInterface.pConnection->ppInStreams = ppTemp;
	_streamInterface.pConnection->inStreamsCount--;
}

void InNetExternalStream::SignalAttachedOrDetached(BaseOutStream *pOutStream, bool attached) {
	if ((pOutStream == NULL)
			|| (!TAG_KIND_OF(pOutStream->GetType(), ST_OUT_NET_RTMP))
			|| (pOutStream->GetProtocol() == NULL)
			|| ((pOutStream->GetProtocol()->GetType() != PT_INBOUND_RTMP)
			&& (pOutStream->GetProtocol()->GetType() != PT_OUTBOUND_RTMP))
			)
		return;
	if (attached)
		_streamInterface.pFactory->events.stream.inStreamAttached(
			&_streamInterface,
			pOutStream->GetUniqueId(),
			pOutStream->GetProtocol()->GetId());
	else
		_streamInterface.pFactory->events.stream.inStreamDetached(
			&_streamInterface,
			pOutStream->GetUniqueId(),
			pOutStream->GetProtocol()->GetId());
}
#endif /* HAS_PROTOCOL_EXTERNAL */

