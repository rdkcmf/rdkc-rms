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

# FYI: MIPS options: http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/MIPS-Options.html

#toolchain path
TOOLCHAIN_BASE=/disk2/opt/qvidium2
TOOLCHAIN_PREFIX=/bin/mipsisa32r2el-timesys-linux-gnu-

#TOOLCHAIN_BASE=/opt/timesys/linux-gnu/toolchain
#TOOLCHAIN_PREFIX=/bin/mipsisa32r2el-timesys-linux-gnu-

STRIP_PARAMS=-s -g -S -d --strip-unneeded

#PLATFORM_INCLUDE = -I/usr/local/ssl/include 
#PLATFORM_LIBS = -L/usr/local/ssl/lib -lssl -lcrypto -ldl
PLATFORM_INCLUDE = -I/disk2/opt/qvidium2/include -I/opt/timesys/linux-gnu/openssl-1.0.0a/include
PLATFORM_LIBS = --sysroot=/disk2/opt/qvidium2 -L /disk2/opt/qvidium2/lib -L/opt/timesys/linux-gnu/openssl-1.0.0a -lssl -lcrypto -ldl 

PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_EPOLL \
	-DGLOBALLY_ACCOUNT_BYTES \
	-DHAS_EPOLL_TIMERS

PLATFROM_COMPILE_SWITCHES = -O3 -fdata-sections -ffunction-sections -mhard-float
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections -mhard-float

TARGET_OS_NAME=Linux
TARGET_OS_VERSION=2.6
TARGET_OS_ARCH=mipsisa32r2el
TARGET_OS_UNAME="Generic Linux"
