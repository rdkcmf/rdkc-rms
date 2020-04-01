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

#ifndef _CACHERULE_H
#define _CACHERULE_H

#include "application/memcache.h"

namespace webserver {
    //Defines the rules on when to perform stream deletion to give way to other stream

    class CacheRule {
    public:
        void Process(MemCache *memCache, size_t mappedBytes);
        virtual ~CacheRule();
    protected:
        virtual MemCache::StreamInfo *Evaluate(MemCache::StreamInfo const *si, bool lastStream) const = 0;
    };
}

#endif

