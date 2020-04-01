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

if [ -z $1 ]
then
        echo "Please provide a domain name to resolve"
        exit
fi

origifs=$IFS

retval=`nslookup $1`

#echo Here is the raw retval: $retval

IFS=" "$'\t'$'\n'",:"
state=0
for word in $retval;
do
        #echo "State = $state Word = $word"
        case $state in
        0 )
                if [ $word == "Name" ]
                then
                        state=1
                fi
        ;;
        1 )
                if [ $word == "Address" ]
                then
                        state=2
                fi
        ;;
        2 )
                if [ $word != "1" ]
                then
                        state=$word
                        break
                fi
        ;;
esac
done

echo $state > /tmp/ip


IFS=$origifs

