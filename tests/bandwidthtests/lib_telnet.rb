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
# telnet functions
#
# requires:
# ruby 1.8 or later
#

require 'net/telnet'

module Telnet

  @@timeout = 30
  @@port = 1112

  def Telnet.initialize(settings)
    @theHost = nil
    # get new settings
    @@timeout = settings["timeout"] if (settings["timeout"] != nil)
    @@port = settings["port"] if (settings["port"] != nil)
    # actual settings
    settings["timeout"] = @@timeout
    settings["port"] = @@port
    p settings if $DEBUG
    settings
  end

  def Telnet.open(ip, settings = {})
    Telnet.initialize(settings)
    if ip == ""
      ip = "localhost"
    end
    @theHost = Net::Telnet::new("Host" => "#{ip}",
        "Timeout" => @@timeout,
        "Port" => @@port,
        #"Output_log" => "output.log",
        #"Dump_log" => "dump.log",
        "Telnetmode" => false,
        #"Waittime" => 0.1,
        "Prompt" => /[}]/n)
    #sleep 5
    raise "error opening telnet" if (@theHost == nil)
    #sleep 5
  rescue
    # error exit: show error message
    puts "-----error: #$!-----"
  end

  def Telnet.close
    @theHost.close if (@theHost != nil)
    @theHost = nil
  end

  def Telnet.host
    @theHost
  end

end
