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
# language: en

Feature: Record to Different Formats

  Check Record to MP4, FLV & TS Formats then Playback

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these player(s) installed: vlc
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Generate a test pattern, record it to 3 types
    When I have created a test pattern as my source stream named testpattern
    When I send the API command for pullstream with parameters below
      | uri         | localStreamName |
      | $live_rtmp1 | record_$rand    |
    And 5 seconds have elapsed
    Then the response should be SUCCESS
    When I send the API command for record with parameters below
      | command            | pathToFile   | localStreamName | type |
      | record overwrite=1 | $path_record | record_$rand    | mp4  |
      | record overwrite=1 | $path_record | record_$rand    | ts   |
      | record overwrite=1 | $path_record | record_$rand    | flv  |
    And 15 seconds have elapsed
    Then the response should be SUCCESS
    When I have cleaned up the RMS
    Then the EWS should still be running

  Scenario: Play the 3 recordings via HTTP
    When I play the URLs below
      | url                                  | player | seconds |
      | http://$rms_ip:8888/$file_record.mp4 | vlc    | 20      |
      | http://$rms_ip:8888/$file_record.ts  | vlc    | 20      |
      | http://$rms_ip:8888/$file_record.flv | vlc    | 20      |
    Then the RMS should still be running
    And the EWS should still be running

