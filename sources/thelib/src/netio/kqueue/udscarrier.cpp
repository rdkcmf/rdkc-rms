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
#ifdef NET_KQUEUE
#include "netio/kqueue/udscarrier.h"
#include "netio/kqueue/iohandlermanager.h"
#include "protocols/baseprotocol.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RAW_HEADER 0xbeef
#define HEADER_SIZE 6

#define ENABLE_WRITE_DATA \
if (!_writeDataEnabled) { \
    _writeDataEnabled = true; \
    IOHandlerManager::EnableWriteData(this); \
} \
_enableWriteDataCalled=true;

#define DISABLE_WRITE_DATA \
if (_writeDataEnabled) { \
	_enableWriteDataCalled=false; \
	_pProtocol->ReadyForSend(); \
	if(!_enableWriteDataCalled) { \
		if(_pProtocol->GetOutputBuffer()==NULL) {\
			_writeDataEnabled = false; \
			IOHandlerManager::DisableWriteData(this); \
		} \
	} \
}

UDSCarrier::UDSCarrier(int32_t fd)
		: IOHandler(fd, fd, IOHT_UDS_CARRIER) {
	IOHandlerManager::EnableReadData(this);
	_writeDataEnabled = false;
	_enableWriteDataCalled = false;
	_sendBufferSize = 1024 * 1024 * 8;
	_recvBufferSize = 1024 * 1024 * 16;
	_rx = 0;
	_tx = 0;
	_ioAmount = 0;
	_lastRecvError = 0;
	_lastSendError = 0;
	_expectedLength = 0;
	_inboundfd = fd;
	_lastThresholdIndex = -1;
	_counter = 0;
	_remainingBuffer = 0;
	_seq = 0;
	_writeSeq = 0;

	Variant stats;
	GetStats(stats);
}
UDSCarrier::~UDSCarrier() {
	Variant stats;
	GetStats(stats);
	CLOSE_SOCKET(_inboundFd);

}


bool UDSCarrier::OnEvent(struct kevent &event) {
	switch (event.filter) {
		case EVFILT_READ:
		{
			//1. Read data
			IOBuffer *pInputBuffer = _pProtocol->GetInputBuffer();
			o_assert(pInputBuffer != NULL);

			int32_t err = 0;
			if (!_readBuffer.ReadFromTCPFd(_inboundFd, _recvBufferSize, _ioAmount, err)) {
				FATAL("Unable to read data. Err: %"PRId32, err);
				return false;
			}

			if (_ioAmount == 0) {
				FATAL("Connection closed");
				return false;
			}
			//		DEBUG("[DEBUG] ReadBuffer size: %"PRIu32". _ioAmount: %"PRIu32, GETAVAILABLEBYTESCOUNT(_readBuffer), _ioAmount);
			//		_remainingBuffer += _ioAmount;
			//		if (_remainingBuffer >= THRESHOLD) {
			//			_counter++;
			//			_remainingBuffer -= THRESHOLD;
			//		}

			while (_ioAmount > 0) {
				uint8_t *pData = GETIBPOINTER(_readBuffer);
				//			_expectedLength = (uint32_t) ENTOHSP((uint16_t *)(pData + 3));
				uint32_t droppedBytes = 0;
				uint32_t droppedPackets = 0;
				// INFO("[UDS] Input: %s", STR(_readBuffer.ToString()));
				while (GETAVAILABLEBYTESCOUNT(_readBuffer) > 2 && GETIBPOINTER(_readBuffer)[0] != 0xbe && GETIBPOINTER(_readBuffer)[1] != 0xef) {
					_readBuffer.Ignore(1);
					droppedBytes++;
				}

				_expectedLength = 0;
				for (uint8_t i = 0; i < 3; i++) {
					_expectedLength |= (*(pData + 3 + i) << ((2 - i) * 8));
				}

				if (droppedBytes > 0)
#ifdef UDS_DEBUG
					FATAL("Dropped bytes: %"PRIu32, droppedBytes);
#else
					WARN("Dropped bytes: %"PRIu32, droppedBytes);
#endif

				uint8_t newSeq = GETIBPOINTER(_readBuffer)[2];

				if (_seq < newSeq) {
					WARN("Missing packet. Expected seq=%"PRIu8" Received=%"PRIu8, _seq, newSeq);
					_seq = newSeq;
				}

				// if data read is shorter than expected length, do not pass the data yet
				if (GETAVAILABLEBYTESCOUNT(_readBuffer) < _expectedLength + HEADER_SIZE) {
#ifdef UDS_DEBUG
					WARN("DATA_SIZE: Insufficient data; waiting for more. Available buffer: %"PRIu32". ExpectedLength: %"PRIu32, GETAVAILABLEBYTESCOUNT(_readBuffer), _expectedLength);
#endif
					return true;
				}

				_seq++;
#ifdef UDS_DEBUG
				FATAL("[DEBUG] Expected: %"PRIu32"; Received: %"PRIu32, _expectedLength, GETAVAILABLEBYTESCOUNT(_readBuffer) - 2);
#endif
				// remove datalength bytes
				_readBuffer.Ignore(HEADER_SIZE);
				pInputBuffer->ReadFromBuffer(GETIBPOINTER(_readBuffer), _expectedLength);
				_readBuffer.Ignore(_expectedLength);

				_rx += _expectedLength;
				ADD_IN_BYTES_MANAGED(_type, _expectedLength);

				if (!_pProtocol->SignalInputData(_expectedLength)) {
					FATAL("Unable to signal data available");
					_readBuffer.IgnoreAll();
					return false;
				}

				_ioAmount -= _expectedLength + droppedBytes + HEADER_SIZE;
			}
			break;
			//		DEBUG("[DEBUG] Read finish. %"PRIu32" bytes remaining.", GETAVAILABLEBYTESCOUNT(_readBuffer));
		}

		case EVFILT_WRITE:
		{
			//2. Write data
			IOBuffer *pOutputBuffer = NULL;
			if ((pOutputBuffer = _pProtocol->GetOutputBuffer()) != NULL) {
				IOBuffer pWriteBuffer;
				pWriteBuffer.IgnoreAll();
				uint32_t payloadLength = GETAVAILABLEBYTESCOUNT(*pOutputBuffer);
#ifdef UDS_DEBUG
				FATAL("Payload length: %"PRIu32, payloadLength);
#endif
				pWriteBuffer.ReadFromByte(0xbe);
				pWriteBuffer.ReadFromByte(0xef);
				pWriteBuffer.ReadFromByte(_writeSeq++);
				pWriteBuffer.ReadFromByte((uint8_t) ((payloadLength & 0x00FF0000) >> 16));
				pWriteBuffer.ReadFromByte((uint8_t) ((payloadLength & 0x0000FF00) >> 8));
				pWriteBuffer.ReadFromByte((uint8_t) (payloadLength & 0x000000FF));
				pWriteBuffer.ReadFromBuffer(GETIBPOINTER(*pOutputBuffer), payloadLength);
				if (!pWriteBuffer.WriteToTCPFd(_inboundFd, _sendBufferSize, _ioAmount,
						_lastSendError)) {
					FATAL("Unable to write data on connection: %s. Error was (%d): %s",
							(_pProtocol != NULL) ? STR(*_pProtocol) : "", _lastSendError,
							strerror(_lastSendError));
					IOHandlerManager::EnqueueForDelete(this);
					return false;
				}
				_tx += _ioAmount;
				pOutputBuffer->IgnoreAll();
				ADD_OUT_BYTES_MANAGED(_type, _ioAmount);
				if (GETAVAILABLEBYTESCOUNT(*pOutputBuffer) == 0) {
					DISABLE_WRITE_DATA;
				}
			} else {
				DISABLE_WRITE_DATA;
			}
			break;
		}
		default:
		{
			FATAL("Invalid event type: %d", event.filter);
			return false;
		}
	}


	return true;
}
bool UDSCarrier::SignalOutputData() {
	ENABLE_WRITE_DATA;
	return true;
}
void UDSCarrier::GetStats(Variant &info, uint32_t namespaceId) {

}

string UDSCarrier::ToString() {
	return format("CUDS(%"PRId32")", _inboundFd);
}
#endif /* NET_KQUEUE */
#endif /* HAS_UDS */
