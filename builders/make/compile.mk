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
CCOMPILER?=$(TOOLCHAIN_BASE)$(TOOLCHAIN_PREFIX)gcc
CXXCOMPILER?=$(TOOLCHAIN_BASE)$(TOOLCHAIN_PREFIX)g++

#output settings
OUTPUT_BASE = ./output
OUTPUT_BIN = $(OUTPUT_BASE)
OUTPUT_OBJ = $(OUTPUT_BASE)
OUTPUT_DYNAMIC = $(OUTPUT_BIN)/dynamic
OUTPUT_STATIC = $(OUTPUT_BIN)/static
dynamic_lib_name = $(OUTPUT_DYNAMIC)$(2)/$(DYNAMIC_LIB_PREFIX)$(1)$(DYNAMIC_LIB_SUFIX)
static_lib_name = $(OUTPUT_STATIC)$(2)/$(STATIC_LIB_PREFIX)$(1)$(STATIC_LIB_SUFIX)
dynamic_exec_name = $(OUTPUT_DYNAMIC)$(2)/$(1)
static_exec_name = $(OUTPUT_STATIC)$(2)/$(1)

#project paths
PROJECT_BASE_PATH=../..

FEATURES_DEFINES = \
	-DHAS_PROTOCOL_HTTP \
	-DHAS_PROTOCOL_RTMP \
	-DHAS_PROTOCOL_LIVEFLV \
	-DHAS_PROTOCOL_RTP \
	-DHAS_PROTOCOL_TS \
	-DHAS_PROTOCOL_VAR \
	-DHAS_LUA \
	-DHAS_MEDIA_MP3 \
	-DHAS_MEDIA_MP4 \
	-DHAS_MEDIA_FLV \
	-DHAS_PROTOCOL_CLI \
	-DHAS_PROTOCOL_HLS \
	-DHAS_PROTOCOL_RAWHTTPSTREAM


DEFINES = $(PLATFORM_DEFINES) $(FEATURES_DEFINES)

#library paths
SSL_INCLUDE=-I$(SSL_BASE)/include
SSL_LIB=-L$(SSL_BASE)/lib -lssl -lcrypto -ldl

#lua
LUA_INCLUDE=-I$(PROJECT_BASE_PATH)/3rdparty/lua-dev
LUA_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/lua-dev -type f -name "*.c")
LUA_OBJS = $(LUA_SRCS:.c=.lua.o)

#tinyxml
TINYXML_INCLUDE=-I$(PROJECT_BASE_PATH)/3rdparty/tinyxml
TINYXML_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/tinyxml -type f -name "*.cpp")
TINYXML_OBJS = $(TINYXML_SRCS:.cpp=.tinyxml.o)

#common
COMMON_INCLUDE=$(LUA_INCLUDE) $(TINYXML_INCLUDE) $(SSL_INCLUDE) -I$(PROJECT_BASE_PATH)/sources/common/include
COMMON_LIBS=-L$(OUTPUT_DYNAMIC) -llua -ltinyxml $(SSL_LIB)
COMMON_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/common/src -type f -name "*.cpp")
COMMON_OBJS = $(COMMON_SRCS:.cpp=.common.o)

#thelib
THELIB_INCLUDE=$(COMMON_INCLUDE) -I$(PROJECT_BASE_PATH)/sources/thelib/include
THELIB_LIBS=-L$(OUTPUT_DYNAMIC) -lcommon $(COMMON_LIBS)
THELIB_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/thelib/src -type f -name "*.cpp")
THELIB_OBJS = $(THELIB_SRCS:.cpp=.thelib.o)

#tests
TESTS_INCLUDE=$(THELIB_INCLUDE) -I$(PROJECT_BASE_PATH)/sources/tests/include
TESTS_LIBS=-L$(OUTPUT_DYNAMIC) -lthelib $(THELIB_LIBS)
TESTS_SRCS=$(shell find $(PROJECT_BASE_PATH)/sources/tests/src -type f -name "*.cpp")
TESTS_OBJS=$(TESTS_SRCS:.cpp=.tests.o)

#rdkcmediaserver
RDKCMEDIASERVER_INCLUDE=$(THELIB_INCLUDE) -I$(PROJECT_BASE_PATH)/sources/rdkcmediaserver/include
RDKCMEDIASERVER_LIBS=-L$(OUTPUT_DYNAMIC) -lthelib $(THELIB_LIBS)
RDKCMEDIASERVER_SRCS=$(shell find $(PROJECT_BASE_PATH)/sources/rdkcmediaserver/src -type f -name "*.cpp")
RDKCMEDIASERVER_OBJS_DYNAMIC=$(RDKCMEDIASERVER_SRCS:.cpp=.rdkcmediaserver_dynamic.o)
RDKCMEDIASERVER_OBJS_STATIC=$(RDKCMEDIASERVER_SRCS:.cpp=.rdkcmediaserver_static.o)

#targets
all: create_output_dirs lua common thelib applications tests rdkcmediaserver

include apps.mk

create_output_dirs:
	@mkdir -p $(OUTPUT_STATIC)
	@mkdir -p $(OUTPUT_DYNAMIC)

lua: create_output_dirs $(LUA_OBJS)
	$(CCOMPILER) -shared -o $(call dynamic_lib_name,lua,) $(call dynamic_lib_flags,lua) $(LUA_OBJS)

%.lua.o: %.c
	$(CCOMPILER) $(COMPILE_FLAGS) -c $< -o $@

tinyxml: lua $(TINYXML_OBJS)
	$(CXXCOMPILER) -shared -o $(call dynamic_lib_name,tinyxml,) $(call dynamic_lib_flags,tinyxml) $(TINYXML_OBJS)

%.tinyxml.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(COMMON_INCLUDE) -c $< -o $@

common: tinyxml $(COMMON_OBJS)
	$(CXXCOMPILER) -shared -o $(call dynamic_lib_name,common,) $(call dynamic_lib_flags,common) $(COMMON_OBJS) $(COMMON_LIBS)

%.common.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(COMMON_INCLUDE) -c $< -o $@

thelib: common $(THELIB_OBJS)
	$(CXXCOMPILER) -shared -o $(call dynamic_lib_name,thelib,) $(call dynamic_lib_flags,thelib) $(THELIB_OBJS) $(THELIB_LIBS)

%.thelib.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(THELIB_INCLUDE) -c $< -o $@

tests: thelib $(TESTS_OBJS)
	$(CXXCOMPILER) -o $(call dynamic_exec_name,tests,) $(call dynamic_exec_flags,tests) $(TESTS_OBJS) $(TESTS_LIBS)
	$(CXXCOMPILER) -o $(call static_exec_name,tests) $(call static_exec_flags,tests) \
		$(TESTS_OBJS) \
		$(LUA_OBJS) \
		$(TINYXML_OBJS) \
		$(COMMON_OBJS) \
		$(THELIB_OBJS) \
		$(SSL_LIB)

%.tests.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(TESTS_INCLUDE) -c $< -o $@

rdkcmediaserver: applications $(RDKCMEDIASERVER_OBJS_DYNAMIC) $(RDKCMEDIASERVER_OBJS_STATIC)
	$(CXXCOMPILER) -o $(call dynamic_exec_name,rdkcmediaserver,) $(call dynamic_exec_flags,rdkcmediaserver) $(RDKCMEDIASERVER_OBJS_DYNAMIC) $(RDKCMEDIASERVER_LIBS)
	$(CXXCOMPILER) -o $(call static_exec_name,rdkcmediaserver,) $(call static_exec_flags,rdkcmediaserver) \
		$(RDKCMEDIASERVER_OBJS_STATIC) \
		$(LUA_OBJS) \
		$(TINYXML_OBJS) \
		$(COMMON_OBJS) \
		$(THELIB_OBJS) \
		$(ALL_APPS_OBJS) \
		$(SSL_LIB)

	@cp $(PROJECT_BASE_PATH)/builders/cmake/rdkcmediaserver/rdkcmediaserver.lua $(OUTPUT_DYNAMIC)
	@cp $(PROJECT_BASE_PATH)/builders/cmake/rdkcmediaserver/rdkcmediaserver.lua $(OUTPUT_STATIC)

%.rdkcmediaserver_dynamic.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(RDKCMEDIASERVER_INCLUDE) -c $< -o $@

%.rdkcmediaserver_static.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) -DCOMPILE_STATIC $(ACTIVE_APPS) $(DEFINES) $(RDKCMEDIASERVER_INCLUDE) -c $< -o $@

clean:
	@rm -rfv $(OUTPUT_BASE)
	@sh cleanupobjs.sh

