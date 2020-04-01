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
TOOLCHAIN_BASE=/disk2/opt/dm368-toolchain/mv_pro_5.0.0/montavista/pro/devkit/arm/v5t_le
TOOLCHAIN_PREFIX=/bin/arm_v5t_le-

STRIP_PARAMS=-s -g -S -d --strip-unneeded

#openssl was installed on the same prefix (/usr/local/arm-none-linux-gnueabi) so no need for additional -I and -L
PLATFORM_INCLUDE =
PLATFORM_LIBS = -lssl -lcrypto -ldl -lrt
PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_SHORT_ALIGNED \
	-DNET_EPOLL \
        -DGLOBALLY_ACCOUNT_BYTES 
PLATFROM_COMPILE_SWITCHES = -O3 -fdata-sections -ffunction-sections
PLATFROM_LINK_SWITCHES = -Wl,--gc-sections 

