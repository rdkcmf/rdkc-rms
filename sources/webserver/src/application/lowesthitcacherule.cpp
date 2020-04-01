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
#include "application/lowesthitcacherule.h"

using namespace webserver;

LowestHitCacheRule::LowestHitCacheRule() : _lowestHit(NULL) {
}

MemCache::StreamInfo *LowestHitCacheRule::Evaluate(MemCache::StreamInfo const *si, bool lastStream) const {
    if (_lowestHit == NULL || (si->_hit < _lowestHit->_hit))
        _lowestHit = si;
    if (lastStream)
        return const_cast<MemCache::StreamInfo *> (_lowestHit);
    return NULL;
}
