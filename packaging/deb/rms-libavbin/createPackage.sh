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

CODE_NAME=`cat $ROOT_FOLDER/CODE_NAME`
RELEASE_NUMBER=`cat $ROOT_FOLDER/RELEASE_NUMBER`.`git rev-list --count HEAD`-1
TMP=./tmp
DIST_NAME=${PACKAGE_NAME}_${RELEASE_NUMBER}_$ARCHITECTURE
DIST=$TMP/$DIST_NAME

sudo rm -rf $TMP
createFolder $TMP


createFolder $DIST/DEBIAN
createFolder $DIST/usr/bin
createFolder $DIST/usr/share/rms-avconv/presets



cat $ROOT_FOLDER/dist_template/scripts/unix/rmsTranscoder.sh \
	| sed "s/__RMS_AVCONV_BIN__/\/usr\/bin\/rms-avconv/" \
	| sed "s/__RMS_AVCONV_PRESETS__/\/usr\/share\/rms-avconv\/presets/" \
	> $DIST/usr/bin/rmsTranscoder.sh

echo "Package: ${PACKAGE_NAME}" >$DIST/DEBIAN/control
echo "Version: $RELEASE_NUMBER" >>$DIST/DEBIAN/control
echo "Section: non-free/video" >>$DIST/DEBIAN/control
echo "Priority: optional" >>$DIST/DEBIAN/control
echo "Depends: libc6" >>$DIST/DEBIAN/control
echo "Architecture: $ARCHITECTURE" >>$DIST/DEBIAN/control

echo "Description: $SHORT_DESC" >>$DIST/DEBIAN/control
echo "$LONG_DESC" >>$DIST/DEBIAN/control

removeSvnFiles $DIST

sudo sh applyPerms.sh $DIST
testError "Debian permissions failed"

dpkg-deb --build $DIST
testError "Debian packaging failed"

echo
echo "package created: `realpath $TMP/$DIST_NAME.deb`"
echo

exitOk
