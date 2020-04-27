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
#get platform specific paths
SET(HWINFO_INCLUDE_PATH "")
SET(HWINFO_LIBRARY_PATH "")

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	MESSAGE(STATUS "WE ARE ADDING PATHS FOR DARWIN")
#	SET(HWINFO_INCLUDE_PATH "/System/Library/Frameworks/IOKit.framework/Headers")
#	SET(HWINFO_INCLUDE_PATH ${HWINFO_INCLUDE_PATH} "/System/Library/Frameworks/CoreFoundation.framework/Headers")
	SET(HWINFO_LIBRARY_PATH "-framework IOKit")
	SET(HWINFO_LIBRARY_PATH ${HWINFO_LIBRARY_PATH} "-framework CoreFoundation")
ELSE(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	MESSAGE(STATUS "Platform ${CMAKE_SYSTEM_NAME} does not need any special paths")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
