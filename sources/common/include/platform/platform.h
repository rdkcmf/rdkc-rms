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


#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "platform/endianess/endianness.h"

#ifdef OSX
#include "platform/osx/osxplatform.h"
#define Platform OSXPlatform
#endif /* OSX */

#ifdef LINUX
#include "platform/linux/linuxplatform.h"
#include "platform/linux/max.h"
#define Platform LinuxPlatform
#endif /* LINUX */

#ifdef FREEBSD
#include "platform/freebsd/freebsdplatform.h"
#define Platform FreeBSDPlatform
#endif /* FREEBSD */

#ifdef SOLARIS
#include "platform/solaris/solarisplatform.h"
#define Platform SolarisPlatform
#endif /* SOLARIS */

#ifdef WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "platform/windows/max.h"
#include "platform/windows/win32platform.h"
#define Platform Win32Platform
#endif /* WIN32 */

#ifdef ANDROID
#include "platform/android/max.h"
#include "platform/android/androidplatform.h"
#define Platform AndroidPlatform
#endif /* ANDROID */

#ifdef _ANDROID_DEBUG
#include <android/log.h>
#define DEBUGLOG(...) __android_log_print(ANDROID_LOG_FATAL, "rdkcms", __VA_ARGS__);
#else
#define DEBUGLOG(...)
#endif /* _ANDROID_DEBUG */


#ifdef ASSERT_OVERRIDE
#define o_assert(x) \
do { \
	if((x)==0) \
		exit(-1); \
} while(0)
#else
#define o_assert(x) assert(x)
#endif

#endif /* _PLATFORM_H */

