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

TOOLCHAIN_BASE=$1
DESTINATION=$2


#wget http://www.openssl.org/source/openssl-1.0.1c.tar.gz -O /tmp/ssl.tar.gz
#rm -rf /tmp/openssl_work
#mkdir /tmp/openssl_work
#tar xvf /tmp/ssl.tar.gz -C /tmp/openssl_work/
cd /tmp/openssl_work/open*
./Configure no-shared --prefix=${DESTINATION} --openssldir=${DESTINATION} android-armv7
make clean
make CC="${TOOLCHAIN_BASE}gcc" AR="${TOOLCHAIN_BASE}ar r" RANLIB="${TOOLCHAIN_BASE}ranlib"
sudo make install
