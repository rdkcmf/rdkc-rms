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
MESSAGE(STATUS "Looking for tinyxml")
FIND_PATH(TINYXML_INCLUDE_PATH 
	NAMES
		tinyxml.h
	PATHS
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		NO_DEFAULT_PATH)

FIND_LIBRARY(TINYXML_LIBRARY_PATH
	NAMES
		tinyxml
	PATHS
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
		/sw/lib
		/opt/local/lib
		NO_DEFAULT_PATH)

IF(TINYXML_INCLUDE_PATH)
	MESSAGE(STATUS "Looking for tinyxml headers - found")
ELSE(TINYXML_INCLUDE_PATH)
	MESSAGE(STATUS "Looking for tinyxml headers - not found")
ENDIF(TINYXML_INCLUDE_PATH)

IF(TINYXML_LIBRARY_PATH)
	MESSAGE(STATUS "Looking for tinyxml library - found")
ELSE(TINYXML_LIBRARY_PATH)
	MESSAGE(STATUS "Looking for tinyxml library - not found")
ENDIF(TINYXML_LIBRARY_PATH)

IF(TINYXML_INCLUDE_PATH AND TINYXML_LIBRARY_PATH)
	SET(TINYXML_FOUND 1 CACHE STRING "Set to 1 if tinyxml is found, 0 otherwise")
ELSE(TINYXML_INCLUDE_PATH AND TINYXML_LIBRARY_PATH)
	SET(TINYXML_FOUND 0 CACHE STRING "Set to 1 if tinyxml is found, 0 otherwise")
	SET(TINYXML_LIBRARY_PATH "tinyxml")
	SET(TINYXML_INCLUDE_PATH "${RDKCMEDIASERVER_3RDPARTY_ROOT}/tinyxml")
	MESSAGE(STATUS "Defaulting to ${RDKCMEDIASERVER_3RDPARTY_ROOT}/tinyxml")
ENDIF(TINYXML_INCLUDE_PATH AND TINYXML_LIBRARY_PATH)

MARK_AS_ADVANCED(TINYXML_FOUND)
