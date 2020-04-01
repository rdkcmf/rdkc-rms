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


#toolchain paths
TOOLCHAIN_BASE=
TOOLCHAIN_PREFIX=
CCOMPILER=$(CC)
CXXCOMPILER=$(CXX)
#output settings
STATIC_LIB_SUFIX=.a
STATIC_LIB_PREFIX=lib
DYNAMIC_LIB_SUFIX=.so
DYNAMIC_LIB_PREFIX=lib

FPIC = -fPIC
#OPTIMIZATIONS = -O3
COMPILE_FLAGS = $(FPIC) $(OPTIMIZATIONS) $(CFLAGS)


#linking flags
dynamic_lib_flags = $(FPIC) $(OPTIMIZATIONS) -Wl,-soname,$(DYNAMIC_LIB_PREFIX)$(1)$(DYNAMIC_LIB_SUFIX) -Wl,-z,origin -Wl,-rpath,"\$$ORIGIN"
dynamic_exec_flags = $(FPIC) $(OPTIMIZATIONS) -Wl,-z,origin -Wl,-rpath,"\$$ORIGIN"

#compile switches
PLATFORM_DEFINES = \
	-DFREEBSD \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_KQUEUE 

SSL_BASE=/usr/local

