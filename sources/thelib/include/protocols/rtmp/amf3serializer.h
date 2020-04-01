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
#ifndef _AMF3SERIALIZER_H
#define	_AMF3SERIALIZER_H

#include "common.h"

class DLLEXP AMF3Serializer {
private:
	vector<Variant> _objects;
	vector<Variant> _traits;
	vector<string> _strings;
	vector<string> _byteArrays;
public:
	AMF3Serializer();
	virtual ~AMF3Serializer();

	bool Read(IOBuffer &buffer, Variant &variant);
	bool Write(IOBuffer &buffer, Variant &variant);


	bool ReadUndefined(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteUndefined(IOBuffer &buffer);

	bool ReadNull(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteNull(IOBuffer &buffer);

	bool ReadFalse(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteFalse(IOBuffer &buffer);

	bool ReadTrue(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteTrue(IOBuffer &buffer);

	bool ReadInteger(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteInteger(IOBuffer &buffer, uint32_t value, bool writeType = true);

	bool ReadDouble(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteDouble(IOBuffer &buffer, double value, bool writeType = true);

	bool ReadString(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteString(IOBuffer &buffer, string value, bool writeType = true);

	bool ReadXMLDoc(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteXMLDoc(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadDate(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteDate(IOBuffer &buffer, Timestamp value, bool writeType = true);

	bool ReadArray(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteArray(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadObject(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteObject(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadXML(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteXML(IOBuffer &buffer, Variant &variant, bool writeType = true);

	bool ReadByteArray(IOBuffer &buffer, Variant &variant, bool readType = true);
	bool WriteByteArray(IOBuffer &buffer, Variant &variant, bool writeType = true);

	static bool ReadU29(IOBuffer &buffer, uint32_t &value);
	static bool WriteU29(IOBuffer &buffer, uint32_t value);
};


#endif	/* _AMF3SERIALIZER_H */

#endif /* HAS_PROTOCOL_RTMP */

