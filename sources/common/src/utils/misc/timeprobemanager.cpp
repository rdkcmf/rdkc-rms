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


#include "utils/misc/timeprobemanager.h"
#include "utils/misc/timeprobe.h"
#include "utils/logging/logging.h"

map<string, uint64_t> TimeProbeManager::_sessTimestampMap;
map<string, uint64_t> TimeProbeManager::_storedVals;
map<string, TimeProbe*> TimeProbeManager::_timeProbeDict;
TimeProbeManager* TimeProbeManager::_instance = NULL;
string TimeProbeManager::_filePathPrefix;

TimeProbeManager::TimeProbeManager(){}

void TimeProbeManager::Initialize(const string& filePathPrefix)
{
    _filePathPrefix = filePathPrefix;
}

TimeProbeManager* TimeProbeManager::GetInstance()
{
    if(_instance == NULL){
        _instance = new TimeProbeManager();
    }
    return _instance;
}

TimeProbe* TimeProbeManager::GetTimeProbe(const string& timeProbeType, const string& sessionIDHash)
{
    if(_timeProbeDict.find(sessionIDHash) == _timeProbeDict.end()){
        // Create new one
        TimeProbe* t = new TimeProbe(timeProbeType, sessionIDHash, _filePathPrefix);
        _timeProbeDict[sessionIDHash] = t;
    }
        
    return _timeProbeDict[sessionIDHash];
}

const string& TimeProbeManager::GetFilePathPrefix()
{
    return _filePathPrefix;
}

uint64_t TimeProbeManager::GetSessionTimestampMS(const string& sessid, int init)
{
    double timestamp = 0;
    GETCLOCKS(timestamp, double);
    timestamp = timestamp/1000;//make milli from micro

    if(_sessTimestampMap.find(sessid) == _sessTimestampMap.end()){
        if(init){
            _sessTimestampMap[sessid] = (uint64_t)timestamp;
        }
        return 0;
    }
    
    return ((uint64_t)timestamp - _sessTimestampMap[sessid]);
}

void TimeProbeManager::DeleteSessionTimestamp(const string& sessid)
{
    if(_sessTimestampMap.find(sessid) != _sessTimestampMap.end()){
        _sessTimestampMap.erase(sessid);
    }
}

void TimeProbeManager::StoreTimestamp( const string& id, uint64_t ts ) {
    _storedVals[id] = ts;
}

uint64_t TimeProbeManager::GetStoredTimestamp( const string& id, bool remove ) {
    uint64_t ts = MAP_HAS1(_storedVals,id)?_storedVals[id]:0;
    if( remove ) {
        MAP_ERASE1(_storedVals, id);
    }
    return ts;
}
