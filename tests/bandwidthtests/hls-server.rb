#!/usr/bin/ruby
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
# test/monitor rms loading (run at server side)
#
# requires:
# ruby 1.8 or later
# ubuntu 12 or later
# rms running on localhost
# rms on remote host optional
#

# gems & modules
require 'rubygems'
require 'json'
require 'pp'
require 'timeout'

require './lib_telnet'
require './lib_rms'
require './lib_utils'
require './class_usage'

include Telnet
include Rms
include Utils

# general settings
NAME_RMS = "rdkcms"
NAME_EWS = "rms-webserver"
LOCAL_IP = "localhost"
INTERFACE = "eth0"
# capture settings
SAMPLE_PERIOD = 10
# pull settings
PULL_FORCE_TCP = 1
PULL_KEEP_ALIVE = 0
#IN_STREAM_NAME = "test"
IN_STREAM_NAME = "sita720p"
# stream sources
IN_URI = "rtmp://localhost/vod/sita720p.mp4"
#IN_URI = "rtmp://localhost/vod/sintel4x.mp4"
#IN_URI = "rtmp://stream.smcloud.net/live2/wawa/wawa_360p"
IN_URI2 = "rtmp://s2pchzxmtymn2k.cloudfront.net/cfx/st/mp4:sintel.mp4"
#IN_URI = "rtmp://rtmp.jim.stream.vmmacdn.be/vmma-jim-rtmplive-live/jim"
#IN_URI = "rtmp://streaming.cityofboston.gov/live/cable"
# record settings
TARGET_PATH = "/var/www"
RECORD_PATH = "/tmp/record"
# hls settings
RAMP_TEST_DURATION = 2000 #sec (HLS_RAMP_DELAY * HLS_RAMP_STEPS)
CHUNK_LENGTH = 10 #sec

begin
  #$DEBUG = true
  puts
  puts "-----Begin [#{LOCAL_IP}]-----"
  Rms.initialize({"target_path" => TARGET_PATH, "record_path" => RECORD_PATH})
  Telnet.open("")
  raise "error opening telnet" if (Telnet.host == nil)
  key = ' '
  timeStamp = 0
  begin
    case key.upcase
      when 'C'
        key = 0
        timeStamp = Time.now.to_i
        usages = Usage.new({"name_rms" => NAME_RMS, "name_ews" => NAME_EWS})
        begin
          streamCount = Rms.getStreamsCount(Telnet.host)
          elapsedSec = Time.now.to_i - timeStamp
          memUsages = usages.getMemUsages
          cpuUsages = usages.getCpuUsages
          bwUsages = usages.getBandwidthUsages(elapsedSec, INTERFACE)
          usages.saveStatistics("hls-#{timeStamp}.csv", elapsedSec, streamCount, cpuUsages, memUsages, bwUsages)
          Timeout::timeout(SAMPLE_PERIOD) do
            key = Utils.getKey
            break
          end
        rescue Timeout::Error
          #puts "Timeout"
        end until key != 0
      when 'L'
        ids = Rms.listStreamsIds(Telnet.host)
        puts "--getStreamInfo"
        if (ids != nil)
          ids.size.times do |i|
            printf("%d) ", i + 1)
            Rms.getStreamInfo(Telnet.host, ids[i])
            sleep 0.02
          end
        end
      when 'S'
        Rms.shutdownLastStream(Telnet.host)
      when '.'
        Rms.shutdownServer(Telnet.host)
      when '1'
        Rms.pullStream(Telnet.host, IN_URI, IN_STREAM_NAME, PULL_KEEP_ALIVE, PULL_FORCE_TCP)
      when '2'
        Rms.pullStream(Telnet.host, IN_URI2, IN_STREAM_NAME, PULL_KEEP_ALIVE, PULL_FORCE_TCP)
      when '3'
        Rms.createHlsStream(Telnet.host, IN_STREAM_NAME, TARGET_PATH, CHUNK_LENGTH, (RAMP_TEST_DURATION / CHUNK_LENGTH).to_i, 0)
      when '5'
        Rms.createMssStream(Telnet.host, IN_STREAM_NAME)  
      when '7'
        Rms.listConfig(Telnet.host)
      when '8'
        Rms.removeLastConfig(Telnet.host)
        Rms.listConfig(Telnet.host)
      else
        Rms.version(Telnet.host)
    end
    puts
    puts "-----1:pull-mp4 2:pull-net 3:createhls 5:createmss 7:listconfig 8:removeconfig"
    puts "     [#{LOCAL_IP}] X:exit C:capture L:liststreams S:shutlast .shutems else:version"
    key = Utils.getKey
  end until key.upcase == 'X'
rescue
  # error exit: show error message
  puts
  puts "-----Error: #$!-----"
ensure
  # normal/error exit: close telnet & cleanup
  puts
  puts "-----Done [#{LOCAL_IP}]-----"
  Telnet.close
end

