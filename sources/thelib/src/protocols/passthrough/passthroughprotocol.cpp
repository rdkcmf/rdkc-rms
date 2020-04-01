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


#include "protocols/passthrough/passthroughprotocol.h"
#include "protocols/passthrough/streaming/innetpassthroughstream.h"
#include "protocols/passthrough/streaming/outnetpassthroughstream.h"
#include "application/baseclientapplication.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "utils/misc/socketaddress.h"

PassThroughProtocol::PassThroughProtocol()
: BaseProtocol(PT_PASSTHROUGH) {
	_pInStream = NULL;
	_pOutStream = NULL;
	_pDummyStream = NULL;
}

PassThroughProtocol::~PassThroughProtocol() {
	CloseStream();
}

IOBuffer * PassThroughProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;
	return NULL;
}

void PassThroughProtocol::GetStats(Variant &info, uint32_t namespaceId) {
	BaseProtocol::GetStats(info, namespaceId);
	if (_pInStream != NULL) {
		Variant si;
		_pInStream->GetStats(si, namespaceId);
		info["streams"].PushToArray(si);
	}
	if (_pOutStream != NULL) {
		Variant si;
		_pOutStream->GetStats(si, namespaceId);
		info["streams"].PushToArray(si);
	}
	if (_pDummyStream != NULL) {
		Variant si;
		_pDummyStream->GetStats(si, namespaceId);
		info["streams"].PushToArray(si);
	}
}

void PassThroughProtocol::SetDummyStream(BaseStream *pDummyStream) {
	_pDummyStream = pDummyStream;
}

bool PassThroughProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

void PassThroughProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (GetIOHandler() != NULL) {
		if (pApplication != NULL) {
			string name = (string)GetCustomParameters()["customParameters"]["externalStreamConfig"]["localStreamName"];
			string tsName = name + "_ts";
			if ((!pApplication->StreamNameAvailable(name, this, true))
					|| (!pApplication->StreamNameAvailable(tsName, this, true))) {
				FATAL("Stream name %s and %s already occupied",
						STR(name), STR(tsName));
				EnqueueForDelete();
				return;
			}
			_pInStream = new InNetPassThroughStream(this, tsName);
			if (!_pInStream->SetStreamsManager(pApplication->GetStreamsManager())) {
				FATAL("Unable to set the streams manager");
				delete _pInStream;
				_pInStream = NULL;
				return;
			}
			if (((string)GetCustomParameters()["customParameters"]["externalStreamConfig"]["uri"]["scheme"])[0] == 'd'
				|| (string)GetCustomParameters()["customParameters"]["externalStreamConfig"]["uri"]["scheme"] == "rtp"
				|| (string)GetCustomParameters()["customParameters"]["externalStreamConfig"]["uri"]["scheme"] == "rtsp") {
				InboundTSProtocol *pTSProtocol = new InboundTSProtocol();
				if (!pTSProtocol->Initialize(GetCustomParameters())) {
					FATAL("Unable to initialize deep parse TS protocol");
					return;
				}
				pTSProtocol->SetFarProtocol(this);
				this->SetNearProtocol(pTSProtocol);
				pTSProtocol->SetApplication(pApplication);
				//				FINEST("pTSProtocol: %s", STR(*pTSProtocol));
				//				FINEST("this: %s", STR(*this));
			}
		} else {
			CloseStream();
		}
	}
}

bool PassThroughProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_TCP || type == PT_UDP
			|| type == PT_INBOUND_RTP;
}

bool PassThroughProtocol::AllowNearProtocol(uint64_t type) {
	return type == PT_INBOUND_TS || type == PT_ICE_TCP;
}

bool PassThroughProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}

bool PassThroughProtocol::SignalInputData(IOBuffer &buffer) {
	if (_pInStream != NULL) {
		if (!_pInStream->FeedData(
				GETIBPOINTER(buffer), //pData
				GETAVAILABLEBYTESCOUNT(buffer), //dataLength
				0, //processedLength
				GETAVAILABLEBYTESCOUNT(buffer), //totalLength
				0, //pts
				0, //dts
				false //isAudio
				)) {
			FATAL("Unable to feed stream");
			return false;
		}
	}
	if (_pNearProtocol != NULL) {
		//deep parse
		_inputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
		if (!_pNearProtocol->SignalInputData(_inputBuffer)) {
			FATAL("Unable to call TS deep parser");
			return false;
		}
	}
	return buffer.IgnoreAll();
}

bool PassThroughProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	return SignalInputData(buffer);
}

void PassThroughProtocol::SetOutStream(OutNetPassThroughStream *pOutStream) {
	if (_pOutStream != NULL) {
		delete _pOutStream;
		_pOutStream = NULL;
	}
	_pOutStream = pOutStream;
}

bool PassThroughProtocol::RegisterOutboundUDP(Variant streamConfig) {
	if (_pInStream == NULL) {
		FATAL("In stream not yet available");
		return false;
	}
	return _pInStream->RegisterOutboundUDP(GetApplication(), streamConfig);
}

bool PassThroughProtocol::SendTCPData(string &data) {
	_outputBuffer.ReadFromString(data);
	return EnqueueForOutbound();
}

bool PassThroughProtocol::SendTCPData(const uint8_t *pBuffer, const uint32_t size) {
	_outputBuffer.ReadFromBuffer(pBuffer, size);
	return EnqueueForOutbound();
}

void PassThroughProtocol::CloseStream() {
	if (_pInStream != NULL) {
		delete _pInStream;
		_pInStream = NULL;
	}

	if (_pOutStream != NULL) {
		delete _pOutStream;
		_pOutStream = NULL;
	}

	if (_pDummyStream != NULL) {
		delete _pDummyStream;
		_pDummyStream = NULL;
	}
}
