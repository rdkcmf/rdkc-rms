module Utility
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
#
# configuration file
#

  def process_file(ifile, ofile, key, value, remark)
    count = 0
    while !ifile.eof
      count += 1
      line = ifile.gets
      key_val = line.split '='
      line_key = key_val[0].strip
      if line_key == key
        key_val[1] = "=#{value}, --#{remark}"
        line = key_val.join
      end
      ofile.puts "#{line}"
    end
    return count
  end

#
# parameters
#

  def replace_uri(cmd)
    cmd.gsub! "$uri_stream1", URI_STREAM1
    cmd.gsub! "$uri_stream2", URI_STREAM2
    cmd.gsub! "$uri_stream3", URI_STREAM3
    cmd.gsub! "$uri_stream4", URI_STREAM4
    cmd.gsub! "$live_rtmp1", LIVE_RTMP1
    cmd.gsub! "$live_rtmp2", LIVE_RTMP2
    cmd.gsub! "$live_rtmp3", LIVE_RTMP3
    cmd.gsub! "$vod_rtmp1", VOD_RTMP1
    cmd.gsub! "$live_http1", LIVE_HTTP1
    cmd.gsub! "$live_http2", LIVE_HTTP2
    cmd.gsub! "$web_media_file1", WEB_MEDIA_FILE1
    cmd.gsub! "$web_media_file2", WEB_MEDIA_FILE2
    cmd.gsub! "$web_media_file3", WEB_MEDIA_FILE3
    cmd.gsub! "$web_media_file4", WEB_MEDIA_FILE4
    cmd.gsub! "$rms_ip", @setup["RMS_IP"]
    cmd.gsub! "$file_record", $file_record
    replace_random(cmd)
  end

  def replace_dir_webroot(cmd)
    cmd.gsub! "$dir_webroot", $dir_webroot
  end

  def replace_dir_record(cmd)
    cmd.gsub! "$dir_record", $dir_record
  end

  def replace_file_record(cmd)
    cmd.gsub! "$file_record", $file_record
    replace_random(cmd)
  end

  def replace_path_record(cmd)
    cmd.gsub! "$path_record", $path_record
    replace_random(cmd)
  end

  def replace_storage(cmd)
    cmd.gsub! "$dir_storage", $dir_storage
    cmd.gsub! "$dir_temp", $dir_temp
  end

  def replace_value(cmd)
    replace_storage(cmd)
    cmd.gsub! "$dir_media", $dir_media
    cmd.gsub! "$rms_codename", @setup[@setup["release"]]["RMS_CODENAME"]
    cmd.gsub! "$rms_release", @setup[@setup["release"]]["RMS_RELEASE"]
  end

  def replace_random(cmd)
    cmd.gsub! "$rand", $rand
  end

#
# process
#

  def run_as(user, cmd)
    if user == ""
      command = cmd
    else
      command = "sudo -H -u #{user} bash -c \"#{cmd}\""
    end
    debugPrint "command = #{command}" #-todo: remove
    `#{command}`
  end

  def process_exists(process_name)
    pids = `sudo pidof #{process_name}`.split
    if pids.size == 0
      alt_name = ALT_PROCESS_NAME[process_name]
      pids = `sudo pidof #{alt_name}`.split if alt_name != nil
    end
    return pids.size > 0
  end

  def wait_process(process_name)
    valid_processes = VALID_PROCESS_NAMES.split
    return false if not valid_processes.include?(process_name)
    300.times do
      break if process_exists(process_name)
      sleep 0.2
    end
    return process_exists(process_name)
  end

  def wait_process_killed(process_name)
    300.times do
      break if not process_exists(process_name)
      sleep 0.2
    end
    return (process_exists(process_name) == false)
  end

#
# player
#

  def play_command_url(player, url)
    case player.downcase
      when "avplay"
        cmd = "avplay #{url} > /dev/null 2>&1"
      when "ffplay"
        #cmd = "avplay #{url} > /dev/null 2>&1"
        cmd = "ffplay #{url} > /dev/null 2>&1"
      when "vlc"
        cmd = "vlc #{url} > /dev/null 2>&1"
      when "vlclive"
        if RTMP_LIVE_FIXED
          cmd = "vlc #{url} > /dev/null 2>&1"
        else
          cmd = "vlc \"#{url} live=1\" > /dev/null 2>&1"
        end
      when "cvlc"
        cmd = "cvlc #{url} --vout caca > /dev/null 2>&1"
        #cmd = "cvlc #{url} --vout caca 2> /dev/null"
      when "mplayer"
        cmd = "mplayer #{url} > /dev/null 2>&1"
      else
        cmd = ""
        errorPrint "** INVALID PLAYER!"
    end
    return cmd
  end


  def play_command(player, protocol, stream)
    port = (protocol == "rtsp") ? ":5544" : ""
    cmd = play_command_url(player.downcase, "#{protocol}://#{@setup["RMS_IP"]}#{port}/vod/#{stream}")
    p "-- cmd: #{cmd}" if $TRACE
    return cmd
  end

  def stop_command(player)
    case player.downcase
      when "avplay"
        cmd = "killall -9 avplay 2> /dev/null ; killall -9 ffplay 2> /dev/null"
      when "ffplay"
        cmd = "killall -9 avplay 2> /dev/null ; killall -9 ffplay 2> /dev/null"
      when "vlc", "vlclive"
        cmd = "killall -9 vlc 2> /dev/null ; killall -9 cvlc 2> /dev/null"
      when "cvlc"
        cmd = "killall -9 vlc 2> /dev/null ; killall -9 cvlc 2> /dev/null"
      when "mplayer"
        cmd = "killall -9 mplayer 2> /dev/null"
      else
        cmd = ""
        errorPrint "** INVALID PLAYER!"
    end
    return cmd
  end

  def play_stream(player, protocol, stream)
    cmd = play_command(player, protocol, stream)
    p "-- cmd: #{cmd}" if $TRACE
    pid = Process.fork
    if pid.nil?
      exec cmd
    else
      Process.detach(pid)
    end
    wait_process(player)
  end

  def play_rtmp(stream)
    play_stream(RTMP_PLAYER, "rtmp", stream)
  end

  def play_rtsp(stream)
    play_stream(RTSP_PLAYER, "rtsp", stream)
  end

  def play_url(player, url)
    cmd = play_command_url(player, url)
    p "-- cmd: #{cmd}" if $TRACE
    pid = Process.fork
    if pid.nil?
      exec cmd
    else
      Process.detach(pid)
    end
    wait_process(player)
  end

  def stop_player(player)
    cmd = stop_command(player)
    p "-- cmd: #{cmd}" if $TRACE
    pid = Process.fork
    if pid.nil?
      exec cmd
    else
      Process.detach(pid)
    end
    wait_process_killed(player)
  end

  def stop_rtmp
    stop_player(RTMP_PLAYER)
  end

  def stop_rtsp
    stop_player(RTSP_PLAYER)
  end

  def find_rtmp_player
    process_exists(RTMP_PLAYER)
  end

  def find_rtsp_player
    process_exists(RTSP_PLAYER)
  end

#
# http
#

  def http_ok? url
    begin
      true if open(url)
    rescue
      false
    end
  end

  def connected?
    items = `ifconfig | grep "inet addr"`.split
    addresses = []
    items.each do |item|
      addresses << item if item =~ /addr:/
    end
    router_ip = ""
    addresses.each do |address|
      ip = address.split(':')[1]
      if ip != '127.0.0.1'
        bytes = ip.split '.'
        bytes[3] = '1'
        router_ip = bytes.join '.'
        break
      end
    end
    ok = false
    3.times do
      response = `ping #{router_ip} -c 1 -W 1`
      ok = (response =~ /, 0 received/).nil?
      break if ok
      sleep 0.5
    end
    ok
  end

end
