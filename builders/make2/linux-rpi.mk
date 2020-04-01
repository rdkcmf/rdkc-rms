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



TOOLCHAIN_BASE=/disk2/opt/raspi/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian
TOOLCHAIN_PREFIX=/bin/arm-linux-gnueabihf-

STRIP_PARAMS=-s -S -g -x -X

PLATFORM_INCLUDE = -I/disk2/opt/raspi/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/include 
PLATFORM_LIBS = -L/disk2/opt/raspi/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/lib -lssl -lcrypto -ldl -lrt 
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_EPOLL
PLATFROM_COMPILE_SWITCHES = -O3 -pipe -march=armv6j -mfpu=vfp -mfloat-abi=hard \
	-fdata-sections -ffunction-sections
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections

TARGET_OS_NAME=Debian
TARGET_OS_VERSION=7.0
TARGET_OS_ARCH=armv6l
TARGET_OS_UNAME="Linux localhost 3.6.11+ 371 PREEMPT Thu Feb 7 16:31:35 GMT 2013 armv6l GNU/Linux"

