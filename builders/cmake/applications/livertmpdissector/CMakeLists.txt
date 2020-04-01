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
PROJECT(livertmpdissector)

#make the list with all the source files
FILE(GLOB_RECURSE SOURCES "${SOURCES_ROOT}/applications/${PROJECT_NAME}/src/*.cpp" )

SET(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${RDKCMEDIASERVER_ROOT}/${PROJECT_NAME}")

#prepare the include directories
INCLUDE_DIRECTORIES(
	${OPENSSL_INCLUDE_PATH}
	${LUA_INCLUDE_PATH}
	${SOURCES_ROOT}/common/include
	${SOURCES_ROOT}/thelib/include
	${SOURCES_ROOT}/applications/${PROJECT_NAME}/include
	)

string(TOUPPER ${PROJECT_NAME} TEMP_APP_NAME)
SET_PROPERTY(GLOBAL APPEND PROPERTY ACTIVE_APPS "-DHAS_APP_${TEMP_APP_NAME}")
SET_PROPERTY(GLOBAL APPEND PROPERTY ACTIVE_APPS_LIBS "${PROJECT_NAME}")

#create the library
ADD_LIBRARY(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})
ADD_PRECOMPILED_HEADER(${PROJECT_NAME} "${SOURCES_ROOT}/common/include/common.h")
IF(${LIB_TYPE} MATCHES "SHARED")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} thelib)
    INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${RDKCMEDIASERVER_ROOT}/${PROJECT_NAME})
ENDIF(${LIB_TYPE} MATCHES "SHARED")


