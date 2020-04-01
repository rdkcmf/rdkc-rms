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
#ifndef _SO_H
#define	_SO_H

#include "common.h"

typedef struct _DirtyInfo {
	string propertyName;
	uint8_t type;
} DirtyInfo;

typedef vector<DirtyInfo> Dirtyness;

class DLLEXP SO {
private:
	string _name;
	uint32_t _version;
	bool _persistent;
	Variant _payload;
	map<uint32_t, uint32_t> _registeredProtocols;
	map<uint32_t, Dirtyness> _dirtyPropsByProtocol;
	bool _versionIncremented;
public:
	SO(string name, bool persistent);
	virtual ~SO();
public:
	string GetName();
	uint32_t GetVersion();
	bool IsPersistent();
	Variant & GetPayload();

	void RegisterProtocol(uint32_t protocolId);
	void UnRegisterProtocol(uint32_t protocolId);
	uint32_t GetSubscribersCount();
	vector<string> GetPropertyNames();
	uint32_t PropertiesCount();

	bool HasProperties();
	bool HasProperty(string propertyName);

	string DumpTrack();
	void Track();

	operator string();

	Variant & Get(string &key);
	Variant & Set(string &key, Variant &value, uint32_t version,
			uint32_t protocolId);
	void UnSet(string key, uint32_t version);
	bool SendMessage(Variant &message);
};

class DLLEXP ClientSO
: public Variant {
private:
	Variant _dummy1;
	Variant _dummy2;
public:

	ClientSO() {
		Variant v1;
		v1.IsArray(false);
		properties(v1);
		Variant v2;
		v2.IsArray(true);
		changedProperties(v2);
	}
	VARIANT_GETSET(string, name, "");
	VARIANT_GETSET(uint32_t, version, 0);
	VARIANT_GETSET(bool, persistent, 0);
	VARIANT_GETSET(Variant&, properties, _dummy1);
	VARIANT_GETSET(Variant&, changedProperties, _dummy2);
};


#endif	/* _SO_H */

#endif /* HAS_PROTOCOL_RTMP */

