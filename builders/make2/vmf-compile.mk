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
FEATURES_DEFINES = -DHAS_APP_VMF \
	-DHAS_APP_LMINTERFACE \
	-DHAS_VMF \
	-DHAS_PROTOCOL_HTTP \
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
        -DHAS_PROTOCOL_DRM \
        -DHAS_MEDIA_MP3 \
        -DHAS_MEDIA_MP4 \
        -DHAS_MEDIA_FLV \
        -DHAS_MEDIA_TS \
        -DHAS_MEDIA_VOD \
        -DHAS_MEDIA_LST \
        -DHAS_RTSP_LAZYPULL \
        -DHAS_LLWM_AUTH \
        -DGLOBALLY_ACCOUNT_BYTES \
        -DHAS_LICENSE \
        -DHAS_LICENSE_MANAGER \
        -DHAS_LUA \
 	-DHAS_PROTOCOL_METADATA \
        -DHAS_V4L2 \
        -DV4L2_DEBUG

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
	-D_FILE_OFFSET_BITS=64

DEFINES = $(PLATFORM_DEFINES) $(FEATURES_DEFINES) $(LOGGING_DEFINES) $(TWEAKING_DEFINES) $(MISC_MISC_DEFINES)

#compile and link switches switches
GENERIC_COMPILE_SWITCHES = -Wall \
	-Werror \
	-Wno-error=literal-suffix \
	-fvisibility=hidden \
	-fno-strict-aliasing \
	-static-libgcc
C_COMPILE_SWITCHES = $(PLATFROM_COMPILE_SWITCHES) $(GENERIC_COMPILE_SWITCHES)
#CPP_COMPILE_SWITCHES = $(C_COMPILE_SWITCHES) -std=c++0x -static-libstdc++ -static-libgcc -O3 -Wall -fmessage-length=0 -pthread -fcommon -MMD -MP  -EL -mhard-float -mno-micromips -mno-mips16 #-fno-rtti
CPP_COMPILE_SWITCHES = $(C_COMPILE_SWITCHES) -std=c++0x -O3 -Wall -fmessage-length=0 -pthread -fcommon -MMD -MP  -EL -mhard-float -mno-micromips -mno-mips16 #-fno-rtti

GENERIC_LINK_SWITCHES =
LINK_SWITCHES = $(PLATFROM_LINK_SWITCHES) $(GENERIC_LINK_SWITCHES)

#includes
PROJECT_INCLUDES = -I$(PROJECT_BASE_PATH)/3rdparty/lua-dev \
	-I$(PROJECT_BASE_PATH)/3rdparty/tinyxml \
	-I$(PROJECT_BASE_PATH)/3rdparty/vmf/include \
	-I$(PROJECT_BASE_PATH)/sources/common/include \
	-I$(PROJECT_BASE_PATH)/sources/thelib/include \
        -I$(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/include \
	-I$(PROJECT_BASE_PATH)/sources/applications/lminterface/include \
	$(PLATFORM_INCLUDE)

#other libs
PROJECT_LIBS = $(PLATFORM_LIBS)

#C source files and objects
C_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/lua-dev -type f -name "*.c")
C_OBJS = $(C_SRCS:.c=.cobj.o)

#CPP source files
CPP_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/tinyxml -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/common/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/thelib/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/3rdparty/vmf -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/src -type f -name "*.cpp") \
	$(shell find $(PROJECT_BASE_PATH)/sources/applications/lminterface/src -type f -name "*.cpp")
CPP_OBJS = $(CPP_SRCS:.cpp=.cppobj.o)

#VMF_SRCS = $(shell find $(PROJECT_BASE_PATH)/3rdparty/vmf/src -type f -name "*.cpp")
#VMF_OBJS = $(VMF_SRCS:.cpp=.cppobj.o)

RDKCMEDIASERVER_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/rdkcmediaserver/src -type f -name "*.cpp")
RDKCMEDIASERVER_OBJS = $(RDKCMEDIASERVER_SRCS:.cpp=.cppobj.o)

C_OBJECTS_COMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(C_COMPILE_SWITCHES)
CPP_OBJECTS_COMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(CPP_COMPILE_SWITCHES) -I$(OUTPUT_STATIC) -include common #-fno-rtti
PRECOMPILE_FLAGS = $(DEFINES) $(PROJECT_INCLUDES) $(CPP_COMPILE_SWITCHES) -c -o $(OUTPUT_STATIC)/common.gch $(PROJECT_BASE_PATH)/sources/common/include/common.h

#targets
all: strip
#all: upx

upx: strip
	upx -9 -o $(OUTPUT_STATIC)/vmfwriter.upx $(OUTPUT_STATIC)/vmfwriter

strip: link
	@echo Create full binaries...
	@cp $(OUTPUT_STATIC)/vmfwriter $(OUTPUT_STATIC)/vmfwriter.full
	@echo Strip the binaries...
	@$(STRIP) $(STRIP_PARAMS) $(OUTPUT_STATIC)/vmfwriter

link: precompiled $(C_OBJS) $(CPP_OBJS) $(RDKCMEDIASERVER_OBJS) 
	@echo Linking $(OUTPUT_STATIC)/vmfwriter
	$(CXXCOMPILER) $(LINK_SWITCHES) -o $(OUTPUT_STATIC)/vmfwriter $(C_OBJS) $(CPP_OBJS) $(RDKCMEDIASERVER_OBJS) $(PROJECT_LIBS)

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

