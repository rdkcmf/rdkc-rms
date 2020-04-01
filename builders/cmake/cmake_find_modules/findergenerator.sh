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
LIBNAME=`echo $1|tr '[:lower:]' '[:upper:]'`
CMAKENAME=Find_$1.cmake

echo "MESSAGE(STATUS \"Looking for $1\")">$CMAKENAME
echo "FIND_PATH(${LIBNAME}_INCLUDE_PATH ">>$CMAKENAME
echo "	NAMES">>$CMAKENAME
echo "		${LIBNAME}.h">>$CMAKENAME
echo "	PATHS">>$CMAKENAME
echo "		/usr/include">>$CMAKENAME
echo "		/usr/local/include">>$CMAKENAME
echo "		/sw/include">>$CMAKENAME
echo "		/opt/local/include">>$CMAKENAME
echo "		NO_DEFAULT_PATH)">>$CMAKENAME
echo "" >>$CMAKENAME
echo "FIND_LIBRARY(${LIBNAME}_LIBRARY_PATH">>$CMAKENAME
echo "	NAMES">>$CMAKENAME
echo "		${LIBNAME}">>$CMAKENAME
echo "	PATHS">>$CMAKENAME
echo "		/usr/lib64">>$CMAKENAME
echo "		/usr/lib">>$CMAKENAME
echo "		/usr/local/lib64">>$CMAKENAME
echo "		/usr/local/lib">>$CMAKENAME
echo "		/sw/lib">>$CMAKENAME
echo "		/opt/local/lib">>$CMAKENAME
echo "		NO_DEFAULT_PATH)">>$CMAKENAME
echo "">>$CMAKENAME
echo "IF(${LIBNAME}_INCLUDE_PATH)">>$CMAKENAME
echo "	SET(${LIBNAME}_FOUND 1 CACHE STRING \"Set to 1 if $1 is found, 0 otherwise\")">>$CMAKENAME
echo "	MESSAGE(STATUS \"Looking for $1 headers - found\")">>$CMAKENAME
echo "ELSE(${LIBNAME}_INCLUDE_PATH)">>$CMAKENAME
echo "	SET(${LIBNAME}_FOUND 0 CACHE STRING \"Set to 1 if $1 is found, 0 otherwise\")">>$CMAKENAME
echo "	MESSAGE(FATAL_ERROR \"Looking for $1 headers - not found\")">>$CMAKENAME
echo "ENDIF(${LIBNAME}_INCLUDE_PATH)">>$CMAKENAME
echo "">>$CMAKENAME
echo "IF(${LIBNAME}_LIBRARY_PATH)">>$CMAKENAME
echo "	SET(${LIBNAME}_FOUND 1 CACHE STRING \"Set to 1 if $1 is found, 0 otherwise\")">>$CMAKENAME
echo "	MESSAGE(STATUS \"Looking for $1 library - found\")">>$CMAKENAME
echo "ELSE(${LIBNAME}_LIBRARY_PATH)">>$CMAKENAME
echo "	SET(${LIBNAME}_FOUND 0 CACHE STRING \"Set to 1 if $1 is found, 0 otherwise\")">>$CMAKENAME
echo "	MESSAGE(FATAL_ERROR \"Looking for $1 library - not found\")">>$CMAKENAME
echo "ENDIF(${LIBNAME}_LIBRARY_PATH)">>$CMAKENAME
echo "">>$CMAKENAME
echo "MARK_AS_ADVANCED(${LIBNAME}_FOUND)">>$CMAKENAME
cat $CMAKENAME

