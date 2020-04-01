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

When(/^I send the API command for record with parameters below$/) do |table1|
  table1.rows.each do |row|
    command = row[0]
    path = row[1]
    replace_path_record(path)
    name = row[2]
    replace_random(name)
    type = row[3]
    cmd = "#{command} pathtofile=#{path}" if path.strip != ""
    cmd += " localstreamname=#{name}" if name.strip != ""
    cmd += " type=#{type}" if type.strip != ""
    response = @comm.sendCommandWithDescWithTime(cmd)
    @status = response[0]
    @description = response[1]
    @details = response[2]
    @elapsed = response[3]
    if (@status != "SUCCESS")
      errorPrint "#\n" +
                "#  PLEASE CHECK WRITE PERMISSIONS TO THE RECORD FOLDER.\n" +
                "#  CHANGE THE FOLDER OWNER:GROUP TO 'rmsd:rmsd' IF REQUIRED.\n" +
                "#  ALSO, PLEASE CHECK IF THE 'type' IN 'settings.yml' IS CORRECT.\n" +
                "#  CURRENTLY IT IS SET TO '#{@comm.settings["type"]}'.\n" +
                "#"
    end
    expect(@status).to be == "SUCCESS"
  end
end
