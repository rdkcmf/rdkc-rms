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
class Timer

  def initialize
    @time_saved = 0
    @time_start = 0
    @time_end = 0
    @sec_elapsed = 0
  end

  def save_time
    #puts "*** MUST BE SUDO BEFORE RUN ***"
    @time_saved = Time.now
    puts "SAVED: " + `date`
  end

  def set_datetime(year, month, day, hour, min, sec)
    yyyy = '%04d'%year
    mm = '%02d'%month
    dd = '%02d'%day
    hh = '%02d'%hour
    nn = '%02d'%min
    ss = '%02d'%sec
    cmd = "date --set=\"#{yyyy}#{mm}#{dd} #{hh}:#{nn}:#{ss}\""
    #puts cmd
    `#{cmd}`
  end

  def restore_time
    supposed_time = @time_saved + @sec_elapsed
    year = supposed_time.year
    month = supposed_time.month
    day = supposed_time.day
    hour = supposed_time.hour
    min = supposed_time.min
    sec = supposed_time.sec
    set_datetime(year, month, day, hour, min, sec)
    puts "RESTORED: " + `date`
  end

  def set_days_time(days, hour, min, sec)
    #puts "BEFORE: " + `date`
    time_new = @time_saved + (days * 86400)
    year = time_new.year
    month = time_new.month
    day = time_new.day
    set_datetime(year, month, day, hour, min, sec)
    puts "AFTER: " + `date`
    @time_start = Time.now
  end

  def set_duration(sec)
    @time_end = @time_start + sec.to_f
  end

  def time_start
    @time_start
  end

  def time_end
    @time_end
  end

  def getResponseTime
    t = Time.now
    start = @time_start.to_f
    t - start
  end

  def setElapsed(value)
    @sec_elapsed = value
  end
end
