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
PROJECT(rdkcmediaserver)

#prepare the include directories
INCLUDE_DIRECTORIES(
	${LUA_INCLUDE_PATH}
	${SOURCES_ROOT}/common/include
	${SOURCES_ROOT}/thelib/include
	${HWINFO_INCLUDE_PATH}
)

IF(${LIB_TYPE} MATCHES "STATIC")
	GET_PROPERTY(APPS_DEFINES GLOBAL PROPERTY ACTIVE_APPS)
	GET_PROPERTY(APPS_LIBS GLOBAL PROPERTY ACTIVE_APPS_LIBS)
	ADD_DEFINITIONS(${APPS_DEFINES})
ELSE(${LIB_TYPE} MATCHES "STATIC")
	SET(APPS_LIBS "")
ENDIF(${LIB_TYPE} MATCHES "STATIC")


ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES_ROOT}/rdkcmediaserver/src/rdkcmediaserver.cpp)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	${APPS_LIBS}
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
	${LUA_LIBRARY_PATH}
	${CMAKE_THREAD_LIBS_INIT}
	${SCTP_LIBRARY_PATH}
)

ADD_PRECOMPILED_HEADER(${PROJECT_NAME} "${SOURCES_ROOT}/common/include/common.h")
INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION sbin)
INSTALL(FILES rdkcmediaserver.lua DESTINATION etc RENAME rdkcmediaserver.lua.sample)
INSTALL(FILES ../../../man/rdkcmediaserver.1 DESTINATION man/man1)

