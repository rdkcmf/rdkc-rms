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
RELEASE_NUMBER=`git rev-list --count HEAD`
echo $RELEASE_NUMBER

NEW_BUILD="rdkcms-1.7.1.$RELEASE_NUMBER-armv7ahf-Linaro_gcc4.9"
echo $NEW_BUILD

VERSION="1.7.1_comcast.$RELEASE_NUMBER-armv7ahf-Linaro_gcc4.9"

PACK_TIME=`date -u`
echo $PACK_TIME

#if have a new binary, go get it
cp ../dist/$NEW_BUILD/bin/rdkcms rms/bin/

#write a new version.txt
rm rms/version.txt
echo "version=$VERSION" >> rms/version.txt
echo "build_time=\"$PACK_TIME\"" >> rms/version.txt

cd rms
#in case we're remaking the same tarball
rm rms.tgz
tar -czf rms.tgz *

echo `readlink -e rms.tgz`
