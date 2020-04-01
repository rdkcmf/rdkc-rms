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



#include "protocols/protocolmanager.h"
#include "protocols/baseprotocol.h"
#include "netio/netio.h"

map<uint32_t, BaseProtocol *> ProtocolManager::_activeProtocols;
map<uint32_t, BaseProtocol *> ProtocolManager::_deadProtocols;

void ProtocolManager::RegisterProtocol(BaseProtocol *pProtocol) {
	if (MAP_HAS1(_activeProtocols, pProtocol->GetId()))
		return;
	if (MAP_HAS1(_deadProtocols, pProtocol->GetId()))
		return;
	_activeProtocols[pProtocol->GetId()] = pProtocol;
}

void ProtocolManager::UnRegisterProtocol(BaseProtocol *pProtocol) {
	if (MAP_HAS1(_activeProtocols, pProtocol->GetId()))
		_activeProtocols.erase(pProtocol->GetId());
	if (MAP_HAS1(_deadProtocols, pProtocol->GetId()))
		_deadProtocols.erase(pProtocol->GetId());
}

void ProtocolManager::EnqueueForDelete(BaseProtocol *pProtocol) {
	//	if (pProtocol->GetNearProtocol() == NULL) {
	//		FINEST("Enqueue for delete for protocol %s", STR(*pProtocol));
	//	}
	pProtocol->SetApplication(NULL);
	if (MAP_HAS1(_activeProtocols, pProtocol->GetId()))
		_activeProtocols.erase(pProtocol->GetId());
	if (!MAP_HAS1(_deadProtocols, pProtocol->GetId()))
		_deadProtocols[pProtocol->GetId()] = pProtocol;
}

uint32_t ProtocolManager::CleanupDeadProtocols() {
	uint32_t result = 0;
	while (_deadProtocols.size() > 0) {
		BaseProtocol *pBaseProtocol = MAP_VAL(_deadProtocols.begin());
		delete pBaseProtocol;
		pBaseProtocol = NULL;
		result++;
	}
	return result;
}

void ProtocolManager::Shutdown() {
    map<uint32_t, BaseProtocol*>::iterator activeProtIterator;
    for (activeProtIterator = _activeProtocols.begin(); activeProtIterator != _activeProtocols.end();) {
        // If the protocol is the TimerProtocol, don't delete it to be able to process
        // the closing of MP4 recording when RMS is stopped using Ctrl-C
        if (activeProtIterator->second->GetId() != 5 ) { 
            EnqueueForDelete(MAP_VAL(activeProtIterator++));
        } else {
            ++activeProtIterator;
        }
    }
}

BaseProtocol * ProtocolManager::GetProtocol(uint32_t id,
		bool includeDeadProtocols) {
	if (!includeDeadProtocols && MAP_HAS1(_deadProtocols, id))
		return NULL;
	if (MAP_HAS1(_activeProtocols, id))
		return _activeProtocols[id];
	if (MAP_HAS1(_deadProtocols, id))
		return _deadProtocols[id];
	return NULL;
}

const map<uint32_t, BaseProtocol *> & ProtocolManager::GetActiveProtocols() {
	return _activeProtocols;
}

void ProtocolManager::GetActiveProtocols(map<uint32_t, BaseProtocol *> &result,
		protocolManagerFilter_f filter) {
	result.clear();
	if (filter == NULL) {
		result = _activeProtocols;
		return;
	}

	FOR_MAP(_activeProtocols, uint32_t, BaseProtocol *, i) {
		if (!filter(MAP_VAL(i)))
			continue;
		result[MAP_VAL(i)->GetId()] = MAP_VAL(i);
	}
}

bool protocolManagerNetworkedProtocolsFilter(BaseProtocol *pProtocol) {
	IOHandler *pIOHandler = pProtocol->GetIOHandler();
	if ((pIOHandler == NULL)
			|| ((pIOHandler->GetType() != IOHT_TCP_CARRIER)
			&& (pIOHandler->GetType() != IOHT_UDP_CARRIER)))
		return false;
	return true;
}

bool protocolManagerNearProtocolsFilter(BaseProtocol *pProtocol) {
	return pProtocol->GetNearProtocol() == NULL;
}
