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
>apps.mk

FINALTARGET="applications: thelib"
ALL_OBJS="ALL_APPS_OBJS="
ACTIVE_APPS="ACTIVE_APPS="

for i in `cat apps|grep -v "#"`
do
	LC=`echo $i|tr "[:upper:]" "[:lower:]"`
	UC=`echo $i|tr "[:lower:]" "[:upper:]"`
	cat template|sed "s/__appname__/$LC/g"|sed "s/__APPNAME__/$UC/g" >>apps.mk
	FINALTARGET="$FINALTARGET $i"
	ALL_OBJS="$ALL_OBJS \$($UC""_OBJS)"
	ACTIVE_APPS="$ACTIVE_APPS -DHAS_APP_$UC"
done



echo $ALL_OBJS >> apps.mk
echo $ACTIVE_APPS >> apps.mk
echo $FINALTARGET >>apps.mk


