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
require 'open-uri'
require 'watir'
require 'watir-webdriver'

Then(/^the WebSocket demo client page should show live streams$/) do |table1|
  table1.rows.each do |row|
    _name = row[0]
    _delay = row[1].to_f
    url = "http://#{@comm.settings["RMS_IP"]}:#{@comm.settings["EWS_PORT"]}/demo/rmswsvideo.html"
    p url if $TRACE
    found = http_ok? url
    expect(found).to be == true
    browser = Watir::Browser.new :chrome
    browser.goto url
    sleep 0.5
    browser.text_field(:id => 'wsUrl').set "ws://#{@comm.settings["RMS_IP"]}:8410"
    sleep 0.5
    browser.text_field(:id => 'streamName').set _name
    sleep 0.5
    browser.button(:xpath => "//button[@onclick='startPlay()']").click
    success = wait_process(BROWSER_NAME)
    expect(success).to be == true
    sleep _delay
    browser.button(:xpath => "//button[@onclick='stop()']").click
    browser.close
    success = wait_process_killed(BROWSER_NAME)
    expect(success).to be == true
  end
end
