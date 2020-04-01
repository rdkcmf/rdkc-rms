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
# constants
#
SETTINGS_FILE = "settings.yml"
RMS_NAME = "rdkcms"
EWS_NAME = "rms-webserver"
LICENSE_FILE = "License.lic"

MEDIA_FILE1 = "poppres240p.mp4"
MEDIA_FILE2 = "popwed240p.mp4"
MEDIA_FILE3 = "bunny.mp4"
MEDIA_FILE4 = "nosound.mp4"
WEB_MEDIA_FILE1 = "bun33s.mp4"
WEB_MEDIA_FILE2 = "bun33s.flv"
WEB_MEDIA_FILE3 = "bun33s.ts"
WEB_MEDIA_FILE4 = "nosound.mp4"
URI_STREAM1 = "rtmp://$rms_ip/live/#{MEDIA_FILE1}"
URI_STREAM2 = "rtmp://$rms_ip/live/#{MEDIA_FILE2}"
URI_STREAM3 = "rtmp://$rms_ip/live/#{MEDIA_FILE3}"
URI_STREAM4 = "rtmp://$rms_ip/live/#{MEDIA_FILE4}"
LIVE_RTMP1 = "rtmp://$rms_ip/live/testpattern"
#LIVE_RTMP2 = "rtmp://streaming.cityofboston.gov/live/cable" #sometimes too slow
#LIVE_RTMP2 = "rtmp://s2pchzxmtymn2k.cloudfront.net/cfx/st/mp4:sintel.mp4" #only 1 minute
LIVE_RTMP2 = "rtmp://fms.105.net:1935/live/rmc1"
#LIVE_RTMP3 = "rtmp://live.hkstv.hk.lxdns.com/live/hks" #need to transcode this source
LIVE_RTMP3 = "rtmp://stream.smcloud.net/live2/eska_rock/eska_rock_480p"
VOD_RTMP1 = "rtmp://$rms_ip/vod/bunny.mp4"
LIVE_HTTP1 = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8"
LIVE_HTTP2 = "http://content.jwplatform.com/manifests/vM7nH0Kl.m3u8"
HLS_TOLERANCE = 70 #percent tolerance for chunk length vs duration
DEFAULT_CHUNK_LENGTH = 10 #seconds, default chunk length for createxxxstream
FOLDER_CREATION_TIMEOUT = (2 * DEFAULT_CHUNK_LENGTH)  #seconds, wait for  playlist/manifest
FILE_CREATION_TIMEOUT = (3 * DEFAULT_CHUNK_LENGTH) #seconds, wait for first few chunks
FILE_RECORD = "record_$rand"
PUSH_VIDEO = "rtmp://s2pchzxmtymn2k.cloudfront.net/cfx/st/mp4:sintel.mp4"
PUSH_IP = "$rms_ip"
PUSH_PORT = "6666"

RTMP_LIVE_FIXED = true
#RTMP_PLAYER = "mplayer" #note: ffplay cannot play rtmp sometimes
RTMP_PLAYER = "cvlc" #note: vlc can't run in root mode
#ng RTSP_PLAYER = "mplayer"
#RTSP_PLAYER = "ffplay" #for centos use ffplay (no avplay)
#RTSP_PLAYER = "avplay" #for ubuntu use avplay (ffplay is a softlink)
RTSP_PLAYER = "cvlc" #note: vlc can't run in root mode
BROWSER_NAME = "chrome"
BROWSER_PATH = "/usr/bin/google-chrome"
#BROWSER_NAME = "chrome"
#BROWSER_PATH = "/usr/bin/google-chrome-beta"
#BROWSER_NAME = "firefox"
#BROWSER_PATH = "/usr/bin/firefox"
VALID_PROCESS_NAMES = "vlc cvlc vlclive ffplay avplay rms-avconv #{RMS_NAME} #{BROWSER_NAME}"
ALT_PROCESS_NAME = {"vlclive" => "vlc", "cvlc" => "vlc", "ffplay" => "avplay", RMS_NAME => EWS_NAME}

