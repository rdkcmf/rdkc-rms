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
echo "---racetest-ec start---"
STEPS=1000000
MAXDELAY=8
SERVERNAME=rdkcms
SERVERDIR=/usr/bin
CONFIGFILE=config.lua
CONFIGDIR=/etc/rdkcms
killall -9 $SERVERNAME
sleep 5
for i in `seq 1 $STEPS`;
do
  sec=$((( $RANDOM % $MAXDELAY ) + 1 ))
  echo $i $sec
  $SERVERDIR/$SERVERNAME --daemon $CONFIGDIR/$CONFIGFILE &
  sleep $sec
  ps -A | grep $SERVERNAME
  killall -9 $SERVERNAME
done
echo "---racetest-ec done---"
