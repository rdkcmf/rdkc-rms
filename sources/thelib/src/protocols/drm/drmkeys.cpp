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

#include "protocols/drm/drmkeys.h"

DRMKeys::DRMKeys(uint16_t reserveKeys, uint16_t reserveIds) {
	_reserveKeys = reserveKeys;
	_reserveIds = reserveIds;
	if (_reserveKeys < 5) {
		_reserveKeys = 5;
	}
	if (_reserveIds < 5) {
		_reserveIds = 5;
	}
	_lastId = 0;
	_unusedId = 0;
	for (int i = 1; i < _reserveIds; i++) {
		CreateDRMEntry();	//prepare initial queues since timer will request keys before hls creation
	}
	_sentPos.clear();
	_sentId = 0;
}

DRMKeys::~DRMKeys() {
}

uint32_t DRMKeys::CreateDRMEntry() {
	_lastId += 1;
	_drmQueue[_lastId].clear();
	return _lastId;
}

bool DRMKeys::DeleteDRMEntry(uint32_t id) {
	if (_drmQueue.empty() ||
			(_drmQueue.count(id) == 0)) {
		return false;
	}
	_drmQueue.erase(id);
	return true;
}

uint32_t DRMKeys::GetDRMEntryCount() {
	return (uint32_t) _drmQueue.size();
}

bool DRMKeys::HasDRMEntry(uint32_t id) {
	if (_drmQueue.empty() ||
			(_drmQueue.count(id) == 0)) {
		return false;
	}
	return true;
}

bool DRMKeys::PutKey(uint32_t id, _DRMEntry entry) {
	if (IsFullOfKeys(id)) {
		return false;
	}

	_drmQueue[id].push_back(entry);
	//FINEST("PUT id=%d, key=%s, loc=%s, pos=%d", id, STR(entry._key), STR(entry._location), entry._position);
	return true;
}

DRMKeys::_DRMEntry DRMKeys::GetKey(uint32_t id) {
	if (GetKeyCount(id) == 0) {
		return _DRMEntry();
	}
	_DRMEntry entry = _drmQueue[id].front();
	_drmQueue[id].erase(_drmQueue[id].begin());
	//FINEST("id=%d, key=%s, loc=%s, pos=%d", id, STR(entry._key), STR(entry._location), entry._position);
	return entry;
}

uint16_t DRMKeys::GetKeyCount(uint32_t id) {
	if (_drmQueue.empty() ||
			(_drmQueue.count(id) == 0)) {
		return 0;
	}
 	return (uint16_t) _drmQueue[id].size();
}

bool DRMKeys::IsFullOfKeys(uint32_t id) {
	if (_drmQueue.empty() ||
			(_drmQueue.count(id) == 0) ||
			(_drmQueue[id].size() < _reserveKeys)) {
		return false;
	}
	return true;
}

uint32_t DRMKeys::GetPos(uint32_t id) {
	if (_sentPos.count(id) == 0) {
		return 0;
	}
	return _sentPos[id];
}

uint32_t DRMKeys::NextPos(uint32_t id) {
	if (_sentPos.count(id) == 0) {
		_sentPos[id] = 0;
	}
	_sentPos[id] += 1;
	return _sentPos[id];
}

uint32_t DRMKeys::GetSentId() {
	return _sentId;
}

bool DRMKeys::SetSentId(uint32_t id) {
	if (id == 0) {
		return false;
	}
	_sentId = id;
	return true;
}

bool DRMKeys::ClearSentId() {
	_sentId = 0;
	return true;
}

uint32_t DRMKeys::GetRandomId(uint32_t maxId) {
	if (maxId == 0) {
		return 0;
	}
	int32_t n = (rand() % maxId) + 1;
	FOR_MAP(_drmQueue, uint32_t, vector <_DRMEntry>, i) {
		n -= 1;
		if (n <= 0) {
			return MAP_KEY(i);
		}
	}
	return 0;
}

uint32_t DRMKeys::GetLastId() {
	return _lastId;
}

uint32_t DRMKeys::GetUnusedId() {
	if ((_lastId == 0) ||
		(_unusedId == _lastId)) {
			return 0;
	}
	_unusedId += 1;
	return _unusedId;
}

#endif  /* HAS_PROTOCOL_DRM */
