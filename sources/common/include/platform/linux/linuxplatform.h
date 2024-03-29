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

#ifdef LINUX
#ifndef _LINUXPLATFORM_H
#define _LINUXPLATFORM_H

#include "platform/baseplatform.h"

#ifndef PRIz
#define PRIz "z"
#endif /* PRIz */

//platform includes
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <cctype>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <list>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <resolv.h>
#include <signal.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#ifndef NO_SPAWN
	#include <spawn.h>
#endif
#include <time.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ifaddrs.h>
using namespace std;


//platform defines
#define DLLEXP
#define HAS_MMAP 1
#define COLOR_TYPE const char *
#define FATAL_COLOR "\033[01;31m"
#define ERROR_COLOR "\033[22;31m"
#define WARNING_COLOR "\033[01;33m"
#define INFO_COLOR "\033[22;36m"
#define DEBUG_COLOR "\033[01;37m"
#define FINE_COLOR "\033[22;37m"
#define FINEST_COLOR "\033[22;37m"
#define NORMAL_COLOR "\033[0m"
#define SET_CONSOLE_TEXT_COLOR(color) fprintf(stdout,"%s",color)
#define READ_FD read
#define WRITE_FD write
#define SOCKET int32_t
#define LASTSOCKETERROR					errno
#define SOCKERROR_EINPROGRESS			EINPROGRESS
#define SOCKERROR_EAGAIN				EAGAIN
#define SOCKERROR_ECONNRESET			ECONNRESET
#define SOCKERROR_ENOBUFS				ENOBUFS
#define LIB_HANDLER void *
#define FREE_LIBRARY(libHandler) dlclose((libHandler))
#define LOAD_LIBRARY(file,flags) dlopen((file), (flags))
#define LOAD_LIBRARY_FLAGS RTLD_NOW | RTLD_LOCAL
#define OPEN_LIBRARY_ERROR STR(string(dlerror()))
#define GET_PROC_ADDRESS(libHandler, procName) dlsym((libHandler), (procName))
#define LIBRARY_NAME_PATTERN "lib%s.so"
#define PATH_SEPARATOR '/'
#define CLOSE_SOCKET(fd) do{ if((fd)>=0) { shutdown(fd, SHUT_WR); close(fd); } fd=-1; } while(0)
#define InitNetworking()
#define MAP_NOCACHE 0
#define MAP_NOEXTEND 0
#define FD_READ_CHUNK 32768
#define FD_WRITE_CHUNK FD_READ_CHUNK
#define SO_NOSIGPIPE 0
#define SET_UNKNOWN 0
#define SET_READ 1
#define SET_WRITE 2
#define SET_TIMER 3
#define FD_READ_CHUNK 32768
#define FD_WRITE_CHUNK FD_READ_CHUNK
#define RESET_TIMER(timer,sec,usec) timer.tv_sec=sec;timer.tv_usec=usec;
#define FD_COPY(src,dst) memcpy(dst,src,sizeof(fd_set));
#define SRAND() srand(time(NULL));
#define Timestamp struct tm
#define Timestamp_init {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PIOFFT off_t
#define GetPid getpid
#define PutEnv putenv
#define TzSet tzset
#define Chmod chmod

#define CLOCKS_PER_SECOND 1000000L
#define GETCLOCKS(result,type) \
do { \
    struct timeval ___timer___; \
    gettimeofday(&___timer___,NULL); \
    result=(type)___timer___.tv_sec*(type)CLOCKS_PER_SECOND+(type) ___timer___.tv_usec; \
}while(0);

#define GETMILLISECONDS(result) \
do { \
    struct timespec ___timer___; \
    clock_gettime(CLOCK_MONOTONIC, &___timer___); \
    result=(uint64_t)___timer___.tv_sec*1000+___timer___.tv_nsec/1000000; \
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
	tv.tv_sec=(uint64_t)value/CLOCKS_PER_SECOND; \
	tv.tv_usec=(uint64_t)value-tv.tv_sec*CLOCKS_PER_SECOND; \
	result=(((uint64_t)tv.tv_sec + 2208988800U)<<32)|((((uint32_t)tv.tv_usec) << 12) + (((uint32_t)tv.tv_usec) << 8) - ((((uint32_t)tv.tv_usec) * 1825) >> 5)); \
}while (0);

class LinuxPlatform
: public BasePlatform {
public:
	LinuxPlatform();
	virtual ~LinuxPlatform();
};

typedef void (*SignalFnc)(void);

typedef struct _select_event {
	uint8_t type;
} select_event;

#define MSGHDR struct msghdr
#define IOVEC iovec
#define MSGHDR_MSG_IOV msg_iov
#define MSGHDR_MSG_IOVLEN msg_iovlen
#define MSGHDR_MSG_IOVLEN_TYPE size_t
#define MSGHDR_MSG_NAME msg_name
#define MSGHDR_MSG_NAMELEN msg_namelen
#define IOVEC_IOV_BASE iov_base
#define IOVEC_IOV_LEN iov_len
#define IOVEC_IOV_BASE_TYPE uint8_t
#define SENDMSG(s,msg,flags,sent) sendmsg(s,msg,flags)

#define ftell64 ftello64
#define fseek64 fseeko64

void InstallCrashDumpHandler(const char *pPrefix);
string GetEnvVariable(const char *pEnvVarName);
void replace(string &target, string search, string replacement);
bool fileExists(string path);
string lowerCase(string value);
string upperCase(string value);
string changeCase(string &value, bool lowerCase);
string tagToString(uint64_t tag);
bool setMaxFdCount(uint32_t &current, uint32_t &max);
bool enableCoreDumps();
bool setFdJoinMulticast(SOCKET sock, string bindIp, uint16_t bindPort, string ssmIp, uint16_t sockFamily);
bool setFdCloseOnExec(int fd);
bool setFdNonBlock(SOCKET fd);
bool setFdNoSIGPIPE(SOCKET fd);
bool setFdKeepAlive(SOCKET fd, bool isUdp);
bool setFdNoNagle(SOCKET fd, bool isUdp);
bool setFdReuseAddress(SOCKET fd);
bool setFdTTL(SOCKET fd, uint8_t ttl);
bool setFdMulticastTTL(SOCKET fd, uint8_t ttl, uint16_t sockFamily);
bool setFdTOS(SOCKET fd, uint8_t tos);
bool setFdMaxSndRcvBuff(SOCKET fd);
bool setFdIPv6Only(SOCKET fd, bool v6Only);
bool setFdOptions(SOCKET fd, bool isUdp);
void killProcess(pid_t pid);
bool deleteFile(string path);
bool deleteFolder(string path, bool force);
bool createFolder(string path, bool recursive);
string getHostByName(string name);
bool getAllHostByName(string name, vector<string> &result, bool ipv4_only = false);
bool isNumeric(string value);
void split(string str, string separator, vector<string> &result);
uint64_t getTagMask(uint64_t tag);
string generateRandomString(uint32_t length);
void lTrim(string &value);
void rTrim(string &value);
void trim(string &value);
int8_t getCPUCount();
map<string, string> mapping(string str, string separator1, string separator2, bool trimStrings);
void splitFileName(string fileName, string &name, string &extension, char separator = '.');
double getFileModificationDate(string path);
string normalizePath(string base, string file);
bool listFolder(string path, vector<string> &result,
		bool normalizeAllPaths = true, bool includeFolders = false,
		bool recursive = true);
bool moveFile(string src, string dst);
bool copyFile(string src, string dst, bool overwrite);
bool isAbsolutePath(string &path);
void installSignal(int sig, SignalFnc pSignalFnc);
void installQuitSignal(SignalFnc pQuitSignalFnc);
void installConfRereadSignal(SignalFnc pConfRereadSignalFnc);
#define getutctime() time(NULL)
time_t getlocaltime();
time_t gettimeoffset();
void GetFinishedProcesses(vector<pid_t> &pids, bool &noMorePids);
bool LaunchProcess(string fullBinaryPath, vector<string> &arguments, vector<string> &envVars, pid_t &pid);
#ifdef HAS_LICENSE
int8_t getMACAddresses(vector<string> & addresses);
string getGID();
string getKernelVersion();
int64_t getTotalSystemMemory();
string getUID();
#endif /* HAS_LICENSE */

#endif /* _LINUXPLATFORM_H */
#endif /* LINUX */

