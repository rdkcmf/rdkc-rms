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
##########################################################################in order to activate debug mode, set assign 1 to $DEBUG

require 'colorize'

module Helper

  def debugPrint(string)
    puts string if $DEBUG
  end

  def debugP(string)
    p string if $DEBUG
  end

  def errorPrint(string)
    STDERR.puts
    STDERR.puts string.light_red
  end

  #converts "true"/"false" string to its boolean counterpart
  def to_bool(stringBool)
    return stringBool == "true"
  end

  #checks if parameter is a boolean
  def is_bool?(object)
    return !!object == object
  end

  def is_numeric? (string)
    Float(string) != nil rescue false
  end

  def is_array? (string)
    if string.is_a? String
      len = string.size
      if string[0] == "[" and string[len-1] == "]"
        return true
      end
    end
    return false
  end

  def convertString(string)
    if is_numeric? string
      return string.to_i
    elsif is_array? string
      string = string.tr('[]','')
      array = string.split(',')
      allNumbers = true
      array.each do |x|
        if not is_numeric? x
          allNumbers = false
          break
        end
      end
      if allNumbers
        newArray = []
        array.each do |x|
          newArray << x.to_i
        end
        return newArray
      end
    elsif string == "true" or string == "false"
      return to_bool string
    elsif string == ""
      return nil
    else
      return string
    end
  end

  def stringPresentInTempDirectory? (substr, directory)
    found = false
    files = Dir.entries(directory)
    found = files.any? { |str| str.include?(substr) }
    return found
  end

  def deleteFile(fileName)
    if File.exist?(fileName)
      `sudo rm #{fileName}`
    elsif stringPresentInTempDirectory?(fileName, '/tmp')
      str = "/tmp/" + fileName + "*"
      `sudo rm #{str}`
    elsif !Dir.glob(fileName).empty?
      `sudo rm -R #{fileName}`
    end
  end

  def getUsername
    user = Dir.home
    if user != "/root"
      user.slice! "/home/"
    else
      userList = Dir.entries("/home")
      userList.delete("..")
      userList.delete(".")
      user = userList[0]
    end
    return user
  end

end
