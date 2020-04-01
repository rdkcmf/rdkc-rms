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
require 'timeout'
require 'helper'
include Helper
require 'utility'
include Utility

#use common.rb to access all the common Hooks which are needed in tests
#initialize all necessary instance variables to nil at Before Hook
Before do |scenario|
  @setup = YAML.load_file(SETTINGS_FILE)
  @comm = Communicator.new(@setup) if @comm == nil
  binding.pry if @comm == nil
  @type = @setup["type"]
  if @comm.isNotService
    p "run non-service #{scenario.source_tag_names.to_s}" if $TRACE
  else
    p "run as service #{scenario.source_tag_names.to_s}" if $TRACE
  end
  @comm.stop_rms
  @result = nil
  @description = nil
  @details = nil
  @enable_abort = true
  @serviceId = nil
  @key = nil
  @streamId = nil
  @fileTime = nil
  @pullStreamCount = nil
  @connectionIdArray = nil
  @noOfFiles = nil
end

After do |scenario|
  Cucumber.wants_to_quit = true if scenario.failed? && @enable_abort
  @comm.stop_rms
  @comm = nil if @comm != nil
  @status = nil
  @result = nil
  @description = nil
  @details = nil
  @serviceId = nil
  @key = nil
  @streamId = nil
  @fileTime = nil
  @pullStreamCount = nil
  @connectionIdArray = nil
  @noOfFiles = nil
end

Given /^trace is enabled$/ do
  $TRACE = true
end

Given /^trace is disabled$/ do
  $TRACE = false
end

Given /^debug is enabled$/ do
  $DEBUG = 1
end

Given /^debug is disabled$/ do
  $DEBUG = 0
end

Given /^only non-fatal errors can occur below$/ do
  @enable_abort = false
end

Given(/^I am not root but I can sudo$/) do
  `sudo ls > /dev/null`
  username = `whoami`.strip
  if username == 'root'
    errorPrint "#\n" +
               "#  USER IS ROOT!\n" +
               "#  Logoff as root user and login as a normal user\n" +
               "#"
  end
  expect(username).not_to be == 'root'
end

Given(/^I have root access$/) do
  username = `whoami`.strip
  if username != 'root'
    errorPrint "#\n" +
               "#  USER IS NOT ROOT!\n" +
               "#"
  end
  expect(username).to be == 'root'
end

Given(/^I have a network connection$/) do
  isOnline = (@setup["RMS_IP"] == "127.0.0.1") || connected?
  if !isOnline
    errorPrint "#\n" +
               "#  NETWORK DISCONNECTED!\n" +
               "#  Is the network cable connected?\n" +
               "#  Is the router on?\n" +
               "#"
  end
  expect(isOnline).to be == true
end

Given(/^I have started the RMS$/) do
  host = @comm.start_rms(@setup["RMS_IP"], @setup["HTTP_CLI_PORT"])
  if host.nil?
    errorPrint "#\n" +
               "#  RMS NOT RUNNING!\n" +
               "#  Is RMS_IP in #{SETTINGS_FILE} correct? #{@setup['RMS_IP']}\n" +
               "#  Is ip=\"0.0.0.0\" for inboundJsonCli & inboundHttpJsonCli in RMS config.lua?\n" +
               "#"
  end
  expect(host).not_to be_nil
  step "the RMS should still be running"
  @comm.removeAllConfig
end

Given(/^I have cleaned up the RMS$/) do
  host = @comm.start_rms(@setup["RMS_IP"], @setup["HTTP_CLI_PORT"])
  if host.nil?
    errorPrint "#\n" +
               "#  RMS NOT RUNNING!\n" +
               "#  Is RMS_IP in #{SETTINGS_FILE} correct? #{@setup['RMS_IP']}\n" +
               "#  Is ip=\"0.0.0.0\" for inboundJsonCli & inboundHttpJsonCli in RMS config.lua?\n" +
               "#"
  end
  expect(host).not_to be_nil
  step "the RMS should still be running"
  @comm.removeAllConfig
end

When(/^I send the API command for pullstream with parameters below$/) do |table1|
  table1.rows.each do |row|
    uri = row[0]
    replace_uri(uri)
    name = row[1]
    replace_random(name)
    cmd = "pullstream uri=#{uri} localstreamname=#{name}"
    response = @comm.sendCommandWithDescWithTime(cmd)
    @status = response[0]
    @description = response[1]
    @details = response[2]
    @elapsed = response[3]
    @pullStreamCount.nil? ? @pullStreamCount = 1 : @pullStreamCount += 1
  end
end

When(/^I send the API command for listConfig, (.*)$/) do |command|
  response = @comm.sendCommandWithDescWithTime(command)
  @status = response[0]
  @description = response[1]
  @details = response[2]
  @elapsed = response[3]
end

When(/^I get the id of the (.*)ed stream$/) do |command|
  if @details.size > 0
    @streamId = @details["#{command}"][0]["configId"]
  else
    @streamId = 0
  end
  p "id=#{@streamId}" if $TRACE
end

When(/^I stop (.*) by removing stream$/) do |action|
  cmd = "removeConfig id=#{@streamId}"
  response = @comm.sendCommandWithDescWithTime(cmd)
  @status = response[0]
  @description = response[1]
  @details = response[2]
  @elapsed = response[3]
end

Then /^the response time should be within (\d+) milliseconds$/ do |delay1|
  expect(@elapsed.to_f * 1000).to be <= delay1.to_f
end

Then /^the response should be (.*)$/ do |status|
  expect(@status).to be == status
end

Then /^the description should be (.*)$/ do |desc|
  expect(@description).to be == desc
end

Then(/^the response parameters should have these values:$/) do |table|
  table.rows.each do |row|
    key = row[0]
    value = row[1]
    if @details.is_a? Array
      details = @details[0]
    else
      details = @details
    end
    actual_value = details["#{row[0]}"].to_s
    value = value.to_s
    debugPrint "\n- raw: key=#{key}, exp=#{value}, got=#{actual_value}" #-todo: remove
    replace_value(value)
    replace_value(actual_value)
    if ["mediaFolder", "metaFolder"].include? key
      value = `readlink -e #{value}`.strip
      actual_value = `readlink -e #{actual_value}`.strip
    end
    debugPrint "\n -sub: key=#{key}, exp=#{value}, got=#{actual_value}" #-todo: remove
    expect(actual_value).to be == value
  end
end

Then(/^I debug$/) do
  binding.pry
end

Then(/^the RMS should still be running$/) do
  expect(@comm.rms_exists?).to be == true
end

Then(/^the EWS should still be running$/) do
  expect(@comm.ews_exists?).to be == true
end

Given(/^I have these player\(s\) installed: (.*)$/) do |list|
  players = list.split
  count = 0
  players.each do |player|
    if `which #{player}` != ''
      count += 1
    else
      errorPrint "#\n" +
                 "#  REQUIRED PLAYER IS NOT INSTALLED!\n" +
                 "#  Install #{player} first before running sanity test\n" +
                 "#"
    end
  end
  expect(count) == players.size
end

Given(/^I have these browser\(s\) installed: (.*)$/) do |list|
  browsers = list.split
  count = 0
  browsers.each do |browser|
    if `which #{browser}` != ''
      count += 1
    else
      errorPrint "#\n" +
                 "#  REQUIRED BROWSER IS NOT INSTALLED!\n" +
                 "#  Install #{browser} first before running sanity test\n" +
                 "#"
    end
  end
  expect(count) == browsers.size
end

