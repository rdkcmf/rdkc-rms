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

print_usage()
{
    echo ""
    echo "Usage: "
    echo "    $0 <company_name> <company_web_address_without_http> <company_contact_email> <release_number>"
    echo ""
    echo "Example:"
    echo "    $0 \"RDKC\" \"www.rdk.com\" \"contact@insertdomainhere.com\" \"3.0.2\""
    echo ""
    exit -1
}

# 1. Test and gather parameters
EXPECTED_ARGS=4
if [ $# -ne $EXPECTED_ARGS ]
then
	print_usage
	exit -1
fi

ROOT_FOLDER=../..

BRANDING_COMPANY_NAME=$1
BRANDING_WEB=$2
BRANDING_EMAIL=$3
RELEASE_NUMBER=$4
BARE_VERSION=`git rev-list --count HEAD`
VERSION="0.$BARE_VERSION"
if [ "$?" -ne "0" ]; then
    echo "git rev-list --count HEAD failed";
    exit -1;
fi
DIST_NAME="rdkcms-src-$RELEASE_NUMBER-$VERSION"
ARCH_NAME="$DIST_NAME.tar.gz"
TEMP_NAME="/tmp/$DIST_NAME"

echo "----------Summary------------"
echo "company_name:                     \"$BRANDING_COMPANY_NAME\""
echo "company_web_address_without_http: \"$BRANDING_WEB\""
echo "company_contact_email:            \"$BRANDING_EMAIL\""
echo "release_number:                   \"$RELEASE_NUMBER\""
echo "arch_name:                        \"$ARCH_NAME\""
echo "-----------------------------"

while true
do
	echo -n "Please confirm (y or n): "
	read CONFIRM
	case $CONFIRM in
		y|Y|YES|yes|Yes)
			break
			;;
		n|N|no|NO|No)
			exit -1
			;;
		*)
			echo Please enter only y or n
			;;
		esac
done


# 2. Copy and process all files
rm -rf $TEMP_NAME
for SRC_FILE in `cat all.txt`
do
	SRC_DIR=`dirname $SRC_FILE`
	DST_DIR=$TEMP_NAME/$SRC_DIR
	DST_FILE=$TEMP_NAME/$SRC_FILE
	mkdir -p $DST_DIR
	if [ "$?" -ne "0" ]; then
    	echo "mkdir failed for $DST_DIR";
    	exit 1;
	fi
	cat $ROOT_FOLDER/$SRC_FILE|sed '/#ifdef HAS_LICENSE/,/#endif \/\* HAS_LICENSE/d' >$DST_FILE
	if [ "$?" -ne "0" ]; then
        echo "processing file $SRC_FILE failed";
        exit 1;
    fi
	#echo "$SRC_FILE -> $DST_FILE"
done

# 3. Restore the link to cleanup.sh
(cd $TEMP_NAME/builders/cmake && ln -s ../../cleanup.sh ./)
if [ "$?" -ne "0" ]; then
    echo "unable to create link";
    exit 1;
fi

# 4. Fix various cmake things
cat $TEMP_NAME/builders/cmake/applications/CMakeLists.txt|grep -v "licensecreator"|grep -v "lminterface"|grep -v "httptests" >$TEMP_NAME/builders/cmake/applications/CMakeLists.txt.new
mv $TEMP_NAME/builders/cmake/applications/CMakeLists.txt.new $TEMP_NAME/builders/cmake/applications/CMakeLists.txt

cat $TEMP_NAME/builders/cmake/CMakeLists.txt|sed '/defines.h/,/ENDIF/d' \
	|grep -v "DHAS_PROTOCOL_MMS" \
	|grep -v "DHAS_PROTOCOL_EXTERNAL" \
	|grep -v "DHAS_PROTOCOL_RAWHTTPSTREAM" \
	|grep -v "DHAS_SYSLOG" \
	|grep -v "protocolmodules" \
	|grep -v "DHAS_LICENSE" \
	|grep -v "license" \
	|grep -v "trafficdissector" \
	|grep -v "mp4writer" \
	|grep -v "EXECUTE_PROCESS(COMMAND git rev-list --count" \
	|sed "s/\(.*\)RMS_VERSION_BUILD_NUMBER \(.*\)..RES_VAR.\(.*\)/\1RMS_VERSION_BUILD_NUMBER \2${BARE_VERSION}\3/"  >$TEMP_NAME/builders/cmake/CMakeLists.txt.new
mv $TEMP_NAME/builders/cmake/CMakeLists.txt.new $TEMP_NAME/builders/cmake/CMakeLists.txt
echo $RELEASE_NUMBER >$TEMP_NAME/RELEASE_NUMBER

# 5. Do the branding
cp -r $ROOT_FOLDER/constants $TEMP_NAME/
if [ "$?" -ne "0" ]; then
    echo "unable to copy constants";
    exit 1;
fi
echo "BRANDING_COMPANY_NAME=\"$BRANDING_COMPANY_NAME\"" >$TEMP_NAME/constants/branding.lua
echo "BRANDING_WEB=\"$BRANDING_WEB\"" >>$TEMP_NAME/constants/branding.lua
echo "BRANDING_EMAIL=\"$BRANDING_EMAIL\"" >>$TEMP_NAME/constants/branding.lua
(cd $TEMP_NAME/constants && lua constants.lua cpp $TEMP_NAME/sources/common/include/defines.h)
rm -rf $TEMP_NAME/constants


# 6. create the compile mini script
echo "sh cleanup.sh" >$TEMP_NAME/builders/cmake/compile.sh
echo "COMPILE_STATIC=1 cmake -DCMAKE_BUILD_TYPE=Release ." >>$TEMP_NAME/builders/cmake/compile.sh
echo "make" >>$TEMP_NAME/builders/cmake/compile.sh
chmod +x $TEMP_NAME/builders/cmake/compile.sh

# 7. Put the version number inside VERSION file
echo "VERSION: $VERSION" >$TEMP_NAME/VERSION
echo "CREATION DATE: `date`" >>$TEMP_NAME/VERSION

# 8. Create the zip
(cd $TEMP_NAME/../ && tar czf $ARCH_NAME $DIST_NAME)


# 9. Print some information
echo ""
echo ""
echo "The following folder contains the source distribution"
echo "    $TEMP_NAME"
echo ""
echo "**********************************************************************************"
echo "* Please go to the folder above inside builders/cmake and try to compile/run it. *"
echo "* If problems are found, please DO NOT distribute the tar.gz file                *"
echo "* For compiling, please use the compile.sh provided script                       *"
echo "**********************************************************************************"
echo ""
echo "Final tar.gz archive"
echo "    $TEMP_NAME.tar.gz"
echo ""

