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
date > checkconfig.txt
echo "ulimit -n :" >> checkconfig.txt
ulimit -n >> checkconfig.txt
echo "--" >> checkconfig.txt
head /etc/rdkcms/webconfig.lua -n50 >> checkconfig.txt
echo "--" >> checkconfig.txt
ps -AH | grep rms | wc >> checkconfig.txt
ps -AH | grep rms >> checkconfig.txt
echo "--" >> checkconfig.txt
ls -alrt /var/log/rdkcms/ | tail -n50 >> checkconfig.txt
ls -aln /usr/bin/rms* >> checkconfig.txt
ls -aln >> checkconfig.txt
