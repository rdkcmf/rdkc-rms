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



#ifdef HAS_PROTOCOL_RTMP
#include "application/clientapplicationmanager.h"
#include "netio/netio.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "protocols/rtmp/messagefactories/messagefactories.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "streaming/streamstypes.h"
#include "protocols/rtmp/streaming/innetrtmpstream.h"
#include "protocols/rtmp/streaming/infilertmpstream.h"
#include "streaming/streamstypes.h"
#include "protocols/rtmp/rtmpoutputchecks.h"
#include "protocols/rtmp/sharedobjects/so.h"
#include "protocols/rtmp/streaming/rtmpplaylist.h"
#include "protocols/rtmp/streaming/rtmpplaylistitem.h"
#include "protocols/rtmp/rtmpmessagefactory.h"
#include "eventlogger/eventlogger.h"

uint8_t BaseRTMPProtocol::serverKey[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}; // 68

uint8_t BaseRTMPProtocol::playerKey[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}; // 62

BaseRTMPProtocol::StreamContainer::StreamContainer() {
	pStream = NULL;
	pPlaylist = NULL;
	pInputFileStream = NULL;
	clientSideBuffer = 1000;
}

BaseRTMPProtocol::StreamContainer::~StreamContainer() {
	Clear(true);
}

void BaseRTMPProtocol::StreamContainer::Clear(bool clearPlaylist) {
	if (clearPlaylist) {
		if (pPlaylist != NULL) {
			delete pPlaylist;
			pPlaylist = NULL;
		}
	}

	if (pStream != NULL) {
		delete pStream;
		pStream = NULL;
	}

	if (pInputFileStream != NULL) {
		delete pInputFileStream;
		pInputFileStream = NULL;
	}
};

BaseRTMPProtocol::BaseRTMPProtocol(uint64_t protocolType)
: BaseProtocol(protocolType) {
	_decodedBytesSubtractor = 0;
	_handshakeCompleted = false;
	_rtmpState = RTMP_STATE_NOT_INITIALIZED;
	//TODO: Make use of winacksize which is in fact the value setted up for
	//the nex bytes sent report
	_winAckSize = RECEIVED_BYTES_COUNT_REPORT_CHUNK;
	_nextReceivedBytesCountReport = _winAckSize;
	for (uint32_t i = 0; i < MAX_CHANNELS_COUNT; i++) {
		_channels[i].id = i;
		_channels[i].Reset();
	}
	_selectedChannel = -1;
	_inboundChunkSize = 128;
	_outboundChunkSize = 128;

	for (uint32_t i = MIN_AV_CHANNLES; i < MAX_AV_CHANNLES; i++)
		ADD_VECTOR_END(_channelsPool, i);

	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++)
		_streamsMap[i] = 0;

	_rxInvokes = 0;
	_txInvokes = 0;

#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	_pMonitor = new RTMPOutputChecks(MAX_STREAMS_COUNT, MAX_CHANNELS_COUNT);
#endif /* ENFORCE_RTMP_OUTPUT_CHECKS */

	_clientId = format("%s%"PRIu32"%s", STR(generateRandomString(4)), GetId(),
			STR(generateRandomString(4)));
}

BaseRTMPProtocol::~BaseRTMPProtocol() {
	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		_streams[i].Clear(true);
	}

#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	if (_pMonitor != NULL) {
		delete _pMonitor;
		_pMonitor = NULL;
	}
#endif /* ENFORCE_RTMP_OUTPUT_CHECKS */

	FOR_MAP(_sos, string, ClientSO *, i) {
		delete MAP_VAL(i);
	}
	_sos.clear();
}

void BaseRTMPProtocol::SetWitnessFile(string path) {
	path = format("%s.%"PRIu64".%"PRIu32, STR(path), (uint64_t) time(NULL), GetId());
	IOBuffer *pInputBuffer = GetInputBuffer();
	if (pInputBuffer != NULL)
		pInputBuffer->SetInWitnessFile(path);
	_outputBuffer.SetOutWitnessFile(path);
}

string BaseRTMPProtocol::GetClientId() {
	return _clientId;
}

ClientSO *BaseRTMPProtocol::GetSO(string &name) {
	map<string, ClientSO *>::iterator i = _sos.find(name);
	if (i == _sos.end())
		return NULL;
	return MAP_VAL(i);
}

bool BaseRTMPProtocol::CreateSO(string &name) {
	if (GetType() != PT_OUTBOUND_RTMP) {
		FATAL("Incorrect RTMP protocol type for opening SO");
		return false;
	}
	if (GetSO(name) != NULL) {
		FATAL("So already present");
		return false;
	}
	_sos[name] = new ClientSO();
	_sos[name]->name(name);
	_sos[name]->version(1);
	return true;
}

void BaseRTMPProtocol::SignalBeginSOProcess(string &name) {

}

void BaseRTMPProtocol::SignalEndSOProcess(string &name, uint32_t versionNumber) {
	ClientSO *pSO = NULL;
	if (!MAP_HAS1(_sos, name)) {
		//FATAL("Client SO %s not found", STR(name));
		return;
	}
	pSO = _sos[name];
	pSO->version(versionNumber);
	if (pSO->changedProperties().MapSize() == 0)
		return;
	_pProtocolHandler->SignalClientSOUpdated(this, pSO);
	pSO->changedProperties().RemoveAllKeys();
	return;
}

bool BaseRTMPProtocol::ClientSOSend(string &name, Variant &parameters) {
	ClientSO *pSO = NULL;
	if (!MAP_HAS1(_sos, name)) {
		FATAL("Client SO %s not found", STR(name));
		return false;
	}
	pSO = _sos[name];
	Variant message = SOMessageFactory::GetSharedObject(3, 0, 0, false, name,
			pSO->version(), pSO->persistent());
	SOMessageFactory::AddSOPrimitiveSend(message, parameters);
	return SendMessage(message);
}

bool BaseRTMPProtocol::ClientSOSetProperty(string &soName, string &propName,
		Variant &propValue) {
	ClientSO *pSO = NULL;
	if (!MAP_HAS1(_sos, soName)) {
		FATAL("Client SO %s not found", STR(soName));
		return false;
	}
	pSO = _sos[soName];
	Variant message = SOMessageFactory::GetSharedObject(3, 0, 0, false, soName,
			pSO->version(), pSO->persistent());
	SOMessageFactory::AddSOPrimitiveSetProperty(message, propName, propValue);
	if (!SendMessage(message)) {
		FATAL("Unable to set property value");
		return false;
	}
	pSO->changedProperties().PushToArray(propName);
	if ((propValue == V_NULL) || (propValue == V_UNDEFINED))
		pSO->properties().RemoveKey(propName);
	else
		pSO->properties()[propName] = propValue;
	_pProtocolHandler->SignalClientSOUpdated(this, pSO);
	pSO->changedProperties().RemoveAllKeys();
	return true;
}

void BaseRTMPProtocol::SignalOutBufferFull(uint32_t outstanding, uint32_t maxValue) {
	_pProtocolHandler->SignalOutBufferFull(this, outstanding, maxValue);
}

bool BaseRTMPProtocol::HandleSOPrimitive(string &name, Variant &primitive) {
	ClientSO *pSO = NULL;
	if (!MAP_HAS1(_sos, name)) {
		FATAL("Client SO %s not found", STR(name));
		return false;
	}
	pSO = _sos[name];
	switch ((uint8_t) primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE]) {
		case SOT_CS_UPDATE_FIELD:
		case SOT_SC_INITIAL_DATA:
		{

			FOR_MAP(primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD], string, Variant, i) {
				pSO->properties()[MAP_KEY(i)] = MAP_VAL(i);
				pSO->changedProperties().PushToArray(MAP_KEY(i));
			}
			if ((uint8_t) primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE] == SOT_SC_INITIAL_DATA) {
				_pProtocolHandler->SignalClientSOConnected(this, pSO);
			}
			return true;
		}
		case SOT_SC_CLEAR_DATA:
		{

			FOR_MAP(pSO->properties(), string, Variant, i) {
				pSO->changedProperties().PushToArray(MAP_KEY(i));
			}
			pSO->properties().RemoveAllKeys();
			return true;
		}
		case SOT_SC_DELETE_FIELD:
		{

			FOR_MAP(primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD], string, Variant, i) {
				pSO->properties().RemoveKey((string) MAP_VAL(i));
				pSO->changedProperties().PushToArray(MAP_VAL(i));
			}
			return true;
		}
		case SOT_BW_SEND_MESSAGE:
		{
			_pProtocolHandler->SignalClientSOSend(this, pSO,
					primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD]);
			return true;
		}
		case SOT_CS_UPDATE_FIELD_ACK:
		{
			return true;
		}
		default:
		{
			FATAL("Primitive not supported\n%s", STR(primitive.ToString()));
			return false;
		}
	}
}

bool BaseRTMPProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool BaseRTMPProtocol::AllowFarProtocol(uint64_t type) {
	if (type == PT_TCP
			|| type == PT_RTMPE
			|| type == PT_INBOUND_SSL
			|| type == PT_OUTBOUND_SSL
			|| type == PT_INBOUND_HTTP_FOR_RTMP)
		return true;
	return false;
}

bool BaseRTMPProtocol::AllowNearProtocol(uint64_t type) {
	FATAL("This protocol doesn't allow any near protocols");
	return false;
}

IOBuffer * BaseRTMPProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;
	return NULL;
}

bool BaseRTMPProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool BaseRTMPProtocol::SignalInputData(IOBuffer &buffer) {
	if (_enqueueForDelete)
		return true;

	bool result = false;
	if (_handshakeCompleted) {
		result = ProcessBytes(buffer);
		uint64_t decodedBytes = GetDecodedBytesCount();
		uint64_t toProcessBytes = decodedBytes - _decodedBytesSubtractor;
		if (toProcessBytes >= 0xf0000000) {
			_decodedBytesSubtractor += decodedBytes;
			decodedBytes = 0;
			toProcessBytes = 0;
            _nextReceivedBytesCountReport = 0;
		}
		if (result && (toProcessBytes >= _nextReceivedBytesCountReport)) {
			Variant _bytesReadMessage = GenericMessageFactory::GetAck(toProcessBytes);
			_nextReceivedBytesCountReport += _winAckSize;
			if (!SendMessage(_bytesReadMessage)) {
				FATAL("Unable to send\n%s", STR(_bytesReadMessage.ToString()));
				return false;
			}
		}
	} else {
		result = PerformHandshake(buffer);
		if (!result) {
			FATAL("Unable to perform handshake");
			return false;
		}
		if (_handshakeCompleted) {
			result = SignalInputData(buffer);
			if (result && (GetType() == PT_OUTBOUND_RTMP)) {
				result = _pProtocolHandler->OutboundConnectionEstablished(
						(OutboundRTMPProtocol *) this);
			}
		}
	}
	return result;
}

bool BaseRTMPProtocol::TimePeriodElapsed() {
	ASSERT("Operation not supported");
	return false;
}

void BaseRTMPProtocol::ReadyForSend() {
	for (size_t i = 0; i < _signaledIFS.size(); i++) {
		if (_streams[_signaledIFS[i]].pInputFileStream == NULL) {
			_signaledIFS.erase(_signaledIFS.begin() + i);
			ReadyForSend();
			return;
		}
		_streams[_signaledIFS[i]].pInputFileStream->ReadyForSend();
	}
}

void BaseRTMPProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pProtocolHandler = (BaseRTMPAppProtocolHandler *)
				pApplication->GetProtocolHandler(this);
	} else {
		_pProtocolHandler = NULL;
	}
}

void BaseRTMPProtocol::GetStats(Variant &info, uint32_t namespaceId) {
	BaseProtocol::GetStats(info, namespaceId);
	info["rxInvokes"] = _rxInvokes;
	info["txInvokes"] = _txInvokes;
	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		if (_streams[i].pStream != NULL) {
			Variant si;
			_streams[i].pStream->GetStats(si, namespaceId);
			info["streams"].PushToArray(si);
		}
	}
}

bool BaseRTMPProtocol::ResetChannel(uint32_t channelId) {
	if (channelId >= MAX_CHANNELS_COUNT) {
		FATAL("Invalid channel id in reset message: %"PRIu32, channelId);
		return false;
	}
	_channels[channelId].Reset();
	return true;
}

bool BaseRTMPProtocol::SendMessage(Variant & message) {
	if (IsEnqueueForDelete()) {
		return true;
	}
#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	_intermediateBuffer.IgnoreAll();
	if (!_rtmpProtocolSerializer.Serialize(_channels[(uint32_t) VH_CI(message)],
			message, _intermediateBuffer, _outboundChunkSize)) {
		FATAL("Unable to serialize RTMP message");
		return false;
	}
	if (!_pMonitor->Feed(_intermediateBuffer)) {
		ASSERT("Server sent invalid data");
	}
	_outputBuffer.ReadFromBuffer(
			GETIBPOINTER(_intermediateBuffer),
			GETAVAILABLEBYTESCOUNT(_intermediateBuffer));
#else  /* ENFORCE_RTMP_OUTPUT_CHECKS */
	//2. Send the message
	if (!_rtmpProtocolSerializer.Serialize(_channels[(uint32_t) VH_CI(message)],
			message, _outputBuffer, _outboundChunkSize)) {
		FATAL("Unable to serialize RTMP message");
		return false;
	}
#endif  /* ENFORCE_RTMP_OUTPUT_CHECKS */

	_txInvokes++;

	//3. Mark the connection as ready for outbound transfer
	return EnqueueForOutbound();
}

bool BaseRTMPProtocol::SendRawData(Header &header, Channel &channel, uint8_t *pData,
		uint32_t length) {
#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	_intermediateBuffer.IgnoreAll();
	if (!header.Write(channel, _intermediateBuffer)) {
		FATAL("Unable to serialize message header");
		return false;
	}

	_intermediateBuffer.ReadFromBuffer(pData, length);
	if (!_pMonitor->Feed(_intermediateBuffer)) {
		ASSERT("Server sent invalid data");
	}
	_outputBuffer.ReadFromBuffer(
			GETIBPOINTER(_intermediateBuffer),
			GETAVAILABLEBYTESCOUNT(_intermediateBuffer));
#else /* ENFORCE_RTMP_OUTPUT_CHECKS */
	if (!header.Write(channel, _outputBuffer)) {
		FATAL("Unable to serialize message header");
		return false;
	}

	_outputBuffer.ReadFromBuffer(pData, length);
#endif /* ENFORCE_RTMP_OUTPUT_CHECKS */
	return EnqueueForOutbound();
}

bool BaseRTMPProtocol::SendRawData(uint8_t *pData, uint32_t length) {
#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	_intermediateBuffer.IgnoreAll();
	_intermediateBuffer.ReadFromBuffer(pData, length);
	if (!_pMonitor->Feed(_intermediateBuffer)) {
		ASSERT("Server sent invalid data");
	}
	_outputBuffer.ReadFromBuffer(
			GETIBPOINTER(_intermediateBuffer),
			GETAVAILABLEBYTESCOUNT(_intermediateBuffer));
#else /* ENFORCE_RTMP_OUTPUT_CHECKS */
	_outputBuffer.ReadFromBuffer(pData, length);
#endif /* ENFORCE_RTMP_OUTPUT_CHECKS */
	return EnqueueForOutbound();
}

void BaseRTMPProtocol::SetWinAckSize(uint32_t winAckSize) {
	_nextReceivedBytesCountReport -= _winAckSize;
	_winAckSize = winAckSize;
	_nextReceivedBytesCountReport += _winAckSize;
}

uint32_t BaseRTMPProtocol::GetOutboundChunkSize() {
	return _outboundChunkSize;
}

bool BaseRTMPProtocol::SetInboundChunkSize(uint32_t chunkSize) {
	/*WARN("Chunk size changed for RTMP connection %p: %u->%u", this,
			_inboundChunkSize, chunkSize);*/
	_inboundChunkSize = chunkSize;
	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		if ((_streams[i].pStream != NULL)&&(TAG_KIND_OF(_streams[i].pStream->GetType(), ST_IN_NET_RTMP)))
			((InNetRTMPStream *) _streams[i].pStream)->SetChunkSize(_inboundChunkSize);
	}
	return true;
}

void BaseRTMPProtocol::TrySetOutboundChunkSize(uint32_t chunkSize) {
	if (_outboundChunkSize >= chunkSize) {
		uint32_t outStreamsSize = 0;
		for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
			if ((_streams[i].pStream != NULL) &&(TAG_KIND_OF(_streams[i].pStream->GetType(), ST_OUT_NET_RTMP)))
				outStreamsSize++;
		}
		if (outStreamsSize > 1)
			return;
	}
	_outboundChunkSize = chunkSize;
	Variant chunkSizeMessage = GenericMessageFactory::GetChunkSize(_outboundChunkSize);
	SendMessage(chunkSizeMessage);
	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		if ((_streams[i].pStream != NULL) &&(TAG_KIND_OF(_streams[i].pStream->GetType(), ST_OUT_NET_RTMP)))
			((OutNetRTMPStream *) _streams[i].pStream)->SetChunkSize(_outboundChunkSize);
	}
}

RTMPPlaylist *BaseRTMPProtocol::GetPlaylist(uint32_t rtmpStreamId) {
	if ((rtmpStreamId == 0)
			|| (rtmpStreamId >= MAX_STREAMS_COUNT)
			|| (_streamsMap[rtmpStreamId] != rtmpStreamId)
			)
		return NULL;

	if (_streams[rtmpStreamId].pPlaylist == NULL)
		_streams[rtmpStreamId].pPlaylist = new RTMPPlaylist(GetId(), rtmpStreamId);

	return _streams[rtmpStreamId].pPlaylist;
}

bool BaseRTMPProtocol::HasRTMPStream(uint32_t rtmpStreamId) {
	return (rtmpStreamId != 0)
			|| (rtmpStreamId < MAX_STREAMS_COUNT)
			|| (_streamsMap[rtmpStreamId] == rtmpStreamId);
}

BaseStream * BaseRTMPProtocol::GetRTMPStream(uint32_t rtmpStreamId) {
	if (!HasRTMPStream(rtmpStreamId))
		return NULL;
	return _streams[rtmpStreamId].pStream;
}

bool BaseRTMPProtocol::SetClientSideBuffer(uint32_t rtmpStreamId, double value) {
	if (rtmpStreamId >= MAX_STREAMS_COUNT)
		return false;

	if (rtmpStreamId != 0) {
		if (!HasRTMPStream(rtmpStreamId))
			return false;

		_streams[rtmpStreamId].clientSideBuffer = value;
		if (_streams[rtmpStreamId].pInputFileStream != NULL)
			_streams[rtmpStreamId].pInputFileStream->SetClientSideBuffer(
				_streams[rtmpStreamId].clientSideBuffer, false);
	} else {
		for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
			_streams[i].clientSideBuffer = value;
			if (_streams[i].pInputFileStream != NULL)
				_streams[i].pInputFileStream->SetClientSideBuffer(
					_streams[i].clientSideBuffer, false);
		}
	}

	return true;
}

bool BaseRTMPProtocol::CloseStream(uint32_t rtmpStreamId, bool createNeutralStream) {
	//1. Validate request
	if (rtmpStreamId == 0 || rtmpStreamId >= MAX_STREAMS_COUNT) {
		FATAL("Invalid stream id: %"PRIu32, rtmpStreamId);
		return false;
	}

	//4. Delete the stream and replace it with a neutral one
	_streams[rtmpStreamId].Clear(false);
	_streamsMap[rtmpStreamId] = 0;
	if (createNeutralStream) {
		if (!CreateNeutralStream(rtmpStreamId))
			return false;
	}

	return true;
}

bool BaseRTMPProtocol::CreateNeutralStream(uint32_t & rtmpStreamId) {
	if (rtmpStreamId == 0) {
		//Automatic allocation
		for (uint32_t i = 1; i < MAX_STREAMS_COUNT; i++) {
			if (_streamsMap[i] == 0) {
				rtmpStreamId = i;
				break;
			}
		}

		if (rtmpStreamId == 0) {
			return false;
		}
	}

	if (rtmpStreamId == 0 || rtmpStreamId >= MAX_STREAMS_COUNT)
		return false;

	if (_streamsMap[rtmpStreamId] != 0)
		return false;

	_streamsMap[rtmpStreamId] = rtmpStreamId;

	return true;
}

InNetRTMPStream * BaseRTMPProtocol::CreateINS(uint32_t channelId,
		uint32_t rtmpStreamId, string publicStreamName, string privateStreamName) {
	if (!HasRTMPStream(rtmpStreamId))
		return NULL;

	if (_streams[rtmpStreamId].pStream != NULL)
		return NULL;

	_streams[rtmpStreamId].Clear(true);
	_streams[rtmpStreamId].pStream = new InNetRTMPStream(this, publicStreamName,
			rtmpStreamId, _inboundChunkSize, channelId, privateStreamName);
	if (!_streams[rtmpStreamId].pStream->SetStreamsManager(
			GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		_streams[rtmpStreamId].Clear(true);
		return NULL;
	}

	return (InNetRTMPStream *) _streams[rtmpStreamId].pStream;
}

OutNetRTMPStream * BaseRTMPProtocol::CreateONS(
		RTMPPlaylistItem *pPlaylistItem, uint32_t rtmpStreamId, string streamName) {
	if (!HasRTMPStream(rtmpStreamId))
		return NULL;

	if (_streams[rtmpStreamId].pStream != NULL)
		return NULL;

	_streams[rtmpStreamId].Clear(false);
	OutNetRTMPStream *pBaseOutNetRTMPStream = OutNetRTMPStream::GetInstance(
			this, pPlaylistItem, GetApplication()->GetStreamsManager(),
			streamName, rtmpStreamId, _outboundChunkSize);
	if (pBaseOutNetRTMPStream == NULL)
		return NULL;

	_streams[rtmpStreamId].pStream = pBaseOutNetRTMPStream;

	return pBaseOutNetRTMPStream;
}

InFileRTMPStream * BaseRTMPProtocol::CreateIFS(uint32_t rtmpStreamId,
		Metadata &metadata) {
	if (!HasRTMPStream(rtmpStreamId))
		return NULL;

	if (_streams[rtmpStreamId].pInputFileStream != NULL) {
		delete _streams[rtmpStreamId].pInputFileStream;
		_streams[rtmpStreamId].pInputFileStream = NULL;
	}

	InFileRTMPStream *pRTMPInFileStream = InFileRTMPStream::GetInstance(
			this, GetApplication()->GetStreamsManager(), metadata);
	if (pRTMPInFileStream == NULL)
		return NULL;

	if (!pRTMPInFileStream->Initialize(metadata)) {
		delete pRTMPInFileStream;
		return NULL;
	}

	if (GetApplication()->GetStreamMetadataResolver()->HasTimers())
		pRTMPInFileStream->SetClientSideBuffer(_streams[rtmpStreamId].clientSideBuffer, false);

	_streams[rtmpStreamId].pInputFileStream = pRTMPInFileStream;
	return pRTMPInFileStream;
}

bool BaseRTMPProtocol::SignalReadyForSendOnIFS(uint32_t rtmpStreamId) {
	if (!HasRTMPStream(rtmpStreamId))
		return false;

	for (size_t i = 0; i < _signaledIFS.size(); i++) {
		if (_signaledIFS[i] == rtmpStreamId)
			return true;
	}

	_signaledIFS.push_back(rtmpStreamId);
	return true;
}

Channel *BaseRTMPProtocol::ReserveChannel() {
	if (_channelsPool.size() == 0)
		return 0;

	uint32_t result = 0;
	result = _channelsPool[0];
	_channelsPool.erase(_channelsPool.begin());

	return &_channels[result];
}

void BaseRTMPProtocol::ReleaseChannel(Channel *pChannel) {
	if (pChannel == NULL)
		return;
	if (pChannel->id <= 63)
		ADD_VECTOR_BEGIN(_channelsPool, pChannel->id);
	else
		ADD_VECTOR_END(_channelsPool, pChannel->id);
}

bool BaseRTMPProtocol::HasStreamsOfType(uint64_t kind) {
	for (uint32_t i = 0; i < MAX_STREAMS_COUNT; i++) {
		if (_streams[i].pStream != NULL && TAG_KIND_OF(_streams[i].pStream->GetType(), kind))
			return true;
	}
	return false;
}

bool BaseRTMPProtocol::EnqueueForTimeEvent(uint32_t seconds) {
	ASSERT("Operation not supported. Please use a timer protocol");
	return false;
}

uint32_t BaseRTMPProtocol::GetDHOffset(uint8_t *pBuffer, uint8_t schemeNumber) {
	switch (schemeNumber) {
		case 0:
		{
			return GetDHOffset0(pBuffer);
		}
		case 1:
		{
			return GetDHOffset1(pBuffer);
		}
		default:
		{
			WARN("Invalid scheme number: %hhu. Defaulting to 0", schemeNumber);
			return GetDHOffset0(pBuffer);
		}
	}
}

uint32_t BaseRTMPProtocol::GetDigestOffset(uint8_t *pBuffer, uint8_t schemeNumber) {
	switch (schemeNumber) {
		case 0:
		{
			return GetDigestOffset0(pBuffer);
		}
		case 1:
		{
			return GetDigestOffset1(pBuffer);
		}
		default:
		{
			WARN("Invalid scheme number: %hhu. Defaulting to 0", schemeNumber);
			return GetDigestOffset0(pBuffer);
		}
	}
}

uint32_t BaseRTMPProtocol::GetDHOffset0(uint8_t *pBuffer) {
	uint32_t offset = pBuffer[1532] + pBuffer[1533] + pBuffer[1534] + pBuffer[1535];
	offset = offset % 632;
	offset = offset + 772;
	if (offset + 128 >= 1536) {
		ASSERT("Invalid DH offset");
	}
	return offset;
}

uint32_t BaseRTMPProtocol::GetDHOffset1(uint8_t *pBuffer) {
	uint32_t offset = pBuffer[768] + pBuffer[769] + pBuffer[770] + pBuffer[771];
	offset = offset % 632;
	offset = offset + 8;
	if (offset + 128 >= 1536) {
		ASSERT("Invalid DH offset");
	}
	return offset;
}

uint32_t BaseRTMPProtocol::GetDigestOffset0(uint8_t *pBuffer) {
	uint32_t offset = pBuffer[8] + pBuffer[9] + pBuffer[10] + pBuffer[11];
	offset = offset % 728;
	offset = offset + 12;
	if (offset + 32 >= 1536) {
		ASSERT("Invalid digest offset");
	}
	return offset;
}

uint32_t BaseRTMPProtocol::GetDigestOffset1(uint8_t *pBuffer) {
	uint32_t offset = pBuffer[772] + pBuffer[773] + pBuffer[774] + pBuffer[775];
	offset = offset % 728;
	offset = offset + 776;
	if (offset + 32 >= 1536) {
		ASSERT("Invalid digest offset");
	}
	return offset;
}

bool BaseRTMPProtocol::ProcessBytes(IOBuffer &buffer) {
	while (true) {
		uint32_t availableBytesCount = GETAVAILABLEBYTESCOUNT(buffer);
		if (_selectedChannel < 0) {
			if (availableBytesCount < 1) {
				return true;
			} else {
				switch (GETIBPOINTER(buffer)[0]&0x3f) {
					case 0:
					{
						if (availableBytesCount < 2) {
							FINEST("Not enough data");
							return true;
						}
						_selectedChannel = 64 + GETIBPOINTER(buffer)[1];
						_channels[_selectedChannel].lastInHeaderType = GETIBPOINTER(buffer)[0] >> 6;
						buffer.Ignore(2);
						availableBytesCount -= 2;
						break;
					}
					case 1:
					{
						//						if (availableBytesCount < 3) {
						//							FINEST("Not enough data");
						//							return true;
						//						}
						//						_selectedChannel = GETIBPOINTER(buffer)[2]*256 + GETIBPOINTER(buffer)[1] + 64;
						//						_channels[_selectedChannel].lastInHeaderType = GETIBPOINTER(buffer)[0] >> 6;
						//						buffer.Ignore(3);
						//						availableBytesCount -= 3;
						//						break;
						FATAL("The server doesn't support channel ids bigger than 319");
						return false;
					};
					default:
					{
						_selectedChannel = GETIBPOINTER(buffer)[0]&0x3f;
						_channels[_selectedChannel].lastInHeaderType = GETIBPOINTER(buffer)[0] >> 6;
						buffer.Ignore(1);
						availableBytesCount -= 1;
						break;
					}
				}
			}
		}

		if (_selectedChannel >= MAX_CHANNELS_COUNT) {
			FATAL("Bogus connection. Drop it like is hot");
			return false;
		}

		Channel &channel = _channels[_selectedChannel];
		Header &header = channel.lastInHeader;

		if (channel.state == CS_HEADER) {
			if (!header.Read(_selectedChannel, channel.lastInHeaderType,
					buffer, availableBytesCount)) {
				FATAL("Unable to read header");
				return false;
			} else {
				if (!header.readCompleted)
					return true;
				channel.state = CS_PAYLOAD;
				switch (channel.lastInHeaderType) {
					case HT_FULL:
					{
						channel.lastInAbsTs = H_TS(header);
						break;
					}
					case HT_SAME_STREAM:
					case HT_SAME_LENGTH_AND_STREAM:
					{
						channel.lastInAbsTs += H_TS(header);
						break;
					}
					case HT_CONTINUATION:
					{
						if (channel.lastInProcBytes == 0) {
							channel.lastInAbsTs += H_TS(header);
						}
						break;
					}
				}
			}
		}

		if (channel.state == CS_PAYLOAD) {
			uint32_t tempSize = H_ML(header) - channel.lastInProcBytes;
			tempSize = (tempSize >= _inboundChunkSize) ? _inboundChunkSize : tempSize;
			uint32_t availableBytes = GETAVAILABLEBYTESCOUNT(buffer);
			if (tempSize > availableBytes)
				return true;
			channel.state = CS_HEADER;
			_selectedChannel = -1;
			switch (H_MT(header)) {
				case RM_HEADER_MESSAGETYPE_VIDEODATA:
				{
					if (H_SI(header) >= MAX_STREAMS_COUNT) {
						FATAL("The server doesn't support stream ids bigger than %"PRIu32,
								(uint32_t) MAX_STREAMS_COUNT);
						return false;
					}
					if ((_streams[H_SI(header)].pStream != NULL)
							&& (_streams[H_SI(header)].pStream->GetType() == ST_IN_NET_RTMP)) {
						if (!((InNetRTMPStream *) _streams[H_SI(header)].pStream)->FeedData(
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								false //isAudio
								)) {
							FATAL("Unable to feed video");
							return false;
						}
					} else {
						if (!_pProtocolHandler->FeedAVData(this,
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								false, //isAudio
								H_IA(header) //isAbsolute
								)) {
							FATAL("Unable to feed video");
							return false;
						}
					}

					channel.lastInProcBytes += tempSize;
					if (H_ML(header) == channel.lastInProcBytes) {
						channel.lastInProcBytes = 0;
					}
					if (!buffer.Ignore(tempSize)) {
						FATAL("V: Unable to ignore %u bytes", tempSize);
						return false;
					}
					break;
				}
				case RM_HEADER_MESSAGETYPE_AUDIODATA:
				{
					if (H_SI(header) >= MAX_STREAMS_COUNT) {
						FATAL("The server doesn't support stream ids bigger than %"PRIu32,
								(uint32_t) MAX_STREAMS_COUNT);
						return false;
					}
					if ((_streams[H_SI(header)].pStream != NULL)
							&& (_streams[H_SI(header)].pStream->GetType() == ST_IN_NET_RTMP)) {
						if (!((InNetRTMPStream *) _streams[H_SI(header)].pStream)->FeedData(
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								true //isAudio
								)) {
							FATAL("Unable to feed audio");
							return false;
						}
					} else {
						if (!_pProtocolHandler->FeedAVData(this,
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								true, //isAudio
								H_IA(header) //isAbsolute
								)) {
							FATAL("Unable to feed audio");
							return false;
						}
					}


					channel.lastInProcBytes += tempSize;
					if (H_ML(header) == channel.lastInProcBytes) {
						channel.lastInProcBytes = 0;
					}
					if (!buffer.Ignore(tempSize)) {
						FATAL("A: Unable to ignore %u bytes", tempSize);
						return false;
					}
					break;
				}
				case RM_HEADER_MESSAGETYPE_AGGREGATE:
				{
					if (H_SI(header) >= MAX_STREAMS_COUNT) {
						FATAL("The server doesn't support stream ids bigger than %"PRIu32,
								(uint32_t) MAX_STREAMS_COUNT);
						return false;
					}
					if ((_streams[H_SI(header)].pStream != NULL)
							&& (_streams[H_SI(header)].pStream->GetType() == ST_IN_NET_RTMP)) {
						if (!((InNetRTMPStream *) _streams[H_SI(header)].pStream)->FeedDataAggregate(
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								true //isAudio
								)) {
							FATAL("Unable to feed aggregate A/V");
							return false;
						}
					} else {
						if (!_pProtocolHandler->FeedAVDataAggregate(this,
								GETIBPOINTER(buffer), //pData,
								tempSize, //dataLength,
								channel.lastInProcBytes, //processedLength,
								H_ML(header), //totalLength,
								channel.lastInAbsTs, //pts,
								channel.lastInAbsTs, //dts,
								H_IA(header) //isAbsolute
								)) {
							FATAL("Unable to feed aggregate A/V");
							return false;
						}
					}


					channel.lastInProcBytes += tempSize;
					if (H_ML(header) == channel.lastInProcBytes) {
						channel.lastInProcBytes = 0;
					}
					if (!buffer.Ignore(tempSize)) {
						FATAL("A: Unable to ignore %u bytes", tempSize);
						return false;
					}
					break;
				}
				default:
				{
					channel.inputData.ReadFromInputBuffer(buffer, tempSize);
					channel.lastInProcBytes += tempSize;
					if (!buffer.Ignore(tempSize)) {
						FATAL("Unable to ignore %u bytes", tempSize);
						return false;
					}
					if (H_ML(header) == channel.lastInProcBytes) {
						channel.lastInProcBytes = 0;
						if (_pProtocolHandler == NULL) {
							FATAL("RTMP connection no longer associated with an application");
							return false;
						}
						if (!_pProtocolHandler->InboundMessageAvailable(this,
								channel, header)) {
							FATAL("Unable to send rtmp message to application");
							return false;
						}
//						WARN("[Debug] Invoke check");
						_rxInvokes++;

						if (GETAVAILABLEBYTESCOUNT(channel.inputData) != 0) {
							FATAL("Invalid message! We have leftovers: %u bytes",
									GETAVAILABLEBYTESCOUNT(channel.inputData));
							return false;
						}
					}
					break;
				}
			}
		}
	}
}
void BaseRTMPProtocol::SignalPlaylistItemStart(uint32_t index, uint32_t itemCount, string playlistName) {
	Variant playlistEvent;
	playlistEvent["index"] = index;
	playlistEvent["itemCount"] = itemCount;
	playlistEvent["playlistName"] = playlistName;
	GetEventLogger()->LogRTMPPlaylistItemStart(playlistEvent);
}
#endif /* HAS_PROTOCOL_RTMP */
