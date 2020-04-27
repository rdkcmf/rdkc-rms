#########################################################################
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
MESSAGE(STATUS "Looking for v8")
FIND_PATH(V8_INCLUDE_PATH 
	NAMES
		v8.h
	PATHS
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		~/work/v8/include
		NO_DEFAULT_PATH)

FIND_LIBRARY(V8_LIBRARY_PATH
	NAMES
		v8_g
	PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		~/work/v8		
		NO_DEFAULT_PATH)

IF(V8_INCLUDE_PATH)
	SET(V8_FOUND 1 CACHE STRING "Set to 1 if v8 is found, 0 otherwise")
	MESSAGE(STATUS "Looking for v8 headers - found")
ELSE(V8_INCLUDE_PATH)
	SET(V8_FOUND 0 CACHE STRING "Set to 1 if v8 is found, 0 otherwise")
	MESSAGE(STATUS "Looking for v8 headers - not found")
	SET(V8_INCLUDE_PATH "")
ENDIF(V8_INCLUDE_PATH)

IF(V8_LIBRARY_PATH)
	SET(V8_FOUND 1 CACHE STRING "Set to 1 if v8 is found, 0 otherwise")
	MESSAGE(STATUS "Looking for v8 library - found")
ELSE(V8_LIBRARY_PATH)
	SET(V8_FOUND 0 CACHE STRING "Set to 1 if v8 is found, 0 otherwise")
	MESSAGE(STATUS "Looking for v8 library - not found")
	SET(V8_LIBRARY_PATH "")
ENDIF(V8_LIBRARY_PATH)

MARK_AS_ADVANCED(V8_FOUND)

