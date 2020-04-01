#!/bin/bash
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

set -e

ROOT_FOLDER=`realpath ../..`
. $ROOT_FOLDER/packaging/functions

# 0. Gather input parameters
if [ "$EMBEDDED" != "" ]
then
	DISTRO_NAME=`cat $ROOT_FOLDER/builders/make2/${EMBEDDED}.mk|grep TARGET_OS_NAME|sed "s/[^=]*=\(.*\)/\1/"`
	DISTRO_VERSION=`cat $ROOT_FOLDER/builders/make2/${EMBEDDED}.mk|grep TARGET_OS_VERSION|sed "s/[^=]*=\(.*\)/\1/"`
	ARCH=`cat $ROOT_FOLDER/builders/make2/${EMBEDDED}.mk|grep TARGET_OS_ARCH|sed "s/[^=]*=\(.*\)/\1/"`
	DO_CLEANUP="yes"
else
	DISTRO_NAME=$OS_NAME
	if [ "$DISTRO_NAME" = "" ]
	then
		DISTRO_NAME=$1
	fi
	DISTRO_VERSION=$OS_VERSION
	if [ "$DISTRO_VERSION" = "" ]
	then
		DISTRO_VERSION=$2
	fi
	DO_CLEANUP=$3
	if [ "$DO_CLEANUP" != "yes" ]
	then
		DO_CLEANUP=$1
		if [ "$DO_CLEANUP" != "yes" ]
		then
			DO_CLEANUP=$2
			if [ "$DO_CLEANUP" != "yes" ]
			then
				DO_CLEANUP=$3
				if [ "$DO_CLEANUP" != "yes" ]
				then
					DO_CLEANUP="no"
				fi
			fi
		fi
	fi

	ARCH=$OS_ARCH
	if [ "$ARCH" = "" ]
	then
		ARCH=`uname -m`
		if [ "$?" -ne "0" ]; then
			testError "uname failed";
		fi
	fi
fi

CODE_NAME=`cat $ROOT_FOLDER/CODE_NAME`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER` #.`git rev-list --count HEAD`

DIST_NAME="rdkcms-$RELEASE_NUMBER-$ARCH-${DISTRO_NAME}_${DISTRO_VERSION}"

echo "----------Summary------------"
echo "RELEASE_NUMBER: $RELEASE_NUMBER"
echo "CODE_NAME:      $CODE_NAME"
echo "DISTRO_NAME:    $DISTRO_NAME"
echo "DISTRO_VERSION: $DISTRO_VERSION"
echo "ARCH:           $ARCH"
echo "DIST_NAME:      $DIST_NAME"
echo "DO_CLEANUP:     $DO_CLEANUP"
echo "-----------------------------"

# 1. compile the binary
if [ "$EMBEDDED" != "" ]
then
	(
		cd $ROOT_FOLDER/builders/make2 && \
		make PLATFORM=$EMBEDDED clean && \
		make -j16 PLATFORM=$EMBEDDED
	)
	testError "compiling failed"
else
	if [ "$DO_CLEANUP" = "yes" ]
	then
		(
			cd $ROOT_FOLDER/builders/cmake && \
			sh cleanup.sh
		)
		testError "cleanup before compile failed";
	fi
	(
		cd $ROOT_FOLDER/builders/cmake && \
		COMPILE_STATIC=1 cmake -DCMAKE_BUILD_TYPE=Release . && \
		make
	)
	testError "compiling failed"
fi

	copyFiles $ROOT_FOLDER/builders/cmake/rdkcmediaserver/rdkcmediaserver ./dist/$DIST_NAME/bin/rdkcms
	copyFiles $ROOT_FOLDER/builders/cmake/tests/tests ./dist/$DIST_NAME/bin/platformTests
	copyFiles $ROOT_FOLDER/builders/cmake/mp4writer/mp4writer ./dist/$DIST_NAME/bin/rms-mp4writer
	# copyFiles $ROOT_FOLDER/builders/cmake/webserver/rms-webserver ./dist/$DIST_NAME/bin/rms-webserver

date > ./dist/$DIST_NAME/BUILD_DATE
	# 4. Strip binaries
	if [ "`uname -s`" = "Darwin" ]
	then
		strip -X ./dist/$DIST_NAME/bin/rdkcms
		strip -X ./dist/$DIST_NAME/bin/platformTests
		# strip -X ./dist/$DIST_NAME/bin/rms-avconv
		strip -X ./dist/$DIST_NAME/bin/rms-mp4writer
		# strip -X ./dist/$DIST_NAME/bin/rms-webserver
	else
		strip -s ./dist/$DIST_NAME/bin/rdkcms
		strip -s ./dist/$DIST_NAME/bin/platformTests
		# strip -s ./dist/$DIST_NAME/bin/rms-avconv
		strip -s ./dist/$DIST_NAME/bin/rms-mp4writer
		# strip -s ./dist/$DIST_NAME/bin/rms-webserver
	fi
	testError "strip failed"

# 5. cleanup things
rm -rf ./dist/$DIST_NAME/config/License.lic


exitOk
