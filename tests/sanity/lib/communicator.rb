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
# control rms
#

require 'class_rms'
require 'process'
require 'constants'
require 'utility'
include Utility
require 'open-uri'

class Communicator

  attr_reader :isNotService, :settings

  def initialize(setup)
    @settings = setup
    @rms = nil
    @rmsPid = nil
    type = @settings["type"]
    @isNotService = (type != "service")
    $rand = rand(10000).to_s if $rand.nil?
    $dir_webroot = @settings[type]["DIR_WEBROOT"]
    $dir_media = @settings[type]["DIR_MEDIA"]
    #$dir_storage = `readlink -e #{@settings[type]["DIR_STORAGE"]}`.strip
    $dir_storage = @settings[type]["DIR_STORAGE"]
    #$dir_temp = `readlink -e #{@settings[type]["DIR_TEMP"]}`.strip
    $dir_temp = @settings[type]["DIR_TEMP"]
    #$dir_record = `readlink -e #{@settings[type]["DIR_RECORD"]}`.strip
    $dir_record = @settings[type]["DIR_RECORD"]
    $file_record = FILE_RECORD
    $path_record = $dir_record + "/" + FILE_RECORD
    @hls_folder = "#{$dir_webroot}/hls_group*"
    @hls_folder_renamed = "#{$dir_webroot}/testhls*"
    @hds_folder = "#{$dir_webroot}/hds_group*"
    @hds_folder_renamed = "#{$dir_webroot}/testhds*"
    @mss_folder = "#{$dir_webroot}/mss_group*"
    @mss_folder_renamed = "#{$dir_webroot}/testmss*"
    @dash_folder = "#{$dir_webroot}/dash_group*"
    @dash_folder_renamed = "#{$dir_webroot}/testdash*"
  end

  def start_rms(rms_ip, http_cli_port)
    @rms = Rms.new(rms_ip, http_cli_port)
    return @rms.host
  end

  def stop_rms
    #
  end

  def sendCommand(cmd)
    debugPrint "-- RMS command: #{cmd}"
    (status, details) = @rms.command(cmd, $TRACE)
    debugPrint "   RMS response: #{[status, details].to_s}"
    return status, details
  end

  def sendCommandWithDesc(cmd)
    debugPrint "-- RMS command: #{cmd}"
    (status, description, details) = @rms.commandWithDesc(cmd, $TRACE)
    debugPrint "   RMS response: #{[status, description, details].to_s}"
    return status, description, details
  end

  def sendCommandWithTime(cmd)
    debugPrint "-- RMS command: #{cmd}"
    elapsed = Time.now.to_f
    (status, details) = @rms.command(cmd, $TRACE)
    elapsed = Time.now.to_f - elapsed
    debugPrint "   RMS response: #{[status, details, elapsed].to_s}"
    [status, details, elapsed]
  end

  def sendCommandWithDescWithTime(cmd)
    debugPrint "-- RMS command: #{cmd}"
    elapsed = Time.now.to_f
    (status, description, details) = @rms.commandWithDesc(cmd, $TRACE)
    elapsed = Time.now.to_f - elapsed
    debugPrint "   RMS response: #{[status, description, details, elapsed].to_s}"
    [status, description, details, elapsed]
  end

  def rmsIp
    @rms.ip
  end

  def shutdownServer
    @rms.shutdownServer if @rms != nil
  end

  def removeAllConfig
    return if @rms == nil
    while @rms.removeLastConfig > 0 do
      sleep 0.1
    end
  end

  def rms_exists?
    true if open("http://#{@settings["RMS_IP"]}:#{@settings["HTTP_CLI_PORT"]}/version")
  rescue
    false
  end

  def ews_exists?
    true if open("http://#{@settings["RMS_IP"]}:#{@settings["EWS_PORT"]}/crossdomain.xml")
  rescue
    false
  end

end
