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
# Script to create the linux, darwin, sun OS, and possibly ARM installers for RRS
# <script name> <OS> <ARCH>
# ./createInstaller.sh linux x64
#

# Set the variables to be used
RRS_VER=1.0.0
RRS_OS=$1
RRS_ARCH=$2

# Sanity checks
if [ "$RRS_OS" == "" ]; then echo "Indicate OS/Platform as first argument!"; exit 1; fi
if [ "$RRS_ARCH" == "" ]; then echo "Indicate Architecture (x86/x64/armv6l/armv7l) as second argument!"; exit 1; fi

# Set the RRS archive code and installer script
DATA_TAR="../archive/rrs-$RRS_VER.tar.gz"
INSTALLER_SH="../installers/rrs-$RRS_VER-$RRS_OS-$RRS_ARCH.sh"

# Create the 'installer' script
echo "Creating the installer..."
printf "#!/bin/sh
#
# Script to install node JS, RRS and PM2 package, and then start RRS.
#

# Get the needed parameters
OS=$RRS_OS
ARCH=$RRS_ARCH
VER=4.1.1
NODE_JS=http://nodejs.org/dist/v\$VER/node-v\$VER-\$OS-\$ARCH.tar.gz

# Download and extract needed node JS
curl \$NODE_JS | tar xzf - --strip-components=1
if [ \$? != 0 ]; then echo \"Could not download node binary: \$NODE_JS!\"; exit 1; fi

# Install dependencies
echo 'Installing RRS...'
bin/npm install http://tarballs.insertdomainhere.com/rrs/rrs-\$RRS_VER.tgz #TODO: replace with correct path
if [ \$? != 0 ]; then echo 'Could not install RRS!'; exit 1; fi

echo 'Installing pm2...'
bin/npm install pm2 -g
if [ \$? != 0 ]; then echo 'Could not install pm2!'; exit 1; fi

# Change to where RRS is
cd node_modules/rrs

# Start RRS
echo 'Starting RRS...'
../../bin/pm2 start rrs.js
if [ \$? != 0 ]; then echo 'Could not start RRS!'; exit 1; fi

exit 0
" > "$INSTALLER_SH"

# Toggle the execution bit
chmod +x "$INSTALLER_SH"

