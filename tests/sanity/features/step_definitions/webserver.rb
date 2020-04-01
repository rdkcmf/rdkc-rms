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

When(/^I send webserver commands below, these results are expected$/) do |table1|
  table1.rows.each do |row|
    _command = row[0].downcase
    _groupName = row[1]
    replace_random(_groupName)
    _aliasName = row[2]
    replace_random(_aliasName)
    _listed = row[3]
    _status = row[4]
    _value = row[5]
    cmd = _command
    key = ""
    case _command
      when "listhttpstreamingsessions"
      when "addgroupnamealias"
        cmd += " groupName=#{_groupName} aliasName=#{_aliasName}"
      when "getgroupnamebyalias"
        cmd += " aliasName=#{_aliasName}"
        key = "groupName"
      when "removegroupnamealias"
        cmd += " aliasName=#{_aliasName}"
      when "flushgroupnamealiases"
      when "listgroupnamealiases"
    end
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    if (@status != _status)
      errorPrint "#\n" +
                 "#  PLEASE CHECK EWS CONFIG: webconfig.lua\n" +
                 "#  THE REQUIRED SETTING IS: hasGroupNameAliases=false\n" +
                 "#"
    end
    expect(@status).to be == _status
    hasDetails = (@details != nil) && (@details.size > 0)
    if hasDetails && (key != "") && (_value != "")
      @details.each do |detail|
        if detail[0] == key
          (expect(detail[1]).to be == _value)
          found = true
        end
      end
    end
  end
end

When(/^I have done pullstream$/) do |table1|
  table1.rows.each do |row|
    _uri = row[0]
    replace_uri(_uri)
    _localstreamname = row[1]
    _status = row[2]
    cmd = "pullstream uri=#{_uri} localstreamname=#{_localstreamname}"
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
  end
end

When(/^I have done createhlsstream$/) do |table1|
  table1.rows.each do |row|
    _localstreamname = row[0]
    _targetfolder = row[1]
    replace_dir_webroot(_targetfolder)
    _groupname = row[2]
    replace_random(_groupname)
    _status = row[3]
    cmd = "createhlsstream localstreamnames=#{_localstreamname} targetfolder=#{_targetfolder} groupname=#{_groupname} playlisttype=rolling cleanupdestination=1"
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
  end
end

When(/^(\d+) seconds have elapsed$/) do |seconds|
  sleep(seconds.to_f)
end

When(/^I play the URLs below$/) do |table1|
  table1.rows.each do |row|
    _url = row[0]
    replace_uri(_url)
    _player = row[1]
    if row[2] != ""
      _duration = row[2].to_f
    else
      _duration = 0
    end
    play_url(_player, _url)
    sleep _duration
    stop_player(_player)
    sleep 0.5
    expect(@comm.ews_exists?).to be == true
  end
end

When(/^I add aliases to HLS streams and play HLS streams using their aliases$/) do |table1|
  table1.rows.each do |row|
    _group = row[0]
    _alias = row[1]
    _url = row[2]
    _player = row[3]
    _seconds = row[4].to_f
    # add group name alias
    cmd = "addgroupnamealias aliasname=#{_alias} groupname=#{_group} playlisttype=rolling cleanupdestination=1"
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == "SUCCESS"
    # play HLS
    play_url(_player, _url)
    sleep _seconds
    stop_player(_player)
    expect(@comm.ews_exists?).to be == true
  end
end
