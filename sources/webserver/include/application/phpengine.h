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


#ifndef _PHPENGINE_H
#define _PHPENGINE_H

#include "common.h"

namespace webserver {

    class PHPEngine {
    private:
#ifdef  WIN32
        HANDLE _redirectFile;
        HANDLE _stdInR;
        HANDLE _stdInW;
#else
        int _pipeHandle;
        int _pipeFd[2];
        posix_spawn_file_actions_t _fileActions;
#endif
        time_t _startRun;
    public:
        PHPEngine();
        ~PHPEngine();
        bool Run(string const &inputFile, string const &uriParams, 
            string const &outputFile, bool isGet, string const &program, 
            string const &clientIPAddress);
    private:
#ifdef WIN32
        bool CopyEnvironmentVariable(LPTSTR currentVariable, char const *value);
#endif
    };
}

#endif //_PHPENGINE_H
