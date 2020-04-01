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
##########################################################################

ROOT_FOLDER=`realpath ../../..`
. $ROOT_FOLDER/packaging/functions

PACKAGE_NAME=$1
SHORT_DESC_FILE=$ROOT_FOLDER/packaging/descriptions/$1/short
LONG_DESC_FILE=$ROOT_FOLDER/packaging/descriptions/$1/long
SHORT_DESC=`cat $SHORT_DESC_FILE`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER`.`git rev-list --count HEAD`
TMP=./tmp
DIST=$TMP/$PACKAGE_NAME
DIST_RPM=$TMP/rpm

createFolder $DIST/var/rms-webroot

ls $ROOT_FOLDER/websources/access_policy/ | while read -a POLICYFILE ; do
	copyFiles $ROOT_FOLDER/websources/access_policy/$POLICYFILE $DIST/var/rms-webroot/
done

copyFiles $ROOT_FOLDER/websources/web_ui/ $DIST/var/rms-webroot/RMS_Web_UI/
copyFiles $ROOT_FOLDER/websources/rmswebservices $DIST/var/rms-webroot/
copyFiles $ROOT_FOLDER/websources/demo $DIST/var/rms-webroot/

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

echo "/var/rms-webroot" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/var/rms-webroot/RMS_Web_UI" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/var/rms-webroot/clientaccesspolicy.xml" >> $DIST_RPM/$PACKAGE_NAME.spec
echo "/var/rms-webroot/crossdomain.xml" >> $DIST_RPM/$PACKAGE_NAME.spec

echo "%config /var/rms-webroot/RMS_Web_UI/settings/account-settings.php" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "%config /var/rms-webroot/rmswebservices/config/config.ini" >>$DIST_RPM/$PACKAGE_NAME.spec

removeSvnFiles $DIST

RMS_DIST_FOLDER=`realpath $DIST` rpmbuild -bb $DIST_RPM/$PACKAGE_NAME.spec
testError "rpmbuild failed"

exitOk
