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

When(/^I send the version command with (.*)$/) do |parameters1|
  cmd = "version #{parameters1}"
  response = @comm.sendCommandWithTime(cmd)
  @status = response[0]
  @details = response[1]
  @elapsed = response[2]
end

Then /^the response status should be (.*) with data (.*) equal to (.*)$/ do |status1, key1, value1|
  expect(@status).to be == status1
  replace_value(value1)
  hasDetails = (@details != nil) && (@details.size > 0)
  if hasDetails && (key1 != "")
    found = false
    @details.each do |detail|
      if detail[0] == key1
        (expect(detail[1]).to be == value1)
        found = true
      end
    end
    expect(found).to be == true
  end
end
