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


#include "pcapfilesource.h"
#include <pcap/pcap.h>

struct PacketIP {
	uint8_t versionIhl;
	uint8_t tos;
	uint16_t totalLength;
	uint16_t identification;
	uint16_t flagsFrameOffset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t headerChecksum;
	struct in_addr sourceAddress;
	struct in_addr destinationAddress;

	uint8_t GetHeadersLength() {
		return (versionIhl & 0x0f)*4;
	}

	operator string() {
		string result = inet_ntoa(sourceAddress);
		result += "->";
		result += inet_ntoa(destinationAddress);
		return result;
	}
};

struct PacketTCP {
	uint16_t sourcePort;
	uint16_t destinationPort;
	uint32_t sequenceNumber;
	uint32_t acknowledgmentNumber;
	uint8_t dataOffset;
	uint8_t flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgentPointer;

	uint8_t GetHeadersLength() {
		return (dataOffset >> 4)*4;
	}

	operator string() {
		return format("%"PRIu16"->%"PRIu16, ENTOHS(sourcePort), ENTOHS(destinationPort));
	}
};

PcapFileSource::PcapFileSource() {
	_pPcapHandle = NULL;
	memset(_pcapErrorBuff, 0, sizeof (_pcapErrorBuff));
	memset(&_bpf, 0, sizeof (_bpf));
	_pPacketHeader = NULL;
	_pPacketRaw = NULL;
	_pPacketIP = NULL;
	_pPacketTCP = NULL;
	_filterIp = 0;
	_filterPort = 0;
	_isServerFilter = false;
	_serverToClientNextSeqNumber = 0;
	_clientToServerNextSeqNumber = 0;
	_etherLength = 0;
}

PcapFileSource::~PcapFileSource() {
	if (_pPcapHandle != NULL) {
		pcap_close(_pPcapHandle);
		_pPcapHandle = NULL;
	}
}

bool PcapFileSource::Init(Variant &parameters) {
	_pPcapHandle = pcap_open_offline(STR(parameters["arguments"]["trafficFile"]), _pcapErrorBuff);
	if (_pPcapHandle == NULL) {
		FATAL("Unable to open pcap file: %s", _pcapErrorBuff);
		return false;
	}

	_etherLength = pcap_datalink(_pPcapHandle);
	switch (_etherLength) {
		case DLT_NULL:
		{
			_etherLength = 4;
			break;
		}
		case DLT_EN10MB:
		{
			_etherLength = 14;
			break;
		}
		default:
		{
			FATAL("Link type %"PRId32" not supported", _etherLength);
			return false;
		}
	}

	if (pcap_compile(_pPcapHandle, &_bpf, STR(parameters["arguments"]["filter"]["complete"]), 0, PCAP_NETMASK_UNKNOWN) != 0) {
		FATAL("Couldn't parse filter %s: %s\n",
				STR(parameters["arguments"]["filter"]["complete"]),
				pcap_geterr(_pPcapHandle));
		return false;
	}

	if (pcap_setfilter(_pPcapHandle, &_bpf) != 0) {
		FATAL("Couldn't install filter %s: %s\n",
				STR(parameters["arguments"]["filter"]["complete"]),
				pcap_geterr(_pPcapHandle));
		return false;
	}

	_filterIp = (uint32_t) parameters["arguments"]["filter"]["ip"];
	_filterPort = (uint16_t) parameters["arguments"]["filter"]["port"];
	_isServerFilter = (bool)parameters["arguments"]["filter"]["isServer"];

	return true;
}

bool PcapFileSource::GetData(DataBlock &destBlock) {
	_pPacketHeader = NULL;
	_pPacketRaw = NULL;
	_pPacketIP = NULL;
	_pPacketTCP = NULL;
	destBlock.Reset();
	switch (pcap_next_ex(
			_pPcapHandle,
			&_pPacketHeader,
			(const u_char **) &_pPacketRaw
			)) {
		case 1: //packet available
		{
			destBlock.timestamp = _pPacketHeader->ts;

			if (_pPacketHeader->len < (_etherLength + sizeof (PacketIP))) {
				FATAL("Inconsistent lengths detected inside ARP->IP->TCP protocols chains");
				return false;
			}

			_pPacketIP = (PacketIP *) (_pPacketRaw + _etherLength);
			if (_pPacketIP->GetHeadersLength() < 20) {
				FATAL("Invalid IP packet header");
				return false;
			}

			if (_pPacketHeader->len < (_etherLength + _pPacketIP->GetHeadersLength() + sizeof (PacketTCP))) {
				FATAL("Inconsistent lengths detected inside ARP->IP->TCP protocols chains");
				return false;
			}

			_pPacketTCP = (PacketTCP *) (_pPacketRaw + _etherLength + _pPacketIP->GetHeadersLength());
			if (_pPacketTCP->GetHeadersLength() < 20) {
				FATAL("Invalid TCP packet header");
				return false;
			}

			if (((uint32_t) _pPacketIP->destinationAddress.s_addr == _filterIp)
					&& ((uint16_t) _pPacketTCP->destinationPort == _filterPort)) {
				if (_isServerFilter)
					destBlock.direction = DIRECTION_CLIENT_TO_SERVER;
				else
					destBlock.direction = DIRECTION_SERVER_TO_CLIENT;
			} else if (((uint32_t) _pPacketIP->sourceAddress.s_addr == _filterIp)
					&& ((uint16_t) _pPacketTCP->sourcePort == _filterPort)) {
				if (_isServerFilter)
					destBlock.direction = DIRECTION_SERVER_TO_CLIENT;
				else
					destBlock.direction = DIRECTION_CLIENT_TO_SERVER;
			}

			if (_pPacketHeader->len < (_etherLength + _pPacketIP->GetHeadersLength() + _pPacketTCP->GetHeadersLength())) {
				FATAL("Inconsistent lengths detected inside ARP->IP->TCP protocols chains");
				return false;
			}

			destBlock.packetPayloadLength = _pPacketHeader->len - (_etherLength + _pPacketIP->GetHeadersLength() + _pPacketTCP->GetHeadersLength());
			if (destBlock.packetPayloadLength > 0) {
				destBlock.pPacketPayload = _pPacketRaw + _etherLength + _pPacketIP->GetHeadersLength() + _pPacketTCP->GetHeadersLength();
			}

			_pPacketTCP->sequenceNumber = ENTOHL(_pPacketTCP->sequenceNumber);


			switch (destBlock.direction) {
				case DIRECTION_CLIENT_TO_SERVER:
				{
					if ((_clientToServerNextSeqNumber != 0) && (_clientToServerNextSeqNumber != _pPacketTCP->sequenceNumber)) {
						if (_pPacketTCP->sequenceNumber < _clientToServerNextSeqNumber) {
							WARN("C->S: Out of order seq number: %"PRIu32, _pPacketTCP->sequenceNumber);
							destBlock.packetPayloadLength = 0;
							return true;
						}
						FATAL("Invalid sequence number. A total of %"PRIu32" bytes of payload missing in capture file",
								_pPacketTCP->sequenceNumber - _clientToServerNextSeqNumber);
						return false;
					}
					if (destBlock.packetPayloadLength != 0) {
						_clientToServerNextSeqNumber = _pPacketTCP->sequenceNumber + destBlock.packetPayloadLength;
					} else {
						_clientToServerNextSeqNumber = _pPacketTCP->sequenceNumber + ((_pPacketTCP->flags & 0x03) != 0 ? 1 : 0);
					}
					//FINEST("C->S: Current: %"PRIu32"; next: %"PRIu32"; length: %"PRIu32, _pPacketTCP->sequenceNumber, _clientToServerNextSeqNumber, destBlock.packetPayloadLength);
					break;
				}
				case DIRECTION_SERVER_TO_CLIENT:
				{
					if ((_serverToClientNextSeqNumber != 0) && (_serverToClientNextSeqNumber != _pPacketTCP->sequenceNumber)) {
						if (_pPacketTCP->sequenceNumber < _serverToClientNextSeqNumber) {
							WARN("S->C: Out of order seq number: %"PRIu32, _pPacketTCP->sequenceNumber);
							destBlock.packetPayloadLength = 0;
							return true;
						}
						FATAL("Invalid sequence number. A total of %"PRIu32" bytes of payload missing in capture file",
								_pPacketTCP->sequenceNumber - _serverToClientNextSeqNumber);
						return false;
					}
					if (destBlock.packetPayloadLength != 0) {
						_serverToClientNextSeqNumber = _pPacketTCP->sequenceNumber + destBlock.packetPayloadLength;
					} else {
						_serverToClientNextSeqNumber = _pPacketTCP->sequenceNumber + ((_pPacketTCP->flags & 0x03) != 0 ? 1 : 0);
					}
					//FINEST("S->C: Current: %"PRIu32"; next: %"PRIu32"; length: %"PRIu32, _pPacketTCP->sequenceNumber, _serverToClientNextSeqNumber, destBlock.packetPayloadLength);
					break;
				}
				default:
				{
					FATAL("Unable to detect the direction of the TCP flow");
					return false;
				}
			}

			return true;
		}
		case 0:
		{
			FATAL("Live capture not supported yet");
			return false;
		}
		case -1:
		{
			FATAL("Error occurred while reading packet: %s", _pcapErrorBuff);
			return false;
		}
		case -2:
		{
			INFO("Capture end");
			destBlock.eof = true;
			return true;
		}
		default:
		{
			FATAL("returned unknown value");
			return false;
		}
	}
}
