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


#ifdef HAS_PROTOCOL_DRM
#ifndef _DRMKEYS_H
#define	_DRMKEYS_H

#include "platform/platform.h"

class DRMKeys {
public:
	struct _DRMEntry {
		string _key;
		string _location;
		uint32_t _position;

		_DRMEntry(string key = "",
				string location = "",
				uint32_t position = 0) {
			_key = key;
			_location = location;
			_position = position;
		}
	};
private:
	uint16_t _reserveKeys;
	uint16_t _reserveIds;
	map<uint32_t, vector <_DRMEntry> > _drmQueue;
	map<uint32_t, uint32_t> _sentPos;
	uint32_t _sentId;
	uint32_t _lastId;
	uint32_t _unusedId;
public:
	DRMKeys(uint16_t reserveKeys, uint16_t reserveIds);
	virtual ~DRMKeys();

	uint32_t CreateDRMEntry();
	bool DeleteDRMEntry(uint32_t id);
	uint32_t GetDRMEntryCount();
	bool HasDRMEntry(uint32_t id);

	bool PutKey(uint32_t id, _DRMEntry);
	_DRMEntry GetKey(uint32_t id);
	uint16_t GetKeyCount(uint32_t id);
	bool IsFullOfKeys(uint32_t id);

	uint32_t GetPos(uint32_t id);
	uint32_t NextPos(uint32_t id);

	uint32_t GetSentId();
	bool SetSentId(uint32_t id);
	bool ClearSentId();
	uint32_t GetRandomId(uint32_t maxId);
	uint32_t GetLastId();
	uint32_t GetUnusedId();
};

#endif	/* _DRMKEYS_H */
#endif /* HAS_PROTOCOL_DRM */
