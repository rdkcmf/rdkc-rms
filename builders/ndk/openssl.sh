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


#ALL_ARCHS="arm armv7 arm64 x86 x86_64"
ALL_ARCHS="arm armv7 arm64 x86 mips"
ARCHS=""
DL_LINK="https://www.openssl.org/source/old/1.0.2/openssl-1.0.2g.tar.gz"

for ARCH in "$@"
do
	case $ARCH in
		arm)
			ARCHS=$ARCHS" "$ARCH
			;;
                x86)
                        ARCHS=$ARCHS" "$ARCH
                        ;;
                mips)
                        ARCHS=$ARCHS" "$ARCH
                        ;;
                arm64)
                        ARCHS=$ARCHS" "$ARCH
                        ;;
                x86_64)
                        ARCHS=$ARCHS" "$ARCH
                        ;;
                mips64)
                        ARCHS=$ARCHS" "$ARCH
                        ;;
		all)
			ARCHS=$ALL_ARCHS
			;;
	esac
done

if [ "$ARCHS" = "" ]
then
	ARCHS=$ALL_ARCHS
fi
echo "Building OpenSSL for "$ARCHS
wget $DL_LINK -O /tmp/ssl.tar.gz

if [ "$ARCHS" = "$ALL_ARCHS" ] && [ -f set-dev-env.sh ]
then
	echo "Removing existing dev setting script"
	rm -f set-dev-env.sh
fi

COMPLETED=""
for TARGET in $ARCHS
do
	echo $TARGET
	sudo sh openssl_arch.sh -a $TARGET
	COMPLETED=$COMPLETED" "$TARGET
	echo "Build done for$COMPLETED"
done
