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

REL_TYPE=`cat $ROOT_FOLDER/RELEASE_NUMBER`
if [ "$REL_TYPE" = "" ]
then
	REL_TYPE="nb"
else
	REL_TYPE="release"
fi

rm -rf $TMP

createFolder $DIST/usr/bin
createFolder $DIST/usr/share/rms-avconv/presets


cat $ROOT_FOLDER/dist_template/scripts/unix/rmsTranscoder.sh \
	| sed "s/__RMS_AVCONV_BIN__/\/usr\/bin\/rms-avconv/" \
	| sed "s/__RMS_AVCONV_PRESETS__/\/usr\/share\/rms-avconv\/presets/" \
	> $DIST/usr/bin/rmsTranscoder.sh
chmod +x $DIST/usr/bin/rmsTranscoder.sh

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

echo "/usr/bin/rms-avconv" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/bin/rmsTranscoder.sh" >>$DIST_RPM/$PACKAGE_NAME.spec
echo "/usr/share/rms-avconv/presets" >>$DIST_RPM/$PACKAGE_NAME.spec

removeSvnFiles $DIST

RMS_DIST_FOLDER=`realpath $DIST` rpmbuild -bb $DIST_RPM/$PACKAGE_NAME.spec
testError "rpmbuild failed"

exitOk
