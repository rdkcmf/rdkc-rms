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
                if [ "`uname -s`" = "FreeBSD" ]; then
			gmake
		else
			make
		fi
	)
	testError "compiling failed"
fi

# 2. Perform the tests
if [ "$EMBEDDED" = "" ]
then
	$ROOT_FOLDER/builders/cmake/tests/tests
	testError "tests failed"
fi

# 3. Create distribution folder and populate it with the proper files
rm -rf ./dist
testError "rm -rf ./dist failed"

createFolder ./dist/$DIST_NAME/bin
#createFolder ./dist/$DIST_NAME/bin/rms-phpengine
createFolder ./dist/$DIST_NAME/logs
createFolder ./dist/$DIST_NAME/media
createFolder ./dist/$DIST_NAME/rms-webroot
cp $ROOT_FOLDER/dist_template/scripts/unix/*.sh ./dist/$DIST_NAME/bin
testError "cp $ROOT_FOLDER/dist_template/scripts/unix/*.sh ./dist/$DIST_NAME/bin failed"
copyFiles $ROOT_FOLDER/dist_template/config ./dist/$DIST_NAME/
cp $ROOT_FOLDER/dist_template/documents/* ./dist/$DIST_NAME/
copyFiles $ROOT_FOLDER/dist_template/demo ./dist/$DIST_NAME/
#copyFiles $ROOT_FOLDER/dist_template/rms-avconv/presets ./dist/$DIST_NAME/rms-avconv-presets
if [ "$EMBEDDED" != "" ]
then
	copyFiles $ROOT_FOLDER/builders/make2/output/static/rdkcmediaserver ./dist/$DIST_NAME/bin/rdkcms
	copyFiles $ROOT_FOLDER/builders/make2/output/static/tests ./dist/$DIST_NAME/bin/platformTests
	#copyFiles $ROOT_FOLDER/builders/make2/output/static/mp4writer ./dist/$DIST_NAME/bin/rms-mp4writer
else
	copyFiles $ROOT_FOLDER/builders/cmake/rdkcmediaserver/rdkcmediaserver ./dist/$DIST_NAME/bin/rdkcms
	copyFiles $ROOT_FOLDER/builders/cmake/tests/tests ./dist/$DIST_NAME/bin/platformTests
	#copyFiles $ROOT_FOLDER/builders/cmake/mp4writer/mp4writer ./dist/$DIST_NAME/bin/rms-mp4writer
	#copyFiles $ROOT_FOLDER/builders/cmake/webserver/rms-webserver ./dist/$DIST_NAME/bin/rms-webserver
	#copyFiles $ROOT_FOLDER/3rdparty/webserver/linux/bin/php-cgi ./dist/$DIST_NAME/bin/rms-phpengine
	#copyFiles $ROOT_FOLDER/websources/access_policy/clientaccesspolicy.xml ./dist/$DIST_NAME/rms-webroot/clientaccesspolicy.xml
	#copyFiles $ROOT_FOLDER/websources/access_policy/crossdomain.xml ./dist/$DIST_NAME/rms-webroot/crossdomain.xml
	#copyFiles $ROOT_FOLDER/websources/web_ui/ ./dist/$DIST_NAME/rms-webroot/RMS_Web_UI/
	#copyFiles $ROOT_FOLDER/websources/rmswebservices ./dist/$DIST_NAME/rms-webroot/
	#copyFiles $ROOT_FOLDER/websources/demo ./dist/$DIST_NAME/rms-webroot
fi

cat $ROOT_FOLDER/dist_template/config/config.lua \
	| sed "s/__STANDARD_LOG_FILE_APPENDER__/..\/logs\/rms/" \
	| sed "s/__STANDARD_TIME_PROBE_APPENDER__/..\/logs\/timeprobe/" \
	| sed "s/__STANDARD_MEDIA_FOLDER__/..\/media/" \
	| sed "s/__STANDARD_CONFIG_PERSISTANCE_FILE__/..\/config\/pushPullSetup.xml/" \
	| sed "s/__STANDARD_AUTH_PERSISTANCE_FILE__/..\/config\/auth.xml/" \
	| sed "s/__STANDARD_CONN_LIMITS_PERSISTANCE_FILE__/..\/config\/connlimits.xml/" \
	| sed "s/__STANDARD_BW_LIMITS_PERSISTANCE_FILE__/..\/config\/bandwidthlimits.xml/" \
	| sed "s/__STANDARD_INGEST_POINTS_PERSISTANCE_FILE__/..\/config\/ingestpoints.xml/" \
	| sed "s/__STANDARD_AUTO_DASH_FOLDER__/..\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_HLS_FOLDER__/..\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_HDS_FOLDER__/..\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_MSS_FOLDER__/..\/rms-webroot/" \
	| sed "s/__STANDARD_RTMP_USERS_FILE__/..\/config\/users.lua/" \
	| sed "s/__STANDARD_RTSP_USERS_FILE__/..\/config\/users.lua/" \
	| sed "s/__STANDARD_EVENT_FILE_APPENDER__/..\/logs\/events.txt/" \
	| sed "s/__STANDARD_TRANSCODER_SCRIPT__/.\/rmsTranscoder.sh/" \
	| sed "s/__STANDARD_MP4WRITER_BIN__/.\/rms-mp4writer/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/..\/config\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/..\/config\/server.cert/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	>./dist/$DIST_NAME/config/config.lua
cat $ROOT_FOLDER/dist_template/config/webconfig.lua \
	| sed "s/__STANDARD_WEBSERVER_LOG_FILE_APPENDER__/..\/logs\/rms-webserver/" \
	| sed "s/__STANDARD_WHITELIST_FILE__/..\/config\/whitelist.txt/" \
	| sed "s/__STANDARD_BLACKLIST_FILE__/..\/config\/blacklist.txt/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/..\/config\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/..\/config\/server.cert/" \
	| sed "s/__STANDARD_WEBROOT_FOLDER__/..\/rms-webroot/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	>./dist/$DIST_NAME/config/webconfig.lua
cat $ROOT_FOLDER/dist_template/scripts/unix/rmsTranscoder.sh \
	| sed "s/__RMS_AVCONV_BIN__/.\/rms-avconv/" \
	| sed "s/__RMS_AVCONV_PRESETS__/..\/rms-avconv-presets/" \
	> ./dist/$DIST_NAME/bin/rmsTranscoder.sh
chmod +x ./dist/$DIST_NAME/bin/rmsTranscoder.sh
testError "chmod +x ./dist/$DIST_NAME/bin/rmsTranscoder.sh"
date > ./dist/$DIST_NAME/BUILD_DATE
if [ "$EMBEDDED" = "" ]
then

	# 4. Strip binaries
	if [ "`uname -s`" = "Darwin" ]
	then
		strip -X ./dist/$DIST_NAME/bin/rdkcms
		strip -X ./dist/$DIST_NAME/bin/platformTests
		#strip -X ./dist/$DIST_NAME/bin/rms-avconv
		#strip -X ./dist/$DIST_NAME/bin/rms-mp4writer
		#strip -X ./dist/$DIST_NAME/bin/rms-webserver
		#strip -X ./dist/$DIST_NAME/bin/rms-phpengine/php-cgi
	else
		strip -s ./dist/$DIST_NAME/bin/rdkcms
		strip -s ./dist/$DIST_NAME/bin/platformTests
		#strip -s ./dist/$DIST_NAME/bin/rms-avconv
		#strip -s ./dist/$DIST_NAME/bin/rms-mp4writer
		#strip -s ./dist/$DIST_NAME/bin/rms-webserver
		#strip -s ./dist/$DIST_NAME/bin/rms-phpengine/php-cgi
	fi
	testError "strip failed"
fi

# 5. cleanup things
removeSvnFiles ./dist/$DIST_NAME

## 6. gzip it
#(
#	cd ./dist && \
#	# --force-local fails on Mac
#	if [ "$DISTRO_NAME" = "MacOSX" ] || [ "$DISTRO_NAME" = "FreeBSD" ]; then
#	 	tar czf $DIST_NAME.tar.gz $DIST_NAME
#	else
#		# --force-local to treat as local file, or will treat call to http(s)
#		tar czf $DIST_NAME.tar.gz $DIST_NAME --force-local
#	fi 	
#)
#testError "tar failed"

exitOk
