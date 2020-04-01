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
PROJECT(tinyxml)

#make the list with all the source files
FILE(GLOB_RECURSE SOURCES "${3RDPARTY_ROOT}/tinyxml/*.cpp" )

#prepare the include directories
INCLUDE_DIRECTORIES(
	${TINYXML_INCLUDE_PATH}
)

#create the library
ADD_LIBRARY(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})
IF(${LIB_TYPE} MATCHES "SHARED")
    INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${RDKCMEDIASERVER_ROOT})
ENDIF(${LIB_TYPE} MATCHES "SHARED")

