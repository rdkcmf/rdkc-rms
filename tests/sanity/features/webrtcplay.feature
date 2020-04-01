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

Feature: WebRTC

  Use WebRTC to Play Internal & External Streams

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have these browser(s) installed: google-chrome
    And I have started the RMS
    And I have cleaned up the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario: Send basic commands to WebRTC
    When I send stopWebRTC command to WebRTC, the status should be FAIL
    When I send startWebRTC command to WebRTC, the response should match
      | rrsip      | rrsport | roomId     | status  |
      | 52.6.14.61 | 3535    | room_$rand | SUCCESS |
    When I send stopWebRTC command to WebRTC, the status should be SUCCESS

  Scenario: Stream 3 sources to 1 room and play thru WebRTC

    When I start WebRTC using the parameters below
      | rrsip      | rrsport | roomid     | name        | status  |
      | 52.6.14.61 | 3535    | room_$rand | rdkcms | SUCCESS |
    Then the RDKC logo should be found on the WebRTC demo root

# NOTE: Requires a live connection to the internet
    When I have done pullstream
      | uri         | localstreamname | status  |
      | $live_rtmp2 | livestream      | SUCCESS |
    #And the source stream "livestream" should be playable
# NOTE: Requires Chrome v46 or later w/ MSE & H264 enabled
    Then the WebRTC demo client page should show live streams from the given room
      | localstreamname | roomid     | delay |
      | livestream      | room_$rand | 45    |

# NOTE: Requires bunny.mp4 in the media folder of RMS
    When I have done pullstream
      | uri         | localstreamname | status  |
      | $vod_rtmp1  | bunny           | SUCCESS |
    #And the source stream "bunny" should be playable
# NOTE: Requires Chrome v46 or later w/ MSE & H264 enabled
    Then the WebRTC demo client page should show live streams from the given room
      | localstreamname | roomid     | delay |
      | bunny           | room_$rand | 45    |

    When I send stopWebRTC command to WebRTC, the status should be SUCCESS
    When I have cleaned up the RMS
    Then the RMS should still be running
    And the EWS should still be running

    When 3 seconds have elapsed
    When I start WebRTC using the parameters below
      | rrsip      | rrsport | roomid     | name        | status  |
      | 52.6.14.61 | 3535    | room_$rand | rdkcms | SUCCESS |
    Then the RDKC logo should be found on the WebRTC demo root

    When I have created a test pattern as my source stream named testpattern
    And I have done pullstream
      | uri         | localstreamname | status  |
      | $live_rtmp1 | colorbars       | SUCCESS |
    #Then the source stream "colorbars" should be playable
# NOTE: Requires Chrome v46 or later w/ MSE & H264 enabled
    Then the WebRTC demo client page should show live streams from the given room
      | localstreamname | roomid     | delay |
      | colorbars       | room_$rand | 45    |

    When I send stopWebRTC command to WebRTC, the status should be SUCCESS
    When I have cleaned up the RMS
    Then the RMS should still be running
    And the EWS should still be running

