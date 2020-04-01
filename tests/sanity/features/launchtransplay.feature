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

Feature: LaunchProcess and Transcode

  Use LaunchProcess and Transcode to Create an Internal Stream then Playback

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these player(s) installed: vlc
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Check transcode and launchprocess commands, then play live streams
    When I have created a test pattern as my source stream named testpattern
    When I have done transcode
      | uri         | localstreamname | status  |
      | $live_rtmp1 | counter         | SUCCESS |
    And 10 seconds have elapsed
    Then the RMS should still be running
    When I play the URLs below
      | url                         | player  | seconds |
      | rtmp://$rms_ip/live/counter | vlclive | 45      |
    And I have cleaned up the RMS
    Then the RMS should still be running
    And the EWS should still be running
