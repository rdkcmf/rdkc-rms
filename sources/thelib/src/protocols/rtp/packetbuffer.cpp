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



#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/packetbuffer.h"

PacketBuffer::PacketBuffer() {

}
PacketBuffer::~PacketBuffer() {
	Clear();
}

void PacketBuffer::Clear() {
	FOR_MAP(_internalBuffer, uint16_t, Packet, i) {
		delete MAP_VAL(i).data;
	}
	_internalBuffer.clear();
}

void PacketBuffer::PushPacket(uint16_t pid, Packet &packet) {
	_internalBuffer[pid] = packet;
}

void PacketBuffer::PushPacket(uint16_t pid, MSGHDR &packet) {
	if (!_enabled)
		return;
	IOBuffer packetBuffer;
	for (int i = 0; i < (int) packet.MSGHDR_MSG_IOVLEN; i++) {
		packetBuffer.ReadFromBuffer((uint8_t*)packet.MSGHDR_MSG_IOV[i].IOVEC_IOV_BASE,
			packet.MSGHDR_MSG_IOV[i].IOVEC_IOV_LEN);
	}
	//PushPacket(pid, packetBuffer);
	CopyFromIOBuffer(_internalBuffer[pid], packetBuffer);
	UpdateStats(pid);
}

uint32_t PacketBuffer::GetPacket(uint16_t pid, IOBuffer &packet) {
	if (!_enabled)
		return 0;
	packet.IgnoreAll();
	if (MAP_HAS1(_internalBuffer, pid)) {
		Packet targetPacket = _internalBuffer[pid];
		packet.ReadFromBuffer(targetPacket.data, targetPacket.dataLength);
		return targetPacket.dataLength;
	}
	return 0;
}

void PacketBuffer::CopyFromIOBuffer(Packet &dest, IOBuffer &src) {
	dest.dataLength = GETAVAILABLEBYTESCOUNT(src);
	dest.data = new uint8_t[dest.dataLength];
	memcpy(dest.data, GETIBPOINTER(src), dest.dataLength);
}

bool PacketBuffer::WithinRange(uint16_t pid) {
	if (_range.first < _range.second) { // no rollover in range
		return _range.first <= pid && pid <= _range.second;
	} else if (_range.first > _range.second){
		return _range.first <= pid || pid <= _range.second;
	} else { // single element range
		return pid == _range.first;
	}
}

void PacketBuffer::UpdateStats(uint16_t pid) {
	if (_internalBuffer.size() == 0) {
		return;
	}
	else if (_internalBuffer.size() == 1) {
		_range.first = pid;
		_range.second = pid;
	}
	else {
		_range.second = pid;
	}

}
#endif	/* HAS_PROTOCOL_RTP */
