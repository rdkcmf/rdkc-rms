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
# Creates tarball that will be embedded on the *nix installers
# Also creates the zip file for windows
# Also creates the tarball for NPM (node JS package)
#

# Set the variables
RRS_VER=1.0.0
RRS_SOURCE=../sources
RRS_ARCHIVE=../archive

# Clean-up
rm $RRS_SOURCE/*.log

# Change to archives folder
cd $RRS_ARCHIVE/

# Remove older copies of the archives with the same version
rm rrs-$RRS_VER.*

# Create the tarball
tar hczf rrs-$RRS_VER.tar.gz rrs
if [ $? != 0 ]; then echo "Failed creating tarball!"; exit 1; fi

# Create a copy for NPM use
cp rrs-$RRS_VER.tar.gz rrs-$RRS_VER.tgz

# Create the zip
zip -r rrs-$RRS_VER.zip rrs
if [ $? != 0 ]; then echo "Failed creating zip file!"; exit 1; fi
