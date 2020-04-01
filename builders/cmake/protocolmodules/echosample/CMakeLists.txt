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
PROJECT(echosample)

#make the list with all the source files
FILE(GLOB_RECURSE C_SOURCES "${SOURCES_ROOT}/protocolmodules/echosample/src/*.c")
FILE(GLOB_RECURSE CXX_SOURCES "${SOURCES_ROOT}/protocolmodules/echosample/src/*.cpp")
SET(SOURCES ${C_SOURCES} ${CXX_SOURCES})

#prepare the include directories
INCLUDE_DIRECTORIES(
	${SOURCES_ROOT}/thelib/include/protocols/external
	${SOURCES_ROOT}/protocolmodules/echosample/include
)

#create the library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCES})
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} thelib)

