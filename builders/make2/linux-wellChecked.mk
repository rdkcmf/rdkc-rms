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
TOOLCHAIN_BASE=/disk2/opt/gcc-linaro-arm-linux-gnueabihf-2012.09-20120921_linux/
TOOLCHAIN_PREFIX=/bin/arm-linux-gnueabihf-

STRIP_PARAMS=-s -g -S -d --strip-unneeded

PLATFORM_INCLUDE = -I/disk2/opt/gcc-linaro-arm-linux-gnueabihf-2012.09-20120921_linux/arm-linux-gnueabihf/include/c++/4.7.2/ \
	-I/disk2/opt/gcc-linaro-arm-linux-gnueabihf-2012.09-20120921_linux/arm-linux-gnueabihf/include/
PLATFORM_LIBS = -L/disk2/opt/gcc-linaro-arm-linux-gnueabihf-2012.09-20120921_linux/arm-linux-gnueabihf/lib/ -lssl -lcrypto -ldl 
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_SHORT_ALIGNED \
	-DNET_EPOLL \
	-DGLOBALLY_ACCOUNT_BYTES
PLATFROM_COMPILE_SWITCHES = -O3 -fdata-sections -ffunction-sections -mcpu=cortex-a9 -mfpu=vfpv3 -march=armv7-a -mfloat-abi=hard -mlittle-endian
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections 

