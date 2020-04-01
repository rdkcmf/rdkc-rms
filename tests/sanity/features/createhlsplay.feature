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

Feature: PullStream, CreateHLSStream, GNA

  Check PullStream, CreateHLSStream and GNA Commands, Playback HLS

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these player(s) installed: ffplay vlc
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Check pullStream, createHlsStream, and GNA commands, then play HLS
    When I have created a test pattern as my source stream named testpattern
    When I have done pullstream
      | uri         | localstreamname | status  |
      | $live_rtmp1 | colorbars       | SUCCESS |
      | $live_rtmp2 | livestream      | SUCCESS |
    Then the RMS should still be running
    And 5 seconds have elapsed
    When I have done createhlsstream
      | localstreamname | targetFolder | groupName  | status  |
      | colorbars       | $dir_webroot | hls1_$rand | SUCCESS |
      | livestream      | $dir_webroot | hls2_$rand | SUCCESS |
    And 30 seconds have elapsed
    Then the RMS should still be running
    When I send webserver commands below, these results are expected
      | command                   | groupName  | aliasName        | listed | status  | value |
      | flushGroupNameAliases     |            |                  |        | FAIL    |       |
      | listGroupNameAliases      |            |                  | 0      | FAIL    |       |
      | listhttpStreamingSessions |            |                  |        | SUCCESS |       |
      | addGroupNameAlias         | hls1_$rand | alias_$rand.m3u8 |        | FAIL    |       |
      | getGroupNameByAlias       |            | alias_$rand.m3u8 |        | FAIL    |       |
      | removeGroupNameAlias      |            | alias_$rand.m3u8 |        | FAIL    |       |
    And 15 seconds have elapsed
    When I play the URLs below
      | url                                          | player | seconds |
      | http://$rms_ip:8888/hls1_$rand/playlist.m3u8 | ffplay | 20      |
      | http://$rms_ip:8888/hls2_$rand/playlist.m3u8 | vlc    | 25      |
    When I have cleaned up the RMS
    Then the EWS should still be running
    And the RMS should still be running

