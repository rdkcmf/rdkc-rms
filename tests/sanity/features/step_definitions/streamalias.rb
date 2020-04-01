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
# encoding: utf-8
begin require 'rspec/expectations'; rescue LoadError; require 'spec/expectations'; end
require 'cucumber/formatter/unicode'
$:.unshift(File.dirname(__FILE__) + '/../../lib')
require 'communicator'
require 'helper'
include Helper
require 'utility'
include Utility

When /^I send stream alias commands, these results are expected$/ do |table1|
  table1.rows.each do |row|
    _target = row[0]
    _command = row[1].downcase
    _uri = row[2]
    replace_uri(_uri)
    _localstreamname = row[3]
    replace_random(_localstreamname)
    _aliasname = row[4]
    replace_random(_aliasname)
    if row[5] == ""
      _listed = -1 #disregard if blank or negative
    else
      _listed = row[5].to_i
    end
    _status = row[6]
    _process = row[7]
    @status = ""
    @details = ""
    case _command
      when "pullstream"
        cmd = "pullstream uri=#{_uri} localstreamname=#{_localstreamname} keepalive=0"
      when "liststreamaliases"
        cmd = "#{_command}"
      when "liststreamsids"
        cmd = "#{_command}"
      when "liststreams"
        cmd = "#{_command}"
      when "addstreamalias"
        cmd = "#{_command} localstreamname=#{_localstreamname} aliasname=#{_aliasname}"
      when "removestreamalias"
        cmd = "#{_command} aliasname=#{_aliasname}"
      when "flushstreamaliases"
        cmd = "#{_command}"
      when "playrtsp"
        uri = "rtsp://#{@setup["RMS_IP"]}:5544/#{_aliasname}"
      when "playrtmplive"
        if RTMP_LIVE_FIXED
          uri = "rtmp://#{@setup["RMS_IP"]}/live/#{_aliasname}"
        else
          uri = "'rtmp://#{@setup["RMS_IP"]}/live/#{_aliasname} live=1'"
        end
      when "playrtmpvod", "playrtmp"
        uri = "rtmp://#{@setup["RMS_IP"]}/vod/#{_aliasname}"
        #cmd = "rtmpdump --stop 15 --rtmp #{uri} --flv rtmp.flv --hashes"
      when "killall"
        cmd = _target.downcase
        #cmd = "sudo killall -9 #{_target.downcase} 2> /dev/null"
      when "delay"
        if _listed < 5
          cmd = "sleep 5"
        else
          cmd = "sleep #{_listed}"
        end
      when "breakpoint"
        cmd = "sleep 0"
        binding.pry #breakpoint here
      else
        errorPrint "** INVALID COMMAND! (#{_command})"
    end
    case _target.downcase
      when "rms"
        debugPrint cmd
        response = @comm.sendCommand(cmd)
        debugPrint response.to_s
      when "ffplay", "avplay", "vlc", "cvlc"
        player = _target.downcase
        if _command == "killall"
          p "-- stop #{player}" if $TRACE
          @comm.stop_player(player)
          success = wait_process_killed(player)
        else
          p "-- start #{player} #{uri}" if $TRACE
          @comm.play_url(player, uri)
          success = wait_process(player)
        end
        response = success ? ["SUCCESS", ""] : ["ERROR", ""]
      else
        p "-- #{cmd}" if $TRACE
        debugPrint cmd
        eval cmd
        response = ["SUCCESS", ""]
    end
    @status = response[0]
    @details = response[1]
    expect(@status).to be == _status
    hasDetails = (@details != nil) && (@details.size > 0)
    if hasDetails
      case _command
        when "liststreamaliases"
          count = @details.size
          expect(count).to be == _listed if _listed >= 0 #disregard if negative
        when "liststreamsids"
          count = 0
          @details.each do |detail|
            if _process == "origin"
              count += 1 if detail.to_i >> 32 == 0
            else
              count += 1
            end
            p detail if $TRACE
          end
          if (_listed >= 0)
            if (count > _listed)
              errorPrint "#\n" +
                         "#  EXTRA STREAM DETECTED!\n" +
                         "#  Set instancesCount=1 in config.lua for sanity tests.\n" +
                         "#"
            elsif (count < _listed)
              errorPrint "#\n" +
                         "#  MISSING STREAM DETECTED!\n" +
                         "#  Set hasIngestPoints=false in config.lua for sanity tests.\n" +
                         "#"
            end
          end
          expect(count).to be == _listed if _listed >= 0 #disregard if negative
        when "liststreams" #-todo: 1 stream is missing compared to liststreamsids on 1.6.6.3716
          count = 0
          @details.each do |detail|
            #if !_direction.empty? and !_process.empty?
            #  count += 1 if detail["type"][0] == _direction[0] and detail["processType"] == _process
            #elsif _direction.empty?
            #  count += 1 if !_process.empty? and detail["processType"] == _process
            #elsif _process.empty?
            #  count += 1 if !_direction.empty? and detail["type"][0] == _direction[0]
            #end
            if _process == "origin"
              count += 1 if detail["uniqueId"].to_i >> 32 == 0
            else
              count += 1
            end
            arrow = detail["type"][0] == "O" ? "==>" : "<=="
            s = "#{count}) T=#{detail["type"]} N=#{detail["name"]} P=#{detail["processType"]} " +
                "(#{detail["nearIp"]}:#{detail["nearPort"]}#{arrow}#{detail["farIp"]}:#{detail["farPort"]}) " +
                "U=#{detail["uniqueId"]}"
            p s if $TRACE
          end
          expect(count).to be == _listed if _listed >= 0 #disregard if negative
      end
    end
  end
end
