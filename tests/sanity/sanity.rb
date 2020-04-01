#!/usr/bin/ruby
require 'colorize'
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
# run sanity tests
#
@total_tested = 0
@total_passed = 0
@total_failed = 0
@total_missing = 0
@full_test_started = 0
@feature_test_started = 0

def test(name)
  # initialize before test
  @total_tested = @total_tested + 1
  @feature_test_started = Time.now.to_i
  filename = "features/#{name}.feature"
  print "#{@total_tested}, #{filename}, "
  if not File.exist? filename
    @total_missing = @total_missing + 1
    puts "NOT FOUND! (#{@total_missing} missing)"
  end
  `rm ?#{name}.html 2>&1 /dev/null`
  # run cucumber
  if @enabled.include? :console
    `cucumber #{filename}`
  else
    `cucumber #{filename} -f html -o #{name}.html`
  end
  # check result if not console
  if @enabled.include? :console
    failed = false
  else
    if File.exist? "#{name}.html"
      failed = `grep -l "scenario.* (.* failed[,)]" #{name}.html | wc -l`.to_i > 0
    else
      failed = true
    end
  end
  # update statistics
  @feature_test_duration = Time.now.to_i - @feature_test_started
  if failed
    @total_failed = @total_failed + 1
    puts "FAILED, #{@total_passed} pass, #{@total_failed} fail, #{@feature_test_duration} sec".light_red
    if File.exist? "#{name}.html"
      `mv #{name}.html _#{name}.html`
    end
  else
    @total_passed = @total_passed + 1
    puts "PASSED, #{@total_passed} pass, #{@total_failed} fail, #{@feature_test_duration} sec".light_green
  end
end

def test_fast
  test "version"
  test "streamplay"
end

def test_slow
  test "launchtransplay"
  test "createhlsplay"
  test "recordplay"
  test "webrtcplay"
  test "websocketplay"
end

begin
  @count = `sudo ls features/*feature | wc -l`.to_i
  @enabled = []
  ARGV.each { |arg| @enabled << arg.to_sym }
  @enabled = [:fast, :slow] if @enabled.size == 0
  puts `date`
  puts "-- SANITY TEST STARTED #{@enabled.to_s}"
  @full_test_started = Time.now.to_i
  test_fast if (@enabled.include? :fast)
  test_moderate if (@enabled.include? :moderate)
  test_slow if (@enabled.include? :slow)
  @full_test_duration = Time.now.to_i - @full_test_started
  puts "-- SANITY TEST COMPLETED (#{@full_test_duration} sec)"
  puts "Tested = #{@total_tested}           Missing = #{@total_missing}"
  puts "Failed = #{@total_failed} (#{100.0 * @total_failed / @total_tested}%)"
  puts "Passed = #{@total_passed} (#{100.0 * @total_passed / @total_tested}%)"
  puts ":) :) CONGRATULATIONS! :) :)".light_green if @total_passed == @total_tested
  puts `date`
end
