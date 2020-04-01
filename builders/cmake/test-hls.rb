#!/usr/bin/ruby
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
# test rms mp4 input, rtmp streaming, and hls output via telnet commands
# 2012-may-14
#
# requires:  ruby 1.8.7
#            mp4 video should be in ..\media directory
# tested on: ubuntu 11.04
# TODO:      replace system commands for use in windows and osx
#
# syntax:    test-hls             # uses ..\media\bunny.mp4 as default input
#            test-hls <name>      # uses ..\media\<name>.mp4 as custom input

require 'net/telnet'
require 'rubygems'
require 'json'
require 'pp'

# definitions
TARGET = "hls"
WEBDIR = "/var/www"
RMS = "rdkcmediaserver"
PLAYLIST = "playlist.m3u8"

# process rms command from telnet
# inputs:
#   host - telnet host
#   cmd - telnet command
# outputs:
#   status - response status: "SUCCESS" if no error, "FAIL" if error encountered
#   details - command description if SUCCESS, error message if FAIL
def command(host, cmd)
	response = ""
	host.cmd(cmd) { |str| response = str }
	puts "--telnet command: #{cmd}"
	text = ""
	response.size.times { |i|
		if ((i > 3) && (response[i] >= 0x20) && (response[i] <= 0x7f))
			text = text + response[i].chr
		end
	}
	result = JSON.parse(text)
	description = result["description"]
	status = result["status"]
	puts "  telnet response: #{status} (#{description})"
	details = result["data"]
	return status, details
end

# main body of script
# tests streaming of input mp4 file to output hls files
# - uses telnet to send/receive commands to/from rms
# - extracts results from json responses of rms
# - uses rms pullstream to convert mp4 file to rtmp stream
# - uses rms createhlsstream to convert rtmp stream to hls files
# - uses ffplay to playback hls video
begin

	# 1. initialize & show settings
	puts
	puts "-----begin-----"
	# a. get video name from command line, if any (default to 'bunny' if none).
	source = "bunny"
	source = ARGV[0] if (ARGV.length > 0)
	# b. set input filename and output hls folder
	puts "input = #{source}.mp4"
	puts "target = #{TARGET}"

	# 2. close rms if open
	# a. get process id of rms
	puts
	puts "ps h -o pid -C #{RMS}"
	pid = `ps h -o pid -C #{RMS}`
	# b. kill process
	puts "kill #{pid}"
	system "kill #{pid}"
	# c. abort if another instance is open
	pid = `ps h -o pid -C #{RMS}`
	raise "rms still open" if (pid.size > 0)

	# 3. start rms fresh
	# a. delete previous output files, if any
	puts
	puts "rm -fr #{WEBDIR}/#{TARGET}/#{source}11g"
	system "rm -fr #{WEBDIR}/#{TARGET}/#{source}11g"
	# b. restore default settings
	puts
	puts "cp ../../dist_template/config/pushPullSetup.xml ../config/"
	system "cp ../../dist_template/config/pushPullSetup.xml ../config/"
	# c. start rms
	puts
	puts "./rdkcmediaserver/#{RMS} ../config/config.lua &"
	system "./rdkcmediaserver/#{RMS} ../config/config.lua &"
	# d. delay before opening telnet
	sleep 3

	# 4. open telnet
	puts
	localhost = Net::Telnet::new("Host" => "localhost",
			"Timeout" => 3,
			"Port" => 1112,
			"Prompt" => /[}]/n)
	sleep 3
	raise "error opening telnet" if (localhost == nil)

	# 5. telnet command: help
	# list available rms commands
	puts
	(status, details) = command(localhost, "help")
	details.size.times do |i|
		printf("    %d) %s\n", i, details[i]["command"])
	end
	raise "error on help" if (status != "SUCCESS")

	# 6. telnet command: pullstream
	# prepare to convert incoming rtmp stream to local stream
	puts
	(status, details) = command(localhost, "pullstream uri=rtmp://localhost/live/mp4:#{source}.mp4 \
			localstreamname=#{source}11 forcetcp=0")
	raise "error on pullstream" if (status != "SUCCESS")

	# 7. telnet command: createhlsstream
	# convert local stream to hls
	puts
	(status, details) = command(localhost, "createhlsstream localstreamnames=#{source}11 \
			targetfolder=#{WEBDIR}/#{TARGET} groupname=#{source}11g playlisttype=rolling")
	raise "error on createhlsstream" if (status != "SUCCESS")

	# 8. telnet command: liststreams
	# list active streams until hls playlist detected or max loops reached
	20.times do |n|
		puts "#{n} . . . waiting for playlist"
		break if (File::exists?("#{WEBDIR}/#{TARGET}/#{source}11g/#{PLAYLIST}"))
		sleep 3
		puts
		(status, details) = command(localhost, "liststreams")
		raise "error one liststreams" if (status != "SUCCESS")
		break if (details == nil)
		details.size.times do |i|
			#p details
			printf("    uniqueId: %d, ", details[i]["uniqueId"])
			printf("name: %s\n", details[i]["name"])
			if (details[i]["pullSettings"] != nil)
				if (details[i]["pullSettings"]["uri"] != nil)
					printf("      scheme: %s, ", details[i]["pullSettings"]["uri"]["scheme"])
					printf(" port: %s, ", details[i]["pullSettings"]["uri"]["port"])
					printf(" originalUri: %s\n",  details[i]["pullSettings"]["uri"]["originalUri"])
				end
			end
		end
	end

	# 9. confirm if hls playlist exists
	puts
	puts "ls -al #{WEBDIR}/#{TARGET}/#{source}11g"
	system "ls -al #{WEBDIR}/#{TARGET}/#{source}11g"
	raise "playlist not found" if (!File::exists?("#{WEBDIR}/#{TARGET}/#{source}11g/#{PLAYLIST}"))

	# 10. use ffplay to playback hls video
	puts
	puts "ffplay http://localhost/#{TARGET}/#{source}11g/playlist.m3u8 &"
	system "ffplay http://localhost/#{TARGET}/#{source}11g/playlist.m3u8 &"

	# 11. close telnet
	puts
rescue
	# a. error exit: show error message
	puts "-----error: #$!-----"
ensure
	# b. normal/error exit: close telnet if required
	puts "-----done-----"
	localhost.close if (localhost != nil)
end
