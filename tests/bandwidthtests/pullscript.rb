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
# pullscript.rb (run at client or server)
#
# requires:
# ruby 1.8 or later
# ubuntu 12 or later
# local rms running (1 or 2 instances)
# remote rms running w/ standard media
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
REMOTE_IP = "192.168.2.13"
LOCAL_IP = "localhost"
INTERFACE = "eth0"
NAME_RMS = "rdkcms" #"rdkcmediaserver"
# capture settings
SAMPLE_PERIOD = 10
SAMPLE_PORT = 1935 #rtmp
#SAMPLE_PORT = 5544 #rtsp
FILE_PREFIX = "rtmp"
# ramp test settings
RAMP_STEPS = 48
RAMP_PULL_COUNT = 50
RAMP_STEP_DELAY = 20
RAMP_PEAK_TIMES = 8
# pull settings
PULL_FORCE_TCP = 1
PULL_KEEP_ALIVE = 1
IN_STREAM_NAME = "sita720p" #also the name of hls subfolder containing child playlist
# push setting
OUT_URI = "rtmp://#{REMOTE_IP}/live"
OUT_STREAM_NAME = "test"
# stream sources
IN_URI_RAMP = "rtmp://#{REMOTE_IP}/live/#{IN_STREAM_NAME}"
IN_URI = "rtmp://localhost/vod/sita720p.mp4" #was sintel4x.mp4
#IN_URI = "rtmp://stream.smcloud.net/live2/wawa/wawa_360p"
#IN_URI = "rtmp://s2pchzxmtymn2k.cloudfront.net/cfx/st/mp4:sintel.mp4"
#IN_URI = "rtmp://rtmp.jim.stream.vmmacdn.be/vmma-jim-rtmplive-live/jim"
#IN_URI = "rtmp://streaming.cityofboston.gov/live/cable"
# record settings
TARGET_DIR = "/tmp"
RECORD_PATH = "/tmp/record"
RECORD_TYPE = "mp4"

@threadRamp = nil
@threadCapture = nil
@stopCapture = false
@stopRamp = false
@streamCount = 0

def getStreamCountAtPort(port)
  count = `netstat -tan|grep ESTABLISHED|grep :#{port}|wc -l`.strip.to_i
end

def rampRecordTest
  puts "-- ramp started --"
  @stopRamp = false
  #@streamCount = Rms.getStreamsCount(Telnet.host, false)
  # ramp up
  tstart = Time.now
  for rampNo in 1..RAMP_STEPS do
    puts "ramp up ##{rampNo} of #{RAMP_STEPS} (x#{RAMP_PULL_COUNT} streams) start"
    for i in 1..RAMP_PULL_COUNT do
      Rms.pullStream(Telnet.host, IN_URI_RAMP, "#{IN_STREAM_NAME}#{'%02d'%rampNo}-#{'%02d'%i}", PULL_KEEP_ALIVE, PULL_FORCE_TCP)
    end
    totalSecLeft = RAMP_STEP_DELAY * (RAMP_STEPS * 2 - rampNo + RAMP_PEAK_TIMES)
    min = totalSecLeft / 60
    sec = totalSecLeft - 60 * min
    puts "ramp up ##{rampNo} of #{RAMP_STEPS} (x#{RAMP_PULL_COUNT} streams) end -- #{'%02d'%min}:#{'%02d'%sec} min:sec left"
    tstep = tstart + RAMP_STEP_DELAY * rampNo
    while Time.now < tstep do
      sleep 0.2
      break if @stopRamp
    end
    #@streamCount = Rms.getStreamsCount(Telnet.host, false)
    break if @stopRamp
  end
  # ramp peak
  sleep RAMP_STEP_DELAY * RAMP_PEAK_TIMES
  # ramp down
  tstart = Time.now
  for rampNo in 1..RAMP_STEPS do
    puts "ramp down ##{RAMP_STEPS - rampNo + 1} of #{RAMP_STEPS} (x#{RAMP_PULL_COUNT} streams) start"
    for i in 1..RAMP_PULL_COUNT do
      Rms.shutdownStream(Telnet.host, "#{IN_STREAM_NAME}#{'%02d'%rampNo}-#{'%02d'%i}")
    end
    totalSecLeft = RAMP_STEP_DELAY * (RAMP_STEPS - rampNo + 1)
    min = totalSecLeft / 60
    sec = totalSecLeft - 60 * min
    puts "ramp down ##{RAMP_STEPS - rampNo + 1} of #{RAMP_STEPS} (x#{RAMP_PULL_COUNT} streams) end -- #{'%02d'%min}:#{'%02d'%sec} min:sec left"
    tstep = tstart + RAMP_STEP_DELAY * rampNo
    while Time.now < tstep do
      sleep 0.2
      break if @stopRamp
    end
    #@streamCount = Rms.getStreamsCount(Telnet.host, false)
    puts "ramp down ##{RAMP_STEPS - rampNo + 1} of #{RAMP_STEPS} (x#{RAMP_PULL_COUNT} streams) end"
  end
  puts "-- ramp done --"
end

def captureStats
  puts "-- capture started --"
  @stopCapture = false
  @streamCount = 0
  usages = Usage.new({"name_rms" => NAME_RMS, "quiet_mode" => true})
  timeStamp = Time.now.to_i
  tsample = Time.now
  begin
    elapsedSec = Time.now.to_i - timeStamp
    memUsages = usages.getMemUsages
    cpuUsages = usages.getCpuUsages
    bwUsages = usages.getBandwidthUsages(elapsedSec, INTERFACE)
    #@streamCount = Rms.getStreamsCount(Telnet.host, false) if @threadRamp == nil
    @streamCount = getStreamCountAtPort(SAMPLE_PORT)
    usages.saveStatistics("#{FILE_PREFIX}-#{timeStamp}.csv", elapsedSec, @streamCount, cpuUsages, memUsages, bwUsages)
    verbose = (@threadRamp == nil)
    if verbose
      printf "\n\r#{elapsedSec} sec, #{@streamCount} streams, "
      printf " #{cpuUsages[cpuUsages.size - 1]}%% cpu, #{memUsages[memUsages.size - 1]}%% mem, "
      printf " #{bwUsages[bwUsages.size - 1]} kbps"
    end
    tsample += SAMPLE_PERIOD
    while Time.now < tsample do
      sleep 0.2
      break if @stopCapture
    end
  end until @stopCapture
  puts "-- capture done --"
end

def stopRamp
  puts ">>> stopping ramp ..."
  @stopRamp = true
  sleep(5)
  @threadRamp.join if @threadRamp != nil
  Thread.kill(@threadRamp) if @threadRamp != nil
  @threadRamp = nil
  puts ">>> stopping ramp done"
end

def stopCapture
  puts ">>> stopping capture ..."
  @stopCapture = true
  sleep(5)
  @threadCapture.join if @threadCapture != nil
  Thread.kill(@threadCapture) if @threadCapture != nil
  @threadCapture = nil
  puts ">>> stopping capture done"
end

def showSettings
  puts
  puts "TEST SETTINGS:"
  puts "SAMPLE_PERIOD = #{SAMPLE_PERIOD}"
  puts "RAMP_STEPS = #{RAMP_STEPS}"
  puts "RAMP_PULL_COUNT = #{RAMP_PULL_COUNT}"
  puts "RAMP_STEP_DELAY = #{RAMP_STEP_DELAY}"
  puts "RAMP_PEAK_TIMES = #{RAMP_PEAK_TIMES}"
  puts "RECORD_PATH = #{RECORD_PATH}"
  puts "RECORD_TYPE = #{RECORD_TYPE}"
  puts "PULL_FORCE_TCP = #{PULL_FORCE_TCP}"
  puts "PULL_KEEP_ALIVE = #{PULL_KEEP_ALIVE}"
end

begin
  #$DEBUG = true
  puts
  puts "-----Begin [#{LOCAL_IP}]-----"
  Rms.initialize({"target_dir" => TARGET_DIR, "record_path" => RECORD_PATH})
  Telnet.open("")
  raise "error opening telnet" if (Telnet.host == nil)
  key = ' '
  begin
    case key.upcase
      when 'C'
        if @threadCapture == nil
          @threadCapture = Thread.new { captureStats }
        else
          stopCapture
        end
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
        Rms.record(Telnet.host, IN_STREAM_NAME, RECORD_PATH, RECORD_TYPE)
      when '3'
        if (@threadRamp == nil)
          @threadRamp = Thread.new { rampRecordTest }
        else
          stopRamp
        end
      when '5'
        Rms.pushStream(Telnet.host, OUT_URI, IN_STREAM_NAME, OUT_STREAM_NAME + "1")
        Rms.pushStream(Telnet.host, OUT_URI, IN_STREAM_NAME, OUT_STREAM_NAME + "2")
      when '7'
        Rms.listConfig(Telnet.host)
      when '8'
        Rms.removeLastConfig(Telnet.host)
        Rms.listConfig(Telnet.host)
      when ' ', '_'
        showSettings
      else
        Rms.version(Telnet.host)
    end
    statRamp = (@threadRamp == nil ? "start" : "STOP")
    statCapture = (@threadCapture == nil ? "start" : "STOP")
    puts
    puts "-----1:pull1 2:record1 3:ramptest(#{statRamp}) 5:push 7:listconfig 8:removeconfig _:settings"
    puts "[#{LOCAL_IP}] X:exit C:capture(#{statCapture}) L:liststreams S:shutlast .shutems else:version"
    key = Utils.getKey
  end until key.upcase == 'X'
rescue
  # error exit: show error message
  puts
  puts "-----Error: #$!-----"
ensure
  # normal/error exit: close telnet & cleanup
  Thread.kill(@threadCapture) if @threadCapture != nil
  Thread.kill(@threadRamp) if @threadRamp != nil
  puts
  puts "-----Done [#{LOCAL_IP}]-----"
  Telnet.close
end
