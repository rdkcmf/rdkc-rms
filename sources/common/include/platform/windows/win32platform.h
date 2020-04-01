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
*/

#ifdef WIN32
#ifndef _WIN32PLATFORM_H
#define _WIN32PLATFORM_H

//define missing PRI_64 specifiers
#ifndef PRId64
#define PRId64 "I64d"
#endif /* PRId64 */

#ifndef PRIu64
#define PRIu64 "I64u"
#endif /* PRIu64 */

#ifndef PRIx64
#define PRIx64 "I64x"
#endif /* PRIx64 */

#ifndef PRIX64
#define PRIX64 "I64X"
#endif /* PRIX64 */

#ifndef PRId32
#define PRId32 "I32d"
#endif /* PRId32 */

#ifndef PRIu32
#define PRIu32 "I32u"
#endif /* PRIu32 */

#ifndef PRIx32
#define PRIx32 "I32x"
#endif /* PRIx32 */

#ifndef PRIX32
#define PRIX32 "I32X"
#endif /* PRIX32 */

#ifndef PRId16
#define PRId16 "hd"
#endif /* PRId16 */

#ifndef PRIu16
#define PRIu16 "hu"
#endif /* PRIu16 */

#ifndef PRIx16
#define PRIx16 "hx"
#endif /* PRIx16 */

#ifndef PRIX16
#define PRIX16 "hX"
#endif /* PRIX16 */

#ifndef PRId8
#define PRId8 "d"
#endif /* PRId8 */

#ifndef PRIu8
#define PRIu8 "u"
#endif /* PRIu8 */

#ifndef PRIx8
#define PRIx8 "x"
#endif /* PRIx8 */

#ifndef PRIX8
#define PRIX8 "X"
#endif /* PRIX8 */

#ifndef PRIz
#define PRIz "I"
#endif /* PRIz */

#include "platform/baseplatform.h"
#include <assert.h>
#include <time.h>
#include <io.h>
#include <Shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <fcntl.h>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdint.h>
#include <stdlib.h>
using namespace std;

/*typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned long int uint32_t;
typedef unsigned long long int uint64_t;
typedef char int8_t;
typedef short int int16_t;
typedef long int int32_t;
typedef long long int int64_t;*/
#define atoll atol

#ifdef COMPILE_STATIC
#define DLLEXP /*COMPILE_STATIC*/
#else
#define DLLEXP __declspec(dllexport)
#endif /* COMPILE_STATIC */

#define __func__ __FUNCTION__
#define COLOR_TYPE WORD
#define FATAL_COLOR FOREGROUND_INTENSITY | FOREGROUND_RED
#define ERROR_COLOR FOREGROUND_RED
#define WARNING_COLOR 6
#define INFO_COLOR FOREGROUND_GREEN | FOREGROUND_BLUE
#define DEBUG_COLOR 7
#define FINE_COLOR 8
#define FINEST_COLOR 8
#define NORMAL_COLOR 8
#define SET_CONSOLE_TEXT_COLOR(color) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color)

#define MSG_NOSIGNAL 0
#define READ_FD _read
#define WRITE_FD _write
#define CLOSE_SOCKET(fd) do{ if(((int)fd)>=0){shutdown(fd, SD_SEND); closesocket(fd);}fd=(SOCKET)-1;}while(0)
#define LASTSOCKETERROR WSAGetLastError()
#define SOCKERROR_EINPROGRESS			WSAEINPROGRESS
#define SOCKERROR_EAGAIN				WSAEWOULDBLOCK
#define SOCKERROR_ECONNRESET			WSAECONNRESET
#define SOCKERROR_ENOBUFS				WSAENOBUFS
#define SET_UNKNOWN 0
#define SET_READ 1
#define SET_WRITE 2
#define SET_TIMER 3
#define RESET_TIMER(timer,sec,usec)
#define FD_READ_CHUNK 32768
#define FD_WRITE_CHUNK FD_READ_CHUNK
#define PATH_SEPARATOR '\\'
#define LIBRARY_NAME_PATTERN "%s.dll"
#define LIB_HANDLER HMODULE
#define LOAD_LIBRARY(file,flags) UnicodeLoadLibrary((file))
#define OPEN_LIBRARY_ERROR "OPEN_LIBRARY_ERROR NOT IMPLEMENTED YET"
#define GET_PROC_ADDRESS(libHandler, procName) GetProcAddress((libHandler), (procName))
#define FREE_LIBRARY(libHandler) FreeLibrary((libHandler))
#define SRAND() srand((uint32_t)time(NULL));
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IRGRP 0000
#define S_IWGRP 0000
#define Timestamp struct tm
#define Timestamp_init {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define snprintf sprintf_s
#define pid_t DWORD
#define PIOFFT __int64
#define GetPid _getpid
#define PutEnv _putenv
#define TzSet _tzset
#define Chmod _chmod

#define gmtime_r(_p_time_t, _p_struct_tm) *(_p_struct_tm) = *gmtime(_p_time_t);

#define CLOCKS_PER_SECOND 1000000L
#define GETCLOCKS(result,type) \
do { \
    struct timeval ___timer___; \
    gettimeofday(&___timer___,NULL); \
    result=(type)___timer___.tv_sec*(type)CLOCKS_PER_SECOND+(type) ___timer___.tv_usec; \
}while(0);

#define GETMILLISECONDS(result) \
do { \
    struct timeval ___timer___; \
    gettimeofday(&___timer___,NULL); \
    result=(uint64_t)___timer___.tv_sec*1000+___timer___.tv_usec/1000; \
}while(0);

#define GETNTP(result) \
do { \
	struct timeval tv; \
	gettimeofday(&tv,NULL); \
	result=(((uint64_t)tv.tv_sec + 2208988800U)<<32)|((((uint32_t)tv.tv_usec) << 12) + (((uint32_t)tv.tv_usec) << 8) - ((((uint32_t)tv.tv_usec) * 1825) >> 5)); \
}while (0);

#define GETCUSTOMNTP(result,value) \
do { \
	struct timeval tv; \
	tv.tv_sec=(long)(value/CLOCKS_PER_SECOND); \
	tv.tv_usec=(long)(value-tv.tv_sec*CLOCKS_PER_SECOND); \
	result=(((uint64_t)tv.tv_sec + 2208988800U)<<32)|((((uint32_t)tv.tv_usec) << 12) + (((uint32_t)tv.tv_usec) << 8) - ((((uint32_t)tv.tv_usec) * 1825) >> 5)); \
}while (0);

typedef void (*SignalFnc)(void);

typedef struct _select_event {
	uint8_t type;
} select_event;

#define FD_COPY(f, t)   (void)(*(t) = *(f))

#define MSGHDR WSAMSG
#define IOVEC WSABUF
#define MSGHDR_MSG_IOV lpBuffers
#define MSGHDR_MSG_IOVLEN dwBufferCount
#define MSGHDR_MSG_IOVLEN_TYPE DWORD
#define MSGHDR_MSG_NAME name
#define MSGHDR_MSG_NAMELEN namelen
#define IOVEC_IOV_BASE buf
#define IOVEC_IOV_LEN len
#define IOVEC_IOV_BASE_TYPE CHAR
#define SENDMSG(s,msg,flags,sent) WSASendMsg(s,msg,flags,(LPDWORD)(sent),NULL,NULL)
#define sleep(x) Sleep((x)*1000)

#define ftell64 _ftelli64
#define fseek64 _fseeki64

DLLEXP void InstallCrashDumpHandler(const char *pPrefix);
DLLEXP string GetEnvVariable(const char *pEnvVarName);
DLLEXP void replace(string &target, string search, string replacement);
DLLEXP bool fileExists(string path);
DLLEXP string lowerCase(string value);
DLLEXP string upperCase(string value);
DLLEXP string changeCase(string &value, bool lowerCase);
DLLEXP string tagToString(uint64_t tag);
DLLEXP bool setMaxFdCount(uint32_t &current, uint32_t &max);
DLLEXP bool enableCoreDumps();
DLLEXP bool setFdJoinMulticast(SOCKET sock, string bindIp, uint16_t bindPort, string ssmIp, uint16_t sockFamily);
DLLEXP bool setFdCloseOnExec(int fd);
DLLEXP bool setFdNonBlock(SOCKET fd);
DLLEXP bool setFdNoSIGPIPE(SOCKET fd);
DLLEXP bool setFdKeepAlive(SOCKET fd, bool isUdp);
DLLEXP bool setFdNoNagle(SOCKET fd, bool isUdp);
DLLEXP bool setFdReuseAddress(SOCKET fd);
DLLEXP bool setFdTTL(SOCKET fd, uint8_t ttl);
DLLEXP bool setFdMulticastTTL(SOCKET fd, uint8_t ttl, uint16_t sockFamily);
DLLEXP bool setFdTOS(SOCKET fd, uint8_t tos);
DLLEXP bool setFdMaxSndRcvBuff(SOCKET fd);
DLLEXP bool setFdIPv6Only(SOCKET fd, bool v6Only);
DLLEXP bool setFdOptions(SOCKET fd, bool isUdp);
DLLEXP void killProcess(pid_t pid);
DLLEXP bool deleteFile(string path);
DLLEXP bool deleteFolder(string path, bool force);
DLLEXP bool createFolder(string path, bool recursive);
DLLEXP string getHostByName(string name);
DLLEXP bool isNumeric(string value);
DLLEXP void split(string str, string separator, vector<string> &result);
DLLEXP uint64_t getTagMask(uint64_t tag);
DLLEXP string generateRandomString(uint32_t length);
DLLEXP string wsaError2String(DWORD err);
DLLEXP void lTrim(string &value);
DLLEXP void rTrim(string &value);
DLLEXP void trim(string &value);
DLLEXP int8_t getCPUCount();
DLLEXP map<string, string> mapping(string str, string separator1, string separator2, bool trimStrings);
DLLEXP void splitFileName(string fileName, string &name, string &extension, char separator = '.');
DLLEXP double getFileModificationDate(string path);
DLLEXP string normalizePath(string base, string file);
DLLEXP bool listFolder(string path, vector<string> &result,
		bool normalizeAllPaths = true, bool includeFolders = false,
		bool recursive = true);
DLLEXP bool moveFile(string src, string dst);
DLLEXP bool copyFile(string src, string dst, bool overwrite);
DLLEXP bool isAbsolutePath(string &path);
DLLEXP void installSignal(int sig, SignalFnc pSignalFnc);
DLLEXP void installQuitSignal(SignalFnc pQuitSignalFnc);
DLLEXP void installConfRereadSignal(SignalFnc pConfRereadSignalFnc);
DLLEXP time_t timegm(struct tm *tm);
DLLEXP char *strptime(const char *buf, const char *fmt, struct tm *timeptr);
DLLEXP int strcasecmp(const char *s1, const char *s2);
DLLEXP int strncasecmp(const char *s1, const char *s2, size_t n);
DLLEXP int vasprintf(char **strp, const char *fmt, va_list ap);
DLLEXP int gettimeofday(struct timeval *tv, void* tz);
DLLEXP void InitNetworking();
DLLEXP HMODULE UnicodeLoadLibrary(string fileName);
DLLEXP int inet_aton(const char *pStr, struct in_addr *pRes);
#define getutctime() time(NULL)
DLLEXP time_t getlocaltime();
DLLEXP time_t gettimeoffset();
DLLEXP void GetFinishedProcesses(vector<pid_t> &pids, bool &noMorePids);
DLLEXP bool LaunchProcess(string fullBinaryPath, vector<string> &arguments, vector<string> &envVars, pid_t &pid);
DLLEXP void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);
#define strtoull _strtoui64
#ifdef HAS_LICENSE
DLLEXP int8_t getMACAddresses(vector<string> & addresses);
string getOSName(OSVERSIONINFOEX osInfoStruc, SYSTEM_INFO si);
string getUID();
string getGID();
int64_t getTotalSystemMemory();
void printError(string functionName);
string getKernelVersion();
size_t getWMIDetails(map<string, string> &details);
string ConvertBSTRToString(BSTR bstr);
wstring ConvertStringToLPWSTR(const string s);
#endif /* HAS_LICENSE */
#endif /* _WIN32PLATFORM_H */
#endif /* WIN32 */
