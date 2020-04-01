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

#include "application/phpengine.h"
#ifdef WIN32
#include <strsafe.h>
#else
extern char **environ;
#endif
#include "application/helper.h"

using namespace webserver;

#define ENVVAR_REDIRECTSTATUS       "REDIRECT_STATUS=true"
#define ENVVAR_REMOTEADDRESS        "REMOTE_ADDR="
#define ENVVAR_SCRIPTFILENAME       "SCRIPT_FILENAME="
#define ENVVAR_REQUESTMETHODPOST    "REQUEST_METHOD=POST"
#define ENVVAR_GATEWAYINTERFACE     "GATEWAY_INTERFACE=CGI/1.1"
#define ENVVAR_CONTENTTYPEAPP       "CONTENT_TYPE=application/x-www-form-urlencoded"
#define ENVVAR_CONTENTLENGTH        "CONTENT_LENGTH="

#ifdef WIN32
#define PHPCGI          "rms-phpengine\\php-cgi.exe"
#define PHPINI          "rms-phpengine\\php.ini"    
#else
#define PHPCGI          "rms-phpengine/php-cgi"
#endif

PHPEngine::PHPEngine() {
#ifdef WIN32
    _redirectFile = INVALID_HANDLE_VALUE;
    _stdInR = INVALID_HANDLE_VALUE;
    _stdInW = INVALID_HANDLE_VALUE;
#else
    _pipeHandle = -1;
    posix_spawn_file_actions_init(&_fileActions);
#endif
    _startRun = getutctime();
}

PHPEngine::~PHPEngine() {
#ifdef WIN32
    if (_redirectFile != INVALID_HANDLE_VALUE)
        CloseHandle(_redirectFile);
    if (_stdInR != INVALID_HANDLE_VALUE)
        CloseHandle(_stdInR);
    if (_stdInW != INVALID_HANDLE_VALUE)
        CloseHandle(_stdInW);
#else
    posix_spawn_file_actions_destroy(&_fileActions);
    if (_pipeHandle != -1) {
        close(_pipeFd[0]);
        close(_pipeFd[1]);
    }
#endif
    uint32_t elapsed = (uint32_t)(getutctime() - _startRun);
    DEBUG("CGI Call Duration: %"PRIu32" sec(s).", elapsed);  
}

#ifdef WIN32

bool PHPEngine::CopyEnvironmentVariable(LPTSTR currentVariable, 
    char const *value) {
    bool ret = SUCCEEDED(StringCchCopy(currentVariable, 4096, TEXT(value)));
    if (ret == false)
        WARN("Failed to copy environment variable: %s.", value);
    return ret;
}
#endif

bool PHPEngine::Run(string const &inputFile, string const &uriParams, 
    string const &outputFile, bool isGet, string const &program, 
    string const &clientIPAddress) {
    bool ret = false;
#ifdef WIN32
    program;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof (sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    //output of process will be directed to this file
    _redirectFile = CreateFile(STR(outputFile), FILE_APPEND_DATA, 
        FILE_SHARE_WRITE | FILE_SHARE_READ, &sa,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (_redirectFile == INVALID_HANDLE_VALUE) {
        FATAL("Could not redirect result to output file(error=%ul).", 
            GetLastError());
        return false;
    }

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD flags = CREATE_NO_WINDOW;

    ::ZeroMemory(&pi, sizeof (PROCESS_INFORMATION));
    ::ZeroMemory(&si, sizeof (STARTUPINFO));
    si.cb = sizeof (STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdError = NULL;
    si.hStdOutput = _redirectFile;

    TCHAR chNewEnv[4096];
    
    if (!isGet) {
        if (::CreatePipe(&_stdInR, &_stdInW, &sa, 0) == 0) {
            FATAL("Could not create pipe(error=%ul).", GetLastError());
            return false;
        }
        if (::SetHandleInformation(_stdInW, HANDLE_FLAG_INHERIT, 0) == 0) {
            FATAL("Could not set properties of standard input handle"
                "(error=%ul).", GetLastError());
            return false;
        }
        si.hStdInput = _stdInR;

        LPTSTR currentVariable = (LPTSTR) chNewEnv;

        if (!CopyEnvironmentVariable(currentVariable, ENVVAR_REDIRECTSTATUS))
            return false;

        currentVariable += lstrlen(currentVariable) + 1;
        string remoteAddress = string(ENVVAR_REMOTEADDRESS) + clientIPAddress;
        if (!CopyEnvironmentVariable(currentVariable, STR(remoteAddress)))
            return false;

        currentVariable += lstrlen(currentVariable) + 1;
        string script_filename = string(ENVVAR_SCRIPTFILENAME) + inputFile;
        if (!CopyEnvironmentVariable(currentVariable, STR(script_filename)))
            return false;

        currentVariable += lstrlen(currentVariable) + 1;
        if (!CopyEnvironmentVariable(currentVariable, ENVVAR_REQUESTMETHODPOST))
            return false;

        currentVariable += lstrlen(currentVariable) + 1;
        if (!CopyEnvironmentVariable(currentVariable, ENVVAR_GATEWAYINTERFACE))
            return false;

        currentVariable += lstrlen(currentVariable) + 1;
        if (!CopyEnvironmentVariable(currentVariable, ENVVAR_CONTENTTYPEAPP))
            return false;

        DWORD dwWritten = 0;
        //Write post data here? 
        if (!WriteFile(_stdInW, (LPCVOID) STR(uriParams), 
            (DWORD) uriParams.size(), &dwWritten, NULL)) {
            FATAL("Could not write to pipe(error=%ul).", GetLastError());
            return false;
        }

        currentVariable += lstrlen(currentVariable) + 1;
        char sizeBuffer[12];
        string content_length = string(ENVVAR_CONTENTLENGTH) + 
            string(_itoa((uint32_t) dwWritten, sizeBuffer, 10));
        if (!CopyEnvironmentVariable(currentVariable, STR(content_length)))
            return false;

        uint32_t prev = 0;
        uint32_t pvSize = 0;
        char const *pVars = GetEnvironmentStrings();
        do {
            if (pVars[pvSize] == '\0') {
                currentVariable += lstrlen(currentVariable) + 1;
                string c = string(pVars + prev, pVars + pvSize);
                if (!CopyEnvironmentVariable(currentVariable, STR(c)))
                    return false;
                // Skip the null terminator
                prev = pvSize + 1;
                if (pVars[pvSize + 1] == '\0') {
                    pvSize += 1; // consider the last null terminator
                    break;
                }
            }
            pvSize++;
        } while (pvSize < 0x0FFFFFFFF);

        // Terminate the block with a NULL byte. 
        currentVariable += lstrlen(currentVariable) + 1;
        *currentVariable = (TCHAR) 0;
    }

    string phpcgiPath = normalizePath(Helper::NormalizeFolder(PHPCGI), "");
    string phpiniPath = normalizePath(Helper::NormalizeFolder(PHPINI), "");
    string cmd = "\"" + phpcgiPath + "\"" + " -c \"" + phpiniPath + "\" \"" + 
        inputFile + "\" " + (isGet ? uriParams : "");
    ret = ::CreateProcess(NULL, (LPSTR) STR(cmd), NULL, NULL, TRUE, flags, 
        isGet ? NULL : (LPVOID) chNewEnv, NULL, &si, &pi);
    if (ret) {
        ::WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    } else
        FATAL("Could not spawn the PHP interpreter.");
#else
    int pid = 0;
    int status = 0;

    char **_ppArgs = NULL;
    char **_ppEnv = NULL;

    string fullBinaryPath = normalizePath(program, "");
    size_t pos = fullBinaryPath.find_last_of("\\/");
    if (pos == string::npos) {
        FATAL("Unable to launch php-cgi: (Invalid binary file path): %s\n", 
            STR(fullBinaryPath));
        return false;
    }
    fullBinaryPath = fullBinaryPath.substr(0, pos + 1) + PHPCGI;
    
    vector<string> arguments, envVars;
    ADD_VECTOR_END(arguments, STR(inputFile));
    ADD_VECTOR_END(arguments, (isGet ? STR(uriParams) : ""));

    ADD_VECTOR_BEGIN(arguments, STR(fullBinaryPath));
    _ppArgs = new char*[arguments.size() + 1];
    for (uint32_t i = 0; i < arguments.size(); i++) {
        _ppArgs[i] = new char[arguments[i].size() + 1];
        strcpy(_ppArgs[i], arguments[i].c_str());
    }
    _ppArgs[arguments.size()] = NULL;

    for (int i = 0; environ[i] != NULL; i++) {
        ADD_VECTOR_END(envVars, environ[i]);
    }

    if (!isGet) {
        ADD_VECTOR_END(envVars, ENVVAR_REDIRECTSTATUS);
        string remoteAddress = string(ENVVAR_REMOTEADDRESS) + clientIPAddress;
        ADD_VECTOR_END(envVars, remoteAddress);
        string scriptFilename = string(ENVVAR_SCRIPTFILENAME) + inputFile;
        ADD_VECTOR_END(envVars, STR(scriptFilename));
        ADD_VECTOR_END(envVars, ENVVAR_REQUESTMETHODPOST);
        ADD_VECTOR_END(envVars, ENVVAR_GATEWAYINTERFACE);
        ADD_VECTOR_END(envVars, ENVVAR_CONTENTTYPEAPP);
        char buffer[12] = {0};
        sprintf(buffer, "%d", (uint16_t) uriParams.size());
        string contentLength = string(ENVVAR_CONTENTLENGTH) + string(buffer);
        ADD_VECTOR_END(envVars, STR(contentLength));
    }

    if (envVars.size() > 0) {
        _ppEnv = new char*[envVars.size() + 1];
        for (uint32_t i = 0; i < envVars.size(); i++) {
            _ppEnv[i] = new char[envVars[i].size() + 1];
            strcpy(_ppEnv[i], envVars[i].c_str());
        }
        _ppEnv[envVars.size()] = NULL;
    }

    //redirect standard output to file and standard error to null device
    if (posix_spawn_file_actions_addopen(&_fileActions, STDOUT_FILENO, 
        STR(outputFile), O_WRONLY | O_CREAT, 0666) != 0) {
        FATAL("Could not redirect standard output to file: %s.", 
            STR(outputFile));
        return false;
    }
    if (posix_spawn_file_actions_addopen(&_fileActions, STDERR_FILENO, 
        "/dev/null", O_WRONLY | O_CREAT, 0666) != 0) {
        FATAL("Could not redirect standard error to null device.");
        return false;
    }
    if (!isGet) {
        if (pipe(_pipeFd) == -1) {
            FATAL("Could not open a pipe.");
            return false;
        }
        posix_spawn_file_actions_adddup2(&_fileActions, _pipeFd[0], 
            STDIN_FILENO);
        posix_spawn_file_actions_addclose(&_fileActions, _pipeFd[1]);
    }
    status = posix_spawn(&pid, STR(fullBinaryPath), &_fileActions, NULL, 
        _ppArgs, _ppEnv);
    if (status != 0) {
        FATAL("Could not spawn the PHP interpreter.");
        return false;
    }
    if (!isGet) {
        close(_pipeFd[0]);
        ssize_t bytesWritten = write(_pipeFd[1], STR(uriParams), 
            uriParams.size());
        if (bytesWritten){}
        close(_pipeFd[1]);
        _pipeHandle = -1;
    }
    wait(&pid);
    ret = (status == 0);
    IOBuffer::ReleaseDoublePointer(&_ppArgs);
    IOBuffer::ReleaseDoublePointer(&_ppEnv);
#endif
    return ret;
}
