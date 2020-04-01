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
#ifndef _PACKETBUFFER_H
#define	_PACKETBUFFER_H

#include "common.h"

struct Packet {
	uint8_t *data;
	uint32_t dataLength;
};

class PacketBuffer {
private:
	map<uint16_t, Packet> _internalBuffer;
	std::pair<uint16_t, uint16_t> _range;
	bool _enabled;
public:
	PacketBuffer();
	~PacketBuffer();

	inline void Enable(bool enabled) { _enabled = enabled; }
	void Clear();
	void PushPacket(uint16_t pid, Packet &packet);
	void PushPacket(uint16_t pid, MSGHDR &packet);
	uint32_t GetPacket(uint16_t pid, IOBuffer &packet);
	inline uint32_t GetBufferSize() { return (uint32_t)_internalBuffer.size(); }
	bool WithinRange(uint16_t pid);
	inline bool IsHigherThanRange(uint16_t pid) { return pid > _range.second; }
private:
	void CopyFromIOBuffer(Packet &dest, IOBuffer &src);
	void UpdateStats(uint16_t pid);
};

#endif	/* _PACKETBUFFER_H */
#endif/* HAS_PROTOCOL_RTP */
