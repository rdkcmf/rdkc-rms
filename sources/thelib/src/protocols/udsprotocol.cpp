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


#ifdef HAS_UDS

#include "protocols/udsprotocol.h"
#include "netio/netio.h"

UDSProtocol::UDSProtocol() : BaseProtocol(PT_UDS) {
	_decodedBytesCount = 0;
	_pCarrier = NULL;
}

UDSProtocol::~UDSProtocol() {
	if (_pCarrier != NULL) {
		IOHandler *pCarrier = _pCarrier;
		_pCarrier = NULL;
		pCarrier->SetProtocol(NULL);
		delete pCarrier;
	}
}

bool UDSProtocol::Initialize(Variant &parameters) {
	return true;
}

IOHandler *UDSProtocol::GetIOHandler() {
	return _pCarrier;
}

void UDSProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if ((pIOHandler->GetType() != IOHT_UDS_CARRIER)
				&& (pIOHandler->GetType() != IOHT_STDIO)) {
			ASSERT("This protocol accepts only UDS carriers");
		}
	}
	_pCarrier = pIOHandler;
}

bool UDSProtocol::AllowFarProtocol(uint64_t type) {
	WARN("This protocol doesn't accept any far protocol");
	return false;
}

bool UDSProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

IOBuffer * UDSProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool UDSProtocol::SignalInputData(int32_t recvAmount) {
	_decodedBytesCount += recvAmount;
	return _pNearProtocol->SignalInputData(_inputBuffer);
}

bool UDSProtocol::SignalInputData(IOBuffer & /* ignored */) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool UDSProtocol::EnqueueForOutbound() {
	if (_pCarrier == NULL) {
		FATAL("UDSProtocol has no carrier");
		return false;
	}
	return _pCarrier->SignalOutputData();
}

uint64_t UDSProtocol::GetDecodedBytesCount() {
	return _decodedBytesCount;
}



#endif	/* HAS_UDS */
