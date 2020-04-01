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

# Generic script to manage the necessary files on the HDS project.
# March 2012

# Help function
help() {
 echo "Usage: $0 <option>"
 echo "\noption:"
 echo "  clean      - basically executes cleanup.sh"
 echo "  prepare    - prepares and copies the necessary folder structure and files relative to this script"
 echo "  cleanXML   - cleans the XML file modified by the server"
 echo "  cleanHDS   - removes any trace of directory changes when running the HDS project (same structure with fresh checkout)"
 echo "  build      - compiles the HDS project by running cmake and make"
 echo "  run        - runs the rms binary using the default configuration file"
 echo "  runFresh   - runs the options clean, prepare, compileHDS and run()"
 echo "  stop       - stops running instance of rdkcmediaserver"
 echo "  testHDS    - runs a ruby script to test HDS streaming"
 echo "  testHLS    - runs a ruby script to test HLS streaming"
 exit 0
}

prepare() {
    # prepares the necessary folder structure and files for input
    # this is only a temporary solution. this needs to be changed when we can commit
    # our own lua configuration file
    
    # copy the config folder
    cp -r ../../dist_template/config ../ && \
    # create the 'logs' folder
    mkdir ../logs && \
    # create the media folder
    mkdir ../media
    # copy the demo license file
    cp ../config/License.lic .

    if [ $? != 0 ]; then
     echo "Error preparing HDS project."
     exit 2
    fi
}

cleanXML() {
    # normally, when streams are pulled/added, the application modifies a certain xml file
    # (pushPullSetup.xml) and this needs to be reset for a clean start-up
    # just copy over
    cp ../../dist_template/config/pushPullSetup.xml ../config/

    if [ $? != 0 ]; then
     echo "Error copying pushPullSetup.xml"
     exit 2
    fi
}

cleanHDS() {
    # removes any trace of directory changes when running the HDS project
    # this will remove anything copied and created by the prepare option
    rm -rf ../config && \
    rm -rf ../logs && \
    rm -rf ../media && \
    rm -f License.lic

    if [ $? != 0 ]; then
     echo "Error cleaning HDS project."
     exit 3
    fi
}

compileHDS() {
    # build the server
    COMPILE_STATIC=1 cmake . && \
    make

    if [ $? != 0 ]; then
     echo "Error compiling HDS project."
     exit 4
    fi
}

run() {
    # executes the rdkcmediaserver binary and uses the default config.lua script copied from
    # the distro template folder (dist_templates)
    ./rdkcmediaserver/rdkcmediaserver ../config/config.lua

    if [ $? != 0 ]; then
     echo "Error running rms with HDS."
     exit 5
    fi
}

clean() {
    # call the clean-up script
    sh cleanup.sh
}

stopHDS() {
    # stop running instance of rdkcmediaserver
    killall ./rdkcmediaserver/rdkcmediaserver
}

testHDS() {
    # stream an input mp4 file as rtmp then output as hds
    ./test-hds.rb
}

testHLS() {
    # stream an input mp4 file as rtmp then output as hls
    ./test-hls.rb
}

DIR="$( cd "$( dirname "$0" )" && pwd )"
# DIR will now contain the absolute location of the script
#echo $DIR

# Test if the script is in the correct folder which is builders/cmake
# In the scenario that it is located somewhere, exit immediately
case "$DIR" in
 *builders/cmake*)
    #echo "Correct path."
    ;;
 *)
    echo "This script needs to be located at builders/cmake"
    exit 1
    ;;
esac

# main section of the script
case "$1" in
 "clean")
    clean
    ;;

 "prepare")
    prepare
    ;;

 "cleanXML")
    cleanXML
    ;;

 "cleanHDS")
    cleanHDS
    ;;

 "build")
    compileHDS
    ;;

 "run")
    run
    ;;

 "runFresh")
    clean
    prepare
    compileHDS
    run
    ;;

 "stop")
    stopHDS
    ;;

 "testHDS")
    testHDS
    ;;

 "testHLS")
    testHLS
    ;;

 *)
    # default option
    help

    ;;
esac

exit 0
