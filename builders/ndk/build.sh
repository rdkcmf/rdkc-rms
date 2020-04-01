#!/bin/sh
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

if [ ! -f set-dev-env.sh ]
then
	echo "Setting up OpenSSL first"
	echo
	sudo sh openssl.sh
fi

. ./set-dev-env.sh

RMS_VERSION_RELEASE_NUMBER=`cat ../../RELEASE_NUMBER`
RMS_VERSION_CODE_NAME=`cat ../../CODE_NAME`
RMS_VERSION_BUILDER_OS_VERSION=2.3.6
RMS_VERSION_BUILDER_OS_ARCH=ARMv7

export HIGHER_API=false

if [ $(echo $ANDROID_API | cut -f2 -d-) -gt 20 ]; then
        HIGHER_API=true
fi

echo "//THIS FILE IS GENERATED" >../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILD_DATE `date -u +%s`" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILD_NUMBER \"`git rev-list --count HEAD`\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_HASH \"`git rev-parse HEAD`\"" >>../../sources/common/include/version.h
echo "#define RMS_BRANCH_NAME \"`git symbolic-ref --short HEAD`\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_RELEASE_NUMBER \"$RMS_VERSION_RELEASE_NUMBER\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_CODE_NAME \"$RMS_VERSION_CODE_NAME\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILDER_OS_NAME \"Android\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILDER_OS_VERSION \"$RMS_VERSION_BUILDER_OS_VERSION\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILDER_OS_ARCH \"$RMS_VERSION_BUILDER_OS_ARCH\"" >>../../sources/common/include/version.h
echo "#define RMS_VERSION_BUILDER_OS_UNAME \"`uname -a`\"" >>../../sources/common/include/version.h
echo "" >>../../sources/common/include/version.h

sudo -E $ANDROID_NDK_ROOT/ndk-build

