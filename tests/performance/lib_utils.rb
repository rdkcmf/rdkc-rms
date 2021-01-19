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
  # read one char w/o hitting enter/return nor echo
  def Utils.getKey
    begin
      # save current stty settings
      prev_setting = `stty -g`
      # the key pressed is not displayed on the screen and enable raw
      system "stty raw -echo"
      inputChr = STDIN.getc.chr
      # fetch next two characters of special keys
      if (inputChr=="\e")
        thr = Thread.new{
          inputChr = inputChr + STDIN.getc.chr
          inputChr = inputChr + STDIN.getc.chr
        }
        # wait for special keys to be read
        thr.join(0.00001)
        # kill thread - not to wait on getc for long
        thr.kill
      end
    rescue => ex
      puts "#{ex.class}: #{ex.message}"
      puts ex.backtrace
    ensure
      # restore previous settings
      system "stty #{prev_setting}"
    end
    return inputChr
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
