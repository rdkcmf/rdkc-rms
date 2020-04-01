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
IN_STREAM_NAME = "mystream"
# create settings
MSS_KEEP_ALIVE = 0
MSS_CHUNK_LENGTH = 10 #seconds
# stream sources
IN_URI = "rtmp://localhost/vod/trans.mp4" #6 min 11 sec
#IN_URI = "rtmp://localhost/vod/sintel4x.mp4" #59 min 12 sec
IN_URI2 = "rtmp://s2pchzxmtymn2k.cloudfront.net/cfx/st/mp4:sintel.mp4" #1 min only
#IN_URI2 = "rtmp://streaming.cityofboston.gov/live/cable" #live
# record settings
TARGET_PATH = "/var/www"
RECORD_PATH = "/tmp/record"
# test1 settigns
TEST1_DURATION = 400 #seconds, maximum test duration
TEST1_COUNT = 10 #maximum simultaneous creates/pulls
TEST1_INTERVAL = 2 #number of creates/pulls before inserting a delay
TEST1_DELAY = 25 #seconds, delay after a number of creates/pulls
# test2 settigns
TEST2_DURATION = 600 #seconds, maximum test duration
TEST2_COUNT = 50 #maximum simultaneous creates/pulls
TEST2_INTERVAL = 10 #number of creates/pulls before inserting a delay
TEST2_DELAY = 25 #seconds, delay after a number of creates/pulls

@captureRunning = false
@captureThread = nil

def captureStatistics(maxSec = 300)
  puts "--- capture started ---"
  startCount = 10
  endCount = 10
  timeStamp = Time.now.to_i
  usages = Usage.new({"name_rms" => NAME_RMS, "name_ews" => NAME_EWS, "quiet_mode" => true})
  begin
    streamCount = Rms.getStreamsCount(Telnet.host, false)
    elapsedSec = Time.now.to_i - timeStamp
    memUsages = usages.getMemUsages
    cpuUsages = usages.getCpuUsages
    bwUsages = usages.getBandwidthUsages(elapsedSec, INTERFACE)
    usages.saveStatistics("mss-#{timeStamp}.csv", elapsedSec, streamCount, cpuUsages, memUsages, bwUsages)
    sleep SAMPLE_PERIOD
    # auto-detect start and end
    if streamCount == 0
      if startCount > 0
        startCount -= 1
      elsif endCount > 0
        endCount -= 1
      end
    else
      startCount = 0
    end
  end until (startCount + endCount == 0) or (elapsedSec > maxSec)
  puts "--- capture done ---"
end

def cleanupConfig
  3.times do
    while (Rms.removeLastConfig(Telnet.host, false) > 0) do sleep 0.1 end
    sleep 2
  end
end

def startBackgroundCapture(maxSec = 300)
  exit if @captureRunning
  puts "--- background capture started ---"
  @captureRunning = true
  puts "--- cleaning up config ---"
  cleanupConfig
  # start data capture
  @captureThread = Thread.new do
    captureStatistics(maxSec) 
  end
  # idle to establishing baseline
  sleep 35
end

def stopBackgroundCapture
  exit if !@captureRunning
  exit if @captureThread == nil
  puts "--- waiting for ramp down end ---"
  @captureThread.join
  @captureRunning = false
  puts "--- cleaning up config ---"
  cleanupConfig
  puts "--- background capture stopped ---"
end

def doMssTest(count = 10, interval = 5, delay = 20)
  # ramp up
  puts "--- ramp up started ---"
  1.upto(count) do |n|
    Rms.createMssStream(Telnet.host, IN_STREAM_NAME + "-#{n}", TARGET_PATH, MSS_CHUNK_LENGTH, MSS_KEEP_ALIVE)
    Rms.pullStream(Telnet.host, IN_URI, IN_STREAM_NAME + "-#{n}", PULL_KEEP_ALIVE, PULL_FORCE_TCP)
    if (n % interval == 0)
      puts "--- #{n} creates & pulls done ---"
      sleep delay
    end
  end
  puts "--- ramp up done ---"
end

begin
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
        captureStatistics(6000) 
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
        Rms.createMssStream(Telnet.host, IN_STREAM_NAME, TARGET_PATH, MSS_CHUNK_LENGTH, MSS_KEEP_ALIVE)  
      when '4'
        startBackgroundCapture(TEST1_DURATION)
        doMssTest(TEST1_COUNT, TEST1_INTERVAL, TEST1_DELAY)
        stopBackgroundCapture
      when '5'
        startBackgroundCapture(TEST2_DURATION)
        doMssTest(TEST2_COUNT, TEST2_INTERVAL, TEST2_DELAY)
        stopBackgroundCapture
      when '7'
        Rms.listConfig(Telnet.host)
      when '8'
        Rms.removeLastConfig(Telnet.host, true)
        Rms.listConfig(Telnet.host)
      when '9'
        puts "cleaning up config"
        cleanupConfig
      else
        Rms.version(Telnet.host)
    end
    puts
    puts "-----1:pull-mp4 2:pull-net 3:createmss 4:test10 5:test50 7:listconfig 8/9:removeconfig/all"
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

