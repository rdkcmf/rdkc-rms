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
MESSAGE(STATUS "Looking for scons")
FIND_PATH(SCONS_BIN_PATH 
	NAMES
		scons
	PATHS
		/usr/bin
		/usr/local/bin
		/sw/bin
		/sw/local/bin
		/opt/bin
		/opt/local/bin
	NO_DEFAULT_PATH)

IF(SCONS_BIN_PATH)
	SET(SCONS_FOUND 1 CACHE STRING "Set to 1 if scons is found, 0 otherwise")
	MESSAGE(STATUS "Looking for scons utility - found")
ELSE(SCONS_BIN_PATH)
	SET(SCONS_FOUND 0 CACHE STRING "Set to 1 if scons is found, 0 otherwise")
	MESSAGE(STATUS "Looking for scons binary - not found")
ENDIF(SCONS_BIN_PATH)

MARK_AS_ADVANCED(SCONS_FOUND)

