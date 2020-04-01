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

ENV_FILE=set-dev-env.sh

. ./setenv-android.sh 

if [ ! -f $ENV_FILE ]
then
	echo "Creating env file"
	echo "#!/bin/sh" > $ENV_FILE
	echo ""
	echo "export ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT" >> $ENV_FILE
	echo "export ANDROID_API=$ANDROID_API" >> $ENV_FILE

fi

CONFIG=android
case $ARCH in
	arm)
		CONFIG=android
		echo "export OPENSSL_DIR_ARM=$OPENSSL_DIR" >> $ENV_FILE
		;;
	armv7)
		CONFIG=android-armv7
		echo "export OPENSSL_DIR_ARM_V7=$OPENSSL_DIR" >> $ENV_FILE
		;;
        arm64)
                CONFIG=android
                echo "export OPENSSL_DIR_ARM_64=$OPENSSL_DIR" >> $ENV_FILE
                ;;
	x86)
		CONFIG=android-x86
		echo "export OPENSSL_DIR_X86=$OPENSSL_DIR" >> $ENV_FILE
		;;
	x86_64)
		CONFIG=android
		echo "export OPENSSL_DIR_X86_64=$OPENSSL_DIR" >> $ENV_FILE
		;;
	mips)
		CONFIG=android-mips
		echo "export OPENSSL_DIR_MIPS=$OPENSSL_DIR" >> $ENV_FILE
		;;
	mips64)
		CONFIG=android
		echo "export OPENSSL_DIR_MIPS_64=$OPENSSL_DIR" >> $ENV_FILE
		;;
	*)
		CONFIG=android
		echo "export OPENSSL_DIR_ARM=$OPENSSL_DIR" >> $ENV_FILE
		;;
esac

rm -rf /tmp/openssl_work
mkdir /tmp/openssl_work
tar xvf /tmp/ssl.tar.gz -C /tmp/openssl_work/
cd /tmp/openssl_work/open*

./Configure no-shared no-ssl2 no-ssl3 no-comp no-hw no-asm $CONFIG --prefix=$OPENSSL_DIR --openssldir=$OPENSSL_DIR
make depend
sudo -E make install CC="${ANDROID_TOOLCHAIN}/${CROSS_COMPILE}gcc -fPIC" AR="${ANDROID_TOOLCHAIN}/${CROSS_COMPILE}ar r" RANLIB=${ANDROID_TOOLCHAIN}/${CROSS_COMPILE}ranlib
cd -

