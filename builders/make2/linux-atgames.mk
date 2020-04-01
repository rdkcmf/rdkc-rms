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



#toolchain path
TOOLCHAIN_BASE=
TOOLCHAIN_PREFIX=arm-linux-gnueabihf-

STRIP_PARAMS=-s -g -S -d --strip-unneeded

#openssl was installed on the same prefix (/usr/local/arm-none-linux-gnueabi) so no need for additional -I and -L
PLATFORM_INCLUDE = -I/disk2/opt/atgames/include
PLATFORM_LIBS = -lssl -lcrypto -ldl -lrt -L/disk2/opt/atgames/lib
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_EPOLL
PLATFROM_COMPILE_SWITCHES = -O3 -march=armv7 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -fdata-sections -ffunction-sections
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections
