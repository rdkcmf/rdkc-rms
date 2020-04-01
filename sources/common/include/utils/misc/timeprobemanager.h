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



#ifndef _TIMEPROBEMANAGER_H
#define	_TIMEPROBEMANAGER_H

#include <string>

#include "utils/misc/timeprobe.h"
 
class TimeProbeManager
{
private:
    // TimeProbe is created per session
    static map<string, TimeProbe*> _timeProbeDict;
    static TimeProbeManager* _instance;
    static string _filePathPrefix;
    // For session timestamping <session id, timestamp>
    static map<string, uint64_t> _sessTimestampMap;
    // Map for temp storage of measured values
    static map<string, uint64_t> _storedVals;
private:
    TimeProbeManager();
public:
    static void Initialize(const string& filePathPrefix);
    static TimeProbeManager* GetInstance();
    static TimeProbe* GetTimeProbe(const string& timeProbeType, const string& sessionIDHash);
    static const string& GetFilePathPrefix();
    static uint64_t GetSessionTimestampMS(const string& sessid, int init);
    static void DeleteSessionTimestamp(const string& sessid);
    static size_t GetSessionCount() {return _sessTimestampMap.size(); };
    static void StoreTimestamp( const string& id, uint64_t ts );
    static uint64_t GetStoredTimestamp( const string& id, bool remove );
};

#ifdef TIMEPROBE_DEBUG

#define PROBE_INIT(filePathPrefix) \
    do { \
        TimeProbeManager::GetInstance()->Initialize(filePathPrefix); \
    } while(0)

#define PROBE_POINT(Type, Sess, Point, Once) \
    do { \
        static bool probed = false; \
        if(!probed){ \
            TimeProbeManager::GetInstance()->GetTimeProbe(Type, Sess)->Probe(Point); \
            if(Once){ \
                probed = true; \
            } \
        } \
    } while(0)

#define PROBE_FLUSH(Type, Sess, Once) \
    do { \
        static bool flushed = false; \
        if(!flushed){ \
            TimeProbeManager::GetInstance()->GetTimeProbe(Type, Sess)->Flush(); \
            if(Once){ \
                flushed = true; \
            } \
        } \
    } while(0)

#define PROBE_RESET(Type, Sess, Once) \
    do { \
        static bool reset = false; \
        if(!reset){ \
            TimeProbeManager::GetInstance()->GetTimeProbe(Type, Sess)->Reset(); \
            if(Once){ \
                reset = true; \
            } \
        } \
    } while(0)
#else //TIMEPROBE_DEBUG
#define PROBE_INIT(filePathPrefix)
#define PROBE_POINT(Type, Sess, Point, Once)
#define PROBE_FLUSH(Type, Sess, Once)
#define PROBE_RESET(Type, Sess, Once)
#endif //TIMEPROBE_DEBUG

#define PROBE_TRACK_INIT_MS(sessid) TimeProbeManager::GetSessionTimestampMS(sessid, 1)
#define PROBE_TRACK_TIME_MS(sessid) TimeProbeManager::GetSessionTimestampMS(sessid, 0)
#define PROBE_CLEAR_TIME(sessid) TimeProbeManager::DeleteSessionTimestamp(sessid)
#define PROBE_COUNT() TimeProbeManager::GetSessionCount()
#define PROBE_STORE_TIME(sessid,ts) TimeProbeManager::StoreTimestamp(sessid, ts)
#define PROBE_GET_STORED_TIME(sessid,remove) TimeProbeManager::GetStoredTimestamp(sessid,remove)

#endif // _TIMEPROBEMANAGER_H
