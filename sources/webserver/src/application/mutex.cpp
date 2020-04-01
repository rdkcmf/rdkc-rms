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
#include "application/mutex.h"

using namespace webserver;

Mutex::Mutex() {
#ifdef WIN32
    _mtx = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&_mtx, NULL);
#endif
}

void Mutex::Lock() {
#ifdef WIN32
    WaitForSingleObject(_mtx, INFINITE);
#else
    pthread_mutex_lock(&_mtx);
#endif
}

void Mutex::Unlock() {
#ifdef WIN32
    ReleaseMutex(_mtx);
#else
    pthread_mutex_unlock(&_mtx);
#endif
}

Mutex::~Mutex() {
#ifdef WIN32
    CloseHandle(_mtx);
#else
    pthread_mutex_destroy(&_mtx);
#endif
}
