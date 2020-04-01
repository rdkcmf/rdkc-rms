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



#include "protocols/tcpprotocol.h"
#include "netio/netio.h"

TCPProtocol::TCPProtocol()
: BaseProtocol(PT_TCP) {
	_decodedBytesCount = 0;
	_pCarrier = NULL;
}

TCPProtocol::~TCPProtocol() {
	if (_pCarrier != NULL) {
		IOHandler *pCarrier = _pCarrier;
		_pCarrier = NULL;
		pCarrier->SetProtocol(NULL);
		delete pCarrier;
	}
}

bool TCPProtocol::Initialize(Variant &parameters) {
	//INFO("$b2$: TCPProtocol::Initialize: %s", STR(*this));
	return true;
}

IOHandler *TCPProtocol::GetIOHandler() {
	return _pCarrier;
}

void TCPProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if ((pIOHandler->GetType() != IOHT_TCP_CARRIER)
				&& (pIOHandler->GetType() != IOHT_STDIO)) {
			ASSERT("This protocol accepts only TCP carriers");
		}
	}
	_pCarrier = pIOHandler;
}

bool TCPProtocol::AllowFarProtocol(uint64_t type) {
	WARN("This protocol doesn't accept any far protocol");
	return false;
}

bool TCPProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

IOBuffer * TCPProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool TCPProtocol::SignalInputData(int32_t recvAmount) {
	_decodedBytesCount += recvAmount;
	return _pNearProtocol->SignalInputData(_inputBuffer);
}

bool TCPProtocol::SignalInputData(IOBuffer & /* ignored */) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool TCPProtocol::EnqueueForOutbound() {
	if (_pCarrier == NULL) {
		FATAL("TCPProtocol has no carrier: [%s]", STR(*this));
		return false;
	}
	return _pCarrier->SignalOutputData();
}

uint64_t TCPProtocol::GetDecodedBytesCount() {
	return _decodedBytesCount;
}

