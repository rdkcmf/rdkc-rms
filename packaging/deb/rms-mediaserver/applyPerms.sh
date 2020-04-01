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

chown -R root:root $1/* \
&& chmod 0755 $1/DEBIAN \
&& chmod 0755 $1/etc \
&& chmod 0755 $1/etc/rdkcms \
&& chmod 0755 $1/etc/init.d \
&& chmod 0755 $1/usr \
&& chmod 0755 $1/usr/bin \
&& chmod 0755 $1/usr/share \
&& chmod 0755 $1/usr/share/doc \
&& chmod 0755 $1/usr/share/doc/rdkcms \
&& chmod 0755 $1/usr/share/lintian \
&& chmod 0755 $1/usr/share/lintian/overrides \
&& chmod 0755 $1/var \
&& chmod 0755 $1/var/log \
&& chmod 0755 $1/var/run \
&& chmod 0775 $1/var/rdkcms \
&& chmod 0775 $1/var/rdkcms/media \
&& chmod 0775 $1/var/rdkcms/xml \
&& chmod 0775 $1/var/log/rdkcms \
&& chmod 0775 $1/var/run/rdkcms \
&& chmod 0644 $1/DEBIAN/conffiles \
&& chmod 0644 $1/DEBIAN/control \
&& chmod 0755 $1/DEBIAN/postinst \
&& chmod 0755 $1/DEBIAN/postrm \
&& chmod 0755 $1/DEBIAN/prerm \
&& chmod 0644 $1/etc/rdkcms/* \
&& chmod 0755 $1/etc/init.d/rdkcms \
&& chmod 0755 $1/usr/bin/rdkcms \
&& chmod 0755 $1/usr/bin/rms-webserver \
&& chmod 0755 $1/usr/bin/rms-phpengine \
&& chmod 0755 $1/usr/bin/rms-phpengine/php-cgi \
&& chmod 0644 $1/usr/share/lintian/overrides/rdkcms \
&& chmod 0644 $1/usr/share/doc/rdkcms/* \
&& chmod 0755 $1/usr/share/doc/rdkcms/version \
&& chmod 0644 $1/usr/share/doc/rdkcms/version/* \
&& chmod 0664 $1/var/rdkcms/xml/* \
&& chmod 0755 $1/DEBIAN/preinst


