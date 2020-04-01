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
CURRENT_DIR=`pwd`
cd $ROOT_FOLDER/packaging/unix_generic
sh createPackage.sh no
testError "Compile failed"
rm ./dist/*.tar.gz
cd $CURRENT_DIR

CODE_NAME=`cat $ROOT_FOLDER/CODE_NAME`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER`.`git rev-list --count HEAD`
TMP=./tmp
DIST=$TMP/$PACKAGE_NAME
DIST_RPM=$TMP/rpm

rm -rf $TMP
createFolder $TMP
copyFiles $ROOT_FOLDER/packaging/unix_generic/dist/rms* $TMP/rms

createFolder $DIST/usr/bin
createFolder $DIST/usr/bin/rms-phpengine
createFolder $DIST/etc/init.d/
createFolder $DIST/etc/rdkcms
createFolder $DIST/var/rdkcms/xml
createFolder $DIST/var/rdkcms/media
createFolder $DIST/var/log/rdkcms
createFolder $DIST/var/run/rdkcms
createFolder $DIST/usr/share/doc/rdkcms/version

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

copyFiles rdkcms $DIST/etc/init.d/
rm -rf $TMP/rms


createFolder $DIST_RPM
DIST_RPM=`realpath $DIST_RPM`

mkdir -p $DIST_RPM/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp}
testError "Unable to create RPM builder folders"

echo "%_topdir      $DIST_RPM" >$HOME/.rpmmacros
testError "Unable to create rpmmacros file"

cat $PACKAGE_NAME.spec.template \
	|sed "s/__VERSION__/$RELEASE_NUMBER/g" \
	|sed "s/__PACKAGE_NAME__/$PACKAGE_NAME/g" \
	|sed "s/__SUMMARY__/$SHORT_DESC/g" \
	|sed -e "/__DESCRIPTION__/r $LONG_DESC_FILE"|sed -e "/__DESCRIPTION__/d" \
	>$DIST_RPM/$PACKAGE_NAME.spec

echo "/etc/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/etc/init.d/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/bin/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/bin/rms-mp4writer" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/bin/rms-webserver" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/bin/rms-phpengine/php-cgi" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/share/doc/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/var/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/var/log/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/var/run/rdkcms" >>$DIST_RPM/$PACKAGE_NAME.spec

echo "%config /etc/rdkcms/config.lua" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /etc/rdkcms/users.lua" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /etc/rdkcms/webconfig.lua" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /etc/rdkcms/blacklist.txt" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /etc/rdkcms/whitelist.txt" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rdkcms/xml/auth.xml" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rdkcms/xml/bandwidthlimits.xml" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rdkcms/xml/connlimits.xml" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rdkcms/xml/ingestpoints.xml" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rdkcms/xml/pushPullSetup.xml" >>$DIST_RPM/$PACKAGE_NAME.spec


removeSvnFiles $DIST

RMS_DIST_FOLDER=`realpath $DIST` rpmbuild -bb $DIST_RPM/$PACKAGE_NAME.spec
testError "rpmbuild failed"

exitOk
