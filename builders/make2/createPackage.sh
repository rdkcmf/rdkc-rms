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

ROOT_FOLDER=`readlink -e ../..`
. $ROOT_FOLDER/packaging/functions

# 0. Gather input parameters
DISTRO_NAME=`cat $ROOT_FOLDER/builders/make2/$1|grep TARGET_OS_NAME|sed "s/[^=]*=\(.*\)/\1/"`
DISTRO_VERSION=`cat $ROOT_FOLDER/builders/make2/$1|grep TARGET_OS_VERSION|sed "s/[^=]*=\(.*\)/\1/"`
ARCH=`cat $ROOT_FOLDER/builders/make2/$1|grep TARGET_OS_ARCH|sed "s/[^=]*=\(.*\)/\1/"`

CODE_NAME=`cat $ROOT_FOLDER/CODE_NAME`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER`.`git rev-list --count HEAD`

DIST_NAME="rdkcms-$RELEASE_NUMBER-$ARCH-${DISTRO_NAME}_${DISTRO_VERSION}"

BUILD_SSL=$3
if [ "$BUILD_SSL" != "ssl" ]
then
	BUILD_SSL=$1
	if [ "$BUILD_SSL" != "ssl" ]
	then
		BUILD_SSL=$2
		if [ "$BUILD_SSL" != "ssl" ]
		then
			BUILD_SSL=$3
			if [ "$BUILD_SSL" != "ssl" ]
			then
				BUILD_SSL="no"
			fi
		fi
	fi
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

echo "----------Summary------------"
echo "RELEASE_NUMBER: $RELEASE_NUMBER"
echo "CODE_NAME:      $CODE_NAME"
echo "DISTRO_NAME:    $DISTRO_NAME"
echo "DISTRO_VERSION: $DISTRO_VERSION"
echo "ARCH:           $ARCH"
echo "DIST_NAME:      $DIST_NAME"
echo "BUILD_SSL:      $BUILD_SSL"
echo "DO_CLEANUP:     $DO_CLEANUP"
echo "-----------------------------"


# 1. compile the binary
if [ "$DO_CLEANUP" = "yes" ]
then
	(
		cd $ROOT_FOLDER/builders/make2 && \
		sh crossmake.sh clean
	)
	testError "cleanup before compile failed";
fi
(
	cd $ROOT_FOLDER/builders/make2 && 
	sh crossmake.sh $1
)
testError "compiling failed"

# 2. Perform the tests
#    Cant when cross compiling

# 3. Create distribution folder and populate it with the proper files
rm -rf ./dist
testError "rm -rf ./dist failed"

createFolder ./dist/$DIST_NAME/bin
createFolder ./dist/$DIST_NAME/bin/phpengine
createFolder ./dist/$DIST_NAME/logs
createFolder ./dist/$DIST_NAME/media
createFolder ./dist/$DIST_NAME/rms-webroot
cp $ROOT_FOLDER/dist_template/scripts/unix/*.sh ./dist/$DIST_NAME/bin
testError "cp $ROOT_FOLDER/dist_template/scripts/unix/*.sh ./dist/$DIST_NAME/bin failed"
copyFiles $ROOT_FOLDER/dist_template/config ./dist/$DIST_NAME/
#copyFiles $ROOT_FOLDER/dist_template/doc ./dist/$DIST_NAME/documents
copyFiles $ROOT_FOLDER/dist_template/demo ./dist/$DIST_NAME/
copyFiles $ROOT_FOLDER/dist_template/documents/README.txt ./dist/$DIST_NAME/
#copyFiles $ROOT_FOLDER/dist_template/rms-avconv/presets ./dist/$DIST_NAME/rms-avconv-presets
    copyFiles $ROOT_FOLDER/builders/make2/output/static/rdkcmediaserver.full ./dist/$DIST_NAME/bin/rdkcms_debug
    copyFiles $ROOT_FOLDER/builders/make2/output/static/rdkcmediaserver ./dist/$DIST_NAME/bin/rdkcms
    copyFiles $ROOT_FOLDER/builders/make2/output/static/tests ./dist/$DIST_NAME/bin/platformTests
    copyFiles $ROOT_FOLDER/builders/make2/output/static/mp4writer ./dist/$DIST_NAME/bin/rms-mp4writer
    #copyFiles $ROOT_FOLDER/builders/cmake/webserver/rms-webserver ./dist/$DIST_NAME/bin/rms-webserver
    #copyFiles $ROOT_FOLDER/3rdparty/webserver/linux/bin/php-cgi ./dist/$DIST_NAME/bin/rms-phpengine
    #copyFiles $ROOT_FOLDER/websources/access_policy/clientaccesspolicy.xml ./dist/$DIST_NAME/rms-webroot/clientaccesspolicy.xml
    #copyFiles $ROOT_FOLDER/websources/access_policy/crossdomain.xml ./dist/$DIST_NAME/rms-webroot/crossdomain.xml
    #copyFiles $ROOT_FOLDER/websources/web_ui/ ./dist/$DIST_NAME/rms-webroot/RMS_Web_UI/
    #copyFiles $ROOT_FOLDER/websources/rmswebservices ./dist/$DIST_NAME/rms-webroot/
    #copyFiles $ROOT_FOLDER/websources/demo ./dist/$DIST_NAME/rms-webroot

cat $ROOT_FOLDER/dist_template/config/config.lua \
    | sed "s/__STANDARD_LOG_FILE_APPENDER__/..\/logs\/rms/" \
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
#Avig requested build info:
BUILD_INFO="./dist/$DIST_NAME/BUILD_INFO"
echo "buildDate: `date`" > $BUILD_INFO
echo "version: $RELEASE_NUMBER" >> $BUILD_INFO
echo "PLATFORM_BREAKPAD_BINARY: $PLATFORM_BREAKPAD_BINARY"

#generate debug symbols
if [ -f $PLATFORM_BREAKPAD_BINARY ]; then
	$PLATFORM_BREAKPAD_BINARY ./dist/$DIST_NAME/bin/rdkcms_debug > ./evostreamms.sym
	mv -f ./evostreamms.sym  $PLATFORM_SYMBOL_PATH
	echo "evostream debug symbol created"
fi

# 5. cleanup things
removeSvnFiles ./dist/$DIST_NAME
rm -rf ./dist/$DIST_NAME/config/License.lic

# 6. gzip it
(
    cd ./dist && \
    # --force-local to treat as local file, or will treat call to http(s)
    tar czf $DIST_NAME.tar.gz $DIST_NAME --force-local
)
testError "tar failed"

exitOk
