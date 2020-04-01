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
TOOLCHAIN_PREFIX=

STRIP_PARAMS=-S

PLATFORM_INCLUDE = -I/opt/local/include 
PLATFORM_LIBS = -L/opt/local/lib -lssl -lcrypto -ldl -framework CoreFoundation -framework IOKit
PLATFORM_DEFINES = \
	-DOSX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_KQUEUE
PLATFROM_COMPILE_SWITCHES = -O3 -fdata-sections -ffunction-sections
PLATFROM_LINK_SWITCHES =  

TARGET_OS_NAME=MacOSX
TARGET_OS_VERSION=10.7
TARGET_OS_ARCH=x86_64
TARGET_OS_UNAME="Generic MacOSX"

