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

BIND_ID=0
CONFIG_DIR=../config/config.lua

# Set to use the first folder that CMake will look at when looking for SSL
SSL_LIB_DIR=/opt/local
SSL_TMP_DIR=/tmp/openssl
SSL_TMP_TAR=/tmp/ssl.tar.gz
# Update as needed
SSL_PKG="https://www.openssl.org/source/openssl-1.1.0g.tar.gz"

initssl() {
    # Sanity check
    if [ `id -u` -ne 0 ]; then echo "Please run as root or sudoer"; exit 1; fi

    # Clean-up
    rm $SSL_TMP_TAR
    rm -rf $SSL_TMP_DIR
    mkdir $SSL_TMP_DIR
    rm CMakeCache.txt
    rm -rf CMakeFiles

    # Download the SSL package
    wget $SSL_PKG -O $SSL_TMP_TAR
    if [ $? != 0 ]; then echo "Could not download SSL package from $SSL_PKG"; exit 1; fi

    # Extract the tarball
    tar xvf $SSL_TMP_TAR -C $SSL_TMP_DIR --strip-components=1
    if [ $? != 0 ]; then echo "Could not extract $SSL_TMP_TAR to $SSL_TMP_DIR"; exit 1; fi

    # Build and install openssl
    cd $SSL_TMP_DIR
    ./config --prefix=$SSL_LIB_DIR --openssldir=$SSL_LIB_DIR && \
    make && make install_sw
    if [ $? != 0 ]; then echo "Could not build openssl"; exit 1; fi
}

prepare() {
    cp -r ../../dist_template/config ../ && \
    mkdir ../logs && \
    mkdir ../media
    cp ../config/License.lic .

    if [ $? != 0 ]; then
		echo "Error preparing RMS project."
		exit 2
    fi
}

build() {
    # build the server
	if [ $# -gt 1 ]; then
		export BIND_ID=$2
	fi
    COMPILE_STATIC=1 cmake . && \
    make
}

clean() {
	cd ../..
	sh cleanup.sh
	cd -
}

run() {
	if [ $# -gt 1 ]; then
		CONFIG_DIR=$2
	fi
	
    ./rdkcmediaserver/rdkcmediaserver $CONFIG_DIR
}
if [ $# -eq 0 ]; then
	echo "Usage: rms-builder [prepare|build|clean|run|initssl]"
else
	case "$1" in

		prepare)
			prepare
			;;
		build)
			if [ $# -gt 1 ]; then
				export BIND_ID=$2
			fi
			build
			;;
		clean)
			clean
			;;
		run)
			if [ $# -gt 1 ]; then
				CONFIG_DIR=$2
			fi
			run
			;;
		initssl)
			initssl
			;;
	esac
fi
