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


#include "utils/misc/timeprobe.h"
#include "utils/logging/logging.h"

#include <sstream>
#include <ctime>

uint64_t TimeProbe::_writeIDGenerator = 0;

TimeProbe::TimeProbe(const string& timeProbeType, const string& sessionIDHash, const string& filePathPrefix)
: _type(timeProbeType), _sessionIDHash(sessionIDHash), _filePathPrefix(filePathPrefix)
{

}

void TimeProbe::Probe(const string& probePointName)
{
    double timestamp = 0;
    GETCLOCKS(timestamp, double);
    _cache.push_back(ProbeData(probePointName, (uint64_t)timestamp));
    DEBUG("probe %s %"PRIu64, STR(probePointName), (uint64_t)timestamp);
}

void TimeProbe::Reset()
{
    _cache.clear();
}

void TimeProbe::Flush()
{
    WriteW3CFormat();
}

const string& TimeProbe::GetSessionIDHash() const
{
    return _sessionIDHash;
}

const string& TimeProbe::GetTimeProbeType() const
{
    return _type;
}

const string& TimeProbe::GetFilePathPrefix() const
{
    return _filePathPrefix;
}

void TimeProbe::WriteW3CFormat()
{
    std::ostringstream data;
    std::ostringstream fields;
    std::ostringstream values;
    std::ostringstream accumulatedvalues;
    std::ostringstream relativevalues;
    std::ostringstream filename;
    uint64_t first_ts = 0;
    uint64_t previous_ts = 0;
    bool isfirst = true;
    for(vector<ProbeData>::const_iterator i=_cache.begin();
        i!=_cache.end();++i){        
        fields << i->GetProbePointName() << "\t";
        values << i->GetTimeStamp() << "\t";
        if(isfirst){ 
            first_ts = previous_ts = i->GetTimeStamp(); 
            isfirst = false;
            accumulatedvalues << 0 << "\t";
            relativevalues << 0 << "\t";
        }else{
            double aval = ((double)((i->GetTimeStamp() - first_ts)/(double)(1000000)));
            double rval = ((double)((i->GetTimeStamp() - previous_ts)/(double)(1000000)));
            previous_ts = i->GetTimeStamp();
            accumulatedvalues << aval << "\t";
            relativevalues << rval << "\t";            
        }        
    }
    // Distinct File names are created each write. This is done by appending a write ID to the filename
    time_t t = time(0);   // get time now
    struct tm* now = localtime(&t);
    filename << _filePathPrefix << "_" << (now->tm_year + 1900) << (now->tm_mon + 1) 
             << now->tm_mday << now->tm_hour << now->tm_min << now->tm_sec << "_" << (_writeIDGenerator++) << "_" << _type << ".tsv";
    data <<"#Version: 1.0\n#Session: " 
         << _sessionIDHash << "\n#Fields:\n" << fields.str()
         << "\n" << values.str() << "\n" << accumulatedvalues.str() << "\n"
         << relativevalues.str() << "\n";

    // INFO("\n%s", STR(data.str()));
    
    File fileStream;
	if (!fileStream.Initialize(filename.str(), FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to open file: %s", STR(filename.str()));
	}

    string dataStr = data.str();
    fileStream.WriteString(dataStr);
}
