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
#########################################################################
cmake_minimum_required(VERSION 2.6)
find_package(Threads)
PROJECT(rms-webserver)

#make the list with all the source files
FILE(GLOB_RECURSE SOURCES "${SOURCES_ROOT}/webserver/src/*.cpp" )

#prepare the include directories
INCLUDE_DIRECTORIES(
    ${LUA_INCLUDE_PATH}
    ${SOURCES_ROOT}/common/include
    ${SOURCES_ROOT}/thelib/include
    ${SOURCES_ROOT}/applications/rdkcrouter/include
    ${3RDPARTY_ROOT}/webserver/linux/include
    ${SOURCES_ROOT}/webserver/include
    ${HWINFO_INCLUDE_PATH}
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    rdkcrouter
    thelib
    common
    ${TINYXML_LIBRARY_PATH}
    ${LUA_LIBRARY_PATH}
    ${OPENSSL_LIBRARY_PATH}
    ${CRYPTO_LIBRARY_PATH}
    ${Z_LIBRARY_PATH}
    ${DL_LIBRARY_PATH}
    ${HWINFO_LIBRARY_PATH}
    ${SOCKET_LIB}
    ${3RDPARTY_ROOT}/webserver/linux/lib/libmicrohttpd.a    
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgnutls.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgnutls-openssl.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgnutlsxx.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libnettle.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libhogweed.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgmp.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libz.a     
    ${3RDPARTY_ROOT}/webserver/linux/lib/libiconv.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgcrypt.a
    ${INTL_LIB}	
    ${3RDPARTY_ROOT}/webserver/linux/lib/libgpg-error.a
    ${3RDPARTY_ROOT}/webserver/linux/lib/libcharset.a
    ${CMAKE_THREAD_LIBS_INIT}
    ${SCTP_LIBRARY_PATH}
)

ADD_PRECOMPILED_HEADER(${PROJECT_NAME} "${SOURCES_ROOT}/common/include/common.h")
INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION sbin)

