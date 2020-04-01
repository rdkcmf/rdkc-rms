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

# Usage:
#	./compilelibav.pi.sh <destination>
#	./compilelibav.pi.sh /c/gigi

# Use when deployed
LIBAV_FILE="https://libav.org/releases/libav-11.3.tar.gz"
LIBX264_FILE="ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20141218-2245-stable.tar.bz2"
FAAC_FILE="http://downloads.sourceforge.net/faac/faac-1.28.tar.gz"
FAAD_FILE="http://downloads.sourceforge.net/faac/faad2-2.7.tar.gz"
GETCMD=download
# Use for local files
#LOCAL_PATH="/home/ic/builds"
#LIBAV_FILE="$LOCAL_PATH/libav-11.3.tar.gz"
#LIBX264_FILE="$LOCAL_PATH/x264-snapshot-20141218-2245-stable.tar.bz2"
#FAAC_FILE="$LOCAL_PATH/faac-1.28.tar.gz"
#FAAD_FILE="$LOCAL_PATH/faad2-2.7.tar.gz"
#GETCMD=cp

ROOT_FOLDER=`realpath ../..`
. $ROOT_FOLDER/packaging/functions

#TODO this will be modified to compare for arm versions
# will change once the PIs arrive
if [ "`uname -s`" = "FreeBSD" ]
then
	MAKECMD=gmake
	AVEXTRALIBS=""
else
	MAKECMD=make
	AVEXTRALIBS="--extra-libs=\"-ldl\""
fi

MAKE_JOBS=-j4

OS_ARCH=$(uname -m | sed 's/x86_//;s/i[3-6]86/32/')

if [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    OS_NAME=$DISTRIB_ID
    OS_VERSION=$DISTRIB_RELEASE
elif [ -f /etc/redhat-release ]; then
    OS_NAME=`cat /etc/redhat-release | sed s/.*\(// | sed s/\)//`
    OS_VERSION=`cat /etc/redhat-release | sed s/.*release\ // | sed s/\ .*//`
else
    OS_NAME=$(uname -s)
    OS_VERSION=$(uname -r)
fi

WORKING_DIR="$1/$OS_NAME/$OS_ARCH"

if [ -d "$WORKING_DIR" ]
then
	echo Folder $WORKING_DIR already present. Abort
	exit 1
fi

mkdir -p $WORKING_DIR
testError "Unable to create folder $WORKING_DIR"

WORKING_DIR=`realpath $WORKING_DIR`

mkdir -p $WORKING_DIR/libav
testError "Unable to create folder $WORKING_DIR/libav"

mkdir -p $WORKING_DIR/libx264
testError "Unable to create folder $WORKING_DIR/libx264"

$GETCMD "$LIBAV_FILE" "$WORKING_DIR/libav.tar.gz"
$GETCMD "$LIBX264_FILE" "$WORKING_DIR/libx264.tar.bz2"

mkdir -p $WORKING_DIR/aac
testError "Unable to create folder $WORKING_DIR/aac"

$GETCMD "$FAAC_FILE" "$WORKING_DIR/aac/faac.tar.gz"
$GETCMD "$FAAD_FILE" "$WORKING_DIR/aac/faad2.tar.gz"

mkdir -p $WORKING_DIR/out
testError "Unable to create folder $WORKING_DIR/out"

cd $WORKING_DIR/aac
testError "Unable to cd into $WORKING_DIR/aac"

mkdir -p $WORKING_DIR/aac/faac

tar xf faac.tar.gz --strip 1 -C faac/
testError "Unable to unzip faac.tar.gz"

mkdir -p $WORKING_DIR/aac/faad2

tar xf faad2.tar.gz --strip 1 -C faad2/
testError "Unable to unzip faad2.tar.gz"

cd $WORKING_DIR/aac/faac
testError "Unable to cd into $WORKING_DIR/aac/faac"

echo "Configure faac"
./configure --enable-shared=no --enable-static=yes --without-mp4v2 --prefix=$WORKING_DIR/out
testError "Unable to configure faac"

echo "Install faac"
$MAKECMD $MAKE_JOBS install
testError "Unable to install faac"

cd $WORKING_DIR/aac/faad2
testError "Unable to cd into $WORKING_DIR/aac/faad2"

echo "Configure faad2"
./configure --enable-shared=no --enable-static=yes --prefix=$WORKING_DIR/out
testError "Unable to configure faad2"

echo "Install faad2"
$MAKECMD $MAKE_JOBS install
testError "Unable to install faad2"

cd $WORKING_DIR/

tar xjf libx264.tar.bz2 --strip 1 -C libx264/
testError "Unable to unzip libx264.tar.bz2"

tar xf libav.tar.gz --strip 1 -C libav/
testError "Unable to unzip libav.tar.gz"

PATH=$PATH:$WORKING_DIR/out/bin

cd $WORKING_DIR/libx264
testError "Unable to cd into $WORKING_DIR/libx264"

echo "Configure libx264"
#TODO this will be adjusted based on arm version
./configure --enable-debug --disable-opencl --enable-static --extra-cflags="-march=armv6 -mfpu=vfp" --prefix=$WORKING_DIR/out
testError "Unable to configure libx264"

echo "Install libx264"
$MAKECMD $MAKE_JOBS install
testError "Unable to install libx264"

cd $WORKING_DIR/libav
testError "Unable to cd into $WORKING_DIR/libav"

export PKG_CONFIG_PATH="$WORKING_DIR/out/lib/pkgconfig"

echo "Configure libav"
./configure --extra-cflags="-I$WORKING_DIR/out/include" --extra-ldflags="-L$WORKING_DIR/out/lib" $AVEXTRALIBS --enable-gpl --enable-nonfree --disable-outdevs --disable-indevs --enable-libx264 --enable-libfaac --prefix=$WORKING_DIR/out
testError "Unable to configure libav"

echo "Install libav"
$MAKECMD $MAKE_JOBS install
testError "Unable to install libav"

echo "Strip avconv"
if [ "`uname -s`" = "Darwin" ]
then
	strip -X $WORKING_DIR/out/bin/avconv
else
	strip -s $WORKING_DIR/out/bin/avconv
fi
testError "Unable to strip $WORKING_DIR/out/bin/avconv"

(cd $WORKING_DIR/out/bin && tar czf ../../avconv-$OS_NAME-$OS_VERSION-$OS_ARCH.tar.gz avconv)
echo $WORKING_DIR/avconv-$OS_NAME-$OS_VERSION-$OS_ARCH.tar.gz
