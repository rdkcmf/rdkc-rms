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

. `realpath ./functions`
TAR=`realpath $1`

#1. Cleanup and create the DIST folder
if [ -d ./tmp ]
then
	rm -rf ./tmp
fi
mkdir -p ./tmp

DIST=`realpath ./tmp`
echo "DIST: $DIST"


#2. Unzip the archive
tar xvf $TAR -C $DIST
testError "tar xvf $TAR -C $DIST"
ORIG=`find $DIST/ -type d -mindepth 1 -maxdepth 1`
echo "ORIG: $ORIG"

createFolder $DIST/usr/local/bin
createFolder $DIST/usr/local/bin/rms-phpengine
createFolder $DIST/usr/local/etc/rc.d
createFolder $DIST/usr/local/etc/rdkcms
createFolder $DIST/var/rdkcms/xml
createFolder $DIST/var/rdkcms/media
createFolder $DIST/var/log/rdkcms
createFolder $DIST/var/run/rdkcms
createFolder $DIST/usr/local/share/doc/rdkcms/version
createFolder $DIST/usr/local/share/rms-avconv/presets

moveFiles $ORIG/bin/rdkcms $DIST/usr/local/bin
moveFiles $ORIG/bin/rms-avconv $DIST/usr/local/bin
moveFiles $ORIG/bin/rms-mp4writer $DIST/usr/local/bin
moveFiles $ORIG/bin/rms-phpengine/php-cgi $DIST/usr/local/bin/rms-phpengine
copyFiles ./rdkcms $DIST/usr/local/etc/rc.d/
for i in `find $ORIG/config -type f -name "*\.xml"`;do moveFiles $i $DIST/var/rdkcms/xml/;done
for i in `find $ORIG/config -type f -name "*\.lua"`;do moveFiles $i $DIST/usr/local/etc/rdkcms/;done
for i in `find $ORIG/config -type f -name "*\list.txt" -o -name "*\server.*"`;do moveFiles $i $DIST/usr/local/etc/rdkcms/;done
for i in `find $ORIG/doc -type f`;do moveFiles $i $DIST/usr/local/share/doc/rdkcms/;done
moveFiles $ORIG/BUILD_DATE $DIST/usr/local/share/doc/rdkcms/version/
mv $ORIG/rms-avconv-presets/*.avpreset $DIST/usr/local/share/rms-avconv/presets/
testError "Unable to copy presets"

cat ./config.lua \
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
	| sed "s/__STANDARD_RTMP_USERS_FILE__/\/usr\/local\/etc\/rdkcms\/users.lua/" \
	| sed "s/__STANDARD_RTSP_USERS_FILE__/\/usr\/local\/etc\/rdkcms\/users.lua/" \
	| sed "s/__STANDARD_EVENT_FILE_APPENDER__/\/var\/log\/rdkcms\/events.txt/" \
	| sed "s/__STANDARD_TRANSCODER_SCRIPT__/\/usr\/local\/bin\/rmsTranscoder.sh/" \
	| sed "s/__STANDARD_MP4WRITER_BIN__/\/usr\/local\/bin\/rms-mp4writer/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/\/usr\/local\/etc\/rdkcms\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/\/usr\/local\/etc\/rdkcms\/server.cert/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	> $DIST/usr/local/etc/rdkcms/config.lua
testError "sed-ing config.lua failed"

cat $ROOT_FOLDER/dist_template/config/webconfig.lua \
	| sed "s/__STANDARD_WEBSERVER_LOG_FILE_APPENDER__/\/var\/log\/rdkcms\/rms-webserver/" \
	| sed "s/__STANDARD_WHITELIST_FILE__/\/usr\/local\/etc\/rdkcms\/whitelist.txt/" \
	| sed "s/__STANDARD_BLACKLIST_FILE__/\/usr\/local\/etc\/rdkcms\/blacklist.txt/" \
	| sed "s/__STANDARD_SERVERKEY_FILE__/\/usr\/local\/etc\/rdkcms\/server.key/" \
	| sed "s/__STANDARD_SERVERCERT_FILE__/\/usr\/local\/etc\/rdkcms\/server.cert/" \
	| sed "s/__STANDARD_WEBROOT_FOLDER__/\/var\/rms-webroot/" \
	| sed "/--\[\[remove/,/remove\]\]--/ d" \
	> $DIST/usr/local/etc/rdkcms/webconfig.lua
testError "sed-ing webconfig.lua failed"	

cat ./rmsTranscoder.sh \
	| sed "s/__RMS_AVCONV_BIN__/\/usr\/local\/bin\/rms-avconv/" \
	| sed "s/__RMS_AVCONV_PRESETS__/\/usr\/local\/share\/rms-avconv\/presets/" \
	> $DIST/usr/local/bin/rmsTranscoder.sh
testError "sed-ing rmsTranscoder.sh failed"

rm -rf $ORIG
testError "Cleanup failed"

pw adduser rmsd -d /nonexistent -s /usr/sbin/nologin -c "RDKC Media Server Daemon"
testError "pw adduser rmsd failed"

chown -R root:wheel $DIST
testError "Apply permissions failed"

chmod -R 644 $DIST
testError "Apply permissions failed"

chmod +x $DIST/usr/local/bin/*
testError "Apply permissions failed"

chmod +x $DIST/usr/local/etc/rc.d/*
testError "Apply permissions failed"

for i in `find $DIST -type d`
do
	chmod +x $i
	testError "Apply permissions failed"
done

chown -R root:rmsd $DIST/var/rdkcms
testError "Apply permissions failed"

chown -R root:rmsd $DIST/var/log/rdkcms
testError "Apply permissions failed"

chown -R root:rmsd $DIST/var/run/rdkcms
testError "Apply permissions failed"

(cd $DIST && tar cf /dist.tar ./*)
testError "Tar failed"

(cd / && tar xvf dist.tar)
testError "Tar failed"

echo "rdkcms_enable=\"YES\"" >>/etc/rc.conf

exitOk;

