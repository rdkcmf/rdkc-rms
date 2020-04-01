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



#ifndef _STATICMEMPOOL_H
#define _STATICMEMPOOL_H

#ifdef USE_MEM_POOL

#include <stdlib.h>
#include <assert.h>
#include <new>
#include <map>
#include <string>
using namespace std;
#include <stdio.h>

#ifdef DEBUG_MEM_POOL
#define LOG_STATS(x) printf("T: %c; S: %lu; C: %lu; U: %lu\n",x,_size,_created, _used)
#else
#define
#define LOG_STATS(x)
#endif

#define DECLARE_MEMORY_POOL_STRUCT(Cls) \
    static void* operator new(size_t s) { \
        o_assert(s == sizeof (Cls)); \
        return StaticMemoryPool<sizeof (Cls)>::GetInstance()->Allocate(); \
    } \
    static void operator delete(void* p) { \
        if (p != NULL) \
            StaticMemoryPool<sizeof (Cls)>::GetInstance()->Deallocate(p); \
    } \

#define DECLARE_MEMORY_POOL(Cls) \
public: \
DECLARE_MEMORY_POOL_STRUCT(Cls) \
private: \

class MemoryPool;

class MemoryPoolManager {
private:
	map<size_t, MemoryPool*> _memoryPools;
public:
	virtual ~MemoryPoolManager();

	static MemoryPoolManager &GetInstance();
	void Register(MemoryPool* pMemoryPool);
	void UnRegister(MemoryPool* pMemoryPool);
	void Cleanup();
	void info();
private:
	MemoryPoolManager();
	void Shutdown();
};

typedef struct _MemPoolEntry {
	struct _MemPoolEntry *pNext;
} MemPoolEntry;

class MemoryPool {
protected:
	MemPoolEntry *_pEntries;
	size_t _created;
	size_t _used;
	size_t _size;
protected:
	MemoryPool(size_t size);
public:
	virtual ~MemoryPool();
	void * Allocate();
	void Deallocate(void *p);
	void Cleanup();
	size_t GetSize();
	void info();
};

template<size_t size>
class StaticMemoryPool
: public MemoryPool {
private:
	static StaticMemoryPool<size> *_pInstance;
protected:

	StaticMemoryPool() : MemoryPool(size) {
	};
public:

	static StaticMemoryPool<size> * GetInstance() {
		if (_pInstance == NULL) {

			_pInstance = new StaticMemoryPool<size > ();
		}
		return _pInstance;
	}
};

template<size_t size> StaticMemoryPool<size> * StaticMemoryPool<size>::_pInstance = NULL;

#else
#define DECLARE_MEMORY_POOL_STRUCT(Cls)
#define DECLARE_MEMORY_POOL(Cls)
#endif /* USE_MEM_POOL */
#endif /* _STATICMEMPOOL_H */

