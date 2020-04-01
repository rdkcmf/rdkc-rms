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



#ifndef _TIMEPROBE_H
#define	_TIMEPROBE_H

#include <string>

#include "platform/platform.h"

class TimeProbe
{
private:
    class ProbeData
    {
    private:
        string _probePointName;
        uint64_t _timestamp;
    public:
        ProbeData(const string& probePointName, uint64_t timestamp)
        :_probePointName(probePointName), _timestamp(timestamp){}
        inline const string& GetProbePointName() const { return _probePointName; }
        inline uint64_t GetTimeStamp() const { return _timestamp; }
    };
private:
    static uint64_t _writeIDGenerator;
private:
    string _type;
    string _sessionIDHash;
    vector<ProbeData> _cache;
    string _filePathPrefix;
public:
    TimeProbe(const string& timeProbeType, const string& sessionIDHash, const string& filePathPrefix);
    // Probe the time on a specific point marked by name
    void Probe(const string& probePointName);
    // Reset the cache
    void Reset();
    // Write to file
    void Flush();
    const string& GetSessionIDHash() const;
    const string& GetTimeProbeType() const;
    const string& GetFilePathPrefix() const;
    // Helpers
    void WriteW3CFormat();
};


#endif // _TIMEPROBE_H
