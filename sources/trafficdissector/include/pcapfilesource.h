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


#ifndef _PCAPFILESOURCE_H
#define	_PCAPFILESOURCE_H

#include "basesource.h"
#include <pcap/pcap.h>

struct PacketIP;
struct PacketTCP;

class PcapFileSource
: public BaseSource {
private:
	pcap_t *_pPcapHandle;
	char _pcapErrorBuff[PCAP_ERRBUF_SIZE];
	bpf_program _bpf;
	pcap_pkthdr *_pPacketHeader;
	uint8_t *_pPacketRaw;
	PacketIP *_pPacketIP;
	PacketTCP *_pPacketTCP;
	int32_t _etherLength;
	uint32_t _filterIp;
	uint16_t _filterPort;
	bool _isServerFilter;
	uint32_t _serverToClientNextSeqNumber;
	map<uint32_t,uint32_t> _serverToClientSeenSequences;
	uint32_t _clientToServerNextSeqNumber;
	map<uint32_t,uint32_t> _clientToServerSeenSequences;
public:
	PcapFileSource();
	virtual ~PcapFileSource();
	virtual bool Init(Variant &parameters);
	virtual bool GetData(DataBlock &destination);
};


#endif	/* _PCAPFILESOURCE_H */
