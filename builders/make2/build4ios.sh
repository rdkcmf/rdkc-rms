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
#
# Build the following static libraries for iOS:
# rdkcms
# ssl
# crypto
#
# A 'fat' library is then created from each of these
#

TARGET_DIR=./output/static
FAT_LIBRARY=./fat
UPDATE_SSL=$1
export TOOLCHAIN_DIR=/Applications/Xcode.app/Contents/Developer

if [ ! -d $FAT_LIBRARY ]; then
	mkdir $FAT_LIBRARY
fi

build()
{
	ARCH=$1

	export IOS_TARGET_ARCH=$ARCH

	echo "Cleaning..."
	make clean
	rm -rf $FAT_LIBRARY/$ARCH
	mkdir $FAT_LIBRARY/$ARCH

	echo "Building for $ARCH $UPDATE_SSL..."
	sh crossmake.sh ios.cmk $UPDATE_SSL

	echo "Done building for $ARCH"

	echo "Copying RMS library from $TARGET_DIR to $FAT_LIBRARY/$ARCH/"
	cp $TARGET_DIR/*.a $FAT_LIBRARY/$ARCH/

	echo "Copying SSL libraries from $TOOLCHAIN_DIR/lib to $FAT_LIBRARY/$ARCH/"
	cp $TOOLCHAIN_DIR/lib/*.a $FAT_LIBRARY/$ARCH/
}

if [ "$UPDATE_SSL" = "test" ]; then
# Test build to quickly validate any code changes
build "x86_64"
else
build "arm64"
build "armv7"
build "x86_64"
#build "i386"
fi

if [ "$UPDATE_SSL" = "ssl" ]; then
echo "Creating SSL fat library..."
lipo \
	"$FAT_LIBRARY/arm64/libcrypto.a" \
	"$FAT_LIBRARY/armv7/libcrypto.a" \
	"$FAT_LIBRARY/x86_64/libcrypto.a" \
	-create -output $FAT_LIBRARY/libcrypto.a
#	"$FAT_LIBRARY/i386/libcrypto.a" \
#	-create -output $FAT_LIBRARY/libcrypto.a

lipo \
	"$FAT_LIBRARY/arm64/libssl.a" \
	"$FAT_LIBRARY/armv7/libssl.a" \
	"$FAT_LIBRARY/x86_64/libssl.a" \
	-create -output $FAT_LIBRARY/libssl.a
#	"$FAT_LIBRARY/i386/libssl.a" \
#	-create -output $FAT_LIBRARY/libssl.a
fi

echo "Creating RMS fat library..."
lipo \
	"$FAT_LIBRARY/arm64/librdkcms.a" \
	"$FAT_LIBRARY/armv7/librdkcms.a" \
	"$FAT_LIBRARY/x86_64/librdkcms.a" \
	-create -output $FAT_LIBRARY/librdkcms.a
#	"$FAT_LIBRARY/i386/librdkcms.a" \
#	-create -output $FAT_LIBRARY/librdkcms.a

echo "Done"

