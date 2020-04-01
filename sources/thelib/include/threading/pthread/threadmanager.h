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

#ifndef _THREADMANAGER_H
#define _THREADMANAGER_H
#include "common.h"
#include <pthread.h>

#define THREAD_PENDING 0
#define THREAD_LAUNCHED 1
#define THREAD_SHUTDOWN 2
#define THREAD_COMPLETED 3

typedef void *ThreadParams;
typedef void (*ThreadCallback)(Variant &, void *);

struct ThreadDetails {
    ThreadCallback asyncLaunchTask;
    ThreadCallback asyncCompletionTask;
    Variant parameters;
    uint32_t threadId;
    uint32_t status;
    void *requestingObject;
};

class ThreadManager {
private:
    static map<uint32_t, pthread_mutex_t> _locks;
    static map<uint32_t, pthread_t> _threads;
    static vector<ThreadDetails> _pendingTasks;
	static map<uint32_t, ThreadDetails> _runningTasks;
    static vector<ThreadDetails> _completingTasks;
    static pthread_mutex_t _mutex;
    static pthread_mutex_t _pendingMutex;
    static uint32_t GenerateTaskId();
public:
    static uint32_t RequestLockId();
    static bool UnregisterLockId(uint32_t lockId);
    static bool QueueAsyncTask(ThreadCallback asyncTask, 
            ThreadCallback asyncCompletionTask, 
            Variant launchParams, 
            void *requestingObject);
    static void EvaluateTasks();
    static bool TryLock(uint32_t lockId);
    static bool Unlock(uint32_t lockId);
    static bool Lock(uint32_t lockId);
	static void PurgeTasks();
};
#endif
