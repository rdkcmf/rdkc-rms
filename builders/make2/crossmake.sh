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

if [ "$1" = "" ]
then
	echo "Usage:"
	echo "sh cross.sh yourBuild.cmk [ssl [Config.patch]]"
	echo "sh cross.sh clean"
	exit
fi

if [ "$1" = "clean" ]
then
	make clean
	exit
fi

#Collect and export build parameters
. ./$1
export TOOLCHAIN_BASE=$TOOLCHAIN_DIR
export TOOLCHAIN_PREFIX
export FULL_TOOLCHAIN=$TOOLCHAIN_BASE$TOOLCHAIN_PREFIX

# this if is here primarily for Yocto installs
if [ "$SDKTARGETSYSROOT" != "" ] 
then
	SSL_INSTALL_DIR=${SDKTARGETSYSROOT}
        SSL_AD_CFLAG="CFLAG=\"--sysroot=${SDKTARGETSYSROOT}\""
else
	SSL_INSTALL_DIR=${TOOLCHAIN_DIR}
	SSL_AD_CFLAG=
fi	
#SSL_INSTALL_DIR=/disk2/opt/gnueabihf-ssl/

#Build OpenSSL into the toolchain directory
if [ "$2" = "ssl" ]
then
	# Get the current working directory
	CWD=$(PWD)

	wget http://www.openssl.org/source/openssl-1.0.2g.tar.gz -O /tmp/ssl.tar.gz

	rm -rf /tmp/openssl_work
	mkdir /tmp/openssl_work
	tar xvf /tmp/ssl.tar.gz -C /tmp/openssl_work/
	cd /tmp/openssl_work/open*

	if [ "$3" != "" ]
	then
		patch -R Configure $3
	fi

	./Configure no-shared --prefix=${SSL_INSTALL_DIR} --openssldir=${SSL_INSTALL_DIR} ${SSL_ARCH}
	
	make clean
	if [ "$IOS_TARGET_ARCH" = "" ]
	then
		make CC="${FULL_TOOLCHAIN}gcc" AR="${FULL_TOOLCHAIN}ar r" RANLIB="${FULL_TOOLCHAIN}ranlib" ${SSL_AD_CFLAG}
	else
		make CC="${FULL_TOOLCHAIN}gcc -fembed-bitcode -arch ${IOS_TARGET_ARCH} -mios-version-min=${MIN_IOS_VERSION}" CFLAG="-isysroot ${PLATFORM_DIR}"
	fi

	sudo make install_sw

	# Return to the working directory earlier
	cd $CWD
fi



#Collect data for version.h
NOW=$(date +%s)
BUILD=$(git rev-list --count HEAD)
HASH=$(git rev-parse HEAD)
BRANCH=$(git symbolic-ref --short HEAD)
RELEASE=1.7.1
RELEASE_NAME=Pacman
if [ -n "$BIND_ID" ]; then
	export RMS_BIND_ID=$BIND_ID
else
	export RMS_BIND_ID=0
fi

echo
echo "RMS_BIND_ID set to $RMS_BIND_ID"
echo

#generate and copy version.h
echo "#define RMS_VERSION_BUILD_DATE $NOW" > version.h
echo "#define RMS_VERSION_BUILD_NUMBER \"$BUILD\"" >> version.h
echo "#define RMS_VERSION_HASH \"$HASH\"" >> version.h
echo "#define RMS_BRANCH_NAME \"$BRANCH\"" >> version.h
echo "#define RMS_BIND_ID $RMS_BIND_ID" >> version.h
echo "#define RMS_VERSION_RELEASE_NUMBER \"$RELEASE\"" >> version.h
echo "#define RMS_VERSION_CODE_NAME \"$RELEASE_NAME\"" >> version.h
echo "#define RMS_VERSION_BUILDER_OS_NAME \"$TARGET_OS_NAME\"" >> version.h
echo "#define RMS_VERSION_BUILDER_OS_VERSION \"$TARGET_OS_VERSION\"" >> version.h
echo "#define RMS_VERSION_BUILDER_OS_ARCH \"$TARGET_OS_ARCH\"" >> version.h
echo "#define RMS_VERSION_BUILDER_OS_UNAME \"$TARGET_OS_UNAME\"" >> version.h

cp version.h ../../sources/common/include/

#Build the RMS
make





