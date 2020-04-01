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


#ifndef _UDPSENDERPROTOCOL_H
#define	_UDPSENDERPROTOCOL_H

#include "protocols/udpprotocol.h"

class ReadyForSendInterface;
class SocketAddress;

class UDPSenderProtocol
: public UDPProtocol {
private:
	SocketAddress _destAddress;
	SOCKET _fd;
	ReadyForSendInterface *_pReadyForSend;
	string _bindIp;
	uint16_t _bindPort;
public:
	UDPSenderProtocol();
	virtual ~UDPSenderProtocol();

	virtual void SetIOHandler(IOHandler *pCarrier);
	virtual void ReadyForSend();

	static UDPSenderProtocol *GetInstance(string bindIp, uint16_t bindPort,
			string targetIp, uint16_t targetPort, uint16_t ttl, uint16_t tos,
			ReadyForSendInterface *pReadyForSend);
	void ResetReadyForSendInterface();

	uint16_t GetUdpBindPort();

	virtual bool SendChunked(uint8_t *pData, uint32_t dataLength,
			uint32_t maxChunkSize);
	virtual bool SendBlock(uint8_t *pData, uint32_t dataLength);
};

#endif	/* _UDPSENDERPROTOCOL_H */
