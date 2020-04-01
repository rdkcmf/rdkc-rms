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


#ifdef HAS_PROTOCOL_RPC
#ifndef _RPCSERIALIZER_H
#define	_RPCSERIALIZER_H

#include "common.h"

#define VARIANT_RPC_SERIALIZER_BIN		1
#define VARIANT_RPC_SERIALIZER_XMLRPC	2
#define VARIANT_RPC_SERIALIZER_XMLVAR	3
#define VARIANT_RPC_SERIALIZER_JSON		4

class BaseRPCSerializer {
protected:
	string _httpContentType;
public:
	BaseRPCSerializer();
	virtual ~BaseRPCSerializer();
	string &GetHTTPContentType();
	virtual bool Serialize(Variant &source, IOBuffer &dest) = 0;
	virtual bool Deserialize(IOBuffer &source, Variant &dest) = 0;

	static BaseRPCSerializer *GetSerializer(string &type);
};

class VariantRPCSerializer
: public BaseRPCSerializer {
private:
	uint8_t _type;
public:
	VariantRPCSerializer(uint8_t type);
	virtual ~VariantRPCSerializer();
	virtual bool Serialize(Variant &source, IOBuffer &dest);
	virtual bool Deserialize(IOBuffer &source, Variant &dest);
};

class KvpRPCSerializer
: public BaseRPCSerializer {
public:
	KvpRPCSerializer();
	virtual ~KvpRPCSerializer();
	virtual bool Serialize(Variant &source, IOBuffer &dest);
	virtual bool Deserialize(IOBuffer &source, Variant &dest);
};

#endif	/* _RPCSERIALIZER_H */
#endif /* HAS_PROTOCOL_RPC */
