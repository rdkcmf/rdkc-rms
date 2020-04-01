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

Feature: StreamAliases and ListStream Commands

  Use TestPattern with StreamAliases and ListStream Commands, then Playback

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these player(s) installed: vlc
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Check basic stream alias API syntax with stream aliasing disabled
    When I send stream alias commands, these results are expected
      | target   | command            | uri          | localstreamname | aliasname | listed | status  | option |
      | rms      | flushStreamAliases |              |                 |           |        | SUCCESS |        |
      | rms      | listStreamAliases  |              |                 |           | 0      | SUCCESS |        |
      | rms      | addStreamAlias     |              |                 |           |        | FAIL    |        |
      | rms      | removeStreamAlias  |              |                 |           |        | FAIL    |        |

  Scenario: Check stream alias interaction with other commands with stream aliasing disabled
    When I have created a test pattern as my source stream named color_$rand
    When I send stream alias commands, these results are expected
      | target   | command            | uri          | localstreamname | aliasname   | listed | status  | process |
      | rms      | liststreams        |              |                 |             | 1      | SUCCESS |         |
      | rms      | liststreamsids     |              |                 |             | 1      | SUCCESS | origin  |
      | rms      | flushStreamAliases |              |                 |             |        | SUCCESS |         |
      | rms      | listStreamAliases  |              |                 |             | 0      | SUCCESS |         |
      | vlc      | playrtmplive       |              | color_$rand     | color_$rand |        | SUCCESS |         |
      |          | delay              |              |                 |             | 25     | SUCCESS |         |
      | rms      | liststreamsids     |              |                 |             | 2      | SUCCESS | origin  |
      | vlc      | killall            |              |                 |             |        | SUCCESS |         |
      | vlc      | playrtmplive       |              | color_$rand     | color_$rand |        | SUCCESS |         |
      |          | delay              |              |                 |             | 25     | SUCCESS |         |
      | rms      | liststreamsids     |              |                 |             | 2      | SUCCESS | origin  |
      | vlc      | killall            |              |                 |             |        | SUCCESS |         |
      | rms      | liststreamsids     |              |                 |             | 1      | SUCCESS | origin  |
    When I have cleaned up the RMS
    Then the RMS should still be running
    And the EWS should still be running

