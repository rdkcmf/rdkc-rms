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

When(/^I send stopWebRTC command to WebRTC, the status should be (.*)$/) do |_status|
  cmd = "stopWebRTC"
  response = @comm.sendCommandWithTime(cmd)
  @status = response[0]
  @details = response[1]
  @elapsed = response[2]
  expect(@status).to be == _status
end

When(/^I send startWebRTC command to WebRTC, the response should match$/) do |table|
  table.rows.each do |row|
    _rrs_ip = row[0]
    _rrs_port = row[1]
    _room_id = row[2]
    _status = row[3]
    replace_random(_room_id)
    cmd = "startWebRTC rrsip=#{_rrs_ip} rrsport=#{_rrs_port} roomid=#{_room_id}"
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
    hasDetails = (@details != nil) && (@details.size > 0)
    count = 0
    if hasDetails
      @details.each do |detail|
        p "#{detail[0]} => #{detail[1]}" if $TRACE
        case detail[0]
          when "rrsip"
            expect(detail[1]).to be == _rrs_ip
            count += 1
          when "rrsport"
            expect(detail[1].to_i).to be == _rrs_port.to_i
            count += 1
          when "roomid"
            expect(detail[1]).to be == _room_id
            count += 1
        end
      end
      if (count != 3)
        errorPrint "#\n" +
                   "#  PLEASE CHECK THE RMS CONFIGURATION FILE 'config.lua'\n" +
                   "#  ARE THE PATHS IN 'webRTC' SETTINGS 'sslKey' and 'sslCert' CORRECT?\n" +
                   "#"
      end
      expect(count).to be == 3
    end
  end
end

Then(/^the source stream "([^"]*)" should be playable$/) do |_name|
  player = "vlclive"
  url = "rtmp://#{@comm.settings["RMS_IP"]}/live/#{_name}"
  p url if $TRACE
  @comm.play_url(player, url)
  success = wait_process(player)
  expect(success).to be == true
  sleep 25
  @comm.stop_player(player)
  success = wait_process_killed(player)
  expect(success).to be == true
end

When(/^I start WebRTC using the parameters below$/) do |table|
  table.rows.each do |row|
    _rrs_ip = row[0]
    _rrs_port = row[1]
    _room_id = row[2]
    _name = row[3]
    _status = row[4]
    replace_random(_room_id)
    cmd = "startWebRTC rrsip=#{_rrs_ip} rrsport=#{_rrs_port} roomid=#{_room_id}"
    response = @comm.sendCommandWithTime(cmd)
    @status = response[0]
    @details = response[1]
    @elapsed = response[2]
    expect(@status).to be == _status
    hasDetails = (@details != nil) && (@details.size > 0)
    count = 0
    if hasDetails
      @details.each do |detail|
        p "#{detail[0]} => #{detail[1]}" if $TRACE
        case detail[0]
          when "rrsip"
            expect(detail[1]).to be == _rrs_ip
            count += 1
          when "rrsport"
            expect(detail[1].to_i).to be == _rrs_port.to_i
            count += 1
          when "roomid"
            expect(detail[1]).to be == _room_id
            count += 1
          when "name"
            expect(detail[1]).to be == _name
            count += 1
          when "sslCert"
            expect(detail[1].size).to be >= 4
            count += 1
          when "sslKey"
            expect(detail[1].size).to be >= 3
            count += 1
        end
      end
      if (count != 6)
        errorPrint "#\n" +
                   "#  PLEASE CHECK THE RMS CONFIGURATION FILE 'config.lua'\n" +
                   "#  ARE THE PATHS IN 'webRTC' SETTINGS 'sslKey' and 'sslCert' CORRECT?\n" +
                   "#"
      end
      expect(count).to be == 6
    end
  end
end

Then(/^the RDKC logo should be found on the WebRTC demo root$/) do
  url = "http://#{@comm.settings["RMS_IP"]}:#{@comm.settings["EWS_PORT"]}/demo/rms.png"
  p url if $TRACE
  found = http_ok? url
  expect(found).to be == true
end

Then(/^the WebRTC demo client page should show live streams from the given room$/) do |table1|
  table1.rows.each do |row|
    _name = row[0]
    _room = row[1]
    _delay = row[2].to_f
    replace_random(_room)
    url = "http://#{@comm.settings["RMS_IP"]}:#{@comm.settings["EWS_PORT"]}/demo/rmswrtcclient.html"
    p url if $TRACE
    found = http_ok? url
    if (!found)
      errorPrint "#\n" +
                 "#  PLEASE CHECK THE RMS CONFIGURATION FILE 'config.lua'.\n" +
                 "#  ARE THE PATHS IN 'webRTC' SETTINGS 'sslKey' and 'sslCert' CORRECT?\n" +
                 "#"
    end
    expect(found).to be == true
    url += "?stream=#{_name}&room=#{_room}"
    p url if $TRACE
    cmd = "#{BROWSER_PATH} \"#{url}\" > /dev/null 2>&1"
    p cmd if $TRACE
    pid = Process.fork
    if pid.nil?
      exec cmd
    else
      Process.detach(pid)
    end
    success = wait_process(BROWSER_NAME)
    expect(success).to be == true
    sleep _delay
    `sudo killall -s TERM #{BROWSER_NAME}`
    success = wait_process_killed(BROWSER_NAME)
    expect(success).to be == true
  end
end
