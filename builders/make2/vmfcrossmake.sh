#!/bin/sh
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

export TOOLCHAIN_BASE="/opt/liquidimage_mips_11/mips-2014.05"
export TOOLCHAIN_PREFIX="/bin/mips-linux-gnu-"
export FULL_TOOLCHAIN=$TOOLCHAIN_BASE/$TOOLCHAIN_PREFIX

LIB_DIR=/opt/liquidimage_mips_11/mips-2014.05/mips-linux-gnu/libc/el/lib/

TARGET_OS_NAME=Linux
TARGET_OS_VERSION=mipselLinux
TARGET_OS_ARCH=mipsel
TARGET_OS_UNAME="MIPSEL GNU/Linux"

export PLATFORM_INCLUDE="-I$TOOLCHAIN_BASE/include"
export PLATFORM_LIBS="-L$TOOLCHAIN_BASE/lib -L$LIB_DIR -lssl -lcrypto -ldl -lrt -L./ -lvmf -lpthread -ldl"
#export PLATFORM_LIBS="-L$TOOLCHAIN_BASE/lib -L$LIB_DIR -lssl -lcrypto -ldl -lrt -L./ -lpthread -ldl"
export PLATFORM_DEFINES=" -DLINUX -DLITTLE_ENDIAN_BYTE_ALIGNED -DNET_EPOLL -DGLOBALLY_ACCOUNT_BYTES"
#export PLATFROM_COMPILE_SWITCHES="-static -shared-libgcc -O3 -fno-builtin -fdata-sections -ffunction-sections -EL"
export PLATFROM_COMPILE_SWITCHES="-static-libstdc++ -static-libgcc -O3 -Wall -g -fmessage-length=0 -pthread -fcommon -MMD -MP -EL -mhard-float -mno-micromips -mno-mips16"
export PLATFROM_LINK_SWITCHES="-static -EL "
export STRIP_PARAMS="-s -g -S -d --strip-unneeded"

make -f vmf-compile.mk
