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
# miscellaneous utilities
#
# requires:
# ruby 1.8 or later
# ubuntu 12 or later
#

require 'rubygems'
require 'timeout'

def osIsWindows
  /mingw/ =~ Gem::Platform.local.os
end
require 'Win32API' if osIsWindows
require 'win32ole' if osIsWindows

module Utils

if osIsWindows
  # read one char w/o enter nor echo
  def Utils.getKey
    key = Win32API.new('crtdll','_getch', [], 'L').call
    key.chr
  end
else
  # read one char w/o enter nor echo
  def Utils.getKey
    begin
      # save previous state of stty
      old_state = `stty -g`
      # disable echoing and enable raw (not having to press enter)
      system "stty raw -echo"
      c = STDIN.getc.chr
      # gather next two characters of special keys
      if (c=="\e")
        extra_thread = Thread.new{
          c = c + STDIN.getc.chr
          c = c + STDIN.getc.chr
        }
        # wait just long enough for special keys to get swallowed
        extra_thread.join(0.00001)
        # kill thread so not-so-long special keys don't wait on getc
        extra_thread.kill
      end
    rescue => ex
      puts "#{ex.class}: #{ex.message}"
      puts ex.backtrace
    ensure
      # restore previous state of stty
      system "stty #{old_state}"
    end
    return c
  end
end

  # get key within timeout period
  def Utils.getKeyWithTimeout(seconds)
    key = 0.chr
    Timeout::timeout(seconds) do
      key = Utils.getKey
      break
    end
  rescue Timeout::Error
    #puts "Timeout"
    key
  end

end
