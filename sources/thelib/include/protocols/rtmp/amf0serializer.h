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
#ifndef _AMF0SERIALIZER_H
#define	_AMF0SERIALIZER_H

#include "common.h"

class DLLEXP AMF0Serializer {
private:
	static vector<string> _keysOrder;
	static uint8_t _endOfObject[];
public:
	AMF0Serializer();
	virtual ~AMF0Serializer();

	bool ReadShortString(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteShortString(IOBuffer &buffer, string &value, bool writeType = true);

	bool ReadLongString(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteLongString(IOBuffer &buffer, string &value, bool writeType = true);

	bool ReadDouble(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteDouble(IOBuffer &buffer, double value, bool writeType = true);

	bool ReadObject(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteObject(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadTypedObject(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteTypedObject(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadMixedArray(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteMixedArray(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadArray(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteArray(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadAMF3Object(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteAMF3Object(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadBoolean(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteBoolean(IOBuffer &buffer, bool value, bool writeType = true);

	bool ReadTimestamp(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteTimestamp(IOBuffer &buffer, Timestamp value, bool writeType = true);

	bool ReadNull(IOBuffer &buffer, Variant &variant);
	bool WriteNull(IOBuffer &buffer);

	bool ReadUndefined(IOBuffer &buffer, Variant &variant);
	bool WriteUndefined(IOBuffer &buffer);

	bool ReadUInt8(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteUInt8(IOBuffer &buffer, uint8_t value, bool writeType = true);

	bool ReadInt16(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteInt16(IOBuffer &buffer, int16_t value, bool writeType = true);

	bool ReadUInt32(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteUInt32(IOBuffer &buffer, uint32_t value, bool writeType = true);

	bool ReadInt32(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteInt32(IOBuffer &buffer, int32_t value, bool writeType = true);

	bool Read(IOBuffer &buffer, Variant &variant);
	bool Write(IOBuffer &buffer, Variant &variant);
private:
	bool IsAMF3(Variant &variant);
};


#endif	/* _AMF0SERIALIZER_H */

#endif /* HAS_PROTOCOL_RTMP */

