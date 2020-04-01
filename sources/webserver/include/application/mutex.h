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

#ifndef _MUTEX_H
#define _MUTEX_H

namespace webserver {

    class Mutex {
    public:
#ifdef WIN32
#define MUTEX HANDLE
#else
#define MUTEX pthread_mutex_t
#endif
        Mutex();
        ~Mutex();
        void Lock();
        void Unlock();
    private:
        MUTEX _mtx;
    };

	class AutoMutex {
	private:
		Mutex *_mutex;
	public:
		explicit AutoMutex(Mutex *mutex) {
			_mutex = mutex;
			if (_mutex) {
				_mutex->Lock();
			}
		}
		~AutoMutex() {
			if (_mutex) {
				_mutex->Unlock();
			}
		}
	};
}

#endif
