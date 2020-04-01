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
# stress test a remote host by simulating hls playback (run at client side) 
#
# requires: 
# ruby 1.8 or later
# ubuntu 12 or later
# web server with pre-created hls files
#

REMOTE_IP = "192.168.4.126"
# hls test
HLS_FOLDER = "hls"
HLS_GROUP = "long"
HLS_STREAM_NAME = "test"
HLS_PLAYLIST_NAME = "playlist.m3u8"  #use #{HLS_STREAM_NAME}.m3u8 for autohls
HLS_PLAYLIST_PATH = "/#{HLS_FOLDER}/#{HLS_GROUP}/#{HLS_STREAM_NAME}/#{HLS_PLAYLIST_NAME}"
HLS_RAMP_DELAY = 60
HLS_RAMP_STREAMS = 100
HLS_RAMP_STEPS = 4
HLS_BIT_RATE = 800000 #300000 #bits/sec
HLS_BYTE_RATE = (HLS_BIT_RATE / 1000 / 8).to_s + "k"
# rtmp test
RTMP_APPLICATION = "rdkcmediaserver"
RTMP_CONFIGURATION = "stresstest.lua"
RTMP_STEPS = 15
RTMP_STEP_DELAY = 60
RTMP_PEAK_DELAY = 60 #600
RTMP_PREP_DELAY = 30

# gems & modules
require 'rubygems'
require 'json'
require 'pp'

require './lib_utils'

include Utils

class HlsTest

  def initialize(uri, folder = "/tmp", rate = "20k")
    @@playCount = 0
    @@uri = uri
    @@folder = folder
    @@rate = rate
    # delete old playlist & segment files
    `rm #{@@folder}/playlist* &> /dev/null`
    `rm #{@@folder}/segment* &> /dev/null`
    puts "source uri = #{uri}"
    puts "destination folder = #{folder}"
    puts "rate = #{rate} bytes/sec"
  end

  def simulatePlayback(step, verbose = false)
    @@playCount += 1
    start = Time.now
    printf "#{step}+#{@@playCount} " if verbose
    # download playlist
    uriParts = @@uri.split("/")
    playlist = @@folder + "/" + uriParts[6]
    location = uriParts[0] + "//" + uriParts[2] + "/" + uriParts[3] + "/" + uriParts[4] + "/" + uriParts[5] + "/"
    if !File.exists?(playlist)
      if verbose
        cmd = "wget -P #{@@folder} --limit-rate=#{@@rate} #{@@uri}"
      else
        cmd = "wget -P #{@@folder} --limit-rate=#{@@rate} -q #{@@uri}"
      end
      puts cmd if verbose
      system cmd
    end
    inFile = File.open(playlist, "r")
    # download segment files
    inFile.readlines.each do |text|
      if (text[0].chr != "#")
        puts text if verbose
        if verbose
          cmd = "wget -P #{@@folder} --no-cache --limit-rate=#{@@rate} #{location}#{text.rstrip} -O /dev/null"
        else
          cmd = "wget -P #{@@folder} --no-cache --limit-rate=#{@@rate} -q #{location}#{text.rstrip} -O /dev/null"
        end
        puts cmd if verbose
        system cmd
      end
    end
    printf "#{step}-#{@@playCount}(#{'%.2f'%(Time.now - start)}s) " if verbose
    @@playCount -= 1
  end

end

def hlsTest
  puts
  puts "--ramp up HLS test--"
  hls = HlsTest.new("http://#{REMOTE_IP}#{HLS_PLAYLIST_PATH}", "/tmp", HLS_BYTE_RATE)
  tests = []
  key = 0.chr
  puts
  HLS_RAMP_STEPS.times do |n|
    puts "ramp up #{n + 1} of #{HLS_RAMP_STEPS} steps " \
        "(#{(n + 1) * HLS_RAMP_STREAMS} of #{(HLS_RAMP_STEPS) * HLS_RAMP_STREAMS} streams)"
    HLS_RAMP_STREAMS.times do
      tests << Thread.new do
        hls.simulatePlayback(n + 1, false)
      end
    end
    puts
    key = Utils.getKeyWithTimeout(HLS_RAMP_DELAY)
    break if key != 0.chr
  end
  puts
  puts "--ramp down HLS test--"
  puts
  HLS_RAMP_STEPS.times do |n|
    puts "ramp down #{n + 1} of #{HLS_RAMP_STEPS} steps " \
        "(#{(n + 1) * HLS_RAMP_STREAMS} of #{(HLS_RAMP_STEPS) * HLS_RAMP_STREAMS} streams)"
    HLS_RAMP_STREAMS.times do |i|
      tests[i].join
    end
    puts
    key = Utils.getKeyWithTimeout(HLS_RAMP_DELAY)
    break if key != 0.chr
  end
  puts "--ABORTED!" if key != 0.chr
  puts "--done--"
end

def rtmpTest
  totalSec = RTMP_STEPS * 2 * RTMP_STEP_DELAY + RTMP_PEAK_DELAY
  totalMin = totalSec / 60
  extraSec = totalSec - (totalMin * 60)
  puts
  puts "TEST SETTINGS: STEPS=#{RTMP_STEPS} X 2, STEP DELAY=#{RTMP_STEP_DELAY}, PEAK DELAY = #{RTMP_PEAK_DELAY} sec"
  puts "EXPECTED DURATION: #{totalMin} min #{extraSec} sec"
  puts
  `killall -9 #{RTMP_APPLICATION}` if `pidof -s #{RTMP_APPLICATION}`.to_i > 0
  puts "START DATA CAPTURE NOW"
  sleep RTMP_PREP_DELAY
  puts
  puts "---[1 of 3] ramp up---"
  tests = []
  key = 0.chr
  RTMP_STEPS.times do |i|
    leftSec = (RTMP_STEPS * 2 - i) * RTMP_STEP_DELAY + RTMP_PEAK_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp up #{i + 1} of #{RTMP_STEPS}, #{leftMin} min #{extraSec} sec left"
    tests << Thread.new { `./#{RTMP_APPLICATION} --daemon #{RTMP_CONFIGURATION} &` }
    key = Utils.getKeyWithTimeout(RTMP_STEP_DELAY)
    puts `ps -A | grep #{RTMP_APPLICATION}`
    break if key != 0.chr
  end
  puts

  puts "---[2 of 3] peak---"
  if key == 0.chr
    key = Utils.getKeyWithTimeout(RTMP_PEAK_DELAY)
    puts `ps -A | grep #{RTMP_APPLICATION}`
  end
  puts

  puts "---[3 of 3] ramp down---"
  RTMP_STEPS.times do |i|
    break if key != 0.chr
    leftSec = (RTMP_STEPS - i) * RTMP_STEP_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp down #{i + 1} of #{RTMP_STEPS}, #{leftMin} min #{extraSec} sec left"
    Thread.kill(tests[i])
    pids = `pidof #{RTMP_APPLICATION}`.split
    `kill #{pids[pids.size - 1]}` if (pids.size > 0)
    key = Utils.getKeyWithTimeout(RTMP_STEP_DELAY) if i != RTMP_STEPS - 1
    puts `ps -A | grep #{RTMP_APPLICATION}`
  end
  puts

  puts "---ABORTED!" if key != 0.chr
  sleep 5
  `killall -9 #{RTMP_APPLICATION}` if `pidof -s #{RTMP_APPLICATION}`.to_i > 0
  sleep 5
  puts "STOP DATA CAPTURE NOW"
  puts "---done---"
end

def consolePlay(uri)
  #cmd = "ffplay -autoexit #{uri}"
  cmd = "cvlc #{uri} --vout caca 2> /dev/null"
  puts cmd
  system cmd
end

begin
  $DEBUG = true
  puts
  puts "-----Begin [#{REMOTE_IP}]-----"
  key = ' '
  begin
    case key.upcase
      when 'S'
        hls = HlsTest.new("http://#{REMOTE_IP}#{HLS_PLAYLIST_PATH}", "/tmp", HLS_BYTE_RATE)
        hls.simulatePlayback(0, true)
      when 'H'
        hlsTest
      when 'R'
        rtmpTest
      when '1'
        consolePlay("http://#{REMOTE_IP}#{HLS_PLAYLIST_PATH}")
      when '2'
        consolePlay("rtmp://#{REMOTE_IP}/vod/#{HLS_STREAM_NAME}")
    end
    puts
    puts "-----[#{REMOTE_IP}] X:exit R:rtmptest H:hlstest S:simulatehls 1:playhls 2:playrtmp"
    key = Utils.getKey
  end until key.upcase == 'X'
rescue
  # error exit: show error message
  puts
  puts "-----Error: #$!-----"
ensure
  # normal/error exit: close telnet & cleanup
  puts
  puts "-----Done [#{REMOTE_IP}]-----"
end
