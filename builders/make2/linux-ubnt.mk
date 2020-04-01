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



TOOLCHAIN_BASE=/opt/cross/toolchains/ubnt
TOOLCHAIN_PREFIX=/bin/arm-unknown-linux-uclibcgnueabi-

STRIP_PARAMS=-s -S -g -x -X

PLATFORM_INCLUDE =  
PLATFORM_LIBS = -lssl -lcrypto -ldl -lrt 
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_EPOLL
PLATFROM_COMPILE_SWITCHES = -O3 -pipe -fdata-sections -ffunction-sections
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections

TARGET_OS_NAME=Linux
TARGET_OS_VERSION=2.6.38
TARGET_OS_ARCH=armv6l
TARGET_OS_UNAME="Linux AirCamDomeIR 2.6.38.8 1 PREEMPT Tue Oct 22 10:17:25 UTC 2013 armv6l GNU/Linux"

