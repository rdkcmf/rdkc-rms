/*
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
 */

#ifndef _MEMCACHE_H
#define _MEMCACHE_H

namespace webserver {
    class CacheRule;
    class Mutex;

    class MemCache {
        friend class CacheRule;
    public:
#ifdef WIN32
        typedef void *FileHandle;
#else
        typedef int FileHandle;
#endif

        struct StreamInfo {
        public:
            string _filePath;
            uint64_t _fileSize;
            size_t _mappedBytes;
#ifdef WIN32
            void *_mappedFile;
#endif
            FileHandle _file;
            void *_mappedView;
            uint32_t _refCtr;
            uint32_t _hit;
        };

    private:
        /******Important: cache size of 0 means no limit ********/
        uint64_t _cacheSize;
        uint64_t _currentCacheSize;
        typedef map<string/*file path*/, StreamInfo * /*stream*/> CacheMap;
        CacheMap _cache;
        uint32_t _attempts;
        uint32_t _hit;
        CacheRule *_rule;
        mutable Mutex *_cacheMutex;
    public:
        explicit MemCache(CacheRule *rule);
        ~MemCache();
        void SetCacheSize(uint64_t cacheSize);
        bool Store(string const &filePath);
        StreamInfo *Retrieve(string const &filePath);
        float GetHitRate() const;
        size_t GetCacheCount() const;
    private:
        MemCache(MemCache const &);
        MemCache &operator=(MemCache const &);
        void Remove(string const &filePath);
        void ReleaseStreamResources(StreamInfo *si);
        StreamInfo *Retrieve(string const &filePath, CacheMap::iterator &iter);
        void ReleaseFileHandles(FileHandle file, FileHandle mappedFile);
        void CacheStream(string const &filePath, StreamInfo *si);
    };
}


#endif
