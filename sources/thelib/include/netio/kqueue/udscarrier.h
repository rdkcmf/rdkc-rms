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


#ifndef _UDS_CARRIER_H
#define _UDS_CARRIER_H

#include "netio/kqueue/iohandler.h"

#define THRESHOLD 1024 * 256
#define THRESHOLD_STEP 1024

class UDSCarrier
: public IOHandler {
private:
	bool _writeDataEnabled;
	bool _enableWriteDataCalled;
	int32_t _inboundfd;
	int32_t _recvBufferSize;
	int32_t _sendBufferSize;
	uint64_t _rx;
	uint64_t _tx;
	int32_t _ioAmount;
	int _lastRecvError;
	int _lastSendError;
	string _fifo;
	sockaddr_un _addr;
	IOBuffer _readBuffer;
	uint32_t _expectedLength;
	int32_t _lastThresholdIndex;
	uint32_t _counter;
	uint32_t _remainingBuffer;
	uint8_t _seq;
	uint8_t _writeSeq;

public:
	UDSCarrier(int32_t fd);
//	static UDSCarrier *Spawn(string name);
	virtual ~UDSCarrier();
	virtual bool SignalOutputData();
	virtual bool OnEvent(struct kevent &event);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	string ToString();
};
#endif /* _UDS_CARRIER_H */
