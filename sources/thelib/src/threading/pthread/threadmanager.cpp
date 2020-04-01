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

#ifdef THREAD_POSIX
#include "threading/pthread/threadmanager.h"

#define ERR_DESC(err) \
    do { \
        switch(retVal) { \
        case EBUSY: \
            FATAL("Mutex locked by other thread"); \
            break; \
        case EINVAL: \
            FATAL("Invalid state"); \
            break; \
        case EAGAIN: \
            FATAL("Maximum recursive locks exceeded"); \
            break; \
        case EDEADLK: \
            FATAL("Current thread already owns mutex"); \
            break; \
        case EPERM: \
            FATAL("Current thread does not own the mutex"); \
            break; \
        } \
    } while (0)

#define HAS_LOCK(x) \
    do { \
        if (!MAP_HAS1(_locks, lockId)) { \
            FATAL("Invalid LockId"); \
            return false; \
        } \
    } while(0)

string stateToString(uint32_t state);

#define SET_STATE(taskDetail, state) do { \
    string str1 = stateToString(taskDetail->status); \
    string str2 = stateToString(state); \
    WARN("Task id=%"PRIu32" state change:\t%s --> %s", taskDetail->threadId, STR(str1), STR(str2)); \
    taskDetail->status = state; \
    } while(0)

#define LOCK pthread_mutex_lock(&_mutex)
#define UNLOCK pthread_mutex_unlock(&_mutex)

#define LOCK_PENDING pthread_mutex_lock(&_pendingMutex)
#define UNLOCK_PENDING pthread_mutex_unlock(&_pendingMutex)

map<uint32_t, pthread_mutex_t> ThreadManager::_locks;
map<uint32_t, pthread_t> ThreadManager::_threads;
vector<ThreadDetails> ThreadManager::_pendingTasks;
//vector<ThreadDetails> ThreadManager::_runningTasks;
map<uint32_t, ThreadDetails> ThreadManager::_runningTasks;
vector<ThreadDetails> ThreadManager::_completingTasks;
pthread_mutex_t ThreadManager::_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ThreadManager::_pendingMutex = PTHREAD_MUTEX_INITIALIZER;

string stateToString(uint32_t state) {
    string retVal = "";
    switch(state) {
        case THREAD_LAUNCHED:
            retVal = "THREAD_LAUNCHED";
            break;
        case THREAD_PENDING:
            retVal = "THREAD_PENDING";
            break;
        case THREAD_SHUTDOWN:
            retVal = "THREAD_SHUTDOWN";
            break;
        case THREAD_COMPLETED:
            retVal = "THREAD_COMPLETED";
            break;
    }
    return retVal;
}

bool threadPending(ThreadDetails details) {
    return details.status == THREAD_PENDING;
}
bool threadLaunched(ThreadDetails details) {
    return details.status == THREAD_LAUNCHED;
}
bool threadCompleted(ThreadDetails details) {
    return details.status == THREAD_COMPLETED;
}
void *launchTask(void *params) {
    ThreadDetails *details = (ThreadDetails *) params;

    // Launch task here
    details->asyncLaunchTask(details->parameters, details->requestingObject);

    // change thread status
    details->status = (details->asyncCompletionTask != NULL) 
            ? THREAD_SHUTDOWN : THREAD_COMPLETED;
    //exit thread
    pthread_exit((void *) 0);
    return (void *)0;
}

uint32_t ThreadManager::RequestLockId() {
    uint32_t retVal = 0;
    do {
        retVal = rand() % (RAND_MAX - 1);
    } while (MAP_HAS1(_locks, retVal));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    _locks[retVal] = mutex;
    return retVal;
}
uint32_t ThreadManager::GenerateTaskId() {
    uint32_t retVal = 0;
    do {
        retVal = rand() % (RAND_MAX - 1);
    } while (MAP_HAS1(_threads, retVal));
//    pthread_t thread = 0;
    _threads[retVal] = 0;
    return retVal;
    
}
bool ThreadManager::UnregisterLockId(uint32_t lockId) {
    if (!TryLock(lockId)) {
        return false;
    }
    Unlock(lockId);
    _locks.erase(lockId);
    return true;
}
bool ThreadManager::QueueAsyncTask(ThreadCallback asyncTask, 
        ThreadCallback asyncCompletionTask, 
        Variant launchParams, 
        void *requestingObject) {
    LOCK_PENDING;
    uint32_t threadId = GenerateTaskId();
    ThreadDetails threadDetails;
    threadDetails.asyncLaunchTask = asyncTask;
    threadDetails.asyncCompletionTask = asyncCompletionTask;
    threadDetails.parameters = launchParams;
    threadDetails.status = THREAD_PENDING;
    threadDetails.threadId = threadId;
    threadDetails.requestingObject = requestingObject;
    ADD_VECTOR_END(_pendingTasks, threadDetails);
    UNLOCK_PENDING;
    return true;
}

void ThreadManager::PurgeTasks() {
    LOCK_PENDING;
    _pendingTasks.clear();
    UNLOCK_PENDING;
    LOCK;
    // cancel all launched threads
	FOR_MAP(_runningTasks, uint32_t, ThreadDetails, i) {
        if (MAP_VAL(i).status == THREAD_LAUNCHED) {
			pthread_cancel(_threads[MAP_VAL(i).threadId]);
        }
    }
    _runningTasks.clear();
    _threads.clear();
    UNLOCK;
}

void ThreadManager::EvaluateTasks() {
    LOCK_PENDING;
    FOR_VECTOR(_pendingTasks, i) {
		_runningTasks[_pendingTasks[i].threadId] = _pendingTasks[i];
    }
    _pendingTasks.clear();
    UNLOCK_PENDING;

	vector<uint32_t> completedTasksIds;

    LOCK;
	FOR_MAP(_runningTasks, uint32_t, ThreadDetails, i) {
		ThreadDetails &details = MAP_VAL(i);
        // ignore launched
        if (details.status == THREAD_LAUNCHED) 
            continue;
        
        // delete if thread is completed
		if (details.status == THREAD_COMPLETED) {
			MAP_ERASE1(_threads, details.threadId);
			ADD_VECTOR_END(completedTasksIds, details.threadId);
		}
        // launch pending threads
        if (details.status == THREAD_PENDING) {
            details.status = THREAD_LAUNCHED;
            pthread_t thread;
            if (pthread_create(&thread, NULL, launchTask, &details)) {
                // set back status to pending and try to launch on the next iteration
                details.status = THREAD_PENDING;
                _threads[details.threadId] = thread;
            }
        }
        
        // shutdown terminating threads
        if (details.status == THREAD_SHUTDOWN) {
            if (details.asyncCompletionTask != NULL) {
                details.asyncCompletionTask(details.parameters, details.requestingObject);
            }
            details.status = THREAD_COMPLETED;
            MAP_ERASE1(_threads, details.threadId);
			ADD_VECTOR_END(completedTasksIds, details.threadId);
        }
    }

	// remove completed tasks
	FOR_VECTOR(completedTasksIds, i) {
		_runningTasks.erase(completedTasksIds[i]);
	}
    UNLOCK;
}

bool ThreadManager::Lock(uint32_t lockId) {
    HAS_LOCK(lockId);
    pthread_mutex_t &mutex = _locks[lockId];

    int32_t retVal = pthread_mutex_lock(&mutex);
    if (retVal) {
        ERR_DESC(retVal);
        return false;
    } 
    
    return true;
}

bool ThreadManager::TryLock(uint32_t lockId) {
    HAS_LOCK(lockId);
    pthread_mutex_t &mutex = _locks[lockId];

    int32_t retVal = pthread_mutex_trylock(&mutex);
    if (retVal) {
        ERR_DESC(retVal);
        return false;
    } 
    
    return true;
}

bool ThreadManager::Unlock(uint32_t lockId) {
    HAS_LOCK(lockId);
    pthread_mutex_t &mutex = _locks[lockId];
    
    int32_t retVal = pthread_mutex_unlock(&mutex);
    if (retVal) {
        ERR_DESC(retVal);
        return false;
    } 
    
    return true;
}

#endif	/* THREAD_POSIX */
