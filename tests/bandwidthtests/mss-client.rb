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
# stress test a remote host by simulating mss playback (run at client side) 
# adapted to EWS
#
# todo: simulate audio download in parallel w/ video download
#
# requires: 
# ruby 1.8 or later
# ubuntu 12 or later
# web server with pre-created mss files
#

REMOTE_IP = "192.168.2.46"
REMOTE_PORT = "8888" #for EWS (use 80 for Apache)
# mss test
MSS_GROUP = "group"
MSS_STREAM_NAME = "mystream"
MSS_MANIFEST_NAME = "manifest.ismc"  #use #{MSS_STREAM_NAME}.ismc for automss
MSS_MANIFEST_PATH = "/#{MSS_GROUP}/#{MSS_STREAM_NAME}/#{MSS_MANIFEST_NAME}"
# http://localhost:8888/group/mystream/manifest.ismc
MSS_RAMP_DELAY = 20
MSS_RAMP_PLAYERS = 2 #50
MSS_RAMP_STEPS = 2 #48
MSS_PEAK_TIMES = 2 #8
#MSS_BIT_RATE = 4143000 #bits/sec
#MSS_BIT_RATE = 1099000 #bits/sec
MSS_BIT_RATE = 900000 #bits/sec
MSS_BYTE_RATE = (MSS_BIT_RATE / 1000 / 8).to_s + "k"

# gems & modules
require 'rubygems'
require 'json'
require 'pp'
require 'timeout'

require './lib_utils'

include Utils

class MssTest

  def initialize(uri, folder = "/tmp", rate = "20k")
    @@playCount = 0
    @@uri = uri
    @@localFolder = folder
    @@rate = rate
    @@videoList = []
    @@webFolder = ""
    # delete old manifest & segment files
    `rm #{@@localFolder}/manifest* 2> /dev/null`
    `rm #{@@localFolder}/segment* 2> /dev/null`
    `ls -al #{@@localFolder}`
    sleep 1
    puts "source uri = #{uri}"
    puts "destination folder = #{folder}"
    puts "rate = #{rate} bytes/sec"
  end

  def downloadPlaylist(verbose = false)
    uriParts = @@uri.split("/")
    manifest = @@localFolder + "/" + uriParts[5]
    @@webFolder = uriParts[0] + "//" + uriParts[2] + "/" + uriParts[3] + "/" + uriParts[4] + "/"
    `rm #{@@localFolder}/manifest* 2> /dev/null`
    if verbose
      cmd = "wget -P #{@@localFolder} --limit-rate=#{@@rate} #{@@uri}"
    else
      cmd = "wget -P #{@@localFolder} --limit-rate=#{@@rate} -q #{@@uri}"
    end
    puts cmd if verbose
    system cmd
    # wait for manifest
    while !File.exists?(manifest) do
      puts "*** WAITING FOR MANIFEST ***"
      sleep 0.1
    end
    # keep copy of manifest
    inFile = File.open(manifest)
    @@videoList = []
    @@audioList = []
    streamIndex = {}
    qualityLevel = {}
    chunk = {}
    inFile.each do |line|
      if (line =~ /[<]StreamIndex /) != nil
        ss = line.chomp.sub('<StreamIndex ','').sub('>','')
        tt = ss.gsub(' ',', ').gsub('=',': ')
        streamIndex = eval("{ " + tt + " }")
        puts streamIndex
      elsif (line =~ /[<]QualityLevel /) != nil
        ss = line.chomp.sub('<QualityLevel ','').sub(' />','')
        tt = ss.gsub(' ',', ').gsub('=',': ')
        qualityLevel = eval("{ " + tt + " }")
        puts qualityLevel
      elsif (line =~ /[<]c /) != nil
        ss = line.chomp.sub('<c ','').sub('/>','')
        tt = ss.gsub(' ',', ').gsub('=',': ')
        chunk = eval("{ " + tt + " }")
        puts chunk
        bitrate = qualityLevel[:Bitrate]
        startTime = chunk[:t]
        url = streamIndex[:Url].sub('{bitrate}', bitrate).sub('{start_time}', startTime)
        case streamIndex[:Type]
          when "video"
            @@videoList << url
          when "audio"
            @@audioList << url
        end
      end
    end
    puts "*** MANIFEST DOWNLOADED ***"
  end

  def simulatePlayback(step, verbose = false, fresh = true)
    @@playCount += 1
    start = Time.now
    printf "#{step}+#{@@playCount} " if verbose
    # download manifest
    downloadPlaylist(verbose) if fresh || (@@videoList.size == 0) || (@@webFolder.size == 0)
    # download segment files
50.times do
    @@videoList.each do |text|
      url = @@webFolder + text
      puts url if verbose
      if verbose
        cmd = "wget -P #{@@localFolder} --no-cache --limit-rate=#{@@rate} #{url} -O /dev/null"
      else
        cmd = "wget -P #{@@localFolder} --no-cache --limit-rate=#{@@rate} -q #{url} -O /dev/null"
      end
      puts cmd if verbose
      system cmd
    end
end
    printf "#{step}-#{@@playCount}(#{'%.2f'%(Time.now - start)}s) " if verbose
    @@playCount -= 1
  end

end

def mssTest
  puts
  puts "--ramp up MSS test--"
  mss = MssTest.new("http://#{REMOTE_IP}:#{REMOTE_PORT}#{MSS_MANIFEST_PATH}", "/tmp", MSS_BYTE_RATE)
  puts
  playerPids = []
  # download manifest
  mss.downloadPlaylist(false)
  # download segments in manifest for each player in ramp step
  MSS_RAMP_STEPS.times do |n|
    step_start = Time.now
    leftSec = (MSS_RAMP_STEPS * 2 - n + MSS_PEAK_TIMES) * MSS_RAMP_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp up #{n + 1} of #{MSS_RAMP_STEPS} steps " \
        "(#{(n + 1) * MSS_RAMP_PLAYERS} of #{(MSS_RAMP_STEPS) * MSS_RAMP_PLAYERS} players)" \
        ", #{leftMin} min #{extraSec} sec left"
    MSS_RAMP_PLAYERS.times do |i|
      playerPids << fork do
        mss.simulatePlayback(n + 1, false, false)
      end
    end
    while (Time.now < step_start + MSS_RAMP_DELAY) do
      sleep 1
    end
  end
  puts
  puts "--ramp peak MSS test--"
  MSS_PEAK_TIMES.downto(1) do |i|
    print "#{i} "
    sleep MSS_RAMP_DELAY
  end
  puts
  puts
  puts "--ramp down MSS test--"
  puts
  MSS_RAMP_STEPS.times do |n|
    step_start = Time.now
    leftSec = (MSS_RAMP_STEPS - n) * MSS_RAMP_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp down #{MSS_RAMP_STEPS - n} of #{MSS_RAMP_STEPS} steps " \
        "(#{(MSS_RAMP_STEPS - n) * MSS_RAMP_PLAYERS} of #{(MSS_RAMP_STEPS) * MSS_RAMP_PLAYERS} players)" \
        ", #{leftMin} min #{extraSec} sec left"
    MSS_RAMP_PLAYERS.times do
      if playerPids.size > 0
        Process.kill 9, playerPids[0]
        Process.wait playerPids[0]
        playerPids.delete_at(0)
      end
    end
    while (Time.now < step_start + MSS_RAMP_DELAY) do
      sleep 1
    end
  end
  puts "--done--"
end

def consolePlay(uri)
  #cmd = "ffplay -autoexit #{uri}"
  puts "Press CTRL-Q and wait for a while to quit playback"
  sleep 3
  cmd = "cvlc #{uri} --vout caca 2> /dev/null"
  puts cmd
  usr = `whoami`.chomp
  cmd = "su - ubuntu -c \"#{cmd}\"" if usr != "ubuntu"
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
        mss = MssTest.new("http://#{REMOTE_IP}:#{REMOTE_PORT}#{MSS_MANIFEST_PATH}", "/tmp", MSS_BYTE_RATE)
        mss.downloadPlaylist(true)
        mss.simulatePlayback(0, true, false)
      when 'M'
        mssTest
      when '1'
        consolePlay("http://#{REMOTE_IP}:#{REMOTE_PORT}#{MSS_MANIFEST_PATH}")
    end
    puts
    puts "-----[#{REMOTE_IP}] X:exit M:msstest S:simulatemss 1:playmss"
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
