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

Feature: Version

  Check Version Info

  Background:
    Given I am not root but I can sudo
    And I have a network connection
    And I have started the RMS
    And only non-fatal errors can occur below
#   And trace is enabled

  Scenario Outline: Send command and check response
    When I send the version command with <parameters>
    Then the response time should be within <delay> milliseconds
    And the response status should be <status> with data <key> equal to <value>

    Examples:
      | parameters | status  | key           | value          | delay |
      |            | SUCCESS | codeName      | $rms_codename  | 1000  |
      |            | SUCCESS | releaseNumber | $rms_release   | 1000  |
