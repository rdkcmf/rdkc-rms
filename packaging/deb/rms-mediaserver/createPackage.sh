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

ROOT_FOLDER=`realpath ../../..`
. $ROOT_FOLDER/packaging/functions

PACKAGE_NAME=$1
SHORT_DESC_FILE=$ROOT_FOLDER/packaging/descriptions/$1/short
LONG_DESC_FILE=$ROOT_FOLDER/packaging/descriptions/$1/long
SHORT_DESC=`cat $SHORT_DESC_FILE`
LONG_DESC=`cat $LONG_DESC_FILE|sed "s/^/ /"|sed "s/^ $/ ./"`
if [ "$OS_ARCH" = "i686" ]
then
	ARCHITECTURE=i386
elif [ "$OS_ARCH" = "x86_64" ]
then
	ARCHITECTURE=amd64
else
	echo "Invalid architecture detected"
	exit 1
fi

CURRENT_DIR=`pwd`
cd $ROOT_FOLDER/packaging/unix_generic
sh createPackage.sh no
testError "Compile failed"
rm ./dist/*.tar.gz
cd $CURRENT_DIR

CODE_NAME=`cat $ROOT_FOLDER/CODE_NAME`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER`.`git rev-list --count HEAD`-1
TMP=./tmp
DIST_NAME=${PACKAGE_NAME}_${RELEASE_NUMBER}_$ARCHITECTURE
DIST=$TMP/$DIST_NAME

sudo rm -rf $TMP
createFolder $TMP
copyFiles $ROOT_FOLDER/packaging/unix_generic/dist/rms* $TMP/rms


createFolder $DIST/DEBIAN
createFolder $DIST/usr/bin
createFolder $DIST/usr/bin/rms-phpengine
createFolder $DIST/etc/init.d/
createFolder $DIST/etc/rdkcms
createFolder $DIST/var/rdkcms/xml
createFolder $DIST/var/rdkcms/media
createFolder $DIST/var/log/rdkcms
createFolder $DIST/var/run/rdkcms
createFolder $DIST/usr/share/doc/rdkcms/version
createFolder $DIST/usr/share/lintian/overrides

moveFiles $TMP/rms/bin/rdkcms $DIST/usr/bin/
moveFiles $TMP/rms/bin/rms-mp4writer $DIST/usr/bin/
moveFiles $TMP/rms/bin/rms-webserver $DIST/usr/bin/
moveFiles $TMP/rms/bin/rms-phpengine/php-cgi $DIST/usr/bin/rms-phpengine
for i in `find $TMP/rms/config -type f -name "*\.xml"`;do moveFiles $i $DIST/var/rdkcms/xml/;done
for i in `find $TMP/rms/config -type f -name "*\.lua"`;do moveFiles $i $DIST/etc/rdkcms/;done
for i in `find $TMP/rms/config -type f -name "*\list.txt" -o -name "*\server.*"`;do moveFiles $i $DIST/etc/rdkcms/;done
moveFiles $TMP/rms/BUILD_DATE $DIST/usr/share/doc/rdkcms/version/
echo $CODE_NAME >$DIST/usr/share/doc/rdkcms/version/CODE_NAME
echo $RELEASE_NUMBER >$DIST/usr/share/doc/rdkcms/version/RELEASE_NUMBER
echo $OS_NAME >$DIST/usr/share/doc/rdkcms/version/OS_NAME
echo $OS_VERSION >$DIST/usr/share/doc/rdkcms/version/OS_VERSION
git rev-list --count HEAD >$DIST/usr/share/doc/rdkcms/version/BUILD_NUMBER
copyFiles $ROOT_FOLDER/dist_template/COPYRIGHT $DIST/usr/share/doc/rdkcms/copyright
cp $ROOT_FOLDER/dist_template/documents/* $DIST/usr/share/doc/rdkcms/

cat $ROOT_FOLDER/dist_template/config/config.lua \
	| sed "s/__STANDARD_LOG_FILE_APPENDER__/\/var\/log\/rdkcms\/rdkcms/" \
	| sed "s/__STANDARD_TIME_PROBE_APPENDER__/\/var\/log\/rdkcms\/timeprobe/" \
	| sed "s/__STANDARD_MEDIA_FOLDER__/\/var\/rdkcms\/media/" \
	| sed "s/__STANDARD_CONFIG_PERSISTANCE_FILE__/\/var\/rdkcms\/xml\/pushPullSetup.xml/" \
	| sed "s/__STANDARD_AUTH_PERSISTANCE_FILE__/\/var\/rdkcms\/xml\/auth.xml/" \
	| sed "s/__STANDARD_CONN_LIMITS_PERSISTANCE_FILE__/\/var\/rdkcms\/xml\/connlimits.xml/" \
	| sed "s/__STANDARD_BW_LIMITS_PERSISTANCE_FILE__/\/var\/rdkcms\/xml\/bandwidthlimits.xml/" \
	| sed "s/__STANDARD_INGEST_POINTS_PERSISTANCE_FILE__/\/var\/rdkcms\/xml\/ingestpoints.xml/" \
	| sed "s/__STANDARD_AUTO_DASH_FOLDER__/\/var\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_HLS_FOLDER__/\/var\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_HDS_FOLDER__/\/var\/rms-webroot/" \
	| sed "s/__STANDARD_AUTO_MSS_FOLDER__/\/var\/rms-webroot/" \
	| sed "s/__STANDARD_RTMP_USERS_FILE__/\/etc\/rdkcms\/users.lua/" \
	| sed "s/__STANDARD_RTSP_USERS_FILE__/\/etc\/rdkcms\/users.lua/" \
	| sed "s/__STANDARD_EVENT_FILE_APPENDER__/\/var\/log\/rdkcms\/events.txt/" \
	| sed "s/__STANDARD_TRANSCODER_SCRIPT__/\/usr\/bin\/rmsTranscoder.sh/" \
	| sed "s/__STANDARD_MP4WRITER_BIN__/\/usr\/bin\/rms-mp4writer/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/\/etc\/rdkcms\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/\/etc\/rdkcms\/server.cert/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	> $DIST/etc/rdkcms/config.lua

cat $ROOT_FOLDER/dist_template/config/webconfig.lua \
	| sed "s/__STANDARD_WEBSERVER_LOG_FILE_APPENDER__/\/var\/log\/rdkcms\/rms-webserver/" \
	| sed "s/__STANDARD_WHITELIST_FILE__/\/etc\/rdkcms\/whitelist.txt/" \
	| sed "s/__STANDARD_BLACKLIST_FILE__/\/etc\/rdkcms\/blacklist.txt/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/\/etc\/rdkcms\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/\/etc\/rdkcms\/server.cert/" \
	| sed "s/__STANDARD_WEBROOT_FOLDER__/\/var\/rms-webroot/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	> $DIST/etc/rdkcms/webconfig.lua

copyFiles preinst $DIST/DEBIAN/
copyFiles postinst $DIST/DEBIAN/
copyFiles prerm $DIST/DEBIAN/
copyFiles postrm $DIST/DEBIAN/
copyFiles rdkcms $DIST/etc/init.d/
copyFiles overrides $DIST/usr/share/lintian/overrides/rdkcms

echo "Package: ${PACKAGE_NAME}" >$DIST/DEBIAN/control
echo "Version: $RELEASE_NUMBER" >>$DIST/DEBIAN/control
echo "Section: non-free/video" >>$DIST/DEBIAN/control
echo "Priority: optional" >>$DIST/DEBIAN/control
echo "Depends: libc6, rms-libavbin, rms-web" >>$DIST/DEBIAN/control
echo "Architecture: $ARCHITECTURE" >>$DIST/DEBIAN/control

echo "Description: $SHORT_DESC" >>$DIST/DEBIAN/control
echo "$LONG_DESC" >>$DIST/DEBIAN/control

echo "/etc/rdkcms/config.lua" >$DIST/DEBIAN/conffiles
echo "/etc/rdkcms/users.lua" >>$DIST/DEBIAN/conffiles
echo "/etc/rdkcms/webconfig.lua" >>$DIST/DEBIAN/conffiles
echo "/etc/rdkcms/blacklist.txt" >>$DIST/DEBIAN/conffiles
echo "/etc/rdkcms/whitelist.txt" >>$DIST/DEBIAN/conffiles
echo "/var/rdkcms/xml/auth.xml" >>$DIST/DEBIAN/conffiles
echo "/var/rdkcms/xml/bandwidthlimits.xml" >>$DIST/DEBIAN/conffiles
echo "/var/rdkcms/xml/connlimits.xml" >>$DIST/DEBIAN/conffiles
echo "/var/rdkcms/xml/ingestpoints.xml" >>$DIST/DEBIAN/conffiles
echo "/var/rdkcms/xml/pushPullSetup.xml" >>$DIST/DEBIAN/conffiles
echo "/etc/init.d/rdkcms" >>$DIST/DEBIAN/conffiles

removeSvnFiles $DIST

sudo sh applyPerms.sh $DIST
testError "Debian permissions failed"

dpkg-deb --build $DIST
testError "Debian packaging failed"

echo
echo "package created: `realpath $TMP/$DIST_NAME.deb`"
echo

exitOk
