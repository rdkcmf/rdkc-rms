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



#include "protocols/udpprotocol.h"
#include "netio/netio.h"
#include "utils/misc/socketaddress.h"

UDPProtocol::UDPProtocol()
: BaseProtocol(PT_UDP) {
	_decodedBytesCount = 0;
	_pCarrier = NULL;
}

UDPProtocol::~UDPProtocol() {
	if (_pCarrier != NULL) {
		IOHandler *pCarrier = _pCarrier;
		_pCarrier = NULL;
		pCarrier->SetProtocol(NULL);
		delete pCarrier;
	}
}

bool UDPProtocol::Initialize(Variant &parameters) {
	return true;
}

IOHandler *UDPProtocol::GetIOHandler() {
	return _pCarrier;
}

void UDPProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if (pIOHandler->GetType() != IOHT_UDP_CARRIER) {
			ASSERT("This protocol accepts only UDP carrier");
		}
	}
	_pCarrier = pIOHandler;
}

bool UDPProtocol::AllowFarProtocol(uint64_t type) {
	WARN("This protocol doesn't accept any far protocol");
	return false;
}

bool UDPProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

IOBuffer * UDPProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool UDPProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool UDPProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	_decodedBytesCount += recvAmount;
	bool retValue = false;
	if (_pNearProtocol->GetType() == PT_INBOUND_RTP)
		retValue = _pNearProtocol->SignalInputData(_inputBuffer, recvAmount, pPeerAddress);
	else
		retValue = _pNearProtocol->SignalInputData(_inputBuffer, pPeerAddress);

	return retValue;
}

bool UDPProtocol::SignalInputData(IOBuffer & /* ignored */) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool UDPProtocol::EnqueueForOutbound() {
	if (_pCarrier == NULL) {
		ASSERT("UDPProtocol has no carrier");
		return false;
	}
	return _pCarrier->SignalOutputData();
}

uint64_t UDPProtocol::GetDecodedBytesCount() {
	return _decodedBytesCount;
}
