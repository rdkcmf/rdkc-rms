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

STEPS=15
STEPDELAY=60
PEAKDELAY=600
TOTALSEC=$(($STEPS*2*$STEPDELAY+$PEAKDELAY))
TOTALMIN=$(($TOTALSEC/60))
EXTRASEC=$(($TOTALSEC-$TOTALMIN*60))
echo
echo "TEST SETTINGS: STEPS=$STEPS X 2, STEPDELAY=$STEPDELAY sec, PEAKDELAY=$PEAKDELAY sec"
echo "EXPECTED DURATION: $TOTALMIN min $EXTRASEC sec"
echo
killall -9 rdkcmediaserver
echo "START DATA CAPTURE NOW"
sleep 30
echo
echo "---[1 of 3] ramp up---"
for i in `seq 1 $STEPS`;
do
  LEFTSEC=$((($STEPS*2-$i+1)*$STEPDELAY+$PEAKDELAY))
  LEFTMIN=$(($LEFTSEC/60))
  EXTRASEC=$(($LEFTSEC-$LEFTMIN*60))
  echo "ramp up $i of $STEPS, $LEFTMIN min $EXTRASEC sec left"
  ./rdkcmediaserver --daemon stresstest.lua &
  sleep $STEPDELAY
  ps -A | grep rdkcmediaserver
done
echo
echo "---[2 of 3] pause---"
sleep $PEAKDELAY
echo "---[2 of 3] ramp down---"
for i in `seq 1 $STEPS`;
do
  LEFTSEC=$((($STEPS-$i+1)*$STEPDELAY))
  LEFTMIN=$(($LEFTSEC/60))
  EXTRASEC=$(($LEFTSEC-$LEFTMIN*60))
  echo "ramp down $(($STEPS - $i + 1)) of $STEPS, $LEFTMIN min $EXTRASEC sec left"
  kill `pidof rdkcmediaserver -s`
  ps -A | grep rdkcmediaserver
  if [ "$i" -ne "$STEPS" ]; then
    sleep $STEPDELAY
  fi
done
echo
sleep 5
killall -9 rdkcmediaserver
sleep 5
echo "STOP DATA CAPTURE NOW"
echo "---done---"
