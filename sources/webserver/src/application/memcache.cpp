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
#include "common.h"
#include "application/memcache.h"
#include "application/helper.h"
#include "application/cacherule.h"
#include "application/mutex.h"

using namespace webserver;

#define NOCACHELIMIT				0

MemCache::MemCache(CacheRule *rule) : _cacheSize(NOCACHELIMIT), _currentCacheSize(0),
_attempts(0), _hit(0), _rule(rule), _cacheMutex(new Mutex) {
}

MemCache::~MemCache() {

    FOR_MAP(_cache, string, StreamInfo *, i) {
        StreamInfo *si = MAP_VAL(i);
        if (si) {
            ReleaseStreamResources(si);
            delete si;
        }
    }
    if (_rule)
        delete _rule;
    if (_cacheMutex)
        delete _cacheMutex;
}

void MemCache::SetCacheSize(uint64_t cacheSize) {
    _cacheSize = cacheSize;
}

bool MemCache::Store(string const &filePath) {
    AutoMutex am(_cacheMutex);
    Helper::AtomicIncrement(&_attempts);
    StreamInfo *si = Retrieve(filePath);
    if (si != NULL) {
        Helper::AtomicIncrement(&_hit);
        Helper::AtomicIncrement(&si->_refCtr);
        Helper::AtomicIncrement(&si->_hit);
        return true;
    }

    uint64_t fileSize;
    FileHandle hFile = 0, hMappedFile = 0;
    void *mappedView;
#ifdef WIN32
    hFile = CreateFile(STR(filePath), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER result;
    if (!GetFileSizeEx(hFile, &result)) {
        ReleaseFileHandles(hFile, hMappedFile);
        return false;
    }
    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMappedFile == NULL) {
        ReleaseFileHandles(hFile, hMappedFile);
        return false;
    }
    fileSize = (uint64_t) result.QuadPart;
#else
#if LINUX    
    hFile = ::open(STR(filePath), O_RDONLY | O_LARGEFILE);
#else
    hFile = ::open(STR(filePath), O_RDONLY);
#endif
    if (hFile == -1)
        return false;
    struct stat64 statInfo;
    if (fstat64(hFile, &statInfo) < 0) {
        ReleaseFileHandles(hFile, hMappedFile);
        return false;
    }
    fileSize = statInfo.st_size;
#endif

    uint64_t offset = 0; //this should be passed when remapping (this is applicable when offset is more than 2gb)
    size_t mappedBytes = (size_t)fileSize; //should not always be the case especially for very large files

    if (offset > fileSize)
        return false;

    if (offset + mappedBytes > fileSize)
        mappedBytes = size_t(fileSize - offset);

    {
		AutoMutex am(_cacheMutex);
		if (_cacheSize > NOCACHELIMIT) {
			if (mappedBytes <= _cacheSize) { //if mappedBytes is not larger than our cache
				if (_currentCacheSize + mappedBytes > _cacheSize) //if cannot fit in cache
					_rule->Process(this, mappedBytes);
				if (_currentCacheSize + mappedBytes > _cacheSize) { //if still cannot fit in cache
					ReleaseFileHandles(hFile, hMappedFile);
					return false;
				}
			} else {
				ReleaseFileHandles(hFile, hMappedFile);
				return false;
			}
		}
	}

#ifdef WIN32
    DWORD offsetLow = DWORD(offset & 0xFFFFFFFF);
    DWORD offsetHigh = DWORD(offset >> 32);
    mappedView = ::MapViewOfFile(hMappedFile, FILE_MAP_READ, offsetHigh, offsetLow, mappedBytes);
    if (mappedView == NULL) {
        mappedBytes = 0;
        mappedView = NULL;
        ReleaseFileHandles(hFile, hMappedFile);
        return false;
    }
#else	
#if LINUX
    mappedView = ::mmap64(NULL, mappedBytes, PROT_READ, MAP_SHARED, hFile, offset);
#else
    mappedView = mmap(NULL, mappedBytes, PROT_READ, MAP_SHARED, hFile, offset);
#endif	//LINUX
    if (mappedView == MAP_FAILED) {
        ReleaseFileHandles(hFile, hMappedFile);
        return false;
    }
    ::madvise(mappedView, mappedBytes, MADV_NORMAL);
#endif	//WIN32

    si = new StreamInfo;
    assert(si);
    si->_filePath = filePath;
    si->_file = hFile;
    si->_fileSize = fileSize;
#ifdef WIN32
    si->_mappedFile = hMappedFile;
#endif
    si->_mappedBytes = mappedBytes;
    si->_mappedView = mappedView;
    si->_refCtr = 0;
    CacheStream(filePath, si);
    return true;
}

void MemCache::CacheStream(string const &filePath, StreamInfo *si) {
    AutoMutex am(_cacheMutex);
    _cache[filePath] = si;
    Helper::AtomicStep(&_currentCacheSize, si->_mappedBytes);
}

MemCache::StreamInfo *MemCache::Retrieve(string const &filePath) {
    CacheMap::iterator iter;
    return Retrieve(filePath, iter);
}

MemCache::StreamInfo *MemCache::Retrieve(string const &filePath,
        CacheMap::iterator &iter) {
    MemCache::StreamInfo *si = NULL;
    iter = _cache.find(filePath);
    if (iter != _cache.end())
        si = (*iter).second;
    return si;
}

void MemCache::Remove(string const &filePath) {
    CacheMap::iterator iter;
    AutoMutex am(_cacheMutex);
    MemCache::StreamInfo *si = Retrieve(filePath, iter);
    if (si == NULL) {
        return;
    }
    ReleaseStreamResources(si);
    delete si;
    _cache.erase(iter);
    Helper::AtomicStep(&_currentCacheSize, -(int) (si->_mappedBytes));
}

void MemCache::ReleaseStreamResources(StreamInfo *si) {
    //release file and mapped handles, including views if necessary
    if (si->_mappedView) {
#ifdef WIN32
        UnmapViewOfFile(si->_mappedView);
#else
        munmap(si->_mappedView, si->_mappedBytes);
#endif
        si->_mappedView = NULL;
    }

#ifdef WIN32
    ReleaseFileHandles(si->_file, si->_mappedFile);
#else
    ReleaseFileHandles(si->_file, 0);
#endif
    si->_fileSize = 0;
}

void MemCache::ReleaseFileHandles(FileHandle file, FileHandle mappedFile) {
    if (file) {
#ifdef WIN32
        CloseHandle(file);
#else
        close(file);
#endif
    }
    if (mappedFile) {
#ifdef WIN32
        CloseHandle(mappedFile);
#else
        close(mappedFile);
#endif
    }    
}

float MemCache::GetHitRate() const {
    if (_attempts == 0)
        return 100.0f;
    return (float) ((_attempts - _hit) / _attempts) * 100;
}

size_t MemCache::GetCacheCount() const {
    return _cache.size();
}

