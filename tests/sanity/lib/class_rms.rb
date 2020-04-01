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
# control rms using rms api commands (thru http cli)
#
# requires:
# rms running on local or remote machine
#

require 'rubygems'
require 'json'
require 'pp'
require 'base64'
require 'constants'
require 'open-uri'

class Rms

  def http_cli(command, parameters)
    params = Base64.encode64(parameters)
    params.gsub!("\n", "")
    begin
      response = open("http://#{@rms_ip}:#{@http_cli_port}/#{command}?params=#{params}")
      text = ""
      response.each do |line|
        text += line
      end
      json = JSON.parse(text)
      #puts JSON.pretty_generate(json)
      return json
    rescue
      #puts "error: #$!"
      return nil
    end
  end

  # initialize rms settings
  def initialize(rms_ip = "localhost", http_cli_port = 8888)
    @rms_ip = rms_ip
    @http_cli_port = http_cli_port
    response = http_cli("version", "")
    if response.nil?
      @rms_host = nil
      errorPrint "#\n" +
                 "#  NO HTTP CLI LISTENER!\n" +
                 "#  Is RMS_IP in #{SETTINGS_FILE} correct? #{rms_ip}\n" +
                 "#  Is ip=\"0.0.0.0\" for inboundHttpJsonCli in RMS config.lua?\n" +
                 "#"
    else
      @rms_host = @rms_ip
    end
    return @rms_host
  end

  # process rms api command
  # inputs:
  #   cmd - api command
  # outputs:
  #   status - response status: "SUCCESS" if no error, "FAIL" if error encountered
  #   details - command description if SUCCESS, error message if FAIL
  def commandWithDesc(cmd, verbose = true)
    response = ""
    puts "--API command: #{cmd}" if verbose
    parts = cmd.split
    command = parts[0]
    parts.delete_at(0)
    parameters = parts.join(' ')
    response = http_cli(command, parameters)
    status = ""
    description = ""
    details = {}
    unless response.nil?
      description = response["description"]
      status = response["status"]
      puts "  API response: #{status} (#{description})" if verbose
      details = response["data"]
    end
    return status, description, details
  end

  def command(cmd, verbose = true)
    (status, description, details) = commandWithDesc(cmd, verbose)
    return status, details
  end

  # process rms api command in verbose mode
  # inputs:
  #   cmd - api command
  # outputs:
  #   status - response status: "SUCCESS" if no error, "FAIL" if error encountered
  #   details - command description if SUCCESS, error message if FAIL
  def commandVerbose(cmd)
    puts
    (status, details) = command("#{cmd}", true)
    #raise "error on #{cmd.split[0]}" if (status != "SUCCESS")
    return status, details
  end

  # process rms api command in silent mode
  # inputs:
  #   cmd - api command
  # outputs:
  #   status - response status: "SUCCESS" if no error, "FAIL" if error encountered
  #   details - command description if SUCCESS, error message if FAIL
  def commandSilent(cmd)
    (status, details) = command("#{cmd}", false)
    #raise "error on #{cmd.split[0]}" if (status != "SUCCESS")
    return status, details
  end

  def version
    cmd = "version"
    (status, details) = command(cmd)
    #puts "  releaseNumber: #{details["releaseNumber"]}"
    #puts "  buildDate: #{details["buildDate"]}"
    #puts "  buildNumber: #{details["buildNumber"]}"
    #puts "  codeName: #{details["codeName"]}"
    #puts "  banner: #{details["banner"]}"
    return details
  end

  def listStreams
    cmd = "listStreams"
    (status, details) = commandVerbose(cmd)
    lastName = ""
    if (details != nil)
      puts "    #{details.size} stream(s) found."
      details.size.times do |i|
        printf("    id=%d, ", details[i]["uniqueId"])
        printf("type=%s, ", details[i]["type"])
        lastName = details[i]["name"]
        printf("name=%s, ", lastName)
        printf("ip=%s, ", details[i]["ip"])
        printf("port=%s\n", details[i]["port"])
        printf("      serverAgent=%s", details[i]["serverAgent"])
        printf("\n")
      end
      return lastName
    else
      puts "    No stream found."
      return ""
    end
  rescue
    puts "-----error: #$!-----"
    return ""
  end

  def getStreamsCount(verbose)
    cmd = "getStreamsCount"
    (status, details) = command(cmd, verbose)
    if (details != nil)
      count = details["count"]
      puts "    #{count} stream(s) total." if verbose
    else
      puts "    No stream." if verbose
    end
    return count
  end

  def listStreamsIds
    cmd = "listStreamsIds"
    (status, details) = commandVerbose(cmd)
    if (details != nil)
      puts "    #{details.size} stream ID(s) found."
      details.size.times do |i|
        printf("%d ", details[i])
      end
      puts
    else
      puts "    No stream ID found."
    end
    return details
  end

  def getStreamInfo(id)
    cmd = "getStreamInfo id=#{id}"
    (status, details) = commandSilent(cmd)
    if (details != nil)
      printf("%s=", details["uniqueId"])
      printf("%s ", details["name"])
      streamType = details["type"]
      printf("[%s] ", streamType)
      printf("%s/%s ", details["video"]["codec"], details["audio"]["codec"])
      printf("%s:%s", details["nearIp"], details["nearPort"])
      case streamType[0].chr
        when "O"
          printf("==>")
        when "I"
          printf("<==")
        else
          printf("<=>")
      end
      printf("%s:%s(far)", details["farIp"], details["farPort"])
      printf("\n")
    end
    return details
  end

  def pullStream(uri, localStreamName = "test", keepAlive = 1, forceTcp = 0)
    cmd = "pullStream uri=#{uri} localStreamName=#{localStreamName} keepAlive=#{keepAlive} forceTcp=#{forceTcp}"
    (status, details) = commandVerbose(cmd)
  end

  def pushStream(uri, localStreamName = "test", targetStreamName = "target", keepAlive = 1)
    cmd = "pushStream uri=#{uri} localStreamName=#{localStreamName} targetStreamName=#{targetStreamName} keepAlive=#{keepAlive}"
    (status, details) = commandVerbose(cmd)
  end

  def record(localStreamName = "test", pathToFile = $dir_record, type = "mp4", keepAlive = 0, chunkLength = 0, waitForIdr = 1)
    cmd = "record localStreamName=#{localStreamName} pathToFile=#{pathToFile} type=#{type} keepAlive=#{keepAlive} chunkLength=#{chunkLength} waitForIdr=#{waitForIdr}"
    (status, details) = commandVerbose(cmd)
  end

  def createMssStream(localStreamNames, targetFolder = $dir_webroot)
    cmd = "createMssStream localStreamNames=#{localStreamNames} targetFolder=#{targetFolder} playlistType=rolling groupName=mss staleretentioncount=16 cleanupDestination=1 chunkonidr=1 playlistlength=8 chunklength=3 maxchunklength=6"
    (status, details) = commandVerbose(cmd)
  end

 def createHlsStream(localStreamNames, targetFolder = $dir_webroot)
    cmd = "createHlsStream localStreamNames=#{localStreamNames} targetFolder=#{targetFolder} playlistType=rolling groupName=hls staleretentioncount=16 cleanupDestination=1 chunkonidr=1 playlistlength=8 chunklength=3 maxchunklength=6"
    (status, details) = commandVerbose(cmd)
  end

  def createHlsStreamEncrypted(localStreamNames, targetFolder = $dir_webroot)
    cmd = "createHlsStream localStreamNames=#{localStreamNames} targetFolder=#{targetFolder} playlistType=rolling groupName=hlsaes staleretentioncount=16 cleanupDestination=1 chunkonidr=1 playlistlength=8 chunklength=10 maxchunklength=15 encryptStream=1 aesKeyCount=3"
    (status, details) = commandVerbose(cmd)
  end

  def createMssStream(localStreamNames, targetFolder = $dir_webroot)
    cmd = "createMssStream localStreamNames=#{localStreamNames} targetFolder=#{targetFolder} playlistType=rolling groupName=mss staleretentioncount=16 cleanupDestination=1 chunkonidr=1 playlistlength=8 chunklength=10 maxchunklength=15"
    commandVerbose(cmd)
  end

  def createIngestPoint(privateName, publicName)
    cmd = "createIngestPoint privateStreamName=#{privateName} publicStreamName=#{publicName}"
    commandVerbose(cmd)
  end

  def listIngestPoints
    cmd = "listIngestPoints"
    (status, details) = commandVerbose(cmd)
    if (details != nil)
      puts "    #{details.size} ingest point(s) found."
      details.size.times do |i|
        printf("    privateStreamName=%s, ", details[i]["privateStreamName"])
        printf("publicStreamName=%s, ", details[i]["publicStreamName"])
        printf("\n")
      end
    else
      puts "    No ingest point found."
    end
  end

  def shutdownStream(streamName)
    puts "    shutting down stream name=#{streamName}"
    cmd = "shutdownStream localStreamName=#{streamName} permanently=1"
    (status, details) = commandSilent(cmd)
  end

  def shutdownLastStream
    cmd = "listStreamsIds"
    (status, ids) = commandVerbose(cmd)
    lastid = 0
    lastname = ""
    if (ids != nil)
      ids.size.times do |k|
        cmd = "getStreamInfo id=#{ids[k]}"
        (status, details) = commandSilent(cmd)
        if (details != nil)
          lastid = ids[k]
          lastname = details["name"]
        end
      end
    end
    # shutdown last stream
    if lastid > 0
      puts "    shutting down stream id=#{lastid} name=#{lastname}"
      cmd = "shutdownStream id=#{lastid} permanently=1"
      (status, details) = commandSilent(cmd)
    else
      puts "no stream found, no stream shutdown"
    end
  end

  def shutdownPriorStream
    cmd = "listStreams"
    (status, details) = commandVerbose(cmd)
    lastid = 0
    lastname = ""
    priorid = 0
    priorname = ""
    if (details != nil)
      # one or more stream(s) found
      details.size.times do |i|
        priorid = lastid
        priorname = lastname
        lastid = details[i]["uniqueId"]
        lastname = details[i]["name"]
        printf("    id=%d, ", lastid)
        printf("name=%s\n", lastname)
      end
      # shutdown stream before last
      if priorid > 0
        puts "shutting down stream id=#{priorid} name=#{priorname}"
        cmd = "shutdownStream id=#{priorid} permanently=1"
        (status, details) = commandVerbose(cmd)
      end
    end
  end

  def listConfig(verbose = true)
    cmd = "listConfig"
    (status, details) = command(cmd, verbose)
    lastId = 0
    if (details != nil)
      #puts JSON.pretty_generate(details)
      details.each do |key, value|
        printf("    %s: ", key) if verbose
        value.size.times do |i|
          lastId = value[i]["configId"]
          printf(" %d ", lastId) if verbose
        end
        printf("\n") if verbose
      end
      return lastId
    else
      puts "    No config found." if verbose
      return 0
    end
  rescue
    puts "-----error: #$!-----"
    return 0
  end

  def removeLastConfig
    lastId = listConfig(false)
    # remove last config
    if lastId > 0
      puts "    removing last config id=#{lastId}" if $TRACE
      cmd = "removeConfig id=#{lastId}"
      (status, details) = commandSilent(cmd)
      return lastId
    else
      puts "no config found, nothing removed" if $TRACE
      return 0
    end
  end

  def shutdownServer
    # step 1 to get key
    cmd = "shutdownServer"
    (status, details) = command(cmd)
    if (details != nil)
      key = details["key"]
      printf("    key=%s\n", key)
      # step 2 to give key
      puts "  API command (part2)"
      puts "  no response expected as server should shutdown immediately"
      cmd = "shutdownServer key=#{key}"
      (status, details) = command(cmd)
    end
  rescue
    puts "-----error: #$!-----"
  end

  def ip
    @rms_ip
  end

  def host
    @rms_host
  end

end
