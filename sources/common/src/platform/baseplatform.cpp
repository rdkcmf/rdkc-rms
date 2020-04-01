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

#include "common.h"

BasePlatform::BasePlatform() {

}

BasePlatform::~BasePlatform() {
}

#if defined FREEBSD || defined LINUX || defined OSX || defined SOLARIS

void GetFinishedProcesses(vector<pid_t> &pids, bool &noMorePids) {
	pids.clear();
	noMorePids = false;
	int statLoc = 0;
	while (true) {
		pid_t pid = waitpid(-1, &statLoc, WNOHANG);
		if (pid < 0) {
			int err = errno;
			if (err != ECHILD)
				WARN("waitpid failed %d %s", err, strerror(err));
			noMorePids = true;
			break;
		}
		if (pid == 0)
			break;
		ADD_VECTOR_END(pids, pid);
	}
}

bool LaunchProcess(string fullBinaryPath, vector<string> &arguments, vector<string> &envVars, pid_t &pid) {
#ifndef NO_SPAWN
	char **_ppArgs = NULL;
	char **_ppEnv = NULL;

	ADD_VECTOR_BEGIN(arguments, fullBinaryPath);
	_ppArgs = new char*[arguments.size() + 1];
	for (uint32_t i = 0; i < arguments.size(); i++) {
		_ppArgs[i] = new char[arguments[i].size() + 1];
		strcpy(_ppArgs[i], arguments[i].c_str());
	}
	_ppArgs[arguments.size()] = NULL;

	if (envVars.size() > 0) {
		_ppEnv = new char*[envVars.size() + 1];
		for (uint32_t i = 0; i < envVars.size(); i++) {
			_ppEnv[i] = new char[envVars[i].size() + 1];
			strcpy(_ppEnv[i], envVars[i].c_str());
		}
		_ppEnv[envVars.size()] = NULL;
	}

	if (posix_spawn(&pid, STR(fullBinaryPath), NULL, NULL, _ppArgs, _ppEnv) != 0) {
		int err = errno;
		FATAL("posix_spawn failed %d %s", err, strerror(err));
		IOBuffer::ReleaseDoublePointer(&_ppArgs);
		IOBuffer::ReleaseDoublePointer(&_ppEnv);
		return false;
	}

	IOBuffer::ReleaseDoublePointer(&_ppArgs);
	IOBuffer::ReleaseDoublePointer(&_ppEnv);
#endif /* NO_SPAWN */	
	return true;
}

bool setFdCloseOnExec(int fd) {
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		int err = errno;
		FATAL("fcntl failed %d %s", err, strerror(err));
		return false;
	}
	return true;
}

void killProcess(pid_t pid) {
	kill(pid, SIGKILL);
}

#ifdef HAS_STACK_DUMP
#include <execinfo.h>
/*
 * what signals are we going to intercept
 */
const int gInterceptedSignals[] = {
	SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};

/*
 * How many signals are we going to intercept?
 */
const size_t gInterceptedSignalsCount = sizeof (gInterceptedSignals) / sizeof (gInterceptedSignals[0]);

/*
 * the handler to be called on all signals
 */
void DumpStackWrapper(int signal, siginfo_t *pInfo, void *pIgnored);

/*
 * this class implements all the stack trace dumper functionality.
 * It is globally initialized and there will only be one instance at
 * any given time.
 */
class CrashDumpHandler {
private:
	//where to deposit the output
	int _outputFd;

	//Where to store the old/default signal handlers. Will need them to call
	//the default handler after we do our magic
	struct sigaction _oldSignals[gInterceptedSignalsCount];

	//the path to the output fd
	string _stackFilePath;

	//this flasg is set inside the signal handler. When true, don't delete
	//the stack file. That means we caught an actual signal, so that file
	//contains valuable data
	bool _deleteFile;

	//how many frames do we prepare for (last BACKTRACE_SIZE)
#define BACKTRACE_SIZE 1024
	void *_pBacktraceBuffer[BACKTRACE_SIZE];

	//stack variables used inside the signal handler. Remember, we are not allowed
	//to declare any new ones
	int _i;
public:

	CrashDumpHandler() {
		memset(&_oldSignals, 0, sizeof (_oldSignals));
		memset(_pBacktraceBuffer, 0, sizeof (_pBacktraceBuffer));
		_deleteFile = true;
	}

	virtual ~CrashDumpHandler() {
		if (_deleteFile)
			deleteFile(_stackFilePath);
	}

	bool Init(const char *pPrefix) {
		_stackFilePath = format("%s%"PRIu32"_%"PRIu64".txt",
				pPrefix != NULL ? pPrefix : "/tmp/rmscore_",
				(uint32_t) getpid(), (uint64_t) time(NULL));
		_outputFd = creat(_stackFilePath.c_str(),
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (_outputFd < 0) {
			FATAL("Unable to create stack file to `%s`", _stackFilePath.c_str());
			return false;
		}

		for (size_t i = 0; i < gInterceptedSignalsCount; i++) {
			//prepare a new signal handler
			struct sigaction newSignal;
			memset(&newSignal, 0, sizeof (newSignal));
			sigemptyset(&newSignal.sa_mask);
			newSignal.sa_sigaction = DumpStackWrapper;
			newSignal.sa_flags = SA_ONSTACK | SA_SIGINFO;

			//install the new signal handler and save the old one
			if (sigaction(gInterceptedSignals[i], &newSignal, &_oldSignals[i]) != 0) {
				FATAL("sigaction failed");
				return false;
			}

			//store the signal type on one of the members. Used later to search for
			//the default action
			_oldSignals[i].sa_flags = gInterceptedSignals[i];
		}
		return true;
	}

	void DumpStack(int signal, siginfo_t *pInfo, void *pCastedContext) {
		//no matter what, we don't delete the stack file anymore! the signal
		//was executed....
		_deleteFile = false;

		//get the stack
		_i = backtrace(_pBacktraceBuffer, BACKTRACE_SIZE);

		//dump it into the file
		backtrace_symbols_fd(_pBacktraceBuffer, _i, _outputFd);

		//done
		exit(signal);
	}
};

CrashDumpHandler gCrashDumper;

void InstallCrashDumpHandler(const char *pPrefix) {
	gCrashDumper.Init(pPrefix);
}

void DumpStackWrapper(int signal, siginfo_t *pInfo, void *pCastedContext) {
	gCrashDumper.DumpStack(signal, pInfo, pCastedContext);
}
#else /* HAS_STACK_DUMP */

void InstallCrashDumpHandler(const char *pPrefix) {
}

#endif /* HAS_STACK_DUMP */
#endif /* defined FREEBSD || defined LINUX || defined OSX || defined SOLARIS */
