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
#include "application/cacherule.h"

using namespace webserver;

void CacheRule::Process(MemCache *memCache, size_t mappedBytes) {
    size_t ctr = 0;
    size_t cacheCount = memCache->GetCacheCount();

    FOR_MAP(memCache->_cache, string, MemCache::StreamInfo *, i) {
        MemCache::StreamInfo *si = MAP_VAL(i);
        assert(si);
        if (MemCache::StreamInfo * siTrg = Evaluate(si, cacheCount == ++ctr)) {
            memCache->ReleaseStreamResources(siTrg);
            delete siTrg;
            if (memCache->_currentCacheSize + mappedBytes > memCache->_cacheSize) //if cache still lacks available space
                continue;
            break;
        }
    }
}

CacheRule::~CacheRule() {
}
