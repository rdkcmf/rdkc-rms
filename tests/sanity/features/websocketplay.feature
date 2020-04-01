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

Feature: WebSocket

  Use WebSocket to Play Internal & External Streams

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these browser(s) installed: google-chrome
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Stream 3 sources and play thru WebSocket
    When I have done pullstream
      | uri         | localstreamname | status  |
# NOTE: Requires a live connection to the internet
      | $live_rtmp2 | livestream      | SUCCESS |
    Then the WebSocket demo client page should show live streams
      | localstreamname | delay |
      | livestream      | 55    |
    When I have cleaned up the RMS
    When I have done pullstream
      | uri         | localstreamname | status  |
# NOTE: Requires bunny.mp4 in the media folder of RMS
      | $vod_rtmp1  | bunny           | SUCCESS |
    Then the WebSocket demo client page should show live streams
      | localstreamname | delay |
      | bunny           | 45    |
    When I have cleaned up the RMS
    When I have created a test pattern as my source stream named testpattern
    When I have done pullstream
      | uri         | localstreamname | status  |
      | $live_rtmp1 | colorbars       | SUCCESS |
# NOTE: Requires Chrome v46 or later, chromedriver installed
    Then the WebSocket demo client page should show live streams
      | localstreamname | delay |
      | colorbars       | 45    |
    When I have cleaned up the RMS
    Then the RMS should still be running
    And the EWS should still be running
