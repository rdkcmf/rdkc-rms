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

chown -R root:root $1/* \
&& chmod 0755 $1/DEBIAN \
&& chmod 0755 $1/etc \
&& chmod 0755 $1/etc/apt \
&& chmod 0755 $1/etc/apt/sources.list.d \
&& chmod 0644 $1/DEBIAN/conffiles \
&& chmod 0644 $1/DEBIAN/control \
&& chmod 0755 $1/DEBIAN/postinst \
&& chmod 0644 $1/etc/apt/sources.list.d/rms.list
