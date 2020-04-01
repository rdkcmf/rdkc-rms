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
LOCAL_PATH := ../../

HAS_NEON=false
HAS_VFP=true
DISABLE_TEXTREL=false
ARCH64=false

ifeq ($(HAS_NEON), true)
	HAS_VFP=true
endif

ifeq ($(TARGET_ARCH_ABI),armeabi)
	LOCAL_ARM_NEON=false
	OPENSSL_DIR=$(OPENSSL_DIR_ARM)
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON=$(HAS_NEON)
	OPENSSL_DIR=$(OPENSSL_DIR_ARM_V7)
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
	OPENSSL_DIR=$(OPENSSL_DIR_ARM_64)
	HAS_VFP=false
	ARCH64=true
	DISABLE_TEXTREL=true
endif
ifeq ($(TARGET_ARCH_ABI),x86)
	OPENSSL_DIR=$(OPENSSL_DIR_X86)
	HAS_VFP=false
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
	OPENSSL_DIR=$(OPENSSL_DIR_X86_64)
	HAS_VFP=false
	ARCH64=true
endif
ifeq ($(TARGET_ARCH_ABI),mips)
	OPENSSL_DIR=$(OPENSSL_DIR_MIPS)
	HAS_VFP=false
	DISABLE_TEXTREL=true
endif
ifeq ($(TARGET_ARCH_ABI),mips64)
	OPENSSL_DIR=$(OPENSSL_DIR_MIPS_64)
	HAS_VFP=false
	DISABLE_TEXTREL=true
	ARCH64=true
endif

PLATFORM_DEFINES = \
	-DANDROID \
	-DLITTLE_ENDIAN_SHORT_ALIGNED \
	-DNET_EPOLL \
	-DGLOBALLY_ACCOUNT_BYTES \
	-DNO_REUSEPORT

FEATURES_DEFINES = -DHAS_PROTOCOL_HTTP \
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
	-DHAS_PROTOCOL_DASH \
	-DHAS_PROTOCOL_DRM \
	-DHAS_PROTOCOL_METADATA \
	-DHAS_PROTOCOL_EXTERNAL \
	-DHAS_PROTOCOL_ASCIICLI \
	-DHAS_MEDIA_MP3 \
	-DHAS_MEDIA_MP4 \
	-DHAS_MEDIA_FLV \
	-DHAS_MEDIA_TS \
	-DHAS_MEDIA_VOD \
	-DHAS_MEDIA_LST \
	-DHAS_RTSP_LAZYPULL \
	-DHAS_LLWM_AUTH \
	-DHAS_LUA \
	-DHAS_LICENSE \
	-DHAS_LICENSE_MANAGER \
	-DHAS_APP_LMINTERFACE \
	-DHAS_APP_RDKCROUTER \
	-DHAS_UDS \
	-DHAS_RTSP_LAZYPULL \
	-DHAS_PROTOCOL_WS \
	-DHAS_PROTOCOL_WS_FMP4 \
	-DSCTP_USE_SENDSPACE \
	-DHAS_H265 \
	-DHAS_PROTOCOL_WEBRTC \
	-DLUA_USE_MKSTEMP \
	-DHAS_PROTOCOL_RAWHTTPSTREAM \
	-DHAS_PROTOCOL_RAWMEDIA 
#	-DUDS_DEBUG
#	-DHAS_WEBSERVER 
#	-DBENCHMARK \
#	-D_ANDROID_DEBUG


LOGGING_DEFINES = -DFILE_OVERRIDE \
	-DLINE_OVERRIDE \
	-DFUNC_OVERRIDE \
	-DASSERT_OVERRIDE

MISC_DEFINES = -DCOMPILE_STATIC \
	-D__USE_FILE_OFFSET64 \
	-D_FILE_OFFSET_BITS=64 \
	-D__Userspace__ \
	-D__Userspace_os_Linux \
	-DSCTP_PROCESS_LEVEL_LOCKS \
	-DSCTP_SIMPLE_ALLOCATOR 

ifeq ($(ARCH64), true)
        PLATFORM_DEFINES+= -DARCH64
endif

ifeq ($(HIGHER_API), true)
	PLATFORM_DEFINES+= -DHAS_LIBRARY_LIMITS
	FEATURES_DEFINES+= -DHAS_EPOLL_TIMERS
endif

ALL_DEFINES=$(PLATFORM_DEFINES) $(FEATURES_DEFINES) $(LOGGING_DEFINES) $(MISC_DEFINES)

GENERIC_COMPILE_SWITCHES = -Wall \
	-Werror \
	-fno-exceptions \
	-fno-rtti \
	-fomit-frame-pointer \
	-fPIC \
	-Wno-error=implicit-function-declaration \
	-Wno-unused-but-set-variable \
	-Wno-unknown-pragmas \
	-Wno-strict-aliasing

CPP_COMPILE_FLAGS = -Wno-literal-suffix
ifeq ($(HAS_VFP),true)
	GENERIC_COMPILE_SWITCHES+=-mfpu=vfp
endif

CORE_SOURCE_FILES=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)3rdparty/lua-dev -type f -name "*.c"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)3rdparty/tinyxml -type f -name "*.cpp"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)3rdparty/usrsctplib -type f -name "*.c"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/common/src -type f -name "*.cpp"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/common/src -type f -name "*.c"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/thelib/src -type f -name "*.cpp"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/applications/rdkcrouter/src -type f -name "*.cpp"))
CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/applications/lminterface/src -type f -name "*.cpp"))
#CORE_SOURCE_FILES+=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/webserver/src -type f -name "*.cpp"))

CORE_INCLUDE_DIRS=$(OPENSSL_DIR)/include \
	$(LOCAL_PATH)3rdparty/lua-dev \
	$(LOCAL_PATH)3rdparty/tinyxml \
	$(LOCAL_PATH)3rdparty/usrsctplib \
	$(LOCAL_PATH)3rdparty/webserver/android/include \
	$(LOCAL_PATH)sources/common/include \
	$(LOCAL_PATH)sources/thelib/include \
	$(LOCAL_PATH)sources/applications/rdkcrouter/include \
	$(LOCAL_PATH)sources/applications/lminterface/include \
	$(LOCAL_PATH)sources/rdkcmediaserver/include 
#	$(LOCAL_PATH)sources/webserver/include

CORE_CFLAGS=$(ALL_DEFINES) \
	$(GENERIC_COMPILE_SWITCHES)

CORE_CPPFLAGS=$(CORE_CFLAGS) \
	$(CPP_COMPILE_FLAGS)

CORE_EXPORT_LDLIBS=-L$(OPENSSL_DIR)/lib \
		    -llog \
		    -lssl \
		    -lcrypto \
		    -lz 
#		    -L$(LOCAL_PATH)3rdparty/webserver/android/lib \
#		    -lmicrohttpd \
#   		-lgnutls \
#    		-lnettle \
#    		-lhogweed \
#    		-lgmp \
#    		-liconv \
#    		-lgcrypt \
#    		-lgpg-error

LD_LIBS=-ldl

ifeq ($(DISABLE_TEXTREL), false)
	LD_LIBS+=-Wl,--no-warn-shared-textrel
endif

TESTS_SOURCE_FILES=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/tests/src -type f -name "*.cpp"))
TESTS_INCLUDE_DIRS=$(CORE_INCLUDE_DIRS) \
	$(LOCAL_PATH)sources/tests/include

RDKCMS_SOURCE_FILES=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/rdkcmediaserver/src -type f -name "*.cpp"))

#WEBSERVER_SOURCE_FILES=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/webserver/src -type f -name "*.cpp"))

HLSTOOL_SOURCE_FILES=$(subst $(LOCAL_PATH),,$(shell find $(LOCAL_PATH)sources/hlstool/src -type f -name "*.cpp"))
HLSTOOL_INCLUDE_DIRS=$(CORE_INCLUDE_DIRS) \
	$(LOCAL_PATH)sources/hlstool/include


#/---------------------------\
#| core lib specific setup   |
#\---------------------------/
include $(CLEAR_VARS)
ifeq ($(HAS_NEON),true)
	LOCAL_ARM_NEON=true
	TARGET_ARCH_ABI=armeabi-v7a
endif
LOCAL_MODULE=		core
LOCAL_SRC_FILES=	$(CORE_SOURCE_FILES)
LOCAL_C_INCLUDES=	$(CORE_INCLUDE_DIRS)
LOCAL_CFLAGS=		$(CORE_CFLAGS)
#LOCAL_CPPFLAGS =	$(CORE_CPPFLAGS)
LOCAL_EXPORT_LDLIBS=	$(CORE_EXPORT_LDLIBS)
LOCAL_STATIC_LIBRARIES=	cpufeatures
include $(BUILD_STATIC_LIBRARY)

#/---------------------------\
#| rdkcms specific setup|
#\---------------------------/
include $(CLEAR_VARS)
ifeq ($(HAS_NEON),true)
	LOCAL_ARM_NEON=true
	TARGET_ARCH_ABI=armeabi-v7a
endif
LOCAL_MODULE=		rdkcms
LOCAL_SRC_FILES=	$(RDKCMS_SOURCE_FILES)
LOCAL_C_INCLUDES=	$(CORE_INCLUDE_DIRS)
LOCAL_CFLAGS=		$(CORE_CFLAGS)
#LOCAL_CPPFLAGS =	$(CORE_CPPFLAGS)
LOCAL_LDLIBS=		$(LD_LIBS)
LOCAL_EXPORT_LDFLAGS=   $(CORE_EXPORT_LDFLAGS)
LOCAL_STATIC_LIBRARIES= core cpufeatures
include $(BUILD_SHARED_LIBRARY)

#/-----------------------------\
#| rms-webserver specific setup|
#\-----------------------------/
#include $(CLEAR_VARS)
#ifeq ($(HAS_NEON),true)
#	LOCAL_ARM_NEON=true
#	TARGET_ARCH_ABI=armeabi-v7a
#endif
#LOCAL_MODULE=		rms-webserver
#LOCAL_SRC_FILES=	$(WEBSERVER_SOURCE_FILES)
#LOCAL_C_INCLUDES=	$(CORE_INCLUDE_DIRS)
#LOCAL_CFLAGS=		$(CORE_CFLAGS)
##LOCAL_CPPFLAGS =	$(CORE_CPPFLAGS)
#LOCAL_LDLIBS=		-ldl
#LOCAL_EXPORT_LDFLAGS=   $(CORE_EXPORT_LDFLAGS)
#LOCAL_STATIC_LIBRARIES= core cpufeatures
#include $(BUILD_SHARED_LIBRARY)

#/---------------------------\
#| hlstool specific setup    |
#\---------------------------/
#include $(CLEAR_VARS)
#ifeq ($(HAS_NEON),true)
#	LOCAL_ARM_NEON=true
#	TARGET_ARCH_ABI=armeabi-v7a
#endif
#LOCAL_MODULE=		hlstool
#LOCAL_SRC_FILES=	$(HLSTOOL_SOURCE_FILES)
#LOCAL_C_INCLUDES=	$(HLSTOOL_INCLUDE_DIRS)
#LOCAL_CFLAGS=		$(CORE_CFLAGS)
#LOCAL_LDLIBS=           $(CORE_LDLIBS)
#LOCAL_EXPORT_LDFLAGS=   $(CORE_EXPORT_LDFLAGS)
#LOCAL_STATIC_LIBRARIES= core cpufeatures
#include $(BUILD_EXECUTABLE)

#/---------------------------\
#| tests specific setup      |
#\---------------------------/
#include $(CLEAR_VARS)
#ifeq ($(HAS_NEON),true)
#	LOCAL_ARM_NEON=true
#	TARGET_ARCH_ABI=armeabi-v7a
#endif
#LOCAL_MODULE=		tests
#LOCAL_SRC_FILES=	$(TESTS_SOURCE_FILES)
#LOCAL_C_INCLUDES=	$(TESTS_INCLUDE_DIRS)
#LOCAL_CFLAGS=		$(CORE_CFLAGS)
#LOCAL_LDLIBS=		$(CORE_LDLIBS)
#LOCAL_EXPORT_LDFLAGS=	$(CORE_EXPORT_LDFLAGS)
#LOCAL_STATIC_LIBRARIES=	core cpufeatures
#include $(BUILD_EXECUTABLE)

$(call import-module,android/cpufeatures)
