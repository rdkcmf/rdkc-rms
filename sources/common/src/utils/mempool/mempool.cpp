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


#ifdef USE_MEM_POOL

#include "mempool/mempool.h"

MemoryPoolManager &MemoryPoolManager::GetInstance() {
	static MemoryPoolManager oInstance;
	return oInstance;
}

MemoryPoolManager::~MemoryPoolManager() {
	info();
	Cleanup();
	info();
	Shutdown();
	info();
}

void MemoryPoolManager::Register(MemoryPool* pMemoryPool) {
	_memoryPools[pMemoryPool->GetSize()] = pMemoryPool;
}

void MemoryPoolManager::UnRegister(MemoryPool* pMemoryPool) {
	_memoryPools.erase(pMemoryPool->GetSize());
}

void MemoryPoolManager::Cleanup() {
	for (map<size_t, MemoryPool*>::iterator i = _memoryPools.begin();
			i != _memoryPools.end(); i++) {
		(i->second)->Cleanup();
	}
}

void MemoryPoolManager::info() {
	printf("---------------\n");
	for (map<size_t, MemoryPool*>::iterator i = _memoryPools.begin();
			i != _memoryPools.end(); i++) {
		(i->second)->info();
	}
	printf("---------------\n");
}

MemoryPoolManager::MemoryPoolManager() {

}

void MemoryPoolManager::Shutdown() {
	while (_memoryPools.size() > 0) {
		delete _memoryPools.begin()->second;
	}
}

MemoryPool::MemoryPool(size_t size) {
	_pEntries = NULL;
	_created = 0;
	_used = 0;
	_size = size >= sizeof (MemPoolEntry) ? size : sizeof (MemPoolEntry);
	MemoryPoolManager::GetInstance().Register(this);
}

MemoryPool::~MemoryPool() {
	Cleanup();
	o_assert(_used == 0);
	MemoryPoolManager::GetInstance().UnRegister(this);
}

void * MemoryPool::Allocate() {
	_used++;
	void* pResult = _pEntries;
	if (pResult == NULL) {
		_created++;
		return ::operator new(_size, nothrow);
	} else {
		_pEntries = _pEntries->pNext;
		return pResult;
	}
}

void MemoryPool::Deallocate(void *p) {
	_used--;
	o_assert(p != NULL);
	MemPoolEntry* pEntry = (MemPoolEntry *) (p);
	pEntry->pNext = _pEntries;
	_pEntries = pEntry;
}

void MemoryPool::Cleanup() {
	MemPoolEntry *pCurrent = _pEntries;
	while (pCurrent != NULL) {
		_pEntries = _pEntries->pNext;
		::operator delete((void *) pCurrent);
		pCurrent = _pEntries;
		_created--;
	}
}

size_t MemoryPool::GetSize() {
	return _size;
}

void MemoryPool::info() {
	LOG_STATS('I');
}

#endif /* USE_MEM_POOL */


