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
# class for collecting usage statistics on rms
#
# requires:
# ruby 1.8 or later
# ubuntu 12 or later
#

class Usage
  
  @@name_rms = "rdkcms"
  @@name_ruby = "ruby"
  @@quiet_mode = false
 
  def initialize(settings)
    # get new settings
    @@name_rms = settings["name_rms"] if (settings["name_rms"] != nil)
    @@name_ruby = settings["name_ruby"] if (settings["name_ruby"] != nil)
    @@quiet_mode = settings["quiet_mode"] if (settings["quiet_mode"] != nil)
    # get pids
    pidsRms = `pidof #{@@name_rms}`.split
    puts "    RMS PIDs: #{pidsRms.join(' ')}" if !@@quiet_mode
    pidsRuby = `pidof #{@@name_ruby}`.split
    puts "    RUBY PIDs: #{pidsRuby.join(' ')}" if !@@quiet_mode
    @pids = pidsRms + pidsRuby
    # bandwidth usage
    @lastRxBytes = 0
    @lastTxBytes = 0
    @lastElapsedSec = 0
    @bwHeader = ["rx_kbps", "tx_kbps", "sum_kbps"]
    getBandwidthUsages(0)
    # mem usage
    tempMem = `head -1 /proc/meminfo | tail -1`.split
    @memTotal = tempMem[1].to_i * 1024 #total mem in bytes
    @pageSize = `getconf PAGESIZE`.split[0].to_i
    puts "    Memory: #{@memTotal} bytes memory, #{@pageSize} bytes/page" if !@@quiet_mode
    @memHeader = []
    pidsRms.size.times { |i| @memHeader << "memE#{i + 1}%" }
    pidsRuby.size.times { |i| @memHeader << "memR#{i + 1}%" }
    # cpu usage
    @cpuCount = `nproc`.split[0].to_i
    @lastCpuTotalTicks = 0
    @lastCpuBusyTicks = 0
    @lastPidBusyTicks = []
    @pids.size.times do |i|
      @lastPidBusyTicks << 0
    end
    @cpuHeader = []
    pidsRms.size.times { |i| @cpuHeader << "cpuE#{i + 1}%" }
    pidsRuby.size.times { |i| @cpuHeader << "cpuR#{i + 1}%" }
    @cpuHeader << "cpuSUM%"
    @cpuHeader << "cpuALL%"
    # show summary
    puts "    CPU: #{@cpuCount} core(s), #{pidsRms.size} RMS instance(s), #{pidsRuby.size} Ruby instance(s)" if !@@quiet_mode
    # actual settings
    settings["name_rms"] = @@name_rms
    settings["name_ruby"] = @@name_ruby
    p settings if $DEBUG
    settings
  end

  def getMemUsages
    memUsages = []
    printf("    MEM usages: ") if !@@quiet_mode
    @pids.size.times do |i|
      usage = `cat /proc/#{@pids[i]}/statm`.split
      memUsed = usage[1].to_i * @pageSize
      memUsages << 100.0 * memUsed / @memTotal
      printf("#{'%.3f'%memUsages[memUsages.size - 1]}%% ") if !@@quiet_mode
    end
    puts if !@@quiet_mode
    return memUsages
  end

  def getCpuUsages
    # get total ticks for all cpus
    usage = `head -1 /proc/stat`.split
    cpuTotalTicks = 0
    usage.size.times do |j|
      cpuTotalTicks += usage[j].to_i if (j > 0)
    end
    cpuIdleTicks = usage[4].to_i
    cpuBusyTicks = cpuTotalTicks - cpuIdleTicks
    printf("    CPU usages: ") if !@@quiet_mode
    # get cpu usage per pid
    pidUsages = []
    @pids.size.times do |i|
      usage = `cat /proc/#{@pids[i]}/stat`.split
      utime = usage[13].to_i
      stime = usage[14].to_i
      cutime = usage[15].to_i
      cstime = usage[16].to_i
      #starttime = usage[22].to_i
      #guesttime = usage[42].to_i
      #cguesttime = usage[43].to_i
      pidBusyTicks = utime + stime + cutime + cstime
      
      if (@lastCpuTotalTicks > 0)
        pidUsages << 100.0 * (pidBusyTicks - @lastPidBusyTicks[i]) / (cpuTotalTicks - @lastCpuTotalTicks)
      else
        pidUsages << 0.0
      end
      @lastPidBusyTicks[i] = pidBusyTicks
      printf("#{'%.3f'%pidUsages[pidUsages.size - 1]}%% ") if !@@quiet_mode
    end
    # get sum of cpu usage for all monitored pids
    sum = 0.0
    pidUsages.each { |usage| sum += usage }
    pidUsages << sum
    printf("#{'%.3f'%pidUsages[pidUsages.size - 1]}%% ") if !@@quiet_mode
    # get cpu usage for all runing pids
    pidUsages << 100.0 * (cpuBusyTicks - @lastCpuBusyTicks) / (cpuTotalTicks - @lastCpuTotalTicks)
    printf("#{'%.3f'%pidUsages[pidUsages.size - 1]}%% ") if !@@quiet_mode
    puts if !@@quiet_mode
    @lastCpuTotalTicks = cpuTotalTicks
    @lastCpuBusyTicks = cpuBusyTicks
    return pidUsages
  end

  def getBandwidthUsages(elapsedSec, interface = "eth0")
    netStats = `cat /proc/net/dev | grep #{interface}`.split
    rxBytes = netStats[1].to_i
    txBytes = netStats[9].to_i
    elapsed = elapsedSec - @lastElapsedSec
    puts "    rx=#{rxBytes} tx=#{txBytes} sum=#{rxBytes + txBytes} bytes, elapsed=#{elapsed}" if !@@quiet_mode
    if (elapsed > 0)
      # convert from bytes/sec to kbps (kilobits/sec)
      rxBandwidth = 0.001 * (rxBytes - @lastRxBytes) / elapsed * 8
      txBandwidth = 0.001 * (txBytes - @lastTxBytes) / elapsed * 8
      result = [rxBandwidth.round, txBandwidth.round, (rxBandwidth + txBandwidth).round]
    else
      result = [0, 0, 0]
    end
    @lastRxBytes = rxBytes
    @lastTxBytes = txBytes
    @lastElapsedSec = elapsedSec
    puts "    BW usage: RX = #{result[0]}, TX = #{result[1]}, SUM = #{result[2]} kbps" if !@@quiet_mode
    return result
  end

  def saveStatistics(fileName, elapsedSec, streamCount, cpuUsages, memUsages, bwUsages)
    elapsedSec = elapsedSec.round
    file = File.new(fileName, "a")
    if (elapsedSec == 0)
      cpuHeader = ""
      @cpuHeader.each { |header| cpuHeader += "#{header}, " }
      memHeader = ""
      @memHeader.each { |header| memHeader += "#{header}, " }
      bwHeader = ""
      @bwHeader.each { |header| bwHeader += "#{header}, " }
      file.puts "sec, count, #{cpuHeader}#{memHeader}#{bwHeader}"
    end
    cpuUsages.size.times do |i| cpuUsages[i] = "#{'%.2f'%cpuUsages[i]}" end
    cpuReadings = cpuUsages.join(', ')
    memUsages.size.times do |i| memUsages[i] = "#{'%.3f'%memUsages[i]}" end
    memReadings = memUsages.join(', ')
    bwReadings = bwUsages.join(', ')
    file.puts "#{elapsedSec}, #{streamCount}, #{cpuReadings}, #{memReadings}, #{bwReadings}"
    file.close
  end
   
end
