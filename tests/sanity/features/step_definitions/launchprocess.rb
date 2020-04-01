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

When(/^I have done transcode$/) do |table1|
  table1.rows.each do |row|
    _uri = row[0]
    _localstreamname = row[1]
    _status = row[2]
    #cmd="transcode source=#{_uri} groupName=group videoBitRates=512k videoSizes=320x240 audioBitRates=64k destinations=#{_localstreamname}"
    cmd="transcode source=#{_uri} groupName=group videoBitRates=500k audioBitRates=48k " +
        "videoSizes=320x240 croppings=140:60:160:120 destinations=#{_localstreamname}"
    replace_uri(cmd)
    p cmd if $TRACE
    response = @comm.sendCommandWithTime(cmd)
    p response if $TRACE
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
  end
end

When(/^I have done launchProcess$/) do |table1|
  table1.rows.each do |row|
    _uri = row[0]
    _localstreamname = row[1]
    _status = row[2]
    cmd = "launchProcess fullBinaryPath=#{@setup[@type]["PATH_AVCONV"]} arguments=-i\\ #{_uri}\\ -b:v\\ 800k\\ -b:a\\ 64k\\ -c:v\\ libx264\\ -c:a\\ libfaac\\ -metadata\\ streamName=#{_localstreamname}\\ -f\\ flv\\ tcp://localhost:6666"
    replace_uri(cmd)
    p cmd if $TRACE
    response = @comm.sendCommandWithTime(cmd)
    p response if $TRACE
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
  end
end

When(/^I have created a test pattern as my source stream named (.*)$/) do |name|
  replace_random(name)
  cmd = "launchProcess keepAlive=0 fullBinaryPath=#{@setup[@type]["PATH_AVCONV"]} keepalive=0 arguments=-y\\ -filter_complex\\ testsrc\\ -vcodec\\ libx264\\ -metadata\\ streamname=#{name}\\ -f\\ flv\\ tcp://localhost:6666/"
  response = @comm.sendCommandWithDescWithTime(cmd)
  @status = response[0]
  @description = response[1]
  @details = response[2]
  @elapsed = response[3]
  expect(@status).to be == "SUCCESS" 
end
