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
#########################################################################
#
# stress test a remote host by simulating hls playback (run at client side) 
# adapted to EWS
#
# requires: 
# ruby 1.8 or later
# ubuntu 12 or later
# web server with pre-created hls files
#

REMOTE_IP = "192.168.2.13"
REMOTE_PORT = "8888" #for EWS (use 80 for Apache)
# hls test
HLS_GROUP = "hls"
HLS_STREAM_NAME = "sita720p"
HLS_PLAYLIST_NAME = "playlist.m3u8"  #use #{HLS_STREAM_NAME}.m3u8 for autohls
HLS_PLAYLIST_PATH = "/#{HLS_GROUP}/#{HLS_STREAM_NAME}/#{HLS_PLAYLIST_NAME}"
HLS_RAMP_DELAY = 20
HLS_RAMP_PLAYERS = 50
HLS_RAMP_STEPS = 48
HLS_PEAK_TIMES = 8
HLS_BIT_RATE = 4143000 #bits/sec
HLS_BYTE_RATE = (HLS_BIT_RATE / 1000 / 8).to_s + "k"

# gems & modules
require 'rubygems'
require 'json'
require 'pp'
require 'timeout'

require './lib_utils'

include Utils

class HlsTest

  def initialize(uri, folder = "/tmp", rate = "20k")
    @@playCount = 0
    @@uri = uri
    @@localFolder = folder
    @@rate = rate
    @@playlistText = []
    @@webFolder = ""
    # delete old playlist & segment files
    `rm #{@@localFolder}/playlist* 2> /dev/null`
    `rm #{@@localFolder}/segment* 2> /dev/null`
    `ls -al #{@@localFolder}`
    sleep 1
    puts "source uri = #{uri}"
    puts "destination folder = #{folder}"
    puts "rate = #{rate} bytes/sec"
  end

  def downloadPlaylist(verbose = false)
    uriParts = @@uri.split("/")
    playlist = @@localFolder + "/" + uriParts[5]
    @@webFolder = uriParts[0] + "//" + uriParts[2] + "/" + uriParts[3] + "/" + uriParts[4] + "/"
    `rm #{@@localFolder}/playlist* 2> /dev/null`
    if verbose
      cmd = "wget -P #{@@localFolder} --limit-rate=#{@@rate} #{@@uri}"
    else
      cmd = "wget -P #{@@localFolder} --limit-rate=#{@@rate} -q #{@@uri}"
    end
    puts cmd if verbose
    system cmd
    # wait for playlist
    while !File.exists?(playlist) do
      puts "*** WAITING FOR PLAYLIST ***"
      sleep 0.1
    end
    # keep copy of playlist
    inFile = File.open(playlist)
    @@playlistText = []
    inFile.each do |line|
      @@playlistText << line.chomp
    end
    puts "*** PLAYLIST DOWNLOADED ***"
  end

  def simulatePlayback(step, verbose = false, fresh = true)
    @@playCount += 1
    start = Time.now
    printf "#{step}+#{@@playCount} " if verbose
    # download playlist
    downloadPlaylist(verbose) if fresh || (@@playlistText.size == 0) || (@@webFolder.size == 0)
    # download segment files
    @@playlistText.each do |text|
      if (text[0].chr != "#")
        puts text if verbose
        if verbose
          cmd = "wget -P #{@@localFolder} --no-cache --limit-rate=#{@@rate} #{@@webFolder}#{text.rstrip} -O /dev/null"
        else
          cmd = "wget -P #{@@localFolder} --no-cache --limit-rate=#{@@rate} -q #{@@webFolder}#{text.rstrip} -O /dev/null"
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
  hls = HlsTest.new("http://#{REMOTE_IP}:#{REMOTE_PORT}#{HLS_PLAYLIST_PATH}", "/tmp", HLS_BYTE_RATE)
  puts
  playerPids = []
  # download playlist
  hls.downloadPlaylist(false)
  # download segments in playlist for each player in ramp step
  HLS_RAMP_STEPS.times do |n|
    step_start = Time.now
    leftSec = (HLS_RAMP_STEPS * 2 - n + HLS_PEAK_TIMES) * HLS_RAMP_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp up #{n + 1} of #{HLS_RAMP_STEPS} steps " \
        "(#{(n + 1) * HLS_RAMP_PLAYERS} of #{(HLS_RAMP_STEPS) * HLS_RAMP_PLAYERS} players)" \
        ", #{leftMin} min #{extraSec} sec left"
    HLS_RAMP_PLAYERS.times do |i|
      playerPids << fork do
        hls.simulatePlayback(n + 1, false, false)
      end
    end
    while (Time.now < step_start + HLS_RAMP_DELAY) do
      sleep 1
    end
  end
  puts
  puts "--ramp peak HLS test--"
  HLS_PEAK_TIMES.downto(1) do |i|
    print "#{i} "
    sleep HLS_RAMP_DELAY
  end
  puts
  puts
  puts "--ramp down HLS test--"
  puts
  HLS_RAMP_STEPS.times do |n|
    step_start = Time.now
    leftSec = (HLS_RAMP_STEPS - n) * HLS_RAMP_DELAY
    leftMin = leftSec / 60
    extraSec = leftSec - (leftMin * 60)
    puts "ramp down #{HLS_RAMP_STEPS - n} of #{HLS_RAMP_STEPS} steps " \
        "(#{(HLS_RAMP_STEPS - n) * HLS_RAMP_PLAYERS} of #{(HLS_RAMP_STEPS) * HLS_RAMP_PLAYERS} players)" \
        ", #{leftMin} min #{extraSec} sec left"
    HLS_RAMP_PLAYERS.times do
      if playerPids.size > 0
        Process.kill 9, playerPids[0]
        Process.wait playerPids[0]
        playerPids.delete_at(0)
      end
    end
    while (Time.now < step_start + HLS_RAMP_DELAY) do
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
        hls = HlsTest.new("http://#{REMOTE_IP}:#{REMOTE_PORT}#{HLS_PLAYLIST_PATH}", "/tmp", HLS_BYTE_RATE)
        hls.downloadPlaylist(true)
        hls.simulatePlayback(0, true, false)
      when 'H'
        hlsTest
      when '1'
        consolePlay("http://#{REMOTE_IP}:#{REMOTE_PORT}#{HLS_PLAYLIST_PATH}")
    end
    puts
    puts "-----[#{REMOTE_IP}] X:exit H:hlstest S:simulatehls 1:playhls"
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
