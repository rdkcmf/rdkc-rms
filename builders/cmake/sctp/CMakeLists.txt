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
PROJECT(sctp)

# Add the generic definitions
ADD_DEFINITIONS(-D__Userspace__)
ADD_DEFINITIONS(-DSCTP_PROCESS_LEVEL_LOCKS)
ADD_DEFINITIONS(-DSCTP_SIMPLE_ALLOCATOR)
ADD_DEFINITIONS(-DINET)

IF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    ADD_DEFINITIONS(-D__Userspace_os_FreeBSD)
    ADD_DEFINITIONS(-U__FreeBSD__)
    ADD_DEFINITIONS(-DHAVE_SCONN_LEN)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    ADD_DEFINITIONS(-D__Userspace_os_Darwin)
    ADD_DEFINITIONS(-DHAVE_SCONN_LEN)
    ADD_DEFINITIONS(-U__APPLE__)
    ADD_DEFINITIONS(-D__APPLE_USE_RFC_2292)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    ADD_DEFINITIONS(-D__Userspace_os_Linux)
ELSE(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    MESSAGE(FATAL_ERROR "Platform ${CMAKE_SYSTEM_NAME} not supported yet")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")

#make the list with all the source files
FILE(GLOB_RECURSE SOURCES "${3RDPARTY_ROOT}/usrsctplib/*.c" )

#prepare the include directories
INCLUDE_DIRECTORIES(
	${SCTP_INCLUDE_PATH}
	${SCTP_INCLUDE_PATH}/netinet
	${SCTP_INCLUDE_PATH}/netinet6
)

#create the library
ADD_LIBRARY(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})
IF(${LIB_TYPE} MATCHES "SHARED")
    INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${RDKCMEDIASERVER_ROOT})
ENDIF(${LIB_TYPE} MATCHES "SHARED")


