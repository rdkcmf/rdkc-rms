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
FIND_PATH(FCGI_INCLUDE_PATH 
	NAMES
		fcgi_config.h
  PATHS
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
  NO_DEFAULT_PATH)

FIND_LIBRARY(FCGI_LIBRARY_PATH
	NAMES
		fcgi
	PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
  NO_DEFAULT_PATH)

IF(FCGI_INCLUDE_PATH)
	SET(FCGI_FOUND 1)
	MESSAGE(STATUS "Looking for fcgi headers - found")
ELSE(FCGI_INCLUDE_PATH)
	SET(FCGI_FOUND 0)
	MESSAGE(STATUS "Looking for fcgi headers - not found")
ENDIF(FCGI_INCLUDE_PATH)

IF(FCGI_LIBRARY_PATH)
  SET(FCGI_FOUND 1)
  MESSAGE(STATUS "Looking for fcgi library - found")
ELSE(FCGI_LIBRARY_PATH)
  SET(FCGI_FOUND 0)
  MESSAGE(STATUS "Looking for fcgi library - not found")
ENDIF(FCGI_LIBRARY_PATH)

MARK_AS_ADVANCED(FCGI_FOUND)
