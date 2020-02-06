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
##########################################################################

#######################################################
#						      #	
# Build Framework standard script for video-analytics #
#						      #
#######################################################

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e

# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}
#default component name
export RDK_TOOLCHAIN_PATH=$RDK_PROJECT_ROOT_PATH/sdk/toolchain/arm-linux-gnueabihf
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
#source $RDK_SOURCE_PATH/src/plugins/soc/build/soc_env.sh
#export FSROOT=$RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk/img/fs/shadow_root/
export RDK_SDROOT=$RDK_PROJECT_ROOT_PATH/sdk/fsroot/src/vendor/img/fs/shadow_root/
export FSROOT=$RDK_FSROOT_PATH
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH
export BUILDS_DIR=$RDK_PROJECT_ROOT_PATH
export RDK_DIR=$BUILDS_DIR
export PLATFORM_SDK=${RDK_TOOLCHAIN_PATH}
export ENABLE_RDKC_LOGGER_SUPPORT=true
export PLATFORM_SYMBOL_PATH=$RDK_PROJECT_ROOT_PATH/sdk/fsroot/syms
export PLATFORM_BREAKPAD_BINARY=${RDK_PROJECT_ROOT_PATH}/utility/prebuilts/breakpad-prebuilts/x86/dump_syms

# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
if [ $XCAM_MODEL == "XHB1" ]; then
 	source ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
	echo "Enable xStreamer for xCam2"
        export ENABLE_XSTREAMER=true
else
#cross compiler
	export CROSS_COMPILE=$RDK_TOOLCHAIN_PATH/bin/arm-linux-gnueabihf-
	export CC=${CROSS_COMPILE}gcc
	export CXX=${CROSS_COMPILE}g++
	export LD=${CROSS_COMPILE}g++
	export AR=${CROSS_COMPILE}ar
	export RANLIB=${CROSS_COMPILE}gcc-ranlib
	export AS=${CROSS_COMPILE}as
fi
# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

ARGS=$@

# This Function to perform pre-build configurations before building webrtc-streaming-node code
function configure()
{
    echo "Nothing to configure for video-analytics.."
}

# This Function to perform clean the build if any exists already
function clean()
{
    echo "Start Clean"
    cd $RDK_SOURCE_PATH/builders/make2
    sh crossmake.sh clean
    rm -rf $FSROOT/usr/local/rms/    
    echo "Cleanup done"
}

# This Function peforms the build to generate the webrtc.node
function build()
{
    cd ${RDK_SOURCE_PATH}
    cd $RDK_SOURCE_PATH/builders/make2

    if [ -f $RDK_SOURCE_PATH/../sysint/devspec/lib/rdk/rms.sh ]; then
    	sh $RDK_SOURCE_PATH/../sysint/devspec/lib/rdk/rms.sh
    fi

    echo "Building  rms"
    if [ $XCAM_MODEL == 'SCHC2' ]; then
		echo "Buiding RMS for XCAM2"
		if [ $ENABLE_XSTREAMER == 'true' ]; then
			echo "Loading linaro-armv7ahf-2015.11-gcc5.2.1.cmk"
			sh createPackage.sh linaro-armv7ahf-2015.11-gcc5.2.1.cmk
		else
			echo "Loading linaro-armv7ahf-2015.11-gcc5.2.cmk"
			sh createPackage.sh linaro-armv7ahf-2015.11-gcc5.2.cmk
		fi
	elif [ $XCAM_MODEL == "XHB1" ]; then
	 	echo "Buiding RMS for XHB1"
		export ENABLE_XSTREAMER=true
	 	sh createPackage.sh linaro-aarch64-2017.08-gcc7.1.cmk
	else
		echo "Building RMS for xCAM/iCAM2"
		sh createPackage.sh linaro-armv7ahf-2014.12-gcc4.9.cmk
	fi
	#sh createPackage.sh comcast-linaro.cmk
}

# This Function peforms the rebuild to generate the webrtc.node
function rebuild()
{
    clean
    build
}

# This functions performs installation of webrtc-streaming-node output created into sercomm firmware binary
function install()
{
 cd $RDK_SOURCE_PATH/builders/make2/CCdist
 
 if [ $XCAM_MODEL == 'SCHC2' ]; then
        echo "Installing  RMS for SCAM2"
	sh makeforLinarogcc5.2.sh
 elif [ $XCAM_MODEL == "XHB1" ]; then
	sh makeforLinarogcc7.1.sh
 else
        echo "Installing RMS for xCAM/iCAM2"
        sh makeforLinarogcc4.9.sh
 fi

 #sh makeit.sh
 cd $RDK_SOURCE_PATH/builders/make2/CCdist/rms
 mkdir -p $FSROOT/usr/local/rms/
 cp -rf bin $FSROOT/usr/local/rms/
 cp -rf config $FSROOT/usr/local/rms/
 cp -f version.txt $FSROOT/usr/local/rms/
 echo "install done" 
 
}

function setxStreamer()
{
    echo "setxStreamer - Enable xStreamer"
    export ENABLE_XSTREAMER=true
}


# run the logic
#these args are what left untouched after parse_args
HIT=false

for i in "$@"; do

    echo "$i"
    case $i in
        enablexStreamer)  HIT=true; setxStreamer ;;
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi


