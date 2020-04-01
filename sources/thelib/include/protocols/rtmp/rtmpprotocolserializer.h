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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _RTMPPROTOCOLSERIALIZER_H
#define	_RTMPPROTOCOLSERIALIZER_H


#include "protocols/rtmp/amf0serializer.h"
#include "protocols/rtmp/amf3serializer.h"
#include "protocols/rtmp/channel.h"

class DLLEXP RTMPProtocolSerializer {
private:
	AMF0Serializer _amf0;
	AMF3Serializer _amf3;
	IOBuffer _internalBuffer;
public:
	RTMPProtocolSerializer();
	virtual ~RTMPProtocolSerializer();

	static const char *GetUserCtrlTypeString(uint16_t type);
	static string GetSOPrimitiveString(uint8_t type);
	bool Deserialize(Header &header, IOBuffer &buffer, Variant &message);
	bool Serialize(Channel &channel, Variant &message,
			IOBuffer &buffer, uint32_t chunkSize);
private:
	bool SerializeInvoke(IOBuffer &buffer, Variant &message);
	bool SerializeNotify(IOBuffer &buffer, Variant &message);
	bool SerializeFlexStreamSend(IOBuffer &buffer, Variant &message);
	bool SerializeAck(IOBuffer &buffer, uint32_t value);
	bool SerializeUsrCtrl(IOBuffer &buffer, Variant message);
	bool SerializeChunkSize(IOBuffer &buffer, uint32_t value);
	bool SerializeWinAckSize(IOBuffer &buffer, uint32_t value);
	bool SerializeAbortMessage(IOBuffer &buffer, uint32_t value);
	bool SerializeClientBW(IOBuffer &buffer, Variant value);
	bool SerializeFlexSharedObject(IOBuffer &buffer, Variant &message);
	bool SerializeSharedObject(IOBuffer &buffer, Variant &message);


	bool DeserializeNotify(IOBuffer &buffer, Variant &message);
	bool DeserializeFlexStreamSend(IOBuffer &buffer, Variant &message);
	bool DeserializeInvoke(IOBuffer &buffer, Variant &message);
	bool DeserializeAck(IOBuffer &buffer, Variant &message);
	bool DeserializeUsrCtrl(IOBuffer &buffer, Variant &message);
	bool DeserializeChunkSize(IOBuffer &buffer, Variant &message);
	bool DeserializeWinAckSize(IOBuffer &buffer, Variant &message);
	bool DeserializePeerBW(IOBuffer &buffer, Variant &message);
	bool DeserializeAbortMessage(IOBuffer &buffer, Variant &message);
	bool DeserializeFlexSharedObject(IOBuffer &buffer, Variant &message);
	bool DeserializeSharedObject(IOBuffer &buffer, Variant &message);


	void ChunkBuffer(IOBuffer &destination, IOBuffer &source,
			uint32_t chunkSize, Channel &channel);
};


#endif	/* _RTMPPROTOCOLSERIALIZER_H */

#endif /* HAS_PROTOCOL_RTMP */

