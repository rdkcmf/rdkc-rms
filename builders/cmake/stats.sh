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

APP_NAME=$1
FILE_NAME=${APP_NAME}_`date -u +%s`.txt
PIDS=`ps ax|grep $APP_NAME|grep -v "grep"|sed "s/^[\t ]*\([0-9]*\).*/\1/"`
TOP_FILTER=`echo $PIDS|tr " " ","`

echo "date cpu mem connCnt rx(kB/s) tx(kB/s)"|tr " " "\t">./$FILE_NAME
for i in `seq 1 3600`
do
	CONNECTIONS_COUNT=`netstat -tan|grep ESTABLISHED|grep 1935|wc -l`
	TRAFFIC=`sar -n DEV 1 1|grep Average|grep eth0|awk '{print $5,$6}'`
	CPU_MEM=`top -b -n1 -p $TOP_FILTER|grep $APP_NAME|awk '{cpu+=$9; memp+=$10;} END {print cpu,memp}'`
	echo `date -u +%s` $CPU_MEM $CONNECTIONS_COUNT $TRAFFIC|tr " " "\t">>./$FILE_NAME
	echo `date -u +%s` $CPU_MEM $CONNECTIONS_COUNT $TRAFFIC|tr " " "\t"
done


