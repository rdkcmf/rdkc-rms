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
TOOLCHAIN_BASE=/opt/FriendlyARM/toolschain/4.4.3
TOOLCHAIN_PREFIX=/bin/arm-linux-

STRIP_PARAMS=-s -g -S -d --strip-unneeded

PLATFORM_INCLUDE = -I/opt/FriendlyARM/toolschain/4.4.3/include
PLATFORM_LIBS = -L/opt/FriendlyARM/toolschain/4.4.3/lib/ -lssl -lcrypto -ldl 
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_SHORT_ALIGNED \
	-DNET_EPOLL \
	-DGLOBALLY_ACCOUNT_BYTES
PLATFROM_COMPILE_SWITCHES = -O3 -fdata-sections -ffunction-sections \
	-mcpu=arm926ej-s
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections 

TARGET_OS_NAME=Linux
TARGET_OS_VERSION=2.6
TARGET_OS_ARCH=arm92ej-s
TARGET_OS_UNAME="Generic Linux"

