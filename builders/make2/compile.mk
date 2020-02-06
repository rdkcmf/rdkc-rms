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
STRIP?=$(TOOLCHAIN_BASE)$(TOOLCHAIN_PREFIX)strip

#output
OUTPUT_BASE = ./output
OUTPUT_BIN = $(OUTPUT_BASE)
OUTPUT_STATIC = $(OUTPUT_BIN)/static

#project paths
PROJECT_BASE_PATH=../..

#project defines
FEATURES_DEFINES = -DHAS_PROTOCOL_HTTP \
        -DHAS_PROTOCOL_HTTP2 \
        -DHAS_PROTOCOL_RTMP \
        -DHAS_PROTOCOL_LIVEFLV \
        -DHAS_PROTOCOL_RTP \
        -DHAS_PROTOCOL_TS \
        -DHAS_PROTOCOL_VAR \
        -DHAS_PROTOCOL_CLI \
        -DHAS_PROTOCOL_RPC \
        -DHAS_PROTOCOL_HLS \
        -DHAS_PROTOCOL_HDS \
        -DHAS_PROTOCOL_MSS \
	-DHAS_PROTOCOL_DASH \
        -DHAS_PROTOCOL_DRM \
	-DHAS_PROTOCOL_METADATA \
	-DHAS_PROTOCOL_ASCIICLI \
        -DHAS_MEDIA_MP3 \
        -DHAS_MEDIA_MP4 \
        -DHAS_MEDIA_FLV \
        -DHAS_MEDIA_TS \
        -DHAS_MEDIA_VOD \
        -DHAS_MEDIA_LST \
        -DHAS_RTSP_LAZYPULL \
	-DHAS_LLWM_AUTH \
        -DGLOBALLY_ACCOUNT_BYTES \
	-DHAS_APP_RDKCROUTER \
        -DHAS_APP_LMINTERFACE \
	-DHAS_W3C_FOR_EWS \
        -DHAS_LUA \
	-DHAS_PROTOCOL_WEBRTC \
	-DHAS_PROTOCOL_WS \
	-DHAS_PROTOCOL_WS_FMP4 \
	-DSCTP_USE_SENDSPACE \
	-DHAS_H265 \
	-DWRTC_CAPAB_HAS_HEARTBEAT
	#-DWEBRTC_DEBUG
	#-DV4L2_DEBUG \
	#-DHAS_PROTOCOL_EXTERNAL \
        #-DHAS_SYSLOG \
        #-DHAS_STREAM_DEBUG

# Add v4l2 if this is not an IOS build
ifeq ($(IOS_TARGET_ARCH),)
	FEATURES_DEFINES := $(FEATURES_DEFINES) -DHAS_V4L2
else
	FEATURES_DEFINES := $(FEATURES_DEFINES) -DHAS_PROTOCOL_RAWMEDIA
endif

LOGGING_DEFINES = -DFILE_OVERRIDE \
	-DLINE_OVERRIDE \
	-DFUNC_OVERRIDE \
	-DASSERT_OVERRIDE
#-DSHORT_PATH_IN_LOGGER=43


TWEAKING_DEFINES = -DCreateRTCPPacket=CreateRTCPPacket_mystyle_only_once \
	-DFeedDataAudioMPEG4Generic=FeedDataAudioMPEG4Generic_one_by_one \
	-DHandleTSVideoData=HandleVideoData_version3

MISC_MISC_DEFINES = -DCOMPILE_STATIC \
	-D__USE_FILE_OFFSET64 \
	-D_FILE_OFFSET_BITS=64 \
	-DSCTP_PROCESS_LEVEL_LOCKS \
	-DSCTP_SIMPLE_ALLOCATOR \
	-D__Userspace__

# Add the needed parameters if IOS build
ifeq ($(IOS_TARGET_ARCH),)
	MISC_MISC_DEFINES := $(MISC_MISC_DEFINES) -D__Userspace_os_Linux
else
	MISC_MISC_DEFINES := $(MISC_MISC_DEFINES) -D__Userspace_os_Darwin \
	-DHAVE_SCONN_LEN \
	-U__APPLE__ \
	-D__APPLE_USE_RFC_2292__
endif

DEFINES = $(PLATFORM_DEFINES) $(FEATURES_DEFINES) $(LOGGING_DEFINES) $(TWEAKING_DEFINES) $(MISC_MISC_DEFINES)

#compile and link switches switches
GENERIC_COMPILE_SWITCHES = -Wall \
	-fvisibility=hidden \
	-fno-strict-aliasing \
	-Wno-unknown-pragmas

# Disable warnings on IOS during build
ifeq ($(IOS_TARGET_ARCH),)
	GENERIC_COMPILE_SWITCHES := $(GENERIC_COMPILE_SWITCHES) -Wno-error
endif

C_COMPILE_SWITCHES = $(PLATFROM_COMPILE_SWITCHES) $(GENERIC_COMPILE_SWITCHES)
CPP_COMPILE_SWITCHES = $(C_COMPILE_SWITCHES) -fno-rtti

GENERIC_LINK_SWITCHES =
LINK_SWITCHES = $(PLATFROM_LINK_SWITCHES) $(GENERIC_LINK_SWITCHES)

#includes
PROJECT_INCLUDES = -I$(PROJECT_BASE_PATH)/3rdparty/lua-dev \
	-I$(PROJECT_BASE_PATH)/3rdparty/tinyxml \
	-I$(PROJECT_BASE_PATH)/3rdparty/usrsctplib \
	-I$(PROJECT_BASE_PATH)/sources/common/include \
	-I$(PROJECT_BASE_PATH)/sources/thelib/include \
	-I$(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/include \
	-I$(PROJECT_BASE_PATH)/sources/applications/lminterface/include \
	-I$(PROJECT_BASE_PATH)/sources/v4l2dumper/include \
	-I$(PROJECT_BASE_PATH)/sources/tests/include \
	-I$(PROJECT_BASE_PATH)/sources/webserver/include \
	-I$(PROJECT_BASE_PATH)/3rdparty/webserver/linux/include \
	-I$(PROJECT_BASE_PATH)/sources/rdkcmediaserver/include \
	$(PLATFORM_INCLUDE)

#other libs
PROJECT_LIBS = $(PLATFORM_LIBS)
WEBSERVER_LIBS = -L$(PROJECT_BASE_PATH)/3rdparty/webserver/linux/lib \
	-lmicrohttpd -lgnutls -lgnutls-openssl -lgnutlsxx \
	-lnettle -lhogweed -lgmp -lz -liconv -lgcrypt -lgpg-error \
	-lcharset


#C source files and objects
C_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/lua-dev -type f -name "*.c") \
        $(shell find $(PROJECT_BASE_PATH)/3rdparty/usrsctplib -type f -name "*.c") \
	$(shell find $(PROJECT_BASE_PATH)/sources/common/src -type f -name "*.c")
C_OBJS = $(C_SRCS:.c=.cobj.o)

#CPP source files
CPP_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/tinyxml -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/common/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/thelib/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/applications/lminterface/src -type f -name "*.cpp")
CPP_OBJS = $(CPP_SRCS:.cpp=.cppobj.o)

RDKCMEDIASERVER_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/rdkcmediaserver/src -type f -name "*.cpp")
RDKCMEDIASERVER_OBJS = $(RDKCMEDIASERVER_SRCS:.cpp=.cppobj.o)

#WEBSERVER_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/webserver/src -type f -name "*.cpp")
#WEBSERVER_OBJS = $(WEBSERVER_SRCS:.cpp=.cppobj.o)

MP4WRITER_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/mp4writer/src -type f -name "*.cpp")
MP4WRITER_OBJS = $(MP4WRITER_SRCS:.cpp=.cppobj.o)

HLSTOOL_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/hlstool/src -type f -name "*.cpp")
HLSTOOL_OBJS = $(HLSTOOL_SRCS:.cpp=.cppobj.o)

TESTS_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/tests/src -type f -name "*.cpp")
TESTS_OBJS = $(TESTS_SRCS:.cpp=.cppobj.o)

ifeq ($(IOS_TARGET_ARCH),)
	V4L2DUMPER_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/v4l2dumper/src -type f -name "*.cpp")
	V4L2DUMPER_OBJS = $(V4L2DUMPER_SRCS:.cpp=.cppobj.o)
endif

C_OBJECTS_COMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(C_COMPILE_SWITCHES)
CPP_OBJECTS_COMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(CPP_COMPILE_SWITCHES) -I$(OUTPUT_STATIC) -fno-rtti

ifeq ($(IOS_TARGET_ARCH),)
	CPP_OBJECTS_COMPILE_FLAGS := $(CPP_OBJECTS_COMPILE_FLAGS) -include common
else
	CPP_OBJECTS_COMPILE_FLAGS := $(CPP_OBJECTS_COMPILE_FLAGS) -include-pch $(OUTPUT_STATIC)/common.gch
endif

PRECOMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(CPP_COMPILE_SWITCHES) -c -o $(OUTPUT_STATIC)/common.gch $(PROJECT_BASE_PATH)/sources/common/include/common.h

#targets
all: strip
#all: upx

upx: strip
ifeq ($(IOS_TARGET_ARCH),)
	upx -9 -o $(OUTPUT_STATIC)/rdkcmediaserver.upx $(OUTPUT_STATIC)/rdkcmediaserver
endif
	upx -9 -o $(OUTPUT_STATIC)/mp4writer.upx $(OUTPUT_STATIC)/mp4writer
	upx -9 -o $(OUTPUT_STATIC)/hlstool.upx $(OUTPUT_STATIC)/hlstool
	upx -9 -o $(OUTPUT_STATIC)/tests.upx $(OUTPUT_STATIC)/tests
ifeq ($(IOS_TARGET_ARCH),)
	upx -9 -o $(OUTPUT_STATIC)/v4l2dumper.upx $(OUTPUT_STATIC)/v4l2dumper
endif
#	upx -9 -o $(OUTPUT_STATIC)/webserver.upx $(OUTPUT_STATIC)/webserver

strip: link
	@echo Create full binaries...
ifeq ($(IOS_TARGET_ARCH),)
	@cp $(OUTPUT_STATIC)/rdkcmediaserver $(OUTPUT_STATIC)/rdkcmediaserver.full
endif
	@cp $(OUTPUT_STATIC)/mp4writer $(OUTPUT_STATIC)/mp4writer.full
	@cp $(OUTPUT_STATIC)/hlstool $(OUTPUT_STATIC)/hlstool.full
	@cp $(OUTPUT_STATIC)/tests $(OUTPUT_STATIC)/tests.full
ifeq ($(IOS_TARGET_ARCH),)
	@cp $(OUTPUT_STATIC)/v4l2dumper $(OUTPUT_STATIC)/v4l2dumper.full
endif
#	@cp $(OUTPUT_STATIC)/webserver $(OUTPUT_STATIC)/webserver.full
	@echo Strip the binaries...
ifeq ($(IOS_TARGET_ARCH),)
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/rdkcmediaserver
endif
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/mp4writer
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/hlstool
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/tests
ifeq ($(IOS_TARGET_ARCH),)
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/v4l2dumper
endif
#	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/webserver

link: precompiled $(C_OBJS) $(CPP_OBJS) $(RDKCMEDIASERVER_OBJS) $(MP4WRITER_OBJS) $(HLSTOOL_OBJS) $(TESTS_OBJS) $(V4L2DUMPER_OBJS) $(WEBSERVER_OBJS)
ifeq ($(IOS_TARGET_ARCH),)
	@echo Linking $(OUTPUT_STATIC)/rdkcmediaserver
	$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/rdkcmediaserver $(C_OBJS) $(CPP_OBJS) $(RDKCMEDIASERVER_OBJS) $(PROJECT_LIBS)
else
	@echo Creating library
	$(AR) rcs $(OUTPUT_STATIC)/librdkcms.a $(C_OBJS) $(CPP_OBJS) $(RDKCMEDIASERVER_OBJS)
endif
	@echo Linking $(OUTPUT_STATIC)/mp4writer
	@$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/mp4writer $(C_OBJS) $(CPP_OBJS) $(MP4WRITER_OBJS) $(PROJECT_LIBS)
	@echo Linking $(OUTPUT_STATIC)/hlstool
	@$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/hlstool $(C_OBJS) $(CPP_OBJS) $(HLSTOOL_OBJS) $(PROJECT_LIBS)
	@echo Linking $(OUTPUT_STATIC)/tests
	@$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/tests $(C_OBJS) $(CPP_OBJS) $(TESTS_OBJS) $(PROJECT_LIBS)
ifeq ($(IOS_TARGET_ARCH),)
	@echo Linking $(OUTPUT_STATIC)/v4l2dumper
	@$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/v4l2dumper $(C_OBJS) $(CPP_OBJS) $(V4L2DUMPER_OBJS) $(PROJECT_LIBS)
endif
#	@echo Linking $(OUTPUT_STATIC)/webserver
#	$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/webserver $(C_OBJS) $(CPP_OBJS) $(WEBSERVER_OBJS) $(PROJECT_LIBS) $(WEBSERVER_LIBS)

precompiled: create_output_dirs
	@echo
	@echo "C compiler: $(CCOMPILER)"
	@echo
	@echo "C++ compiler: $(CXXCOMPILER)"
	@echo
	@echo Precompile flags:
	@echo $(PRECOMPILE_FLAGS)
	@echo
	@echo C compile flags
	@echo $(C_OBJECTS_COMPILE_FLAGS)
	@echo
	@echo CPP compile flags
	@echo $(CPP_OBJECTS_COMPILE_FLAGS)
	@echo
	@echo Link switches
	@echo $(LINK_SWITCHES)
	@echo
	@echo
	@echo "Create precompiled header..."
	@$(CXXCOMPILER) $(PRECOMPILE_FLAGS)

create_output_dirs:
	@echo Create output dir: $(OUTPUT_STATIC)
	@mkdir -p $(OUTPUT_STATIC)

%.cobj.o: %.c
	@echo [C] $<
	@$(CCOMPILER) $(C_OBJECTS_COMPILE_FLAGS) -c $< -o $@

%.cppobj.o: %.cpp
	@echo [CPP] $<
	@$(CXXCOMPILER) $(CPP_OBJECTS_COMPILE_FLAGS) -c $< -o $@

clean:
	@echo Begin cleanup
	@rm -rf $(OUTPUT_BASE)
	@sh cleanupobjs.sh
	@echo Cleanup done

